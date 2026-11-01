"""Run the actual portal response script: no success message on failed joins."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import pytest


@pytest.mark.parametrize("states,expected", [
    ([{"state":"failed","message":"Passwort abgelehnt","ip":""}],"Passwort abgelehnt"),
    ([{"state":"save_failed","message":"Speichern fehlgeschlagen","ip":""}],"Speichern fehlgeschlagen"),
    ([{"state":"connecting","message":"Bitte warten","ip":""},
      {"state":"connected","message":"Verbindung bestätigt","ip":"192.168.1.42"}],"192.168.1.42"),
])
def test_real_portal_status_script(states, expected):
    node=shutil.which("node")
    if not node:
        pytest.skip("Node required for portal script execution")
    source=(Path(__file__).parents[1]/"firmware/include/bandfunk_wm_strings_de.h").read_text(encoding="utf-8")
    literal=re.search(r'HTTP_SAVED\[\].*?=\s*("(?:\\.|[^"\\])*");',source).group(1)
    html=json.loads(literal)
    script=re.search(r'<script>(.*?)</script>',html).group(1)
    program="""
const vm=require('node:vm'); const assert=require('node:assert/strict');
const states=STATES; const output={textContent:''}; const timers=[]; let index=0;
const context={document:{getElementById:()=>output},
 fetch:async()=>({json:async()=>states[Math.min(index++,states.length-1)]}),
 setTimeout:(fn)=>timers.push(fn)};
vm.runInNewContext(SCRIPT,context);
(async()=>{
 for(let i=0;i<8;i++){await new Promise(setImmediate);if(timers.length)timers.shift()();}
 assert(output.textContent.includes(EXPECTED));
 if(states.at(-1).state!=='connected')assert(!output.textContent.includes('Jetzt mit dem Heim-WLAN verbinden'));
 assert.equal(timers.length,0);
})();
""".replace("STATES",json.dumps(states)).replace("SCRIPT",json.dumps(script)).replace("EXPECTED",json.dumps(expected))
    subprocess.run([node,"-e",program],check=True,capture_output=True,text=True,timeout=10)
