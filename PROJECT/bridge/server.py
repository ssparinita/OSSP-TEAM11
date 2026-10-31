from fastapi import FastAPI
from fastapi.responses import FileResponse, StreamingResponse
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from pathlib import Path
import json, time, subprocess, os, signal

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "demo_source"
BACKUP = ROOT / "demo_backup"
VERSIONS = ROOT / "versions"
LOGS = ROOT / "logs"
EVENTS = LOGS / "events.jsonl"
LOGFILE = LOGS / "backup.log"
PIDFILE = LOGS / "backup.pid"
WEB = ROOT / "web"

app = FastAPI(title="OSSP Backup Bridge")
app.add_middleware(CORSMiddleware, allow_origins=["*"], allow_methods=["*"], allow_headers=["*"])

def files_in(folder):
    folder.mkdir(exist_ok=True)
    return sorted([p.name for p in folder.iterdir() if p.is_file()])

def running():
    if not PIDFILE.exists():
        return False
    try:
        pid = int(PIDFILE.read_text().strip())
        os.kill(pid, 0)
        return True
    except Exception:
        return False

def exe():
    return str(ROOT / "backup_sync")

class RestoreRequest(BaseModel):
    file: str
    version: str

@app.get("/")
def home():
    return FileResponse(WEB / "index.html")

@app.get("/style.css")
def css():
    return FileResponse(WEB / "style.css")

@app.get("/app.js")
def js():
    return FileResponse(WEB / "app.js")

@app.get("/api/status")
def status():
    return {
        "monitoring": running(),
        "source": str(SOURCE),
        "backup": str(BACKUP),
        "files": {"source": files_in(SOURCE), "backup": files_in(BACKUP)}
    }

@app.post("/api/monitor/start")
def start():
    if not running():
        subprocess.Popen([exe(), str(SOURCE), str(BACKUP)],
                         cwd=str(ROOT),
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    return status()

@app.post("/api/monitor/stop")
def stop():
    subprocess.run([exe(), str(SOURCE), str(BACKUP), "--stop"],
                   cwd=str(ROOT), capture_output=True, text=True)
    return status()

@app.get("/api/log")
def log():
    if not LOGFILE.exists():
        return []
    lines = LOGFILE.read_text(errors="ignore").splitlines()
    return lines[-100:]

@app.get("/api/versions")
def versions(file: str):
    items = []
    prefix = file + "_"
    for p in sorted(VERSIONS.glob(prefix + "*"), reverse=True):
        items.append({"version": p.name, "label": "Previous", "timestamp": time.ctime(p.stat().st_mtime)})
    return {"file": file, "versions": items[:10]}

@app.post("/api/restore")
def restore(req: RestoreRequest):
    candidates = [p for p in VERSIONS.iterdir() if p.name.startswith(req.file + "_")]
    matches = [p for p in candidates if p.name == req.version or req.version in p.name]
    if not matches:
        return {"ok": False, "message": "Version not found"}

    exact = [p for p in matches if p.name == req.version]
    version = exact[0] if exact else max(matches, key=lambda p: p.stat().st_mtime)
    data = version.read_bytes()
    dst_source = SOURCE / req.file
    dst_backup = BACKUP / req.file

    # Restore the selected historical content as the live file.
    # The running inotify daemon will synchronize the source to the backup.
    dst_source.parent.mkdir(parents=True, exist_ok=True)
    dst_source.write_bytes(data)
    if not running():
        dst_backup.parent.mkdir(parents=True, exist_ok=True)
        dst_backup.write_bytes(data)

    write_log_line = f"RESTORED {req.file} from {version.name}"
    with LOGFILE.open("a") as fp:
        fp.write(f"[{time.strftime('%H:%M:%S')}] {write_log_line}\n")

    return {"ok": True, "message": f"Restored {req.file} from {version.name}", "version": version.name}

@app.get("/api/events/stream")
def event_stream():
    def generator():
        pos = EVENTS.stat().st_size if EVENTS.exists() else 0
        while True:
            if EVENTS.exists():
                with EVENTS.open("r", errors="ignore") as f:
                    f.seek(pos)
                    for line in f:
                        pos = f.tell()
                        line=line.strip()
                        if line:
                            yield f"data: {line}\n\n"
            time.sleep(0.25)
    return StreamingResponse(generator(), media_type="text/event-stream",
                             headers={"Cache-Control":"no-cache","X-Accel-Buffering":"no"})

@app.get("/api/events")
def events():
    if not EVENTS.exists():
        return []
    lines = EVENTS.read_text(errors="ignore").splitlines()
    out=[]
    for line in lines[-100:]:
        try: out.append(json.loads(line))
        except: pass
    return out
