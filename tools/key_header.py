"""PlatformIO hook: fail closed if no persistent OTA public key exists."""
from pathlib import Path
Import("env")  # noqa: F821
root = Path(env["PROJECT_DIR"]).parent
key = (root / "keys/ota-public.pem").read_text().strip()
if "BEGIN PUBLIC KEY" not in key:
    raise RuntimeError("Zuerst tools/keys.py ausführen")
(root / "firmware/include/ota_key.h").write_text(
    '#pragma once\nstatic const char OTA_PUBLIC_KEY[] PROGMEM = R"KEY(' + key + ')KEY";\n')
