"""Read-only diagnostic sensors; no unsupported light entity."""
from homeassistant.components.sensor import SensorEntity
from .const import DOMAIN
from .entity import BandFunkEntity

SENSORS = (("rssi", "WLAN-Signal", "dBm"), ("uptime", "Betriebszeit", "s"),
           ("free_heap", "Freier Speicher", "B"), ("edges", "Empfangene Flanken", None),
           ("dropped", "Verworfene Flanken", None), ("protocol", "Protokollstatus", None),
           ("version", "Firmware-Version", None))


async def async_setup_entry(hass, entry, async_add_entities):
    async_add_entities(BandFunkSensor(hass.data[DOMAIN][entry.entry_id], *args) for args in SENSORS)


class BandFunkSensor(BandFunkEntity, SensorEntity):
    def __init__(self, coordinator, key, name, unit):
        super().__init__(coordinator, key, name)
        self._attr_native_unit_of_measurement = unit

    @property
    def native_value(self):
        return self.coordinator.data[self.key]
