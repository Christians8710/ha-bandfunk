"""Persistent master RF enable shared with the device's standalone UI."""
from homeassistant.components.switch import SwitchEntity
from homeassistant.helpers.entity import EntityCategory
from homeassistant.exceptions import HomeAssistantError
from .api import CannotConnect, InvalidAuth
from .const import DOMAIN
from .entity import BandFunkEntity


async def async_setup_entry(hass, entry, async_add_entities):
    async_add_entities([BandFunkTX(hass.data[DOMAIN][entry.entry_id], "tx_enabled",
                                 "Funkübertragung")])


class BandFunkTX(BandFunkEntity, SwitchEntity):
    _attr_entity_category = EntityCategory.CONFIG
    @property
    def is_on(self):
        return self.coordinator.data["tx_enabled"]

    async def _set(self, enabled):
        try:
            await self.coordinator.api.command("tx", {"enabled": enabled})
            await self.coordinator.async_request_refresh()
        except (CannotConnect, InvalidAuth) as err:
            raise HomeAssistantError("Funkfreigabe konnte nicht gespeichert werden; Geräteverbindung prüfen") from err

    async def async_turn_on(self, **kwargs):
        await self._set(True)

    async def async_turn_off(self, **kwargs):
        await self._set(False)
