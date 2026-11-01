"""Radio, recovery and update-window diagnostics."""
from homeassistant.components.binary_sensor import BinarySensorEntity
from .const import DOMAIN
from .entity import BandFunkEntity


async def async_setup_entry(hass, entry, async_add_entities):
    coordinator = hass.data[DOMAIN][entry.entry_id]
    async_add_entities(BandFunkBinarySensor(coordinator, key, name) for key, name in (
        ("radio_ok", "CC1101 erkannt"), ("safe_mode", "Wiederherstellungsmodus"),
        ("ota_armed", "OTA-Fenster geöffnet")))


class BandFunkBinarySensor(BandFunkEntity, BinarySensorEntity):
    @property
    def is_on(self):
        return self.coordinator.data[self.key]
