// EssentialWatch - configuration web page (colours and fonts as on essentialsail.com)
#pragma once
#include <pgmspace.h>

const char WEB_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>EssentialWatch</title>
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
#fwlog{white-space:pre-wrap;background:var(--muted);border-radius:10px;padding:10px 12px;margin:8px 0 0;max-height:220px;overflow:auto;font-size:14px}
.keys .btn{padding:10px 0}
.hint{font-size:13px;color:var(--mfg)}
.bar{height:10px;border-radius:5px;background:var(--muted);margin-top:10px;overflow:hidden}
.bar div{height:100%;width:0;background:linear-gradient(90deg,var(--primary),var(--accent))}
.warn{color:var(--bad);font-weight:700}
.btn.danger{background:var(--bad);border-color:var(--bad)}
</style></head><body>
<header><div class="top">
 <div class="brand"><div class="logo"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2"><path d="M12 3v15M12 3l7 13h-7M12 6L6 16h6M4 20h16"/></svg></div>
 <div><h1>EssentialWatch</h1><div class="sub">Configuration</div></div></div>
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
 <div class="actions"><button class="btn alt small" type="button" id="nmeascan">Search NMEA server</button></div>
 <select id="nmealist" hidden></select>
 <div class="hint" id="nmeamsg">Searches this WiFi network for NMEA servers (ports 10110, 10111, 10112, 2000, 4242, 3000). Common ports: 10110, 2000, 4242 (see your gateway).</div>
</fieldset>
<fieldset><legend>Display</legend>
 <div class="row"><div><label for="dalarm">Minimum depth (m)</label><input id="dalarm" type="number" step="0.1" min="0" max="100"></div>
 <div><label for="rot">Orientation</label><select id="rot"><option value="0">Landscape</option><option value="2">Landscape, rotated 180&deg;</option></select></div></div>
 <div class="hint">Minimum depth: below this depth the depth turns red and the alarm starts (flashing frame on the watch; Boot = snooze 5 minutes).</div>
 <label for="tz">Time zone (POSIX string, automatic daylight saving)</label><input id="tz" maxlength="63" placeholder="CET-1CEST,M3.5.0,M10.5.0/3">
 <div class="hint">Time comes from NTP (if the WiFi has Internet) or from the GPS (NMEA RMC/ZDA). Italy: CET-1CEST,M3.5.0,M10.5.0/3 &middot; UTC: UTC0</div>
 <div class="check"><input type="checkbox" id="night"><span>Night mode (inverted colours)</span></div>
</fieldset>
<fieldset><legend>Date and time</legend>
 <div class="hint">Watch time: <b id="wtime">--</b> <span id="wsrc"></span></div>
 <div class="row"><input id="tset" type="datetime-local" step="1"><button class="btn alt small" type="button" id="tgo">Set time</button><button class="btn alt small" type="button" id="tpc">Use computer time</button></div>
 <div class="hint">The time is kept by the watch battery-backed clock. NTP (if the WiFi has Internet) and the GPS (NMEA) correct it automatically.</div>
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
 <button class="btn alt" data-k="right">Right</button><button class="btn alt" data-k="left">Left</button><button class="btn alt" data-k="dtap">Double Tap</button>
 <button class="btn alt" data-k="power">Power</button><button class="btn alt" data-k="boot">Boot</button>
</div>
<p class="hint">The picture refreshes every 5 s. Right / Left = swipe to the previous / next page &middot; Double Tap = day / night &middot; Power = screen on / off (hold the real button 4 s to power the watch off) &middot; Boot = snooze the collision alarm for 5 minutes.</p>
</section>

<section class="card"><h2>Firmware</h2>
<p>Installed version <b id="fwv">...</b></p>
<div class="actions"><button class="btn" id="fwfind" type="button">Search firmware</button></div>
<label for="fwsel">Firmware on GitHub (latest first)</label>
<select id="fwsel" disabled><option>Press "Search firmware"</option></select>
<pre id="fwlog" hidden></pre>
<div class="actions"><button class="btn" id="fwgo" type="button" disabled>Install selected firmware</button></div>
<details style="margin-top:10px"><summary class="hint">Install a .bin file from this computer</summary>
<label for="fwfile">Firmware file (.bin)</label><input id="fwfile" type="file" accept=".bin">
<div class="actions"><button class="btn alt" id="fwfilego" type="button">Install file</button></div></details>
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
 +tile('Depth',s.depth!=null?s.depth+' m':'--')+tile('Battery',s.batt!=null?`${s.batt}% · ${s.vbat} V${s.chg?' · charging':''}`:(s.pmu?'no battery':'--'))
 +tile('Watch time',s.time||'--')+(s.alert?tile('Collision alarm','ACTIVE'):s.snooze?tile('Alarm snoozed',Math.floor(s.snooze/60)+':'+String(s.snooze%60).padStart(2,'0')):'')
 +(s.ap?tile('Setup WiFi',s.ap):'');
 $('wtime').textContent=s.time||'not set';$('wsrc').textContent=s.time&&s.tsrc?'('+s.tsrc+')':''}catch(e){}}
