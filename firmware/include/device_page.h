#pragma once
#include "ui_style.h"
const char PORTAL_THEME[] PROGMEM="<meta name='referrer' content='no-referrer'><link rel='icon' href='/assets/icon.png'><style>" BANDFUNK_CSS "</style>";
const char DEVICE_HEAD[] PROGMEM=R"HTML(<!doctype html><html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="referrer" content="no-referrer"><link rel="icon" href="/assets/icon.png"><title>BandFunk</title><style>)HTML" BANDFUNK_CSS R"HTML(</style></head><body><main><header><img src="/assets/icon.png" alt="BandFunk"><div><h1>BandFunk</h1><span class="muted">Deine Armbänder. Direkt im Browser.</span></div></header>)HTML";
const char DEVICE_FOOTER[] PROGMEM=R"HTML(<footer>© 2026 BandFunk contributors · <a href="/license">MIT-Lizenz und Drittanbieter</a></footer>)HTML";
const char DEVICE_APP[] PROGMEM=R"HTML(
<div id="message" class="notice" role="status" aria-live="polite">Verbindung wird hergestellt …</div>
<div class="grid"><div><section class="card"><h2>Armbänder steuern</h2><div class="row"><input id="radio" type="checkbox"><label for="radio">Funkübertragung dauerhaft aktiv</label></div>
<label for="group">Gruppe auf dem Armband</label><select id="group"><option value="1">Gruppe 1</option><option value="2">Gruppe 2</option><option value="3">Gruppe 3</option><option value="4">Gruppe 4</option><option value="5">Gruppe 5</option><option value="6" selected>Gruppe 6</option><option value="7">Gruppe 7</option><option value="8">Gruppe 8</option><option value="9">Gruppe 9</option><option value="10">Gruppe 10</option></select>
<div class="row"><button id="on" type="button">Einschalten</button><button id="off" type="button" class="secondary">Ausschalten</button><strong id="group-state"></strong></div>
<label for="color">Farbe</label><input id="color" type="color" value="#ffffff"><div class="palette"><button class="swatch" data-color="#ff0000" aria-label="Rot" style="background:#f00"></button><button class="swatch" data-color="#00ff00" aria-label="Grün" style="background:#0f0"></button><button class="swatch" data-color="#0000ff" aria-label="Blau" style="background:#00f"></button><button class="swatch" data-color="#ffff00" aria-label="Gelb" style="background:#ff0"></button><button class="swatch" data-color="#ff00ff" aria-label="Magenta" style="background:#f0f"></button><button class="swatch" data-color="#ffffff" aria-label="Weiß" style="background:#fff"></button></div>
<label for="brightness">Helligkeit <output id="brightness-value">100 %</output></label><input id="brightness" type="range" min="0" max="100" value="100"><label for="effect">Effekt</label><select id="effect"></select>
</section>
<section class="card"><h2>Verbindung zu Home Assistant</h2>)HTML";
const char DEVICE_SETTINGS[] PROGMEM=R"HTML(</section></div><div>
<section class="card"><h2>Gerätestatus</h2><p id="device-status" class="status">Wird geladen …</p></section>
<section class="card"><details><summary>Admin-Passwort ändern</summary><p>Benutzer: <strong>admin</strong>. Startpasswort: <strong>bandfunk2026</strong>. Dieses Passwort ist unabhängig vom HA-API-Schlüssel.</p><form id="password-form"><label for="current-password">Bisheriges Passwort</label><input id="current-password" type="password" autocomplete="current-password" required><label for="new-password">Neues Passwort (8–64 Zeichen)</label><input id="new-password" type="password" minlength="8" maxlength="64" autocomplete="new-password" required><label for="repeat-password">Neues Passwort wiederholen</label><input id="repeat-password" type="password" minlength="8" maxlength="64" autocomplete="new-password" required><p><button>Passwort speichern</button></p></form></details></section>
<section class="card"><details><summary>Firmware aktualisieren</summary><p>Den RESET-Taster kurz drücken: signierte OTA-Updates sind dann fünf Minuten freigegeben. Die Funkübertragung pausiert während des eigentlichen Uploads.</p><form id="update-form" action="/update" method="post" enctype="multipart/form-data"><input type="hidden" name="csrf" id="update-csrf"><label for="firmware">Signierte Firmware (.bin.signed)</label><input id="firmware" name="firmware" type="file" accept=".signed" required><p><button>Firmware installieren</button></p></form><button id="close-ota" class="secondary">Updatefenster schließen</button></details></section>
<section class="card"><form method="post" action="/logout"><input id="logout-csrf" type="hidden" name="csrf"><button class="secondary">Abmelden</button></form></section></div></div>
<footer>© 2026 BandFunk contributors · <a href="/license">MIT-Lizenz und Drittanbieter</a></footer>
<script>
const $=id=>document.getElementById(id);
const csrf=document.querySelector('meta[name="bf-csrf"]').content;
const effects=['Dauerlicht','Blinken 6 Hz','Blinken 10 Hz','Blinken 14 Hz','Blinken 20 Hz','Blinken 25 Hz','Zufallsblitz langsam','Zufallsblitz schnell','Ausblenden langsam','Ausblenden schnell','Einblenden langsam','Einblenden schnell','Zufall aus/ein langsam','Zufall aus/ein schnell','Zustand halten','Energiesparen'];
effects.forEach((name,i)=>{const o=document.createElement('option');o.value=i;o.textContent=name;$('effect').appendChild(o)});
['update-csrf','logout-csrf'].forEach(id=>$(id).value=csrf);
let state=null,busy=false,polling=false,jobs=[],desired={},revision=0;
function message(text){$('message').textContent=text}
async function request(path,data){
 const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),6000);
 const options={cache:'no-store',signal:controller.signal};
 if(data!==undefined){options.method='POST';options.headers={'Content-Type':'application/json','X-BandFunk-CSRF':csrf};options.body=JSON.stringify(data)}
 try{const response=await fetch(path,options);if(response.status===401){location.assign('/login');throw Error('Bitte erneut anmelden.')}if(!response.ok)throw Error(await response.text());return await response.json()}
 catch(error){if(controller.signal.aborted)throw Error('Keine Antwort innerhalb von sechs Sekunden. Verbindung prüfen und erneut versuchen.');throw error}finally{clearTimeout(timer)}
}
function renderGroup(){if(!state)return;const number=Number($('group').value),g=desired[number]||state.control[number-1];$('color').value='#'+g.slice(0,3).map(v=>v.toString(16).padStart(2,'0')).join('');$('brightness').value=g[3]?Math.max(1,Math.round(g[3]*100/255)):0;$('brightness-value').textContent=$('brightness').value+' %';$('effect').value=g[4];$('group-state').textContent=g[5]&&g[3]&&g[4]!==15?'Ein':'Aus'}
function renderStatus(){if(!state)return;$('radio').checked=state.tx_enabled;const text=state.safe_mode?'Wiederherstellungsmodus: Funk pausiert.':!state.radio_ok?'Funkfehler: '+(state.tx_error||'CC1101 prüfen.'):state.tx_enabled?'Funkübertragung ist dauerhaft aktiv.':'Funkübertragung ist ausgeschaltet. Sollwerte bleiben gespeichert.';message(text+(state.config_error?' Speichern fehlgeschlagen.':state.config_saved?'':' Änderungen werden gespeichert …'));$('device-status').textContent='Firmware '+state.version+'\nBetriebszeit: '+state.uptime+' s\nWLAN-Signal: '+state.rssi+' dBm\nFunkfehler: '+state.tx_errors+'\nUpdatefenster: '+(state.ota_armed?'offen':'geschlossen')}
async function refresh(){if(polling||busy)return;polling=true;const started=revision;try{const next=await request('/web/status');if(!busy&&started===revision){state=next;renderStatus();if(!['color','brightness','effect'].includes(document.activeElement?.id))renderGroup()}}finally{polling=false}}
async function change(path,data){
 ++revision;
 if(path==='/web/control')desired[data.group]=[data.r,data.g,data.b,data.brightness,data.effect,data.on];
 const pending=jobs.findIndex(job=>job.path===path&&(path!=='/web/control'||job.data.group===data.group));
 if(pending>=0)jobs[pending]={path,data};else jobs.push({path,data});
 if(busy)return;busy=true;let error='';
 try{while(jobs.length){const job=jobs.shift();try{const result=await request(job.path,job.data);state=result.control?result:await request('/web/status');error=''}catch(e){error=e.message;jobs=[]}}}
 finally{busy=false;desired={};renderStatus();if(!['color','brightness','effect'].includes(document.activeElement?.id))renderGroup();if(error)message(error)}
}
function sendGroup(on){if(!state)return;const group=Number($('group').value),old=desired[group]||state.control[group-1],hex=$('color').value;if(on===true&&Number($('brightness').value)===0){$('brightness').value=100;$('brightness-value').textContent='100 %'}return change('/web/control',{group,r:parseInt(hex.slice(1,3),16),g:parseInt(hex.slice(3,5),16),b:parseInt(hex.slice(5,7),16),brightness:Math.round(Number($('brightness').value)*255/100),effect:Number($('effect').value),on:on===undefined?old[5]:on})}
$('group').addEventListener('change',renderGroup);$('on').addEventListener('click',()=>sendGroup(true));$('off').addEventListener('click',()=>sendGroup(false));$('radio').addEventListener('change',()=>change('/web/tx',{enabled:$('radio').checked}));$('color').addEventListener('change',()=>sendGroup(true));$('brightness').addEventListener('input',()=>{$('brightness-value').textContent=Number($('brightness').value)+' %'});$('brightness').addEventListener('change',()=>sendGroup());$('effect').addEventListener('change',()=>sendGroup());document.querySelectorAll('[data-color]').forEach(button=>button.addEventListener('click',()=>{$('color').value=button.dataset.color;sendGroup(true)}));$('close-ota').addEventListener('click',()=>change('/web/ota-close',{}));
$('password-form').addEventListener('submit',async event=>{event.preventDefault();const next=$('new-password').value;if(next!==$('repeat-password').value){message('Die neuen Passwörter stimmen nicht überein.');return}try{await request('/web/password',{current:$('current-password').value,password:next});location.assign('/login?changed=1')}catch(e){message(e.message)}});
$('update-form').addEventListener('submit',async event=>{event.preventDefault();message('Signierte Firmware wird übertragen. Bitte warten …');try{const response=await fetch('/update',{method:'POST',headers:{'X-BandFunk-CSRF':csrf},body:new FormData($('update-form'))});const result=await response.text();if(!response.ok)throw Error(result);message(result+' Bitte nach dem Neustart neu anmelden.');busy=true;setTimeout(()=>location.assign('/login'),12000)}catch(e){message(e.message)}});
refresh().catch(e=>message(e.message));setInterval(()=>{if(!busy)refresh().catch(e=>message(e.message))},2000);
</script></main></body></html>)HTML";
