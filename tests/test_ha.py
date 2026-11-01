"""Exercise actual HA flow classes in CI, without a running HA server."""
import asyncio
from unittest.mock import AsyncMock, MagicMock, patch
import pytest

pytest.importorskip("homeassistant")
from custom_components.bandfunk.config_flow import ConfigFlow
from custom_components.bandfunk.api import InvalidAuth, CannotConnect


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
