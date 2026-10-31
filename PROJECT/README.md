# OSSP Final — Linux File Backup & Synchronization System

This package contains the complete starter implementation:
1. C Linux backup daemon using inotify and POSIX system calls.
2. Local FastAPI bridge.
3. Browser control center.

## 1. Install dependencies in Ubuntu/WSL

```bash
sudo apt update
sudo apt install build-essential python3 python3-pip
python3 -m pip install fastapi uvicorn
```

If pip refuses a system install, create a virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install fastapi uvicorn
```

## 2. Build the C program

From `OSSP-Final`:

```bash
make
```

Test in foreground first:

```bash
./backup_sync demo_source demo_backup --foreground
```

Keep that terminal running. In another terminal:

```bash
touch demo_source/machine.txt
echo "hello" >> demo_source/machine.txt
rm demo_source/machine.txt
```

Press Ctrl+C to stop the foreground watcher.

## 3. Run the bridge + website

From `OSSP-Final`:

```bash
python3 -m uvicorn bridge.server:app --host 127.0.0.1 --port 8000
```

Open:

http://127.0.0.1:8000

The bridge serves the frontend and exposes:
- GET /api/status
- POST /api/monitor/start
- POST /api/monitor/stop
- GET /api/events
- GET /api/events/stream
- GET /api/log
- GET /api/versions?file=...
- POST /api/restore

## 4. Faculty demo

The main demonstration uses a real file in `demo_source/`. The C daemon watches it with `inotify` and keeps `demo_backup/` synchronized. Historical versions are stored in `versions/`.

Create/open a file with nano:

```bash
nano demo_source/faculty_demo.txt
```

Type content, then save with `Ctrl+O`, press Enter, and exit with `Ctrl+X`. The daemon creates the backup automatically.

Edit the same file again with `nano`; the previous backup is preserved as a timestamped version before the new content is synchronized.

If the source file is deleted, the daemon first copies the last active backup into `versions/<file>_<timestamp>_DELETED`, then removes the active backup with `unlink()`. Therefore the deleted data remains recoverable.

The dashboard's **RESTORE LATEST** button restores the latest stored version to the source directory; the running inotify daemon then synchronizes it back to the active backup.

## Important note about paths

The bridge uses paths inside this project:
- demo_source/
- demo_backup/
- versions/
- logs/

The C program uses the source and backup paths supplied on its command line.

## Project concepts demonstrated

- fork()
- setsid()
- inotify_init1()
- inotify_add_watch()
- stat()
- open()
- read()
- write()
- close()
- unlink()
- incremental synchronization
- timestamped previous versions
- daemon process
- real-time GUI visualization
