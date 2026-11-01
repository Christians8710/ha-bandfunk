"""Experimental, assumed command state. A one-way wristband cannot confirm RGB."""
from homeassistant.components.light import LightEntity, ColorMode, LightEntityFeature
from homeassistant.exceptions import HomeAssistantError
from .api import CannotConnect, InvalidAuth
from .const import DOMAIN
from .entity import BandFunkEntity

EFFECTS = ["Dauerlicht", "Blinken 6 Hz", "Blinken 10 Hz", "Blinken 14 Hz", "Blinken 20 Hz",
    "Blinken 25 Hz", "Zufallsblitz langsam", "Zufallsblitz schnell", "Ausblenden langsam",
    "Ausblenden schnell", "Einblenden langsam", "Einblenden schnell", "Zufall aus/ein langsam",
    "Zufall aus/ein schnell", "Zustand halten", "Energiesparen"]


async def async_setup_entry(hass, entry, async_add_entities):
    c = hass.data[DOMAIN][entry.entry_id]
    async_add_entities(BandFunkLight(c, i) for i in range(10))


class BandFunkLight(BandFunkEntity, LightEntity):
    _attr_supported_color_modes = {ColorMode.RGB}
    _attr_color_mode = ColorMode.RGB
    _attr_supported_features = LightEntityFeature.EFFECT
    _attr_effect_list = EFFECTS
    _attr_assumed_state = True
    _attr_entity_category = None

    def __init__(self, coordinator, group):
        super().__init__(coordinator, f"group_{group+1}", f"Gruppe {group+1} (experimentell)")
        self.group = group

    @property
    def is_on(self):
        return any(self.coordinator.data["groups"][self.group][:3]) and self.coordinator.data["groups"][self.group][3] != 15

    @property
    def rgb_color(self):
        return tuple(v * 17 for v in self.coordinator.data["groups"][self.group][:3])

    @property
    def effect(self):
        return EFFECTS[self.coordinator.data["groups"][self.group][3]]

    async def _send(self, rgb, effect):
        try:
            await self.coordinator.api.command("light", {"group": self.group+1,
                "r": round(rgb[0]/17), "g": round(rgb[1]/17), "b": round(rgb[2]/17), "effect": effect})
            await self.coordinator.async_request_refresh()
        except (CannotConnect, InvalidAuth) as err:
            raise HomeAssistantError("Befehl abgelehnt. Experimentelle Funkübertragung aktivieren; keine Empfangsbestätigung möglich.") from err

    async def async_turn_on(self, **kwargs):
        rgb = kwargs.get("rgb_color", self.rgb_color if self.is_on else (255,255,255))
        brightness = kwargs.get("brightness", 255)
        rgb = tuple(round(v * brightness / 255) for v in rgb)
        effect = EFFECTS.index(kwargs.get("effect", self.effect if self.is_on else EFFECTS[0]))
        await self._send(rgb, effect)

    async def async_turn_off(self, **kwargs):
        await self._send((0,0,0), 0)
