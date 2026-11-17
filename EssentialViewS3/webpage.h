// EssentialWatch - configuration web page (colours and fonts as on essentialsail.com)
// Languages: English, French, Spanish. Every visible text has an id (data-t / data-tp attributes, t() in the
// script) in the D table at the start of the script; firmware messages are translated through the E table.
#pragma once
#include <pgmspace.h>

const char WEB_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>EssentialWatch</title>
<style>
@font-face{font-family:"Essentialsail Display";src:url("/fonts/EssentialsailDisplay-Bold.ttf") format("truetype");font-weight:700;font-style:normal;font-display:swap}
@font-face{font-family:"Essentialsail Text";src:url("/fonts/EssentialsailText-Regular.ttf") format("truetype");font-weight:400;font-style:normal;font-display:swap}
:root{--bg:#F2FAFE;--fg:#06233D;--card:#fff;--primary:#0087F3;--accent:#1AD1D1;--muted:#DFF2FD;
--mfg:#4E6D83;--border:#C5DBE9;--navy:#001B3A;--navyfg:#F2FAFE;--ok:#0a8f4d;--bad:#d63a3a}
@media (prefers-color-scheme:dark){:root{--bg:#030B1A;--fg:#E9F1F8;--card:#0B1A2E;--muted:#13233A;
--mfg:#8DA4BA;--border:#22344C;--primary:#2A95F5;--accent:#2BC4C4}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.45 "Essentialsail Text",system-ui,-apple-system,"Segoe UI",sans-serif;font-weight:400}
h1,h2{font-family:"Essentialsail Display","Essentialsail Text",system-ui,sans-serif;font-weight:700;margin:0;line-height:1.15}
header{background:var(--navy);color:var(--navyfg);position:relative}
header::after{content:"";position:absolute;left:0;right:0;bottom:0;height:4px;
background:linear-gradient(90deg,var(--primary),var(--accent),var(--primary))}
.top{max-width:1040px;margin:0 auto;padding:18px 16px 22px;display:flex;gap:12px;align-items:center;justify-content:space-between;flex-wrap:wrap}
.brand{display:flex;gap:12px;align-items:center}
.logo{width:42px;height:42px;border-radius:0;background:linear-gradient(135deg,var(--primary),var(--accent));display:grid;place-items:center}
h1{font-size:28px;font-weight:800}
.sub{color:var(--accent);font-family:"Essentialsail Display",system-ui,sans-serif;font-weight:700;font-size:14px;letter-spacing:.04em;text-transform:uppercase}
.pill{border:1px solid rgba(242,250,254,.25);border-radius:0;padding:4px 12px;font-size:14px}
.tr{display:flex;gap:8px;align-items:center}
#langsel{width:auto;padding:4px 8px;background:transparent;color:var(--navyfg);border-color:rgba(242,250,254,.35)}
#langsel option{color:#06233D}
main{max-width:1040px;margin:0 auto;padding:16px;display:grid;gap:16px;grid-template-columns:1fr}
@media(min-width:900px){main{grid-template-columns:1.15fr 1fr;align-items:start}.wide{grid-column:1/-1}}
.col{display:grid;gap:16px;align-content:start}
.card{background:var(--card);border:1px solid var(--border);border-radius:0;padding:16px;box-shadow:0 8px 30px -12px rgba(0,60,120,.22)}
h2{font-size:21px;font-weight:700;margin-bottom:10px}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:8px}
.tile{background:var(--muted);border-radius:0;padding:8px 10px}
.tile b{display:block;font-size:18px;font-family:"Essentialsail Display",system-ui,sans-serif;overflow-wrap:anywhere}
.tile span{font-size:13px;color:var(--mfg);font-weight:600}
.dot{display:inline-block;width:10px;height:10px;border-radius:0;margin-right:6px;background:var(--bad)}
.dot.on{background:var(--ok)}
label{display:block;font-family:"Essentialsail Display",system-ui,sans-serif;font-weight:700;font-size:14px;margin:10px 0 4px}
input,select{width:100%;font:inherit;color:var(--fg);background:var(--bg);border:1px solid var(--border);border-radius:0;padding:9px 10px}
input:focus,select:focus{outline:2px solid var(--primary);outline-offset:1px}
.row{display:flex;gap:8px;align-items:center}.row>*{flex:1}
.check{display:flex;gap:8px;align-items:center;font-weight:600;font-size:14px;margin-top:6px}
.check input{width:auto}
fieldset{border:1px solid var(--border);border-radius:0;padding:4px 12px 12px;margin:0 0 12px}
legend{font-family:"Essentialsail Display",system-ui,sans-serif;font-weight:700;color:var(--primary);padding:0 6px}
.btn{font:inherit;font-family:"Essentialsail Display",system-ui,sans-serif;font-weight:700;border-radius:0;padding:10px 16px;border:1px solid var(--primary);background:var(--primary);color:#fff;cursor:pointer}
.btn.alt{background:transparent;color:var(--primary)}
.btn.small{padding:6px 10px;flex:0 0 auto}
.btn:disabled{opacity:.5}
.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:8px}
#msg{margin-top:10px;font-weight:700;min-height:1.4em}
#msg.ok{color:var(--ok)}#msg.bad{color:var(--bad)}
.screen{width:100%;height:auto;border-radius:0;border:1px solid var(--border);background:#fff;image-rendering:auto}
.keys{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:10px}
#fwlog{white-space:pre-wrap;background:var(--muted);border-radius:0;padding:10px 12px;margin:8px 0 0;max-height:220px;overflow:auto;font-size:14px}
.keys .btn{padding:10px 0}
.hint{font-size:13px;color:var(--mfg)}
.bar{height:10px;border-radius:0;background:var(--muted);margin-top:10px;overflow:hidden}
.bar div{height:100%;width:0;background:linear-gradient(90deg,var(--primary),var(--accent))}
.warn{color:var(--bad);font-weight:700}
.btn.danger{background:var(--bad);border-color:var(--bad)}
</style></head><body>
<header><div class="top">
 <div class="brand"><div class="logo"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2"><path d="M12 3v15M12 3l7 13h-7M12 6L6 16h6M4 20h16"/></svg></div>
 <div><h1>EssentialWatch</h1><div class="sub" data-t="sub"></div></div></div>
 <div class="tr"><select id="langsel" aria-label="Language"><option value="en">English</option><option value="fr">Français</option><option value="es">Español</option></select>
 <div class="pill" id="who">...</div></div>
</div></header>
<main>
<section class="card wide"><h2 data-t="status"></h2><div class="tiles" id="tiles"></div></section>

<section class="card settings"><h2 data-t="settings"></h2>
<form id="f" onsubmit="return false">
<fieldset><legend data-t="boatwifi"></legend>
 <label for="ssid" data-t="network"></label>
 <div class="row"><input id="ssid" list="nets" autocomplete="off"><button class="btn alt small" type="button" id="scan" data-t="scan"></button></div>
 <datalist id="nets"></datalist>
 <label for="pass" data-t="password"></label>
 <input id="pass" type="password" autocomplete="new-password">
 <div class="check"><input type="checkbox" id="showp"><span data-t="show"></span>&nbsp;&nbsp;<input type="checkbox" id="openn"><span data-t="opennet"></span></div>
</fieldset>
<fieldset><legend data-t="nmea"></legend>
 <div class="row"><div><label for="host" data-t="ip"></label><input id="host" placeholder="192.168.1.40"></div>
 <div style="flex:.5"><label for="port" data-t="port"></label><input id="port" type="number" min="1" max="65535"></div></div>
 <div class="actions"><button class="btn alt small" type="button" id="nmeascan" data-t="nmeascan"></button></div>
 <select id="nmealist" hidden></select>
 <div class="hint" id="nmeamsg" data-t="nmeahint"></div>
</fieldset>
<fieldset><legend data-t="mobleg"></legend>
 <div class="check"><input type="checkbox" id="mobon"><span data-t="mobon"></span></div>
 <label for="mobcode" data-t="mobcode"></label>
 <div class="row"><input id="mobcode" inputmode="numeric" maxlength="7" placeholder="482 913" autocomplete="off"><button class="btn small" type="button" id="mobpair" data-t="mobpair"></button></div>
 <div class="hint" id="mobpairmsg"></div>
 <label for="moblink" data-t="moblink"></label><input id="moblink" placeholder="essentialsail-watch://pair?v=1&amp;id=...&amp;key=...&amp;host=...&amp;port=10120" autocomplete="off">
 <div class="row"><div><label for="mobhost" data-t="mobhost"></label><input id="mobhost" placeholder="192.168.1.50" maxlength="63"></div>
 <div style="flex:.5"><label for="mobport" data-t="port"></label><input id="mobport" type="number" min="1" max="65535"></div></div>
 <div class="row"><div style="flex:.5"><label for="mobid" data-t="mobid"></label><input id="mobid" maxlength="8" placeholder="W7K2" autocomplete="off"></div>
 <div><label for="mobkey" data-t="mobkey"></label><input id="mobkey" type="password" maxlength="64" autocomplete="new-password" data-tp="unchanged"></div></div>
 <div class="actions"><button class="btn alt small" type="button" id="mobscan" data-t="mobscan"></button></div>
 <select id="moblist" hidden></select>
 <div class="hint" id="mobscanmsg"></div>
 <label for="mobowner" data-t="mobowner"></label><input id="mobowner" maxlength="24">
 <div class="hint" data-t="mobhint"></div>
 <div class="hint" id="mobstat"></div>
</fieldset>
<fieldset><legend data-t="display"></legend>
 <label for="lang" data-t="lang"></label>
 <select id="lang"><option value="en">English</option><option value="fr">Français</option><option value="es">Español</option></select>
 <div class="row"><div><label for="dalarm" data-t="dalarm"></label><input id="dalarm" type="number" step="0.1" min="0" max="100"></div>
 <div><label for="rot" data-t="rot"></label><select id="rot"><option value="0" data-t="rot0"></option><option value="2" data-t="rot2"></option></select></div></div>
 <div class="row"><div><label for="cpa" data-t="cpa"></label><input id="cpa" type="number" step="0.05" min="0.05" max="5"></div>
 <div><label for="tcpa" data-t="tcpa"></label><input id="tcpa" type="number" step="1" min="1" max="120"></div></div>
 <div class="hint" data-t="cpahint"></div>
 <div class="hint" data-t="dhint"></div>
 <label for="tz" data-t="tz"></label><input id="tz" maxlength="63" placeholder="CET-1CEST,M3.5.0,M10.5.0/3">
 <div class="hint" data-t="tzhint"></div>
 <div class="row"><div><label for="sleep" data-t="sleep"></label><select id="sleep"><option value="0" data-t="never"></option><option value="15">15 s</option><option value="30">30 s</option><option value="60">1 min</option><option value="120">2 min</option><option value="300">5 min</option></select></div>
 <div class="check" style="margin-top:28px"><input type="checkbox" id="raise"><span data-t="raise"></span></div></div>
 <div class="hint" data-t="sleephint"></div>
 <label for="theme" data-t="theme"></label>
 <select id="theme"><option value="0" data-t="th0"></option><option value="1" data-t="th1"></option><option value="2" data-t="th2"></option></select>
</fieldset>
<fieldset><legend data-t="datetime"></legend>
 <div class="hint"><span data-t="wtime"></span> <b id="wtime">--</b> <span id="wsrc"></span></div>
 <div class="row"><input id="tset" type="datetime-local" step="1"><button class="btn alt small" type="button" id="tgo" data-t="tgo"></button><button class="btn alt small" type="button" id="tpc" data-t="tpc"></button></div>
 <div class="hint" data-t="thint"></div>
</fieldset>
<fieldset><legend data-t="device"></legend>
 <label for="name" data-t="name"></label><input id="name" maxlength="24">
 <label for="vib" data-t="vib"></label>
 <div class="row"><div><input id="vib" type="number" min="-1" max="48" step="1"></div>
 <div><button class="btn alt" type="button" id="vibtest" data-t="vibtest"></button></div></div>
 <div class="hint" data-t="vibhint"></div>
 <div class="row"><div><label for="gpsrx" data-t="gpsrx"></label><input id="gpsrx" type="number" min="-1" max="48" step="1"></div>
 <div><label for="gpsbaud" data-t="gpsbaud"></label><select id="gpsbaud"><option>4800</option><option>9600</option><option>19200</option><option>38400</option><option>57600</option><option>115200</option></select></div></div>
 <div class="hint" data-t="gpshint"></div>
</fieldset>
<fieldset><legend data-t="admin"></legend>
 <div class="hint"><span data-t="user"></span> <b>admin</b>. <span id="pwdef"></span> <span data-t="digest"></span></div>
 <div class="row"><div><label for="webpass" data-t="newpass"></label><input id="webpass" type="password" autocomplete="new-password" data-tp="unchanged"></div>
 <div><label for="webpass2" data-t="repeat"></label><input id="webpass2" type="password" autocomplete="new-password"></div></div>
</fieldset>
<div class="actions">
 <button class="btn" id="save" data-t="save"></button>
 <button class="btn alt" id="saver" data-t="saver"></button>
</div>
<div id="msg"></div>
</form></section>

<div class="col">
<section class="card"><h2 data-t="screen"></h2>
<img class="screen" id="scr" alt="display" width="410" height="502">
<div class="keys">
 <button class="btn alt" data-k="right" data-t="right"></button><button class="btn alt" data-k="left" data-t="left"></button><button class="btn alt" data-k="dtap" data-t="dtap"></button>
 <button class="btn alt" data-k="power">Power</button><button class="btn alt" data-k="boot">Boot</button>
</div>
<p class="hint" data-t="screenhint"></p>
</section>

<section class="card"><h2>Firmware</h2>
<p><span data-t="installed"></span> <b id="fwv">...</b></p>
<div class="actions"><button class="btn" id="fwfind" type="button" data-t="fwfind"></button></div>
<label for="fwsel" data-t="fwsel"></label>
<select id="fwsel" disabled><option data-t="fwpress"></option></select>
<pre id="fwlog" hidden></pre>
<div class="actions"><button class="btn" id="fwgo" type="button" disabled data-t="fwgo"></button></div>
<details style="margin-top:10px"><summary class="hint" data-t="fwfilesum"></summary>
<label for="fwfile" data-t="fwfile"></label><input id="fwfile" type="file" accept=".bin">
<div class="actions"><button class="btn alt" id="fwfilego" type="button" data-t="fwfilego"></button></div></details>
<div class="bar"><div id="fwbar"></div></div>
<div id="fwmsg" class="hint" data-t="fwmsg"></div>
</section>

<section class="card"><h2 data-t="maint"></h2>
<div class="actions">
 <button class="btn alt" id="restart" type="button" data-t="restart"></button>
 <button class="btn alt" id="backup" type="button" data-t="backup"></button>
</div>
<label for="rsfile" data-t="rsfile"></label>
<input id="rsfile" type="file" accept=".json,application/json">
<div class="actions"><button class="btn alt" id="restore" type="button" data-t="restore"></button></div>
<div class="actions"><button class="btn danger" id="factory" type="button" data-t="factory"></button></div>
<div id="mmsg" class="hint" data-t="mmsg"></div>
</section>
</div>
</main>
<script>
// ---------- texts: id -> [English, French, Spanish] ----------
const D={
sub:['Configuration','Configuration','Configuración'],
status:['Status','État','Estado'],
settings:['Settings','Réglages','Ajustes'],
boatwifi:['Boat WiFi','WiFi du bateau','WiFi del barco'],
network:['Network (2.4 GHz only)','Réseau (2,4 GHz seulement)','Red (solo 2,4 GHz)'],
scan:['Scan','Rechercher','Buscar'],
password:['Password','Mot de passe','Contraseña'],
show:['Show','Afficher','Mostrar'],
opennet:['Open network (no password)','Réseau ouvert (sans mot de passe)','Red abierta (sin contraseña)'],
nmea:['NMEA 0183 server (TCP)','Serveur NMEA 0183 (TCP)','Servidor NMEA 0183 (TCP)'],
ip:['IP address','Adresse IP','Dirección IP'],
port:['Port','Port','Puerto'],
nmeascan:['Search NMEA server','Chercher le serveur NMEA','Buscar servidor NMEA'],
nmeahint:['Searches this WiFi network for NMEA servers (ports 10110, 10111, 10112, 2000, 4242, 3000). Common ports: 10110, 2000, 4242 (see your gateway).',
 'Cherche des serveurs NMEA sur ce réseau WiFi (ports 10110, 10111, 10112, 2000, 4242, 3000). Ports courants : 10110, 2000, 4242 (voir votre passerelle).',
 'Busca servidores NMEA en esta red WiFi (puertos 10110, 10111, 10112, 2000, 4242, 3000). Puertos habituales: 10110, 2000, 4242 (vea su pasarela).'],
mobleg:['Man overboard (MOB) link to EssentialSail','Liaison homme à la mer (MOB) vers EssentialSail','Enlace hombre al agua (MOB) con EssentialSail'],
mobon:['Send MOB messages (adds a MOB page to the watch)','Envoyer les messages MOB (ajoute une page MOB à la montre)','Enviar mensajes MOB (añade una página MOB al reloj)'],
moblink:['Pairing link (paste it from the tablet: fills the fields below)',"Lien d'appairage (collez-le depuis la tablette : remplit les champs ci-dessous)",'Enlace de emparejamiento (péguelo desde la tableta: rellena los campos de abajo)'],
mobhost:['EssentialSail tablet address (or Meshtastic gateway)','Adresse de la tablette EssentialSail (ou passerelle Meshtastic)','Dirección de la tableta EssentialSail (o pasarela Meshtastic)'],
mobid:['Device ID','ID appareil','ID del dispositivo'],
mobkey:['Key (64 hex characters)','Clé (64 caractères hexadécimaux)','Clave (64 caracteres hexadecimales)'],
mobowner:['Owner name (sent to the tablet on a MOB)',"Nom du porteur (envoyé à la tablette lors d'un MOB)",'Nombre del dueño (se envía a la tableta en un MOB)'],
mobhint:['On the tablet: Settings, MOB Messages, switch it on, Pair a watch, copy the link and paste it above. Messages go over WiFi (UDP, port 10120) and the tablet answers each one. The watch has no GPS: the alarm uses the boat position. On the watch, hold the MOB page to raise the alarm. Meshtastic: only through a gateway, enter its address.',
 "Sur la tablette : Réglages, Messages MOB, activez, Appairer une montre, copiez le lien et collez-le ci-dessus. Les messages passent par WiFi (UDP, port 10120) et la tablette répond à chacun. La montre n'a pas de GPS : l'alarme utilise la position du bateau. Sur la montre, maintenez la page MOB pour déclencher l'alarme. Meshtastic : seulement via une passerelle, saisissez son adresse.",
 'En la tableta: Ajustes, Mensajes MOB, actívelo, Emparejar un reloj, copie el enlace y péguelo arriba. Los mensajes van por WiFi (UDP, puerto 10120) y la tableta responde a cada uno. El reloj no tiene GPS: la alarma usa la posición del barco. En el reloj, mantenga pulsada la página MOB para activar la alarma. Meshtastic: solo mediante una pasarela, escriba su dirección.'],
gpsrx:['GPS module: RX pad (GPIO, -1 = none)','Module GPS : pad RX (GPIO, -1 = aucun)','Módulo GPS: pad RX (GPIO, -1 = ninguno)'],
gpsbaud:['GPS baud rate','Débit GPS (bauds)','Velocidad GPS (baudios)'],
gpshint:["The board has no GPS. If you wire a GPS module (NMEA 0183, 3.3 V) to a free pad, set the pad that receives its data (GPIO 1 2 3 17 21 43 44 47 48) and the baud rate (usually 9600). The position of the wearer then appears in the status and goes into the MOB messages.",
 "La carte n'a pas de GPS. Si vous branchez un module GPS (NMEA 0183, 3,3 V) sur un pad libre, indiquez le pad qui reçoit ses données (GPIO 1 2 3 17 21 43 44 47 48) et le débit (généralement 9600). La position du porteur apparaît alors dans l'état et part dans les messages MOB.",
 "La placa no tiene GPS. Si conecta un módulo GPS (NMEA 0183, 3,3 V) a un pad libre, indique el pad que recibe sus datos (GPIO 1 2 3 17 21 43 44 47 48) y la velocidad (normalmente 9600). La posición del portador aparece entonces en el estado y va en los mensajes MOB."],
gpsnofix:['no fix yet','pas encore de position','aún sin posición'],
gpstile:['Wearer GPS','GPS du porteur','GPS del portador'],
moitile:['Boat (from the tablet)','Bateau (de la tablette)','Barco (de la tableta)'],
moisiren:['SIREN','SIRÈNE','SIRENA'],
mobcode:['Pairing code shown on the tablet (Settings, MOB Messages, Pair a watch)','Code affiché sur la tablette (Réglages, Messages MOB, Appairer une montre)','Código que muestra la tableta (Ajustes, Mensajes MOB, Emparejar un reloj)'],
mobpair:['Pair','Appairer','Emparejar'],
pairbusy:['Looking for the tablet... keep the code open on the tablet.','Recherche de la tablette... gardez le code ouvert sur la tablette.','Buscando la tableta... mantenga el código abierto en la tableta.'],
pairok:['Paired with the tablet {0} (device ID {1}). MOB messages are on.','Appairée avec la tablette {0} (ID appareil {1}). Les messages MOB sont activés.','Emparejado con la tableta {0} (ID del dispositivo {1}). Los mensajes MOB están activados.'],
pairfail:['Pairing failed: no valid answer in 5 seconds. Check the 6 digits (the code lasts 2 minutes), that the tablet shows it, and that the watch is on the same WiFi network.',"Échec de l'appairage : pas de réponse valide en 5 secondes. Vérifiez les 6 chiffres (le code dure 2 minutes), que la tablette l'affiche et que la montre est sur le même réseau WiFi.",'Falló el emparejamiento: sin respuesta válida en 5 segundos. Compruebe los 6 dígitos (el código dura 2 minutos), que la tableta lo muestra y que el reloj está en la misma red WiFi.'],
mobscan:['Search EssentialSail','Chercher EssentialSail','Buscar EssentialSail'],
mobnone:['EssentialSail not found. On the tablet switch on Settings, MOB Messages, Listen for MOB messages, and check that it is on the same WiFi network as the watch.',"EssentialSail introuvable. Sur la tablette activez Réglages, Messages MOB, écoute des messages MOB, et vérifiez qu'elle est sur le même réseau WiFi que la montre.",'EssentialSail no encontrado. En la tableta active Ajustes, Mensajes MOB, escuchar mensajes MOB, y compruebe que está en la misma red WiFi que el reloj.'],
mobver:['EssentialSail, verified (it answered)','EssentialSail, vérifié (a répondu)','EssentialSail, verificado (respondió)'],
mobopen:['MOB port open (not verified: paste the pairing link first)',"port MOB ouvert (non vérifié : collez d'abord le lien d'appairage)",'puerto MOB abierto (sin verificar: pegue antes el enlace de emparejamiento)'],
mobok:['Tablet answering ({0} s ago)','Tablette répond (il y a {0} s)','Tableta responde (hace {0} s)'],
mobno:['No answer from the tablet yet','Pas encore de réponse de la tablette','Aún sin respuesta de la tableta'],
mobalarm:['MOB ALARM RAISED','ALARME MOB DÉCLENCHÉE','ALARMA MOB ACTIVADA'],
mobbad:['Link not complete: set the address, the device ID and the key',"Liaison incomplète : renseignez l'adresse, l'ID et la clé",'Enlace incompleto: indique la dirección, el ID y la clave'],
mobbadlink:['Not a valid pairing link',"Lien d'appairage non valide",'Enlace de emparejamiento no válido'],
display:['Display','Affichage','Pantalla'],
lang:['Language (watch and page)','Langue (montre et page)','Idioma (reloj y página)'],
dalarm:['Minimum depth (m)','Profondeur minimale (m)','Profundidad mínima (m)'],
rot:['Orientation','Orientation','Orientación'],
rot0:['Normal','Normale','Normal'],
rot2:['Rotated 180°','Tournée de 180°','Girada 180°'],
cpa:['Collision: CPA below (nm)','Collision : CPA inférieur à (nm)','Colisión: CPA menor de (nm)'],
tcpa:['... within (minutes)','... dans (minutes)','... en (minutos)'],
cpahint:['Collision threshold: a boat that will pass closer than the CPA within the time (or is already closer than the CPA) turns red and starts the alarm and the vibration.',
 "Seuil de collision : un bateau qui passera plus près que le CPA dans ce délai (ou qui est déjà plus près que le CPA) devient rouge et déclenche l'alarme et la vibration.",
 'Umbral de colisión: un barco que pasará más cerca que el CPA en ese tiempo (o que ya está más cerca que el CPA) se vuelve rojo y activa la alarma y la vibración.'],
dhint:['Minimum depth: below this depth the depth turns red and the alarm starts (flashing frame on the watch; Boot = snooze 5 minutes).',
 "Profondeur minimale : en dessous, la profondeur devient rouge et l'alarme se déclenche (cadre clignotant sur la montre ; Boot = pause 5 minutes).",
 'Profundidad mínima: por debajo, la profundidad se vuelve roja y salta la alarma (marco parpadeante en el reloj; Boot = pausa 5 minutos).'],
tz:['Time zone (POSIX string, automatic daylight saving)',"Fuseau horaire (chaîne POSIX, heure d'été automatique)",'Zona horaria (cadena POSIX, horario de verano automático)'],
tzhint:['Time comes from NTP (if the WiFi has Internet) or from the GPS (NMEA RMC/ZDA). Central Europe: CET-1CEST,M3.5.0,M10.5.0/3 · UTC: UTC0',
 "L'heure vient du NTP (si le WiFi a Internet) ou du GPS (NMEA RMC/ZDA). Europe centrale : CET-1CEST,M3.5.0,M10.5.0/3 · UTC : UTC0",
 'La hora viene de NTP (si el WiFi tiene Internet) o del GPS (NMEA RMC/ZDA). Europa central: CET-1CEST,M3.5.0,M10.5.0/3 · UTC: UTC0'],
theme:['Theme','Thème','Tema'],
sleep:['Screen timeout',"Veille de l'écran",'Apagado de pantalla'],
never:['Never','Jamais','Nunca'],
raise:['Raise to wake','Lever pour réveiller','Levantar para activar'],
sleephint:['The screen turns off after this time without touch or buttons (never during an alarm). Touch, a button, an alarm, or raising the watch to look at it wakes it.',"L'écran s'éteint après ce délai sans toucher ni bouton (jamais pendant une alarme). Un toucher, un bouton, une alarme ou le geste de lever la montre pour la regarder le réveille.",'La pantalla se apaga tras este tiempo sin toque ni botones (nunca durante una alarma). Un toque, un botón, una alarma o levantar el reloj para mirarlo la activa.'],
th0:['Day','Jour','Día'],
th1:['Elegant Black','Noir élégant','Negro elegante'],
th2:['Night (red only)','Nuit (rouge seulement)','Noche (solo rojo)'],
datetime:['Date and time','Date et heure','Fecha y hora'],
wtime:['Watch time:','Heure de la montre :','Hora del reloj:'],
tgo:['Set time',"Régler l'heure",'Ajustar hora'],
tpc:['Use computer time',"Heure de l'ordinateur",'Hora del ordenador'],
thint:['The time is kept by the watch battery-backed clock. NTP (if the WiFi has Internet) and the GPS (NMEA) correct it automatically.',
 "L'heure est gardée par l'horloge sauvegardée de la montre. Le NTP (si le WiFi a Internet) et le GPS (NMEA) la corrigent automatiquement.",
 'La hora la guarda el reloj interno con batería. NTP (si el WiFi tiene Internet) y el GPS (NMEA) la corrigen automáticamente.'],
device:['Device','Appareil','Dispositivo'],
name:['Name (web address http://name.local)','Nom (adresse web http://nom.local)','Nombre (dirección web http://nombre.local)'],
vib:['Vibration motor GPIO (-1 = off)','GPIO du moteur de vibration (-1 = désactivé)','GPIO del motor de vibración (-1 = desactivado)'],
vibtest:['Test vibration','Tester la vibration','Probar vibración'],
vibhint:['The vibration motor of the board is on GPIO 18 (the default). It pulses during a collision or depth alarm.',
 "Le moteur de vibration de la carte est sur le GPIO 18 (par défaut). Il vibre pendant une alarme de collision ou de profondeur.",
 'El motor de vibración de la placa está en el GPIO 18 (por defecto). Vibra durante una alarma de colisión o de profundidad.'],
admin:['Administrator login','Accès administrateur','Acceso de administrador'],
user:['User','Utilisateur','Usuario'],
digest:['The page uses a hashed (Digest) login; after 5 wrong passwords the client is locked out for a while.',
 "La page utilise une connexion chiffrée (Digest) ; après 5 mauvais mots de passe, le client est bloqué un moment.",
 'La página usa un acceso cifrado (Digest); tras 5 contraseñas erróneas el cliente queda bloqueado un rato.'],
newpass:['New password','Nouveau mot de passe','Nueva contraseña'],
repeat:['Repeat','Répéter','Repetir'],
unchanged:['(unchanged)','(inchangé)','(sin cambios)'],
nonep:['(none)','(aucun)','(ninguna)'],
save:['Save','Enregistrer','Guardar'],
saver:['Save and restart','Enregistrer et redémarrer','Guardar y reiniciar'],
screen:['Screen','Écran','Pantalla'],
right:['Right','Droite','Derecha'],
left:['Left','Gauche','Izquierda'],
dtap:['Double Tap','Double tap','Doble toque'],
screenhint:['The picture refreshes every 10 s. Right / Left = previous / next page · Double Tap = next theme · Power = screen on / off (hold the real button 4 s to power the watch off) · Boot = snooze the collision alarm for 5 minutes.',
 "L'image se met à jour toutes les 10 s. Droite / Gauche = page précédente / suivante · Double tap = thème suivant · Power = écran allumé / éteint (maintenir le vrai bouton 4 s pour éteindre la montre) · Boot = pause de l'alarme de collision pendant 5 minutes.",
 'La imagen se actualiza cada 10 s. Derecha / Izquierda = página anterior / siguiente · Doble toque = tema siguiente · Power = pantalla encendida / apagada (mantenga el botón real 4 s para apagar el reloj) · Boot = pausa de la alarma de colisión durante 5 minutos.'],
installed:['Installed version','Version installée','Versión instalada'],
fwfind:['Search firmware','Chercher un firmware','Buscar firmware'],
fwsel:['Firmware on GitHub (latest first)',"Firmware sur GitHub (le plus récent d'abord)",'Firmware en GitHub (el más reciente primero)'],
fwpress:['Press "Search firmware"','Appuyez sur « Chercher un firmware »','Pulse «Buscar firmware»'],
fwgo:['Install selected firmware','Installer le firmware choisi','Instalar el firmware elegido'],
fwfilesum:['Install a .bin file from this computer','Installer un fichier .bin depuis cet ordinateur','Instalar un archivo .bin desde este ordenador'],
fwfile:['Firmware file (.bin)','Fichier firmware (.bin)','Archivo de firmware (.bin)'],
fwfilego:['Install file','Installer le fichier','Instalar archivo'],
fwmsg:['The device restarts by itself when the update is done (about 30 s). Settings are kept.',
 "L'appareil redémarre seul une fois la mise à jour terminée (environ 30 s). Les réglages sont conservés.",
 'El equipo se reinicia solo al terminar la actualización (unos 30 s). Los ajustes se conservan.'],
maint:['Maintenance','Maintenance','Mantenimiento'],
restart:['Restart (reset)','Redémarrer (reset)','Reiniciar (reset)'],
backup:['Download backup','Télécharger la sauvegarde','Descargar copia de seguridad'],
rsfile:['Restore configuration from a backup file (.json)','Restaurer la configuration depuis une sauvegarde (.json)','Restaurar la configuración desde una copia (.json)'],
restore:['Restore configuration','Restaurer la configuration','Restaurar configuración'],
factory:['Factory reset (hard reset)','Réinitialisation usine (hard reset)','Restablecer fábrica (hard reset)'],
mmsg:['The backup contains the boat WiFi password, not the administrator password. Factory reset erases every setting: the device restarts with the open WiFi "EssentialView Network" and login admin / admin.',
 "La sauvegarde contient le mot de passe du WiFi du bateau, pas celui de l'administrateur. La réinitialisation usine efface tous les réglages : l'appareil redémarre avec le WiFi ouvert « EssentialView Network » et l'accès admin / admin.",
 'La copia contiene la contraseña del WiFi del barco, no la del administrador. Restablecer fábrica borra todos los ajustes: el equipo se reinicia con el WiFi abierto «EssentialView Network» y acceso admin / admin.'],
// status tiles
signal:['Signal','Signal','Señal'],
nmeasrv:['NMEA server','Serveur NMEA','Servidor NMEA'],
notset:['not set','non défini','sin definir'],
sentok:['Sentences ok','Phrases ok','Frases ok'],
depth:['Depth','Profondeur','Profundidad'],
battery:['Battery','Batterie','Batería'],
charging:['charging','en charge','cargando'],
nobatt:['no battery','pas de batterie','sin batería'],
watchtime:['Watch time','Heure montre','Hora del reloj'],
colal:['Collision alarm','Alarme collision','Alarma de colisión'],
active:['ACTIVE','ACTIVE','ACTIVA'],
snoozed:['Alarm snoozed','Alarme en pause','Alarma en pausa'],
setupwifi:['Setup WiFi','WiFi de config','WiFi de config.'],
setup:['(setup)','(config)','(config.)'],
// messages
pwwarn:['The password is still the default one (admin): set a new one to unlock the page.',
 "Le mot de passe est encore celui d'origine (admin) : choisissez-en un nouveau pour débloquer la page.",
 'La contraseña sigue siendo la de fábrica (admin): elija una nueva para desbloquear la página.'],
pwfirst:['Set a new administrator password and press Save: the other functions stay locked until you do.',
 "Choisissez un nouveau mot de passe administrateur et appuyez sur Enregistrer : les autres fonctions restent bloquées d'ici là.",
 'Elija una nueva contraseña de administrador y pulse Guardar: las demás funciones siguen bloqueadas hasta entonces.'],
pwfirst2:['Set a new administrator password first (not "admin").',"Choisissez d'abord un nouveau mot de passe administrateur (pas « admin »).",'Primero elija una nueva contraseña de administrador (no «admin»).'],
pwdiff:['The two passwords are different','Les deux mots de passe sont différents','Las dos contraseñas son distintas'],
savedr:['Saved. WiFi and name changes apply after a restart.',"Enregistré. Les changements de WiFi et de nom s'appliquent après un redémarrage.",'Guardado. Los cambios de WiFi y nombre se aplican tras reiniciar.'],
saved:['Saved.','Enregistré.','Guardado.'],
restarting:['Restarting... then open http://{0}.local (about 20 s).','Redémarrage... puis ouvrez http://{0}.local (environ 20 s).','Reiniciando... luego abra http://{0}.local (unos 20 s).'],
confrestart:['Restart the device? Unsaved changes are lost.',"Redémarrer l'appareil ? Les changements non enregistrés seront perdus.",'¿Reiniciar el equipo? Se pierden los cambios no guardados.'],
bkok:['Backup downloaded.','Sauvegarde téléchargée.','Copia descargada.'],
bkfail:['Backup failed','Échec de la sauvegarde','Fallo de la copia'],
rschoose:['Choose a backup file first',"Choisissez d'abord un fichier de sauvegarde",'Elija primero un archivo de copia'],
rsinvalid:['Not a valid backup file','Fichier de sauvegarde non valide','Archivo de copia no válido'],
rsnot:['This is not an EssentialWatch backup',"Ce n'est pas une sauvegarde EssentialWatch",'No es una copia de EssentialWatch'],
rsconf:['Restore the configuration of "{0}" (WiFi {1}, server {2}:{3}) and restart?','Restaurer la configuration de « {0} » (WiFi {1}, serveur {2}:{3}) et redémarrer ?','¿Restaurar la configuración de «{0}» (WiFi {1}, servidor {2}:{3}) y reiniciar?'],
none:['none','aucun','ninguna'],
rsfail:['Restore failed: ','Échec de la restauration : ','Fallo al restaurar: '],
rsok:['Restored. Restarting... then open http://{0}.local','Restauré. Redémarrage... puis ouvrez http://{0}.local','Restaurado. Reiniciando... luego abra http://{0}.local'],
facconf:['FACTORY RESET: erase ALL settings (WiFi, server, name, admin password)?','RÉINITIALISATION USINE : effacer TOUS les réglages (WiFi, serveur, nom, mot de passe admin) ?','RESTABLECER FÁBRICA: ¿borrar TODOS los ajustes (WiFi, servidor, nombre, contraseña admin)?'],
factype:['Type RESET to confirm','Tapez RESET pour confirmer','Escriba RESET para confirmar'],
facok:['Settings erased. The device restarts with the open WiFi "EssentialView Network" (http://192.168.4.1, admin / admin).',
 "Réglages effacés. L'appareil redémarre avec le WiFi ouvert « EssentialView Network » (http://192.168.4.1, admin / admin).",
 'Ajustes borrados. El equipo se reinicia con el WiFi abierto «EssentialView Network» (http://192.168.4.1, admin / admin).'],
netsfound:['{0} networks found: pick one in the Network field.','{0} réseaux trouvés : choisissez-en un dans le champ Réseau.','{0} redes encontradas: elija una en el campo Red.'],
scanfail:['Scan failed','Échec de la recherche','Fallo de la búsqueda'],
openw:['open','ouvert','abierta'],
vibok:['Vibration test: 3 pulses.','Test de vibration : 3 impulsions.','Prueba de vibración: 3 pulsos.'],
vibno:['Set a GPIO and press Save first.',"Indiquez un GPIO et appuyez d'abord sur Enregistrer.",'Indique un GPIO y pulse antes Guardar.'],
upl:['Uploading {0}%...','Envoi {0} %...','Enviando {0}%...'],
instok:['Installed. The device is restarting: this page reloads in 30 s.',"Installé. L'appareil redémarre : cette page se recharge dans 30 s.",'Instalado. El equipo se reinicia: esta página se recarga en 30 s.'],
updfail:['Update failed: ','Échec de la mise à jour : ','Fallo de la actualización: '],
connlost:['Connection lost during the upload',"Connexion perdue pendant l'envoi",'Conexión perdida durante el envío'],
binfirst:['Choose a .bin file first',"Choisissez d'abord un fichier .bin",'Elija primero un archivo .bin'],
instconf:['Install {0} ({1} KB)? The device restarts at the end.',"Installer {0} ({1} Ko) ? L'appareil redémarre à la fin.",'¿Instalar {0} ({1} KB)? El equipo se reinicia al final.'],
nochl:['(no changelog)','(pas de notes)','(sin notas)'],
ghsearch:['Searching GitHub...','Recherche sur GitHub...','Buscando en GitHub...'],
nofw:['No firmware found on GitHub.','Aucun firmware trouvé sur GitHub.','No se encontró firmware en GitHub.'],
latest:['(latest)','(le plus récent)','(el más reciente)'],
instd:['(installed)','(installé)','(instalado)'],
fwfound:['{0} firmware found. Pick one, read the changelog, then install.','{0} firmware trouvés. Choisissez-en un, lisez les notes, puis installez.','{0} firmware encontrados. Elija uno, lea las notas e instale.'],
ghfail:['Cannot reach GitHub ({0}). The computer needs Internet access to list them; you can also install a .bin file.',
 "Impossible de joindre GitHub ({0}). L'ordinateur a besoin d'Internet pour les lister ; vous pouvez aussi installer un fichier .bin.",
 'No se puede acceder a GitHub ({0}). El ordenador necesita Internet para listarlos; también puede instalar un archivo .bin.'],
ghconf:['Install {0}? The watch downloads it from GitHub (it needs a WiFi with Internet), then restarts. Settings are kept.',
 'Installer {0} ? La montre le télécharge depuis GitHub (il faut un WiFi avec Internet), puis redémarre. Les réglages sont conservés.',
 '¿Instalar {0}? El reloj lo descarga de GitHub (necesita un WiFi con Internet) y se reinicia. Los ajustes se conservan.'],
ghdl:['The watch is downloading the firmware from GitHub (about 30-60 s)...','La montre télécharge le firmware depuis GitHub (environ 30-60 s)...','El reloj está descargando el firmware de GitHub (unos 30-60 s)...'],
instok2:['Installed. The watch is restarting: this page reloads in 30 s.','Installé. La montre redémarre : cette page se recharge dans 30 s.','Instalado. El reloj se reinicia: esta página se recarga en 30 s.'],
instfail:['Install failed: {0}. You can also download {1} and use "Install a .bin file".',"Échec de l'installation : {0}. Vous pouvez aussi télécharger {1} et utiliser « Installer un fichier .bin ».",'Fallo de la instalación: {0}. También puede descargar {1} y usar «Instalar un archivo .bin».'],
lost2:['Connection lost: the watch is probably restarting. This page reloads in 30 s.','Connexion perdue : la montre redémarre probablement. Cette page se recharge dans 30 s.','Conexión perdida: probablemente el reloj se está reiniciando. Esta página se recarga en 30 s.'],
nsearch:['Searching the network...','Recherche sur le réseau...','Buscando en la red...'],
nsearchp:['Searching the network... {0}%','Recherche sur le réseau... {0} %','Buscando en la red... {0}%'],
nnone:['No NMEA server found. Check that the server is on and connected to the same WiFi network.','Aucun serveur NMEA trouvé. Vérifiez que le serveur est allumé et sur le même réseau WiFi.','No se encontró ningún servidor NMEA. Compruebe que el servidor está encendido y en la misma red WiFi.'],
nopen:['open, no data','ouvert, pas de données','abierto, sin datos'],
nfound:['{0} found. The first one is filled in: press Save to use it.',"{0} trouvé(s). Le premier est rempli : appuyez sur Enregistrer pour l'utiliser.",'{0} encontrados. El primero ya está rellenado: pulse Guardar para usarlo.'],
nnostart:['The search could not start',"La recherche n'a pas pu démarrer",'No se pudo iniciar la búsqueda'],
tsetok:['Time set.','Heure réglée.','Hora ajustada.'],
tpick:['Pick a date and time first',"Choisissez d'abord une date et une heure",'Elija primero fecha y hora'],
};
// messages of the firmware (English) -> [French, Spanish]
const E={
'invalid JSON':['JSON non valide','JSON no válido'],
'name: 1-24 characters':['nom : 1 à 24 caractères','nombre: 1 a 24 caracteres'],
'name: only letters, digits and -':['nom : lettres, chiffres et - seulement','nombre: solo letras, cifras y -'],
'WiFi password: 8-63 characters (empty = open network)':['mot de passe WiFi : 8 à 63 caractères (vide = réseau ouvert)','contraseña WiFi: 8 a 63 caracteres (vacía = red abierta)'],
'port: 1-65535':['port : 1 à 65535','puerto: 1 a 65535'],
'depth alarm: 0-100 m':['alarme profondeur : 0 à 100 m','alarma de profundidad: 0 a 100 m'],
'collision CPA: 0.05-5 nm':['CPA collision : 0,05 à 5 nm','CPA colisión: 0,05 a 5 nm'],
'collision TCPA: 1-120 min':['TCPA collision : 1 à 120 min','TCPA colisión: 1 a 120 min'],
'language: en, fr, es':['langue : en, fr, es','idioma: en, fr, es'],
'brightness: 5-100 %':['luminosité : 5 à 100 %','brillo: 5 a 100 %'],
'time zone: 1-63 characters (POSIX TZ string)':['fuseau horaire : 1 à 63 caractères (chaîne POSIX)','zona horaria: 1 a 63 caracteres (cadena POSIX)'],
'web password: 4-32 characters':['mot de passe web : 4 à 32 caractères','contraseña web: 4 a 32 caracteres'],
'choose a password different from the default one':["choisissez un mot de passe différent de celui d'origine",'elija una contraseña distinta de la de fábrica'],
'vibration pin: -1 (none) or one of GPIO 1 2 3 17 18 21 43 44 47 48':['broche de vibration : -1 (aucune) ou GPIO 1 2 3 17 18 21 43 44 47 48','pin de vibración: -1 (ninguno) o GPIO 1 2 3 17 18 21 43 44 47 48'],
'time: invalid date':['heure : date non valide','hora: fecha no válida'],
'change the default administrator password first (Administrator login, then Save)':["changez d'abord le mot de passe administrateur d'origine (Accès administrateur, puis Enregistrer)",'cambie antes la contraseña de administrador de fábrica (Acceso de administrador y Guardar)'],
'not an ESP32 firmware file (.bin)':["ce n'est pas un firmware ESP32 (.bin)",'no es un firmware ESP32 (.bin)'],
'upload aborted':['envoi interrompu','envío interrumpido'],
'only firmware from the EssentialWatch GitHub releases can be installed':['seuls les firmwares des versions GitHub EssentialWatch peuvent être installés','solo se pueden instalar firmware de las versiones de EssentialWatch en GitHub'],
'the watch is not connected to a WiFi network':["la montre n'est pas connectée à un réseau WiFi",'el reloj no está conectado a una red WiFi'],
'the watch clock is not set (needed for HTTPS): set the date and time first':["l'horloge de la montre n'est pas réglée (nécessaire pour HTTPS) : réglez d'abord la date et l'heure",'el reloj no tiene la hora (necesaria para HTTPS): ajuste antes la fecha y la hora'],
'cannot start the download':['impossible de lancer le téléchargement','no se puede iniciar la descarga'],
'download timed out':['délai de téléchargement dépassé','tiempo de descarga agotado'],
'download incomplete':['téléchargement incomplet','descarga incompleta'],
'checksum mismatch: the file was rejected':['somme de contrôle erronée : le fichier a été refusé','suma de control errónea: el archivo fue rechazado'],
'MOB address: up to 63 characters':["adresse MOB : 63 caractères au plus",'dirección MOB: hasta 63 caracteres'],
'MOB address: IP address or host name':["adresse MOB : adresse IP ou nom d'hôte",'dirección MOB: dirección IP o nombre de host'],
'MOB port: 1-65535':['port MOB : 1-65535','puerto MOB: 1-65535'],
'MOB device ID: 4-8 characters, A-Z and 0-9':["ID appareil MOB : 4 à 8 caractères, A-Z et 0-9",'ID del dispositivo MOB: 4 a 8 caracteres, A-Z y 0-9'],
'MOB key: exactly 64 hexadecimal characters':['clé MOB : exactement 64 caractères hexadécimaux','clave MOB: exactamente 64 caracteres hexadecimales'],
'MOB owner name: up to 24 characters':['nom du porteur MOB : 24 caractères au plus','nombre del dueño MOB: hasta 24 caracteres'],
'GPS pin: -1 (none) or one of GPIO 1 2 3 17 21 43 44 47 48':["pad GPS : -1 (aucun) ou un des GPIO 1 2 3 17 21 43 44 47 48",'pad GPS: -1 (ninguno) o uno de los GPIO 1 2 3 17 21 43 44 47 48'],
'GPS pin: already used by the vibration motor':['pad GPS : déjà utilisé par le moteur de vibration','pad GPS: ya lo usa el motor de vibración'],
'GPS baud rate: 4800, 9600, 19200, 38400, 57600 or 115200':['débit GPS : 4800, 9600, 19200, 38400, 57600 ou 115200','velocidad GPS: 4800, 9600, 19200, 38400, 57600 o 115200'],
'pairing code: the 6 digits shown on the tablet':['code : les 6 chiffres affichés sur la tablette','código: los 6 dígitos que muestra la tableta'],
'pairing: cannot create the key':["appairage : impossible de créer la clé",'emparejamiento: no se puede crear la clave'],
// WiFi state and clock source
'setup mode (no boat WiFi set)':['mode config (pas de WiFi bateau)','modo config. (sin WiFi del barco)'],
'connected':['connecté','conectado'],
'SSID not found (5 GHz network?)':['SSID introuvable (réseau 5 GHz ?)','SSID no encontrado (¿red 5 GHz?)'],
'connect failed (wrong password?)':['échec (mauvais mot de passe ?)','fallo (¿contraseña errónea?)'],
'connection lost':['connexion perdue','conexión perdida'],
'disconnected / connecting':['déconnecté / connexion','desconectado / conectando'],
'idle':['inactif','inactivo'],
'unknown':['inconnu','desconocido'],
'internal clock':['horloge interne','reloj interno'],
'none':['aucune','ninguna'],
};
let LI=0;   // 0 English, 1 French, 2 Spanish
const LC=['en','fr','es'];
function t(id,...a){const r=D[id];let s=r?(r[LI]||r[0]):id;a.forEach((v,i)=>{s=s.split('{'+i+'}').join(v)});return s}
function te(m){const r=E[m];return LI&&r?r[LI-1]:m}
function setLang(code){LI=Math.max(0,LC.indexOf(code));document.documentElement.lang=LC[LI];
 document.querySelectorAll('[data-t]').forEach(e=>e.textContent=t(e.dataset.t));
 document.querySelectorAll('[data-tp]').forEach(e=>e.placeholder=t(e.dataset.tp));
 $('langsel').value=LC[LI];$('lang').value=LC[LI];try{localStorage.setItem('ewlang',LC[LI])}catch(e){}}
// ---------- page ----------
const $=id=>document.getElementById(id);let cfg={};
function msg(s,ok){const m=$('msg');m.textContent=s;m.className=ok?'ok':'bad'}
const U=p=>location.origin+p;
async function api(p,o){const r=await fetch(U(p),o);return r.json()}
const post=(p,b)=>api(p,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
function tile(l,v){return `<div class="tile"><span>${l}</span><b>${v??'--'}</b></div>`}
async function status(){try{const s=await api('/api/status');
 $('who').textContent=`${s.name}.local · fw ${s.fw}`;
 const w=s.ip?`<span class="dot on"></span>${s.ssid}`:`<span class="dot"></span>${te(s.wifi)}`;
 $('tiles').innerHTML=tile('WiFi',w)+tile(t('ip'),s.ip||(s.ap_ip?s.ap_ip+' '+t('setup'):'--'))
 +tile(t('signal'),s.ip?s.rssi+' dBm':'--')
 +tile(t('nmeasrv'),`<span class="dot ${s.server?'on':''}"></span>${s.host||t('notset')}:${s.port}`)
 +tile(t('sentok'),s.ok)+tile('AWA / AWS',s.awa!=null?`${s.awa}° / ${s.aws} kn`:'--')
 +tile('TWA / TWS',s.twa!=null?`${s.twa}° / ${s.tws} kn`:'--')+tile('SOG / STW',`${s.sog??'--'} / ${s.stw??'--'} kn`)
 +tile(t('depth'),s.depth!=null?s.depth+' m':'--')+tile(t('battery'),s.batt!=null?`${s.batt}% · ${s.vbat} V${s.chg?' · '+t('charging'):''}`:(s.pmu?t('nobatt'):'--'))
 +tile(t('watchtime'),s.time||'--')+(s.alert?tile(t('colal'),t('active')):s.snooze?tile(t('snoozed'),Math.floor(s.snooze/60)+':'+String(s.snooze%60).padStart(2,'0')):'')
 +(s.ap?tile(t('setupwifi'),s.ap):'')
 +(s.gps?tile(t('gpstile'),s.gpsfix?`<a href="https://www.openstreetmap.org/?mlat=${s.gpslat}&mlon=${s.gpslon}#map=17/${s.gpslat}/${s.gpslon}" target="_blank" rel="noopener">${s.gpslat}, ${s.gpslon}</a> · ${s.gpsage} s${s.gpssats>=0?' · '+s.gpssats+' sat':''}`:t('gpsnofix')):'')
 +(s.moi!=null?tile(t('moitile'),`${s.moisirenon?'<b class="warn">'+t('moisiren')+'</b> ':''}${s.moidist>=0?(s.moidist>=1000?(s.moidist/1000).toFixed(1)+' km':s.moidist+' m'):'--'}${s.moibrg>=0?' · '+s.moibrg+'°':''} · ${s.moi} s`):'');
 {const m=$('mobstat');if(s.mob==='off')m.textContent='';else if(s.mob!=='idle'){m.textContent=t('mobalarm');m.className='hint warn'}
  else{m.className='hint';m.textContent=!s.mobok?t('mobbad'):(s.mobackage>=0&&s.mobackage<30?t('mobok',s.mobackage):t('mobno'))}}
 $('wtime').textContent=s.time||t('notset');$('wsrc').textContent=s.time&&s.tsrc?'('+te(s.tsrc)+')':''}catch(e){}}
async function load(){cfg=await api('/api/config');
 if(cfg.lang)setLang(cfg.lang);
 for(const k of['ssid','host','port','dalarm','cpa','tcpa','name','tz'])$(k).value=cfg[k];
 $('rot').value=cfg.rot;$('theme').value=cfg.theme??(cfg.night?2:0);$('sleep').value=cfg.sleep;$('raise').checked=!!cfg.raise;$('pass').value='';$('webpass').value='';
 $('pass').placeholder=cfg.pass_set?t('unchanged'):t('nonep');$('webpass2').value='';$('fwv').textContent=cfg.fw;
 $('vib').value=cfg.vib;$('gpsrx').value=cfg.gpsrx;$('gpsbaud').value=cfg.gpsbaud;
 $('mobon').checked=!!cfg.mobon;$('mobhost').value=cfg.mobhost||'';$('mobport').value=cfg.mobport||10120;$('mobid').value=cfg.mobid||'';$('mobowner').value=cfg.mobowner||'';
 $('mobkey').value='';$('mobkey').placeholder=cfg.mobkey_set?t('unchanged'):'';$('moblink').value='';
 $('pwdef').innerHTML=cfg.webpass_default?`<span class="warn">${t('pwwarn')}</span>`:'';
 if(cfg.webpass_default){$('webpass').focus();msg(t('pwfirst'),false)}
 $('openn').checked=false}
async function save(andRestart){
 if(cfg&&cfg.webpass_default&&!$('webpass').value){msg(t('pwfirst2'),false);return}
 const b={name:$('name').value.trim(),vib:+$('vib').value,ssid:$('ssid').value,host:$('host').value.trim(),lang:$('lang').value,
  port:+$('port').value,dalarm:+$('dalarm').value,cpa:+$('cpa').value,tcpa:+$('tcpa').value,tz:$('tz').value.trim(),theme:+$('theme').value,sleep:+$('sleep').value,raise:$('raise').checked,rot:+$('rot').value,
  mobon:$('mobon').checked,mobhost:$('mobhost').value.trim(),mobport:+$('mobport').value||10120,mobid:$('mobid').value.trim().toUpperCase(),mobowner:$('mobowner').value.trim(),gpsrx:+$('gpsrx').value,gpsbaud:+$('gpsbaud').value};
 if($('mobkey').value.trim())b.mobkey=$('mobkey').value.trim();
 if($('openn').checked)b.pass='';else if($('pass').value)b.pass=$('pass').value;
 if($('webpass').value){if($('webpass').value!==$('webpass2').value){msg(t('pwdiff'),false);return}
  b.webpass=$('webpass').value}
 if(cfg.webpass_default){for(const k of Object.keys(b))if(k!=='webpass')delete b[k]}   // only the password is accepted while it is the default one
 const r=await post('/api/config',b);
 if(!r.ok){msg(te(r.error),false);return}
 if(andRestart){await restart(b.name);return}
 msg(r.restart?t('savedr'):t('saved'),true);load()}
async function restart(n){msg(t('restarting',n||cfg.name),true);
 try{await fetch(U('/api/restart'),{method:'POST'})}catch(e){}}
$('save').onclick=()=>save(false);$('saver').onclick=()=>save(true);
$('restart').onclick=()=>{if(confirm(t('confrestart')))restart()};
// language: the page changes at once; the watch follows (saved right away unless the password is still the default one)
function langPick(code){setLang(code);if(cfg&&!cfg.webpass_default)post('/api/config',{lang:LC[LI]}).then(()=>{cfg.lang=LC[LI];shot()}).catch(()=>{})}
$('langsel').onchange=e=>langPick(e.target.value);$('lang').onchange=e=>langPick(e.target.value);
function mm(s,ok){const m=$('mmsg');m.textContent=s;m.className=ok?'hint':'warn'}
$('backup').onclick=async()=>{try{const b=await api('/api/backup');
 const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([JSON.stringify(b,null,2)],{type:'application/json'}));
 a.download=`${b.name}-config-${new Date().toISOString().slice(0,10)}.json`;a.click();mm(t('bkok'),true)}catch(e){mm(t('bkfail'),false)}};
$('restore').onclick=async()=>{const f=$('rsfile').files[0];if(!f){mm(t('rschoose'),false);return}
 let b;try{b=JSON.parse(await f.text())}catch(e){mm(t('rsinvalid'),false);return}
 if(b.type!=='essentialview-config'){mm(t('rsnot'),false);return}
 if(!confirm(t('rsconf',b.name,b.ssid||t('none'),b.host||'-',b.port)))return;
 const r=await post('/api/config',b);
 if(!r.ok){mm(t('rsfail')+te(r.error),false);return}
 mm(t('rsok',b.name),true);try{await fetch(U('/api/restart'),{method:'POST'})}catch(e){}};
$('factory').onclick=async()=>{if(!confirm(t('facconf')))return;
 if(prompt(t('factype'))!=='RESET')return;
 try{await fetch(U('/api/factory'),{method:'POST'})}catch(e){}
 mm(t('facok'),true)};
$('showp').onchange=e=>{$('pass').type=e.target.checked?'text':'password'};
$('scan').onclick=async()=>{$('scan').disabled=true;$('scan').textContent='...';
 try{const l=await api('/api/scan');$('nets').innerHTML=l.map(n=>`<option value="${n.ssid}">${n.rssi} dBm${n.open?' '+t('openw'):''}</option>`).join('');
 msg(t('netsfound',l.length),true)}catch(e){msg(t('scanfail'),false)}
 $('scan').disabled=false;$('scan').textContent=t('scan')};
$('vibtest').onclick=async()=>{const r=await api('/api/key?k=vibtest',{method:'POST'});msg(r.ok?t('vibok'):t('vibno'),!!r.ok)};
document.querySelectorAll('[data-k]').forEach(b=>b.onclick=async()=>{await fetch(U('/api/key?k='+b.dataset.k),{method:'POST'});setTimeout(shot,1500)});
function fwUpload(blob,name){const m=$('fwmsg');
 const fd=new FormData();fd.append('firmware',blob,name);const x=new XMLHttpRequest();
 x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(100*e.loaded/e.total);$('fwbar').style.width=p+'%';m.textContent=t('upl',p);m.className='hint'}};
 x.onload=()=>{let r={};try{r=JSON.parse(x.responseText)}catch(e){}
  if(x.status==200&&r.ok){m.textContent=t('instok');m.className='hint';setTimeout(()=>location.reload(),30000)}
  else{m.textContent=t('updfail')+te(r.error||String(x.status));m.className='warn';$('fwbar').style.width='0'}};
 x.onerror=()=>{m.textContent=t('connlost');m.className='warn'};
 x.open('POST',U('/api/update'));x.send(fd);$('fwgo').disabled=true;$('fwfilego').disabled=true;
 setTimeout(()=>{$('fwgo').disabled=!rels.length;$('fwfilego').disabled=false},40000)}
$('fwfilego').onclick=()=>{const f=$('fwfile').files[0];const m=$('fwmsg');
 if(!f){m.textContent=t('binfirst');m.className='warn';return}
 if(!confirm(t('instconf',f.name,Math.round(f.size/1024))))return;fwUpload(f,f.name)};
// Firmware on GitHub: every version is a Release with the .bin file and the changelog in its notes
const GH='https://api.github.com/repos/davideschiavon/EssentialWatch/releases?per_page=30';let rels=[];
function relAsset(r){return(r.assets||[]).find(a=>/\.bin$/i.test(a.name)&&!/merged|bootloader|partitions/i.test(a.name))}
function relShow(){const r=rels[$('fwsel').selectedIndex];if(!r)return;const L=$('fwlog');L.hidden=false;
 L.textContent=(r.name||r.tag_name)+'  ('+(r.published_at||'').slice(0,10)+')\n\n'+(r.body||t('nochl')).replace(/\r/g,'').replace(/^#+\s*/gm,'')}
$('fwsel').onchange=relShow;
$('fwfind').onclick=async()=>{const m=$('fwmsg');m.textContent=t('ghsearch');m.className='hint';$('fwfind').disabled=true;
 try{const r=await fetch(GH,{headers:{Accept:'application/vnd.github+json'}});if(!r.ok)throw new Error('GitHub '+r.status);
  rels=(await r.json()).filter(x=>!x.draft&&relAsset(x));
  if(!rels.length){m.textContent=t('nofw');m.className='warn';return}
  const sel=$('fwsel');sel.innerHTML='';
  rels.forEach((x,i)=>{const o=document.createElement('option');const v=(x.tag_name||'').replace(/^v/,'');
   o.textContent=(x.name||x.tag_name)+(i==0?'  '+t('latest'):'')+(cfg.fw&&cfg.fw.startsWith(v)?'  '+t('instd'):'');sel.appendChild(o)});
  sel.disabled=false;$('fwgo').disabled=false;relShow();
  m.textContent=t('fwfound',rels.length);m.className='hint'}
 catch(e){m.textContent=t('ghfail',e.message);m.className='warn'}
 $('fwfind').disabled=false};
$('fwgo').onclick=async()=>{const r=rels[$('fwsel').selectedIndex],m=$('fwmsg');if(!r)return;const a=relAsset(r);
 if(!confirm(t('ghconf',r.name||r.tag_name)))return;
 m.textContent=t('ghdl');m.className='hint';$('fwgo').disabled=true;
 try{const j=await post('/api/fwurl',{url:a.browser_download_url,sha256:(a.digest||'').replace(/^sha256:/,'')});
  if(j.ok){m.textContent=t('instok2');m.className='hint';setTimeout(()=>location.reload(),30000)}
  else{m.innerHTML=t('instfail',te(j.error),'<a href="'+a.browser_download_url+'" target="_blank">'+a.name+'</a>');m.className='warn';$('fwgo').disabled=false}}
 catch(e){m.textContent=t('lost2');m.className='hint';setTimeout(()=>location.reload(),30000)}};
// Search NMEA server: the watch scans its own network, this page polls the result
let nmeaHits=[];
function nmeaMsg(s,ok){const m=$('nmeamsg');m.textContent=s;m.className=ok?'hint':'warn'}
$('moblink').oninput=()=>{const v=$('moblink').value.trim();if(!v)return;
 const q=v.indexOf('?');const P=new URLSearchParams(q>=0?v.slice(q+1):'');
 if(!v.startsWith('essentialsail-watch://')||P.get('v')!=='1'||!/^[A-Z0-9]{4,8}$/.test(P.get('id')||'')||!/^[0-9a-fA-F]{64}$/.test(P.get('key')||'')){msg(t('mobbadlink'),false);return}
 $('mobid').value=P.get('id');$('mobkey').value=P.get('key');if(P.get('host'))$('mobhost').value=P.get('host');$('mobport').value=P.get('port')||10120;$('mobon').checked=true;msg('',true)};
$('mobpair').onclick=async()=>{const b=$('mobpair'),m=$('mobpairmsg'),code=$('mobcode').value.replace(/\s/g,'');
 if(!/^[0-9]{6}$/.test(code)){m.textContent=te('pairing code: the 6 digits shown on the tablet');m.className='warn';return}
 b.disabled=true;m.textContent=t('pairbusy');m.className='hint';
 try{const r=await post('/api/mobpair',{code});if(!r.ok){m.textContent=te(r.error);m.className='warn';b.disabled=false;return}
  const tm=setInterval(async()=>{try{const s=await api('/api/mobpair');if(s.state==='busy')return;
   clearInterval(tm);b.disabled=false;
   if(s.state==='ok'){m.textContent=t('pairok',s.host,s.id);m.className='hint';$('mobcode').value='';await load()}
   else{m.textContent=t('pairfail');m.className='warn'}}catch(e){}},1000)}
 catch(e){m.textContent=t('nnostart');m.className='warn';b.disabled=false}};
let mobHits=[];
function mobScanMsg(s,ok){const m=$('mobscanmsg');m.textContent=s;m.className=ok?'hint':'warn'}
$('moblist').onchange=()=>{const h=mobHits[$('moblist').selectedIndex];if(h){$('mobhost').value=h.ip;$('mobport').value=h.port}};
$('mobscan').onclick=async()=>{const b=$('mobscan');b.disabled=true;$('moblist').hidden=true;mobScanMsg(t('nsearch'),true);
 try{const r=await api('/api/mobscan',{method:'POST'});if(!r.ok){mobScanMsg(te(r.error),false);b.disabled=false;return}
  const tm=setInterval(async()=>{try{const s=await api('/api/mobscan');
   if(s.running){mobScanMsg(t('nsearchp',s.progress),true);return}
   clearInterval(tm);b.disabled=false;
   mobHits=s.hits.sort((a,b)=>b.nmea-a.nmea);
   if(!mobHits.length){mobScanMsg(t('mobnone'),false);return}
   const L=$('moblist');L.innerHTML='';
   mobHits.forEach(h=>{const o=document.createElement('option');o.textContent=`${h.ip}:${h.port}  ${h.nmea?t('mobver'):t('mobopen')}`;L.appendChild(o)});
   L.hidden=false;L.selectedIndex=0;L.onchange();
   mobScanMsg(t('nfound',mobHits.length),true)}catch(e){}},1000)}
 catch(e){mobScanMsg(t('nnostart'),false);b.disabled=false}};
$('nmealist').onchange=()=>{const h=nmeaHits[$('nmealist').selectedIndex];if(h){$('host').value=h.ip;$('port').value=h.port}};
$('nmeascan').onclick=async()=>{const b=$('nmeascan');b.disabled=true;$('nmealist').hidden=true;nmeaMsg(t('nsearch'),true);
 try{const r=await api('/api/nmeascan',{method:'POST'});if(!r.ok){nmeaMsg(te(r.error),false);b.disabled=false;return}
  const tm=setInterval(async()=>{try{const s=await api('/api/nmeascan');
   if(s.running){nmeaMsg(t('nsearchp',s.progress),true);return}
   clearInterval(tm);b.disabled=false;
   nmeaHits=s.hits.sort((a,b)=>b.nmea-a.nmea);
   if(!nmeaHits.length){nmeaMsg(t('nnone'),false);return}
   const L=$('nmealist');L.innerHTML='';
   nmeaHits.forEach(h=>{const o=document.createElement('option');o.textContent=`${h.ip}:${h.port}  ${h.nmea?'NMEA ('+h.sample+')':t('nopen')}`;L.appendChild(o)});
   L.hidden=false;L.selectedIndex=0;L.onchange();
   nmeaMsg(t('nfound',nmeaHits.length),true)}catch(e){}},1000)}
 catch(e){nmeaMsg(t('nnostart'),false);b.disabled=false}};
// time
async function setTime(ms){const r=await post('/api/config',{epoch:Math.round(ms/1000)});
 msg(r.ok?t('tsetok'):te(r.error),!!r.ok);status()}
$('tgo').onclick=()=>{const v=$('tset').value;if(!v){msg(t('tpick'),false);return}setTime(new Date(v).getTime())};
$('tpc').onclick=()=>setTime(Date.now());
(()=>{const d=new Date(),p=n=>String(n).padStart(2,'0');$('tset').value=`${d.getFullYear()}-${p(d.getMonth()+1)}-${p(d.getDate())}T${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}`})();
function shot(){$('scr').src=U('/screen.bmp?half=1&t='+Date.now())}  // half size: a full copy keeps the watch busy for seconds
// first paint: the language last used in this browser, then the one of the watch (load)
let l0='en';try{l0=localStorage.getItem('ewlang')||(navigator.language||'en').slice(0,2)}catch(e){}
setLang(l0);
load();status();shot();setInterval(status,3000);setInterval(()=>{if(!document.hidden)shot()},10000);
</script></body></html>)rawliteral";
