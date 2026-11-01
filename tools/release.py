"""Deterministic ZIPs and ESP8266 Arduino-compatible signed OTA image."""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding

ROOT = Path(__file__).resolve().parents[1]


def sign_image(data, key):
    signature = key.sign(data, padding.PKCS1v15(), hashes.SHA256())
    return data + signature + struct.pack("<I", len(signature))


def verify_image(data, public):
    n = struct.unpack("<I", data[-4:])[0]
    if n != 256 or len(data) <= n + 4:
        raise ValueError("Ungültige Signaturlänge")
    public.verify(data[-n-4:-4], data[:-n-4], padding.PKCS1v15(), hashes.SHA256())


def deterministic_zip(path, files, base):
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for f in sorted(files):
            info = zipfile.ZipInfo(f.relative_to(base).as_posix(), (2026, 1, 1, 0, 0, 0))
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, f.read_bytes(), compresslevel=9)


def build(private, binary):
    out = ROOT / "dist"
    out.mkdir(exist_ok=True)
    data = binary.read_bytes()
    if not data or data[0] != 0xE9 or len(data) > 1024 * 1024 - 4096:
        raise ValueError("Firmware ist ungültig oder zu groß für die OTA-Reserve")
    key = serialization.load_pem_private_key(private.read_bytes(), password=None)
    public = serialization.load_pem_public_key((ROOT / "keys/ota-public.pem").read_bytes())
    signed = sign_image(data, key)
    verify_image(signed, public)
    (out / "bandfunk-0.1.2.bin").write_bytes(data)
    (out / "bandfunk-0.1.2.bin.signed").write_bytes(signed)
    files = [f for f in (ROOT / "custom_components/bandfunk").rglob("*") if f.is_file() and "__pycache__" not in f.parts]
    deterministic_zip(out / "bandfunk.zip", files, ROOT / "custom_components/bandfunk")
    manifest = {"name": "BandFunk HA", "version": "0.1.2", "new_install_prompt_erase": True,
        "builds": [{"chipFamily": "ESP8266", "parts": [{"path": "bandfunk-0.1.2.bin", "offset": 0}]}]}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2)+"\n")
    artifacts = sorted(f for f in out.iterdir() if f.name != "SHA256SUMS" and f.is_file())
    (out / "SHA256SUMS").write_text("".join(f"{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.name}\n" for f in artifacts))
    print("Release-Artefakte einschließlich geprüfter OTA-Signatur erstellt")


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--private", type=Path, required=True)
    p.add_argument("--binary", type=Path, default=ROOT/"firmware/.pio/build/d1_mini/firmware.bin")
    args = p.parse_args()
    build(args.private, args.binary)
