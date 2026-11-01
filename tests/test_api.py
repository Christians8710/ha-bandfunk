"""Pure API tests run without importing HA's package initializer."""
import asyncio
import importlib.util
from pathlib import Path
import pytest

spec = importlib.util.spec_from_file_location("api", Path(__file__).parents[1]/"custom_components/bandfunk/api.py")
api = importlib.util.module_from_spec(spec)
spec.loader.exec_module(api)


@pytest.mark.parametrize("host", ["http://esp.local", "esp.local/x", "a@evil", "a:80", "a..b", "", "a\nfoo"])
def test_invalid_hosts(host):
    with pytest.raises(ValueError):
        api.validate_host(host)


def test_valid_host_and_status():
    assert api.validate_host(" ESP.local ") == "esp.local"
    d = dict(api=1,id="aabbcc112233",version="0.1.0",protocol="nicht_entschluesselt",rssi=-50,
             uptime=10,free_heap=20000,edges=0,dropped=0,radio_ok=True,safe_mode=False,ota_armed=False,
             tx_enabled=False,groups=[[0,0,0,0] for _ in range(10)])
    assert api.validate_status(d) == d
    d["id"] = "wrong"
    with pytest.raises(api.CannotConnect): api.validate_status(d)


def test_rejects_auth_and_redirects():
    class Response:
        content_length = 0
        def __init__(self,status): self.status=status
        async def __aenter__(self): return self
        async def __aexit__(self,*args): pass
    class Session:
        def __init__(self,status): self.status=status
        def get(self,*args,**kwargs):
            assert kwargs["allow_redirects"] is False
            return Response(self.status)
    for status, error in ((401,api.InvalidAuth),(403,api.InvalidAuth),(302,api.CannotConnect),(500,api.CannotConnect)):
        with pytest.raises(error): asyncio.run(api.BandFunkAPI(Session(status),"esp.local","secret").status())


@pytest.mark.parametrize("kind",["status","legacy","oversize","invalid"])
def test_command_reads_bounded_authoritative_state_or_legacy_ok(kind):
    import json
    data=dict(api=1,id="aabbcc112233",version="0.2.1",protocol="test",rssi=-50,
              uptime=10,free_heap=20000,edges=0,dropped=0,radio_ok=True,safe_mode=False,
              ota_armed=False,tx_enabled=True,groups=[[0,0,0,0] for _ in range(10)])
    body=json.dumps(data).encode() if kind=="status" else b'{"ok":true}' if kind=="legacy" else b'x'*8193 if kind=="oversize" else b'not-json'
    class Content:
        async def iter_chunked(self,size):
            for i in range(0,len(body),size):yield body[i:i+size]
    class Response:
        status=200;content=Content()
        async def __aenter__(self):return self
        async def __aexit__(self,*args):pass
    class Session:
        def post(self,*args,**kwargs):return Response()
    request=api.BandFunkAPI(Session(),"esp.local","secret").command("tx",{"enabled":True})
    if kind in ("oversize","invalid"):
        with pytest.raises(api.CannotConnect):asyncio.run(request)
    else:assert asyncio.run(request)==(data if kind=="status" else None)
