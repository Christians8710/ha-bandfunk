"""Explicit allowlist: no token, SSID, MAC, hostname, IP or raw captures."""
from .const import DOMAIN


async def async_get_config_entry_diagnostics(hass, entry):
    c = hass.data[DOMAIN][entry.entry_id]
    allowed = ("api", "version", "protocol", "rssi", "uptime", "free_heap", "edges",
               "dropped", "radio_ok", "safe_mode", "ota_armed")
    return {"status": {k: c.data[k] for k in allowed}, "available": c.last_update_success}