async function load(){cfg=await api('/api/config');
 for(const k of['ssid','host','port','dalarm','name','tz'])$(k).value=cfg[k];
 $('rot').value=cfg.rot;$('night').checked=cfg.night;$('pass').value='';$('webpass').value='';
 $('pass').placeholder=cfg.pass_set?'(unchanged)':'(none)';$('webpass2').value='';$('fwv').textContent=cfg.fw;
 $('pwdef').innerHTML=cfg.webpass_default?'<span class="warn">The password is still the default one (admin): change it.</span>':'';
 $('openn').checked=false}
async function save(andRestart){
 const b={name:$('name').value.trim(),ssid:$('ssid').value,host:$('host').value.trim(),
  port:+$('port').value,dalarm:+$('dalarm').value,tz:$('tz').value.trim(),night:$('night').checked,rot:+$('rot').value};
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
function fwUpload(blob,name){const m=$('fwmsg');
 const fd=new FormData();fd.append('firmware',blob,name);const x=new XMLHttpRequest();
 x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(100*e.loaded/e.total);$('fwbar').style.width=p+'%';m.textContent=`Uploading ${p}%...`;m.className='hint'}};
 x.onload=()=>{let r={};try{r=JSON.parse(x.responseText)}catch(e){}
  if(x.status==200&&r.ok){m.textContent='Installed. The device is restarting: this page reloads in 30 s.';m.className='hint';setTimeout(()=>location.reload(),30000)}
  else{m.textContent='Update failed: '+(r.error||x.status);m.className='warn';$('fwbar').style.width='0'}};
 x.onerror=()=>{m.textContent='Connection lost during the upload';m.className='warn'};
 x.open('POST',U('/api/update'));x.send(fd);$('fwgo').disabled=true;$('fwfilego').disabled=true;
 setTimeout(()=>{$('fwgo').disabled=!rels.length;$('fwfilego').disabled=false},40000)}
$('fwfilego').onclick=()=>{const f=$('fwfile').files[0];const m=$('fwmsg');
 if(!f){m.textContent='Choose a .bin file first';m.className='warn';return}
 if(!confirm(`Install ${f.name} (${Math.round(f.size/1024)} KB)? The device restarts at the end.`))return;fwUpload(f,f.name)};
