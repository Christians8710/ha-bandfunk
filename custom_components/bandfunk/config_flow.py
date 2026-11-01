"""German manual setup, mDNS discovery and reauthentication."""
import voluptuous as vol
from homeassistant import config_entries
from homeassistant.helpers.aiohttp_client import async_get_clientsession
from homeassistant.helpers import selector
from .api import BandFunkAPI, CannotConnect, InvalidAuth
from .const import DOMAIN, FLASHER_URL


class ConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    VERSION = 1
    _host = ""
    _discovered_id = None
    _reauth_entry = None

    async def async_step_user(self, user_input=None):
        errors = {}
        if user_input:
            try:
                api = BandFunkAPI(async_get_clientsession(self.hass), user_input["host"], user_input["token"])
                data = await api.status()
                if self._discovered_id and data["id"] != self._discovered_id:
                    return self.async_abort(reason="wrong_device")
                if self._reauth_entry:
                    if data["id"] != self._reauth_entry.unique_id:
                        return self.async_abort(reason="wrong_device")
                    return self.async_update_reload_and_abort(
                        self._reauth_entry, data_updates={"host": api.host, "token": user_input["token"]})
                await self.async_set_unique_id(data["id"])
                self._abort_if_unique_id_configured()
                return self.async_create_entry(title=f"BandFunk {data['id'][-6:]}",
                    data={"host": api.host, "token": user_input["token"]})
            except InvalidAuth:
                errors["base"] = "invalid_auth"
            except CannotConnect:
                errors["base"] = "cannot_connect"
            except ValueError:
                errors["host"] = "invalid_host"
        return self.async_show_form(step_id="user", data_schema=vol.Schema({
            vol.Required("host", default=self._host): str,
            vol.Required("token"): selector.TextSelector(selector.TextSelectorConfig(
                type=selector.TextSelectorType.PASSWORD))}), errors=errors,
            description_placeholders={"flasher_url": FLASHER_URL})

    async def async_step_zeroconf(self, discovery_info):
        self._host = discovery_info.host
        self._discovered_id = discovery_info.properties.get("id", "")
        import re
        if not re.fullmatch(r"[0-9a-f]{12}", self._discovered_id):
            return self.async_abort(reason="invalid_discovery")
        await self.async_set_unique_id(self._discovered_id)
        # Only an authenticated manual flow may update the address of an existing entry.
        self._abort_if_unique_id_configured()
        self.context["title_placeholders"] = {"name": f"BandFunk {self._discovered_id[-6:]}"}
        return await self.async_step_user()

    async def async_step_reauth(self, entry_data):
        self._reauth_entry = self.hass.config_entries.async_get_entry(self.context["entry_id"])
        self._host = entry_data["host"]
        return await self.async_step_user()

    async def async_step_reconfigure(self, user_input=None):
        self._reauth_entry = self.hass.config_entries.async_get_entry(self.context["entry_id"])
        self._host = self._reauth_entry.data["host"]
        return await self.async_step_user(user_input)
