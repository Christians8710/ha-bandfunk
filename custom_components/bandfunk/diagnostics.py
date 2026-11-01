"""Explicit allowlist: no token, SSID, MAC, hostname, IP or raw captures."""
from .const import DOMAIN


async def async_get_config_entry_diagnostics(hass, entry):
    c = hass.data[DOMAIN][entry.entry_id]
    allowed = ("api", "version", "protocol", "rssi", "uptime", "free_heap", "edges",
               "dropped", "radio_ok", "safe_mode", "ota_armed", "tx_enabled", "tx_packets",
               "tx_attempts", "tx_completed", "tx_errors", "tx_config_verified",
               "tx_idle_state", "tx_chip_state", "tx_fscal1", "tx_error", "tx_wave_us",
               "tx_late_edges", "tx_max_late_us", "tx_mismatch_register",
               "tx_expected_value", "tx_actual_value", "tx_active", "control_api", "config_saved", "config_error",
               "heap_max_block", "heap_fragmentation", "reset_reason", "http_last_ms", "http_max_ms", "loop_max_gap_ms", "brightness_profile")
    return {"status": {k: c.data[k] for k in allowed if k in c.data}, "available": c.last_update_success}
