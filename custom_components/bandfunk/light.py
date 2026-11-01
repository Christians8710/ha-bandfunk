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
        if self.control is not None:
            return self.control[5] and self.control[3]>0 and self.control[4]!=15
        return any(self.coordinator.data["groups"][self.group][:3]) and self.coordinator.data["groups"][self.group][3] != 15

    @property
    def control(self):
        if self.coordinator.data.get("control_api")==2:
            return self.coordinator.data["control"][self.group]
        return None

    @property
    def rgb_color(self):
        if self.control is not None:
            return tuple(self.control[:3])
        return tuple(v * 17 for v in self.coordinator.data["groups"][self.group][:3])

    @property
    def brightness(self):
        return self.control[3] if self.control is not None else 255

    @property
    def effect(self):
        if self.control is not None:
            return EFFECTS[self.control[4]]
        return EFFECTS[self.coordinator.data["groups"][self.group][3]]

    async def _send_control(self, rgb, brightness, effect, on):
        try:
            await self.coordinator.api.command("control", {"group":self.group+1,
                "r":rgb[0],"g":rgb[1],"b":rgb[2],"brightness":brightness,"effect":effect,"on":on})
            await self.coordinator.async_request_refresh()
        except (CannotConnect,InvalidAuth) as err:
            raise HomeAssistantError("Sollwert konnte nicht gespeichert werden; Geräteverbindung prüfen") from err

    async def _send(self, rgb, effect):
        try:
            await self.coordinator.api.command("light", {"group": self.group+1,
                "r": round(rgb[0]/17), "g": round(rgb[1]/17), "b": round(rgb[2]/17), "effect": effect})
            await self.coordinator.async_request_refresh()
        except (CannotConnect, InvalidAuth) as err:
            raise HomeAssistantError("Befehl abgelehnt. Experimentelle Funkübertragung aktivieren; keine Empfangsbestätigung möglich.") from err

    async def async_turn_on(self, **kwargs):
        if self.control is not None:
            await self._send_control(kwargs.get("rgb_color",self.rgb_color),
                kwargs.get("brightness",self.brightness or 255),
                EFFECTS.index(kwargs.get("effect",self.effect)),True)
            return
        rgb = kwargs.get("rgb_color", self.rgb_color if self.is_on else (255,255,255))
        brightness = kwargs.get("brightness", 255)
        rgb = tuple(round(v * brightness / 255) for v in rgb)
        effect = EFFECTS.index(kwargs.get("effect", self.effect if self.is_on else EFFECTS[0]))
        await self._send(rgb, effect)

    async def async_turn_off(self, **kwargs):
        if self.control is not None:
            await self._send_control(self.rgb_color,self.brightness,EFFECTS.index(self.effect),False)
            return
        await self._send((0,0,0), 0)
