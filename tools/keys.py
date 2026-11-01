"""Generate an owner-controlled key ONCE. Private key stays outside the repo."""
import argparse
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--private", type=Path, required=True)
    args = p.parse_args()
    root = Path(__file__).resolve().parents[1]
    public = root / "keys/ota-public.pem"
    if args.private.exists() or public.exists():
        p.error("Vorhandene Schlüssel werden niemals überschrieben")
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    args.private.parent.mkdir(parents=True, exist_ok=True)
    args.private.write_bytes(key.private_bytes(serialization.Encoding.PEM,
        serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
    public.parent.mkdir(exist_ok=True)
    public.write_bytes(key.public_key().public_bytes(serialization.Encoding.PEM,
        serialization.PublicFormat.SubjectPublicKeyInfo))
    print("Schlüsselpaar erstellt. Privaten Schlüssel offline sichern; nicht veröffentlichen.")
