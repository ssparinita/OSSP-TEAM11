const $ = id => document.getElementById(id);
let lastFile = "machine.txt";

function esc(s=""){return String(s).replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));}
function now(){return new Date().toLocaleTimeString([], {hour12:false});}

function renderFiles(source=[], backup=[]){
  const row = (name, ok) => `<div class="file"><span>▱ &nbsp;${esc(name)}</span><b>${ok?"✓ SYNCED":"— PENDING"}</b></div>`;
  $("sourceFiles").innerHTML = source.map(n=>row(n,backup.includes(n))).join("") || `<div class="file"><span>Directory empty</span></div>`;
  $("backupFiles").innerHTML = backup.map(n=>row(n,true)).join("") || `<div class="file"><span>Directory empty</span></div>`;
  $("scount").textContent = `${source.length} files`;
  $("bcount").textContent = `${backup.length} files`;
}
function setStatus(active){
  $("statusText").textContent = active ? "MONITORING" : "STOPPED";
  $("inotify").textContent = active ? "ACTIVE" : "INACTIVE";
  $("statusDot").style.background = active ? "var(--g)" : "var(--r)";
  $("statusDot").style.boxShadow = active ? "0 0 10px var(--g)" : "0 0 10px var(--r)";
}
function addLog(text, cls=""){
  const box=$("log");
  const line=document.createElement("div");
  line.innerHTML=`<span style="color:#46536a">[${now()}]</span> <span class="${cls}">${esc(text)}</span>`;
  box.appendChild(line);
  while(box.children.length>120) box.removeChild(box.firstChild);
  box.scrollTop=box.scrollHeight;
}
function renderCalls(syscalls="", stage=""){
  const all=["inotify","stat()","open()","read()","write()","close()","unlink()"];
  const done=syscalls.split(",").map(x=>x.trim()).filter(Boolean);
  $("calls").innerHTML=all.map(name=>{
    const ok=done.includes(name);
    const running=!ok && stage.toLowerCase().includes(name.replace(/[()]/g,"").toLowerCase());
    return `<div class="call ${ok?"ok":running?"run":""}">
      <span class="left"><i></i><span>${name}</span></span>
      <span class="state">${ok?"COMPLETED":running?"EXECUTING":"WAITING"}</span>
    </div>`;
  }).join("");
}
function setPipeline(stage, type, file){
  const stages = type==="DELETE" ? ["DETECTED","ARCHIVING","REMOVED"] :
    ["DETECTED","CHECKING","VERSIONING","COPYING","SYNCHRONIZED"];
  let idx=stages.indexOf(stage);
  if(idx<0) idx=0;
  const pct=stages.length===1?100:Math.round((idx/(stages.length-1))*100);
  const nodes=[...document.querySelectorAll("#nodes div")];
  nodes.forEach((n,i)=>{n.classList.remove("active","done"); if(i<=idx) n.classList.add(i===idx?"active":"done");});
  $("progress").style.width=pct+"%";
  $("percent").textContent=pct+"%";
  $("stage").textContent=stage;
  $("sync").textContent=stage;
  $("current").textContent=file || "System event";
  $("operation").textContent=file ? `${type} • ${file}` : "Waiting for filesystem event";
  $("badge").textContent=file ? type : "IDLE";
  $("orb").classList.toggle("busy", !!file && stage!=="SYNCHRONIZED" && stage!=="REMOVED");
}
function processEvent(e){
  if(!e || !e.type) return;
  if(e.file) lastFile=e.file;
  const file=e.file || "";
  setPipeline(e.stage,e.type,file);
  renderCalls(e.syscalls,e.stage);

  let cls="";
  if(e.type==="DELETE") cls="warn";
  if(e.stage==="SYNCHRONIZED" || e.stage==="REMOVED") cls="ok";

  const text = e.type==="SYSTEM" ? e.stage : `${e.type} ${file} → ${e.stage}`;
  addLog(text,cls);

  if(e.stage==="SYNCHRONIZED" || e.stage==="REMOVED"){
    refreshStatus();
    if(file) loadVersions(file);
  }
}
async function refreshStatus(){
  try{
    const r=await fetch("/api/status",{cache:"no-store"}), d=await r.json();
    setStatus(d.monitoring);
    $("sourcePath").textContent=d.source;
    $("backupPath").textContent=d.backup;
    renderFiles(d.files.source,d.files.backup);
  }catch(e){
    $("statusText").textContent="BRIDGE OFFLINE";
  }
}
async function loadVersions(file){
  $("versionFile").textContent=file;
  try{
    const d=await (await fetch(`/api/versions?file=${encodeURIComponent(file)}`)).json();
    const list=d.versions||[];
    $("versions").innerHTML = list.length ? list.map((v,i)=>{
      const deleted=v.version.endsWith("_DELETED");
      return `<div class="ver"><span class="${i===0?"current":""}">v${list.length-i} &nbsp; ${deleted?"Deleted backup":i===0?"Latest":"Previous"}</span><span>${esc(v.timestamp)}</span></div>`;
    }).join("") : `<div class="ver"><span>No previous versions</span></div>`;
  }catch(e){}
}

$("start").onclick=async()=>{
  await fetch("/api/monitor/start",{method:"POST"});
  addLog("monitor start requested","ok");
  refreshStatus();
};
$("stop").onclick=async()=>{
  await fetch("/api/monitor/stop",{method:"POST"});
  addLog("monitor stop requested","warn");
  refreshStatus();
};
$("clear").onclick=()=>{$("log").innerHTML="";};
$("restore").onclick=async()=>{
  if(!lastFile){addLog("No file selected for restore","warn");return;}
  try{
    const d=await (await fetch(`/api/versions?file=${encodeURIComponent(lastFile)}`)).json();
    const list=d.versions||[];
    if(!list.length){addLog(`No stored version found for ${lastFile}`,"warn");return;}
    const latest=list[0].version;
    const r=await fetch("/api/restore",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({file:lastFile,version:latest})});
    const result=await r.json();
    addLog(result.message || "Restore completed", result.ok?"ok":"warn");
    refreshStatus();
    loadVersions(lastFile);
  }catch(e){addLog("Restore failed","warn");}
};

async function initial(){
  await refreshStatus();
  await loadVersions(lastFile);
  try{
    const old=await (await fetch("/api/events")).json();
    old.slice(-20).forEach(processEvent);
  }catch(e){}
  renderCalls("","");
  const stream=new EventSource("/api/events/stream");
  stream.onmessage=e=>{try{processEvent(JSON.parse(e.data));}catch(_) {}};
  stream.onerror=()=>{};
  setInterval(refreshStatus,1000);
}
initial();
