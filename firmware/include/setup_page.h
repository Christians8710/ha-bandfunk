#pragma once
#include <Arduino.h>
static const char SETUP_FORM[] PROGMEM = R"HTML(
<h2>Mit deinem Heim-WLAN verbinden</h2>
<p>Hier erscheinen die 2,4-GHz-WLANs in Reichweite des ESP.</p>
<button id="scan" type="button">Neu suchen</button><p id="scan-status" role="status">WLANs werden gesucht …</p>
<label>Gefundene WLANs <select id="networks"><option value="">WLAN auswählen oder SSID unten eingeben</option></select></label>
<form method="post" action="/provision">
<input type="hidden" name="nonce" value="__PROVISION_NONCE__">
<p><label>SSID <input id="ssid" name="ssid" maxlength="32" required autocomplete="off"></label></p>
<p><label>WLAN-Passwort <input name="password" type="password" maxlength="64" autocomplete="off"></label></p>
<button>Speichern und neu starten</button></form>
<p>Versteckte WLANs bitte manuell eintragen. API-Schlüssel vor dem Speichern kopieren. Später bleibt er auf der Geräte-Webseite nach Anmeldung als admin mit diesem Schlüssel sichtbar. Portal endet nach 10 Minuten.</p>
<script>
const scanButton=document.getElementById('scan');
const scanStatus=document.getElementById('scan-status');
const networks=document.getElementById('networks');
networks.addEventListener('change',()=>{if(networks.value)document.getElementById('ssid').value=networks.value;});
async function scanWiFi(){
  scanButton.disabled=true; scanStatus.textContent='WLANs werden gesucht …';
  try{
    let result=null;
    for(let attempt=0;attempt<40;attempt++){
      const response=await fetch('/api/wifi',{cache:'no-store'});
      if(response.status===202){await new Promise(resolve=>setTimeout(resolve,500));continue;}
      if(!response.ok)throw new Error('Suche fehlgeschlagen');
      result=await response.json(); break;
    }
    if(!result)throw new Error('Zeitüberschreitung');
    networks.replaceChildren(new Option('WLAN auswählen oder SSID unten eingeben',''));
    const seen=new Set();
    result.networks.sort((a,b)=>b.rssi-a.rssi).forEach(network=>{
      if(!network.ssid||seen.has(network.ssid))return;
      seen.add(network.ssid);
      // SSIDs are untrusted radio data: Option uses text, never HTML.
      networks.add(new Option(network.ssid+' ('+network.rssi+' dBm'+(network.secure?', geschützt':', offen')+')',network.ssid));
    });
    scanStatus.textContent=seen.size?seen.size+' WLANs gefunden. Bitte auswählen.':'Keine sichtbaren WLANs gefunden. SSID manuell eingeben oder erneut suchen.';
  }catch(error){scanStatus.textContent='WLAN-Suche nicht verfügbar. Bitte erneut suchen oder SSID manuell eingeben.';}
  finally{scanButton.disabled=false;}
}
scanButton.addEventListener('click',scanWiFi); scanWiFi();
</script>)HTML";