// Firmware on GitHub: every version is a Release with the .bin file and the changelog in its notes
const GH='https://api.github.com/repos/davideschiavon/EssentialWatch/releases?per_page=30';let rels=[];
function relAsset(r){return(r.assets||[]).find(a=>/\.bin$/i.test(a.name)&&!/merged|bootloader|partitions/i.test(a.name))}
function relShow(){const r=rels[$('fwsel').selectedIndex];if(!r)return;const L=$('fwlog');L.hidden=false;
 L.textContent=(r.name||r.tag_name)+'  ('+(r.published_at||'').slice(0,10)+')\n\n'+(r.body||'(no changelog)').replace(/\r/g,'').replace(/^#+\s*/gm,'')}
$('fwsel').onchange=relShow;
$('fwfind').onclick=async()=>{const m=$('fwmsg');m.textContent='Searching GitHub...';m.className='hint';$('fwfind').disabled=true;
 try{const r=await fetch(GH,{headers:{Accept:'application/vnd.github+json'}});if(!r.ok)throw new Error('GitHub answered '+r.status);
  rels=(await r.json()).filter(x=>!x.draft&&relAsset(x));
  if(!rels.length){m.textContent='No firmware found on GitHub.';m.className='warn';return}
  const sel=$('fwsel');sel.innerHTML='';
  rels.forEach((x,i)=>{const o=document.createElement('option');const v=(x.tag_name||'').replace(/^v/,'');
   o.textContent=(x.name||x.tag_name)+(i==0?'  (latest)':'')+(cfg.fw&&cfg.fw.startsWith(v)?'  (installed)':'');sel.appendChild(o)});
  sel.disabled=false;$('fwgo').disabled=false;relShow();
  m.textContent=`${rels.length} firmware found. Pick one, read the changelog, then install.`;m.className='hint'}
 catch(e){m.textContent='Cannot reach GitHub ('+e.message+'). The computer needs Internet access to list them; you can also install a .bin file.';m.className='warn'}
 $('fwfind').disabled=false};
$('fwgo').onclick=async()=>{const r=rels[$('fwsel').selectedIndex],m=$('fwmsg');if(!r)return;const a=relAsset(r);
 if(!confirm(`Install ${r.name||r.tag_name}? The watch downloads it from GitHub (it needs a WiFi with Internet), then restarts. Settings are kept.`))return;
 m.textContent='The watch is downloading the firmware from GitHub (about 30-60 s)...';m.className='hint';$('fwgo').disabled=true;
 try{const x=await fetch(U('/api/fwurl'),{method:'POST',headers:{'Content-Type':'application/json'},
   body:JSON.stringify({url:a.browser_download_url,sha256:(a.digest||'').replace(/^sha256:/,'')})});
  const j=await x.json();
  if(j.ok){m.textContent='Installed. The watch is restarting: this page reloads in 30 s.';m.className='hint';setTimeout(()=>location.reload(),30000)}
  else{m.innerHTML='Install failed: '+j.error+'. You can also download <a href="'+a.browser_download_url+'" target="_blank">'+a.name+'</a> and use "Install a .bin file".';m.className='warn';$('fwgo').disabled=false}}
 catch(e){m.textContent='Connection lost: the watch is probably restarting. This page reloads in 30 s.';m.className='hint';setTimeout(()=>location.reload(),30000)}};
// Search NMEA server: the watch scans its own network, this page polls the result
let nmeaHits=[];
function nmeaMsg(t,ok){const m=$('nmeamsg');m.textContent=t;m.className=ok?'hint':'warn'}
$('nmealist').onchange=()=>{const h=nmeaHits[$('nmealist').selectedIndex];if(h){$('host').value=h.ip;$('port').value=h.port}};
$('nmeascan').onclick=async()=>{const b=$('nmeascan');b.disabled=true;$('nmealist').hidden=true;nmeaMsg('Searching the network...',true);
 try{const r=await api('/api/nmeascan',{method:'POST'});if(!r.ok){nmeaMsg(r.error,false);b.disabled=false;return}
  const t=setInterval(async()=>{try{const s=await api('/api/nmeascan');
   if(s.running){nmeaMsg(`Searching the network... ${s.progress}%`,true);return}
   clearInterval(t);b.disabled=false;
   nmeaHits=s.hits.sort((a,b)=>b.nmea-a.nmea);
   if(!nmeaHits.length){nmeaMsg('No NMEA server found. Check that the server is on and connected to the same WiFi network.',false);return}
   const L=$('nmealist');L.innerHTML='';
   nmeaHits.forEach(h=>{const o=document.createElement('option');o.textContent=`${h.ip}:${h.port}  ${h.nmea?'NMEA ('+h.sample+')':'open, no data'}`;L.appendChild(o)});
   L.hidden=false;L.selectedIndex=0;L.onchange();
   nmeaMsg(`${nmeaHits.length} found. The first one is filled in: press Save to use it.`,true)}catch(e){}},1000)}
 catch(e){nmeaMsg('The search could not start',false);b.disabled=false}};
// time
async function setTime(ms){const r=await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({epoch:Math.round(ms/1000)})});
 msg(r.ok?'Time set.':r.error,!!r.ok);status()}
$('tgo').onclick=()=>{const v=$('tset').value;if(!v){msg('Pick a date and time first',false);return}setTime(new Date(v).getTime())};
$('tpc').onclick=()=>setTime(Date.now());
(()=>{const d=new Date(),p=n=>String(n).padStart(2,'0');$('tset').value=`${d.getFullYear()}-${p(d.getMonth()+1)}-${p(d.getDate())}T${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}`})();
function shot(){$('scr').src=U('/screen.bmp?'+Date.now())}
load();status();shot();setInterval(status,3000);setInterval(()=>{if(!document.hidden)shot()},5000);
</script></body></html>)rawliteral";
