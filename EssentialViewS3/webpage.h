// EssentialView - pagina web di configurazione (colori e font come essentialsail.com)
#pragma once
#include <pgmspace.h>

const char WEB_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>EssentialView Configuration</title>
<link rel="stylesheet" media="print" onload="this.media='all'"
 href="https://fonts.googleapis.com/css2?family=Baloo+2:wght@600;700;800&family=Nunito+Sans:wght@400;600;700&display=swap">
<style>
:root{--bg:#F2FAFE;--fg:#06233D;--card:#fff;--primary:#0087F3;--accent:#1AD1D1;--muted:#DFF2FD;
--mfg:#4E6D83;--border:#C5DBE9;--navy:#001B3A;--navyfg:#F2FAFE;--ok:#0a8f4d;--bad:#d63a3a}
@media (prefers-color-scheme:dark){:root{--bg:#030B1A;--fg:#E9F1F8;--card:#0B1A2E;--muted:#13233A;
--mfg:#8DA4BA;--border:#22344C;--primary:#2A95F5;--accent:#2BC4C4}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.45 "Nunito Sans",system-ui,-apple-system,"Segoe UI",sans-serif}
h1,h2{font-family:"Baloo 2","Nunito Sans",system-ui,sans-serif;margin:0;line-height:1.15}
header{background:var(--navy);color:var(--navyfg);position:relative}
header::after{content:"";position:absolute;left:0;right:0;bottom:0;height:4px;
background:linear-gradient(90deg,var(--primary),var(--accent),var(--primary))}
.top{max-width:1040px;margin:0 auto;padding:18px 16px 22px;display:flex;gap:12px;align-items:center;justify-content:space-between;flex-wrap:wrap}
.brand{display:flex;gap:12px;align-items:center}
.logo{width:42px;height:42px;border-radius:12px;background:linear-gradient(135deg,var(--primary),var(--accent));display:grid;place-items:center}
h1{font-size:28px;font-weight:800}
.sub{color:var(--accent);font-weight:700;font-size:14px;letter-spacing:.04em;text-transform:uppercase}
.pill{border:1px solid rgba(242,250,254,.25);border-radius:999px;padding:4px 12px;font-size:14px}
main{max-width:1040px;margin:0 auto;padding:16px;display:grid;gap:16px;grid-template-columns:1fr}
@media(min-width:900px){main{grid-template-columns:1.15fr 1fr;align-items:start}.wide{grid-column:1/-1}}
.col{display:grid;gap:16px;align-content:start}
.card{background:var(--card);border:1px solid var(--border);border-radius:14px;padding:16px;box-shadow:0 8px 30px -12px rgba(0,60,120,.22)}
h2{font-size:21px;font-weight:700;margin-bottom:10px}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:8px}
.tile{background:var(--muted);border-radius:10px;padding:8px 10px}
.tile b{display:block;font-size:18px;font-family:"Baloo 2",system-ui,sans-serif;overflow-wrap:anywhere}
.tile span{font-size:13px;color:var(--mfg);font-weight:600}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px;background:var(--bad)}
.dot.on{background:var(--ok)}
label{display:block;font-weight:700;font-size:14px;margin:10px 0 4px}
input,select{width:100%;font:inherit;color:var(--fg);background:var(--bg);border:1px solid var(--border);border-radius:8px;padding:9px 10px}
input:focus,select:focus{outline:2px solid var(--primary);outline-offset:1px}
.row{display:flex;gap:8px;align-items:center}.row>*{flex:1}
.check{display:flex;gap:8px;align-items:center;font-weight:600;font-size:14px;margin-top:6px}
.check input{width:auto}
fieldset{border:1px solid var(--border);border-radius:12px;padding:4px 12px 12px;margin:0 0 12px}
legend{font-family:"Baloo 2",system-ui,sans-serif;font-weight:700;color:var(--primary);padding:0 6px}
.btn{font:inherit;font-weight:800;border-radius:8px;padding:10px 16px;border:1px solid var(--primary);background:var(--primary);color:#fff;cursor:pointer}
.btn.alt{background:transparent;color:var(--primary)}
.btn.small{padding:6px 10px;flex:0 0 auto}
.btn:disabled{opacity:.5}
.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:8px}
#msg{margin-top:10px;font-weight:700;min-height:1.4em}
#msg.ok{color:var(--ok)}#msg.bad{color:var(--bad)}
.screen{width:100%;height:auto;border-radius:8px;border:1px solid var(--border);background:#fff;image-rendering:auto}
.keys{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:10px}
.keys .btn{padding:10px 0}
.hint{font-size:13px;color:var(--mfg)}
.bar{height:10px;border-radius:5px;background:var(--muted);margin-top:10px;overflow:hidden}
.bar div{height:100%;width:0;background:linear-gradient(90deg,var(--primary),var(--accent))}
.warn{color:var(--bad);font-weight:700}
.btn.danger{background:var(--bad);border-color:var(--bad)}
</style></head><body>
<header><div class="top">
 <div class="brand"><div class="logo"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2"><path d="M12 3v15M12 3l7 13h-7M12 6L6 16h6M4 20h16"/></svg></div>
 <div><h1>EssentialView</h1><div class="sub">Configuration</div></div></div>
 <div class="pill" id="who">...</div>
</div></header>
<main>
<section class="card wide"><h2>Status</h2><div class="tiles" id="tiles"></div></section>

<section class="card settings"><h2>Settings</h2>
<form id="f" onsubmit="return false">
<fieldset><legend>Boat WiFi</legend>
 <label for="ssid">Network (2.4 GHz only)</label>
 <div class="row"><input id="ssid" list="nets" autocomplete="off"><button class="btn alt small" type="button" id="scan">Scan</button></div>
 <datalist id="nets"></datalist>
 <label for="pass">Password</label>
 <input id="pass" type="password" autocomplete="new-password">
 <div class="check"><input type="checkbox" id="showp"><span>Show</span>&nbsp;&nbsp;<input type="checkbox" id="openn"><span>Open network (no password)</span></div>
</fieldset>
<fieldset><legend>NMEA 0183 server (TCP)</legend>
 <div class="row"><div><label for="host">IP address</label><input id="host" placeholder="192.168.1.40"></div>
 <div style="flex:.5"><label for="port">Port</label><input id="port" type="number" min="1" max="65535"></div></div>
 <div class="hint">Common ports: 10110, 2000, 4242 (see your gateway).</div>
</fieldset>
<fieldset><legend>Display</legend>
 <div class="row"><div><label for="dalarm">Depth alarm (m)</label><input id="dalarm" type="number" step="0.1" min="0" max="100"></div>
 <div><label for="rot">Orientation</label><select id="rot"><option value="0">Landscape</option><option value="2">Landscape, rotated 180&deg;</option></select></div></div>
 <div class="check"><input type="checkbox" id="night"><span>Night mode (inverted colours)</span></div>
</fieldset>
<fieldset><legend>Device</legend>
 <label for="name">Name (web address http://name.local)</label><input id="name" maxlength="24">
</fieldset>
<fieldset><legend>Administrator login</legend>
 <div class="hint">User <b>admin</b>. <span id="pwdef"></span></div>
 <div class="row"><div><label for="webpass">New password</label><input id="webpass" type="password" autocomplete="new-password" placeholder="(unchanged)"></div>
 <div><label for="webpass2">Repeat</label><input id="webpass2" type="password" autocomplete="new-password"></div></div>
</fieldset>
<div class="actions">
 <button class="btn" id="save">Save</button>
 <button class="btn alt" id="saver">Save and restart</button>
</div>
<div id="msg"></div>
</form></section>

<div class="col">
<section class="card"><h2>Screen</h2>
<img class="screen" id="scr" alt="display" width="410" height="502">
<div class="keys">
 <span></span><button class="btn alt" data-k="U">UP</button><span></span>
 <button class="btn alt" data-k="B">BACK</button><button class="btn" data-k="O">OK</button><button class="btn alt" data-k="D">DOWN</button>
 <button class="btn alt" data-k="L" style="grid-column:1/3">OK long (menu)</button><button class="btn alt" data-k="K">BACK long</button>
</div>
<p class="hint">The picture refreshes every 5 s. Buttons act like the device buttons.</p>
</section>

<section class="card"><h2>Firmware</h2>
<p>Installed version <b id="fwv">...</b></p>
<label for="fwfile">New firmware file (.bin)</label>
<input id="fwfile" type="file" accept=".bin">
<div class="actions"><button class="btn" id="fwgo" type="button">Install firmware</button></div>
<div class="bar"><div id="fwbar"></div></div>
<div id="fwmsg" class="hint">The device restarts by itself when the update is done (about 30 s). Settings are kept.</div>
</section>

<section class="card"><h2>Maintenance</h2>
<div class="actions">
 <button class="btn alt" id="restart" type="button">Restart (reset)</button>
 <button class="btn alt" id="backup" type="button">Download backup</button>
</div>
<label for="rsfile">Restore configuration from a backup file (.json)</label>
<input id="rsfile" type="file" accept=".json,application/json">
<div class="actions"><button class="btn alt" id="restore" type="button">Restore configuration</button></div>
<div class="actions"><button class="btn danger" id="factory" type="button">Factory reset (hard reset)</button></div>
<div id="mmsg" class="hint">The backup contains the boat WiFi password, not the administrator password. Factory reset erases every setting:
the device restarts with the open WiFi "EssentialView Network" and login admin / admin.</div>
</section>
</div>
</main>
<script>
const $=id=>document.getElementById(id);let cfg={};
function msg(t,ok){const m=$('msg');m.textContent=t;m.className=ok?'ok':'bad'}
const U=p=>location.origin+p;
async function api(p,o){const r=await fetch(U(p),o);return r.json()}
function tile(l,v){return `<div class="tile"><span>${l}</span><b>${v??'--'}</b></div>`}
async function status(){try{const s=await api('/api/status');
 $('who').textContent=`${s.name}.local · fw ${s.fw}`;
 const w=s.ip?`<span class="dot on"></span>${s.ssid}`:`<span class="dot"></span>${s.wifi}`;
 $('tiles').innerHTML=tile('WiFi',w)+tile('IP address',s.ip||(s.ap_ip?s.ap_ip+' (setup)':'--'))
 +tile('Signal',s.ip?s.rssi+' dBm':'--')
 +tile('NMEA server',`<span class="dot ${s.server?'on':''}"></span>${s.host||'not set'}:${s.port}`)
 +tile('Sentences ok',s.ok)+tile('AWA / AWS',s.awa!=null?`${s.awa}° / ${s.aws} kn`:'--')
 +tile('TWA / TWS',s.twa!=null?`${s.twa}° / ${s.tws} kn`:'--')+tile('SOG / STW',`${s.sog??'--'} / ${s.stw??'--'} kn`)
 +tile('Depth',s.depth!=null?s.depth+' m':'--')+(s.ap?tile('Setup WiFi',s.ap):'')}catch(e){}}
async function load(){cfg=await api('/api/config');
 for(const k of['ssid','host','port','dalarm','name'])$(k).value=cfg[k];
 $('rot').value=cfg.rot;$('night').checked=cfg.night;$('pass').value='';$('webpass').value='';
 $('pass').placeholder=cfg.pass_set?'(unchanged)':'(none)';$('webpass2').value='';$('fwv').textContent=cfg.fw;
 $('pwdef').innerHTML=cfg.webpass_default?'<span class="warn">The password is still the default one (admin): change it.</span>':'';
 $('openn').checked=false}
async function save(andRestart){
 const b={name:$('name').value.trim(),ssid:$('ssid').value,host:$('host').value.trim(),
  port:+$('port').value,dalarm:+$('dalarm').value,night:$('night').checked,rot:+$('rot').value};
 if($('openn').checked)b.pass='';else if($('pass').value)b.pass=$('pass').value;
 if($('webpass').value){if($('webpass').value!==$('webpass2').value){msg('The two passwords are different',false);return}
  b.webpass=$('webpass').value}
 const r=await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
 if(!r.ok){msg(r.error,false);return}
 if(andRestart){await restart(b.name);return}
 msg(r.restart?'Saved. WiFi and name changes apply after a restart.':'Saved.',true);load()}
async function restart(n){msg(`Restarting... then open http://${n||cfg.name}.local (about 20 s).`,true);
 try{await fetch(U('/api/restart'),{method:'POST'})}catch(e){}}
$('save').onclick=()=>save(false);$('saver').onclick=()=>save(true);
$('restart').onclick=()=>{if(confirm('Restart the device? Unsaved changes are lost.'))restart()};
function mm(t,ok){const m=$('mmsg');m.textContent=t;m.className=ok?'hint':'warn'}
$('backup').onclick=async()=>{try{const b=await api('/api/backup');
 const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([JSON.stringify(b,null,2)],{type:'application/json'}));
 a.download=`${b.name}-config-${new Date().toISOString().slice(0,10)}.json`;a.click();mm('Backup downloaded.',true)}catch(e){mm('Backup failed',false)}};
$('restore').onclick=async()=>{const f=$('rsfile').files[0];if(!f){mm('Choose a backup file first',false);return}
 let b;try{b=JSON.parse(await f.text())}catch(e){mm('Not a valid backup file',false);return}
 if(b.type!=='essentialview-config'){mm('This is not an EssentialView backup',false);return}
 if(!confirm(`Restore the configuration of "${b.name}" (WiFi ${b.ssid||'none'}, server ${b.host||'-'}:${b.port}) and restart?`))return;
 const r=await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
 if(!r.ok){mm('Restore failed: '+r.error,false);return}
 mm(`Restored. Restarting... then open http://${b.name}.local`,true);try{await fetch(U('/api/restart'),{method:'POST'})}catch(e){}};
$('factory').onclick=async()=>{if(!confirm('FACTORY RESET: erase ALL settings (WiFi, server, name, admin password)?'))return;
 if(prompt('Type RESET to confirm')!=='RESET')return;
 try{await fetch(U('/api/factory'),{method:'POST'})}catch(e){}
 mm('Settings erased. The device restarts with the open WiFi "EssentialView Network" (http://192.168.4.1, admin / admin).',true)};
$('showp').onchange=e=>{$('pass').type=e.target.checked?'text':'password'};
$('scan').onclick=async()=>{$('scan').disabled=true;$('scan').textContent='...';
 try{const l=await api('/api/scan');$('nets').innerHTML=l.map(n=>`<option value="${n.ssid}">${n.rssi} dBm${n.open?' open':''}</option>`).join('');
 msg(`${l.length} networks found: pick one in the Network field.`,true)}catch(e){msg('Scan failed',false)}
 $('scan').disabled=false;$('scan').textContent='Scan'};
document.querySelectorAll('[data-k]').forEach(b=>b.onclick=async()=>{await fetch(U('/api/key?k='+b.dataset.k),{method:'POST'});setTimeout(shot,1500)});
$('fwgo').onclick=()=>{const f=$('fwfile').files[0];const m=$('fwmsg');
 if(!f){m.textContent='Choose a .bin file first';m.className='warn';return}
 if(!confirm(`Install ${f.name} (${Math.round(f.size/1024)} KB)? The device restarts at the end.`))return;
 const fd=new FormData();fd.append('firmware',f,f.name);const x=new XMLHttpRequest();
 x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(100*e.loaded/e.total);$('fwbar').style.width=p+'%';m.textContent=`Uploading ${p}%...`;m.className='hint'}};
 x.onload=()=>{let r={};try{r=JSON.parse(x.responseText)}catch(e){}
  if(x.status==200&&r.ok){m.textContent='Installed. The device is restarting: this page reloads in 30 s.';m.className='hint';setTimeout(()=>location.reload(),30000)}
  else{m.textContent='Update failed: '+(r.error||x.status);m.className='warn';$('fwbar').style.width='0'}};
 x.onerror=()=>{m.textContent='Connection lost during the upload';m.className='warn'};
 x.open('POST',U('/api/update'));x.send(fd);$('fwgo').disabled=true;setTimeout(()=>$('fwgo').disabled=false,40000)};
function shot(){$('scr').src=U('/screen.bmp?'+Date.now())}
load();status();shot();setInterval(status,3000);setInterval(()=>{if(!document.hidden)shot()},5000);
</script></body></html>)rawliteral";
