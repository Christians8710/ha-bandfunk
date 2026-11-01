import importlib.util
from pathlib import Path
import pytest
from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.asymmetric import rsa

spec = importlib.util.spec_from_file_location("release", Path(__file__).parents[1]/"tools/release.py")
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


def test_signature_accepts_only_matching_key_and_unmodified_image():
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    image = release.sign_image(b"\xe9" + bytes(4095), key)
    release.verify_image(image, key.public_key())
    with pytest.raises(InvalidSignature):
        release.verify_image(bytes([image[0] ^ 1]) + image[1:], key.public_key())
    other = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    with pytest.raises(InvalidSignature):
        release.verify_image(image, other.public_key())
    with pytest.raises(ValueError):
        release.verify_image(image[:-1], key.public_key())


def test_zip_is_deterministic(tmp_path):
    source = tmp_path/"data.txt"
    source.write_text("BandFunk")
    a, b = tmp_path/"a.zip", tmp_path/"b.zip"
    release.deterministic_zip(a,[source],tmp_path)
    release.deterministic_zip(b,[source],tmp_path)
    assert a.read_bytes() == b.read_bytes()
