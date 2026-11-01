"""Exercise actual HA flow classes in CI, without a running HA server."""
import asyncio
from unittest.mock import AsyncMock, MagicMock, patch
import pytest

pytest.importorskip("homeassistant")
from custom_components.bandfunk.config_flow import ConfigFlow
from custom_components.bandfunk.api import InvalidAuth, CannotConnect
from custom_components.bandfunk.diagnostics import async_get_config_entry_diagnostics
from types import SimpleNamespace
from custom_components.bandfunk.light import BandFunkLight
from custom_components.bandfunk.switch import BandFunkTX


def run_flow(api_result=None, error=None):
    flow = ConfigFlow()
    flow.hass = MagicMock()
    flow.async_set_unique_id = AsyncMock()
    flow._abort_if_unique_id_configured = MagicMock()
    flow.async_create_entry = MagicMock(side_effect=lambda **kw: kw)
    flow.async_show_form = MagicMock(side_effect=lambda **kw: kw)
    status = AsyncMock(return_value=api_result, side_effect=error)
    with patch("custom_components.bandfunk.config_flow.async_get_clientsession"), patch(
        "custom_components.bandfunk.config_flow.BandFunkAPI") as api:
        api.return_value.status = status
        api.return_value.host = "esp.local"
        result = asyncio.run(flow.async_step_user({"host":"esp.local","token":"secret"}))
    return flow, result


def test_manual_setup_identity():
    flow, result = run_flow({"id":"aabbcc112233"})
    assert result["data"] == {"host":"esp.local","token":"secret"}
    flow.async_set_unique_id.assert_awaited_once_with("aabbcc112233")


@pytest.mark.parametrize("error,expected", [(InvalidAuth(),"invalid_auth"),(CannotConnect(),"cannot_connect")])
def test_setup_errors(error, expected):
    _, result = run_flow(error=error)
    assert result["errors"]["base"] == expected


def test_discovery_rejects_untrusted_id():
    flow=ConfigFlow()
    flow.hass=MagicMock()
    result=asyncio.run(flow.async_step_zeroconf(MagicMock(host="esp.local",properties={"id":"invalid"})))
    assert result["reason"] == "invalid_discovery"


@pytest.mark.parametrize("extra", [{}, {"tx_attempts":3,"tx_packets":2,"tx_completed":1,
    "tx_errors":1,"tx_chip_state":19,"tx_error":"CC1101 erreicht TX nicht"}])
def test_diagnostics_allowlist_with_old_and_new_firmware(extra):
    data={"api":1,"version":"0.1.5","token":"private-token","ssid":"private-network",
          "password":"private-password","id":"aabbcc112233","host":"private-host",
          "ip":"192.0.2.1",**extra}
    coordinator=SimpleNamespace(data=data,last_update_success=True)
    hass=SimpleNamespace(data={"bandfunk":{"entry":coordinator}})
    result=asyncio.run(async_get_config_entry_diagnostics(hass,SimpleNamespace(entry_id="entry")))
    assert result["available"] is True
    assert result["status"] == {"api":1,"version":"0.1.5",**extra}


def test_standalone_light_preserves_color_brightness_when_off():
    c=MagicMock();c.device_id="aabbcc112233"
    c.data={"control_api":2,"control":[[31,85,255,90,2,True] for _ in range(10)]}
    c.api.command=AsyncMock();c.async_request_refresh=AsyncMock()
    light=BandFunkLight(c,5)
    asyncio.run(light.async_turn_off())
    c.api.command.assert_awaited_once_with("control",{"group":6,"r":31,"g":85,"b":255,"brightness":90,"effect":2,"on":False})
    c.data["control"][5][5]=False
    c.api.command.reset_mock()
    asyncio.run(light.async_turn_on())
    c.api.command.assert_awaited_once_with("control",{"group":6,"r":31,"g":85,"b":255,"brightness":90,"effect":2,"on":True})


def test_persistent_master_switch_uses_same_api_without_duration():
    c=MagicMock();c.device_id="aabbcc112233";c.data={"tx_enabled":True}
    c.api.command=AsyncMock();c.async_request_refresh=AsyncMock()
    switch=BandFunkTX(c,"tx_enabled","Funkübertragung")
    assert switch.is_on
    asyncio.run(switch.async_turn_off())
    c.api.command.assert_awaited_once_with("tx",{"enabled":False})
