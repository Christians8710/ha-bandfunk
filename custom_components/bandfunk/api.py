"""Small local API client. Never logs credentials or follows redirects."""
import asyncio
import json
import re
from urllib.parse import urlsplit

import aiohttp


class InvalidAuth(Exception):
    """Credential rejected."""


class CannotConnect(Exception):
    """Device unavailable or incompatible."""


def validate_host(host: str) -> str:
    """Accept a hostname/IPv4 only; no injected paths, ports or credentials."""
    host = host.strip().lower()
    if not re.fullmatch(r"[a-z0-9](?:[a-z0-9.-]{0,251}[a-z0-9])?", host):
        raise ValueError("Hostname oder IPv4 erforderlich")
    if urlsplit("http://" + host).hostname != host or ".." in host:
        raise ValueError("Ungültiger Host")
    return host


def validate_status(data: dict) -> dict:
    """Reject unrelated HTTP services and incompatible API schemas."""
    if not isinstance(data, dict) or data.get("api") != 1:
        raise CannotConnect("Inkompatible API")
    if not re.fullmatch(r"[0-9a-f]{12}", str(data.get("id", ""))):
        raise CannotConnect("Ungültige Geräte-ID")
    for field in ("version", "protocol"):
        if not isinstance(data.get(field), str):
            raise CannotConnect("Ungültiger Status")
    for field in ("rssi", "uptime", "free_heap", "edges", "dropped"):
        if type(data.get(field)) is not int:
            raise CannotConnect("Ungültiger Messwert")
    for field in ("radio_ok", "safe_mode", "ota_armed"):
        if type(data.get(field)) is not bool:
            raise CannotConnect("Ungültiger Status")
    if type(data.get("tx_enabled")) is not bool or not isinstance(data.get("groups"), list) or len(data["groups"]) != 10:
        raise CannotConnect("Ungültiger RGB-Status")
    for group in data["groups"]:
        if not isinstance(group, list) or len(group) != 4 or any(type(v) is not int or not 0 <= v <= 15 for v in group):
            raise CannotConnect("Ungültige Gruppendaten")
    if "control_api" in data:
        controls=data.get("control")
        if data["control_api"]!=2 or not isinstance(controls,list) or len(controls)!=10:
            raise CannotConnect("Ungültige Steuerdaten")
        for control in controls:
            if not isinstance(control,list) or len(control)!=6 or type(control[5]) is not bool:
                raise CannotConnect("Ungültige Steuerdaten")
            if any(type(v) is not int or not 0<=v<=(15 if i==4 else 255) for i,v in enumerate(control[:5])):
                raise CannotConnect("Ungültige Steuerdaten")
    return data


class BandFunkAPI:
    def __init__(self, session, host, token):
        self.session = session
        self.host = validate_host(host)
        self.token = token

    async def status(self):
        try:
            async with self.session.get(
                f"http://{self.host}/api/status",
                headers={"Authorization": f"Bearer {self.token}"},
                timeout=aiohttp.ClientTimeout(total=8), allow_redirects=False,
            ) as response:
                if response.status in (401, 403):
                    raise InvalidAuth
                if response.status != 200 or response.content_length and response.content_length > 8192:
                    raise CannotConnect
                raw = bytearray()
                async for chunk in response.content.iter_chunked(1024):
                    raw.extend(chunk)
                    if len(raw) > 8192:
                        raise CannotConnect
                if len(raw) > 8192:
                    raise CannotConnect
                import json
                return validate_status(json.loads(raw))
        except (aiohttp.ClientError, asyncio.TimeoutError, ValueError, TypeError) as err:
            raise CannotConnect from err

    async def command(self, endpoint, payload):
        if endpoint not in ("tx", "light", "control"):
            raise ValueError("Unbekannter Befehl")
        try:
            async with self.session.post(f"http://{self.host}/api/{endpoint}", json=payload,
                headers={"Authorization": f"Bearer {self.token}"},
                timeout=aiohttp.ClientTimeout(total=8), allow_redirects=False) as response:
                if response.status in (401, 403):
                    raise InvalidAuth
                if response.status != 200:
                    raise CannotConnect("Befehl abgelehnt; Funkfreigabe und Gerätestatus prüfen")
                raw=bytearray()
                async for chunk in response.content.iter_chunked(1024):
                    raw.extend(chunk)
                    if len(raw)>8192: raise CannotConnect("Antwort zu groß")
                data=json.loads(raw)
                if isinstance(data,dict) and data.get("api")==1:
                    return validate_status(data)
                if isinstance(data,dict) and data.get("ok") is True:
                    return None
                raise CannotConnect("Ungültige Befehlsantwort")
        except (aiohttp.ClientError, asyncio.TimeoutError, ValueError, TypeError) as err:
            raise CannotConnect from err
