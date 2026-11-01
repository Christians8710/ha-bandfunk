"""Execute the actual offline UI JavaScript against a tiny browser mock."""
import json,re,shutil,subprocess
from pathlib import Path
import pytest

def test_standalone_browser_controls_share_state_and_use_csrf():
    node=shutil.which('node')
    if not node: pytest.skip('Node required')
    page=(Path(__file__).parents[1]/'firmware/include/device_page.h').read_text(encoding='utf-8')
    script=re.search(r'<script>(.*?)</script>',page,re.S).group(1)
    program=r'''
const vm=require('node:vm'),assert=require('node:assert/strict');
const elements=new Map();function el(id){if(!elements.has(id))elements.set(id,{id,value:id==='group'?'6':'',checked:false,textContent:'',listeners:{},addEventListener(t,fn){this.listeners[t]=fn},appendChild(){}});return elements.get(id)}
const controls=Array.from({length:10},()=>[255,255,255,255,0,false]);controls[1]=[0,255,0,99,0,false];controls[5]=[255,0,0,128,0,true];
const state={control:controls,radio_ok:true,safe_mode:false,tx_enabled:true,config_saved:true,version:'0.2.0',rssi:-50,tx_completed:1,tx_errors:0,ota_armed:false};
const posts=[],swatch={dataset:{color:'#0000ff'},addEventListener(t,fn){this.click=fn}};
let deny=false,redirect='';
const context={document:{getElementById:el,querySelector:()=>({content:'test-nonce'}),querySelectorAll:()=>[swatch],createElement:()=>({}),activeElement:null},location:{assign:path=>redirect=path},setInterval(){},setTimeout(){},FormData:class{},fetch:async(path,options)=>{
 if(path==='/web/status')return{status:200,ok:true,json:async()=>state};
 if(deny)return{status:401,ok:false};
 const data=JSON.parse(options.body);posts.push({path,data,headers:options.headers});
 if(path==='/web/control')state.control[data.group-1]=[data.r,data.g,data.b,data.brightness,data.effect,data.on];
 if(path==='/web/tx')state.tx_enabled=data.enabled;
 return{status:200,ok:true,json:async()=>({ok:true})};
}};
vm.runInNewContext(SCRIPT,context);
const flush=async()=>{for(let i=0;i<12;i++)await new Promise(setImmediate)};
(async()=>{
 await flush();assert.equal(el('color').value,'#ff0000');assert.equal(el('brightness').value,128);
 swatch.click();await flush();let command=posts.at(-1);assert.equal(command.path,'/web/control');assert.equal(command.headers['X-BandFunk-CSRF'],'test-nonce');
 assert.equal(command.data.group,6);assert.equal(command.data.b,255);assert.equal(command.data.brightness,128);assert.equal(command.data.on,true);
 el('off').listeners.click();await flush();assert.equal(posts.at(-1).data.on,false);assert.equal(posts.at(-1).data.b,255);
 el('group').value='2';el('group').listeners.change();assert.equal(el('color').value,'#00ff00');el('on').listeners.click();await flush();assert.equal(posts.at(-1).data.group,2);assert.equal(posts.at(-1).data.brightness,99);
 el('radio').checked=false;el('radio').listeners.change();await flush();assert.equal(posts.at(-1).path,'/web/tx');assert.equal(posts.at(-1).data.enabled,false);
 deny=true;el('on').listeners.click();await flush();assert.equal(redirect,'/login');
})().catch(e=>{console.error(e);process.exitCode=1});
'''.replace('SCRIPT',json.dumps(script))
    subprocess.run([node,'-e',program],capture_output=True,text=True,check=True,timeout=15)
