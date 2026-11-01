"""Shared device identity."""
from homeassistant.helpers.entity import DeviceInfo, EntityCategory
from homeassistant.helpers.update_coordinator import CoordinatorEntity
from .const import DOMAIN


class BandFunkEntity(CoordinatorEntity):
    _attr_has_entity_name = True
    _attr_entity_category = EntityCategory.DIAGNOSTIC

    def __init__(self, coordinator, key, name):
        super().__init__(coordinator)
        self.key = key
        self._attr_name = name
        self._attr_unique_id = f"{coordinator.device_id}_{key}"

    @property
    def device_info(self):
        return DeviceInfo(identifiers={(DOMAIN, self.coordinator.device_id)},
            name=f"BandFunk {self.coordinator.device_id[-6:]}", manufacturer="BandFunk Projekt",
            model="D1 mini / CC1101", sw_version=self.coordinator.data["version"],
            configuration_url=f"http://{self.coordinator.api.host}/")
