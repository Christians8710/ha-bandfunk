"""BandFunk HA local diagnostics integration."""
from datetime import timedelta
import asyncio
from homeassistant.exceptions import ConfigEntryAuthFailed
from homeassistant.helpers.aiohttp_client import async_get_clientsession
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator, UpdateFailed
from .api import BandFunkAPI, CannotConnect, InvalidAuth
from .const import DOMAIN, PLATFORMS


class BandFunkCoordinator(DataUpdateCoordinator):
    def __init__(self, hass, entry):
        import logging
        super().__init__(hass, logging.getLogger(__name__), name=DOMAIN,
                         update_interval=timedelta(seconds=5))
        self.api = BandFunkAPI(async_get_clientsession(hass), entry.data["host"], entry.data["token"])
        self.device_id = entry.unique_id
        self.command_lock=asyncio.Lock()
        self._command_revision=0

    def accept_command(self,data):
        if data["id"]!=self.device_id:
            raise CannotConnect("Unter dieser Adresse wurde ein anderes Gerät gefunden")
        self._command_revision+=1
        self.async_set_updated_data(data)

    async def _async_update_data(self):
        revision=self._command_revision
        try:
            data = await self.api.status()
            if data["id"] != self.device_id:
                raise UpdateFailed("Unter dieser Adresse wurde ein anderes Gerät gefunden")
            if revision!=self._command_revision and getattr(self,"data",None) is not None:
                return self.data
            return data
        except InvalidAuth as err:
            if revision!=self._command_revision and getattr(self,"data",None) is not None: return self.data
            raise ConfigEntryAuthFailed("API-Schlüssel ungültig") from err
        except CannotConnect as err:
            if revision!=self._command_revision and getattr(self,"data",None) is not None: return self.data
            raise UpdateFailed("BandFunk nicht erreichbar") from err


async def async_setup_entry(hass, entry):
    coordinator = BandFunkCoordinator(hass, entry)
    await coordinator.async_config_entry_first_refresh()
    hass.data.setdefault(DOMAIN, {})[entry.entry_id] = coordinator
    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    return True


async def async_unload_entry(hass, entry):
    if await hass.config_entries.async_unload_platforms(entry, PLATFORMS):
        hass.data[DOMAIN].pop(entry.entry_id)
        return True
    return False
