"""Invoke the C++ `secure_core` binary (hybrid encryption) via subprocess."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path


def _binary_path() -> Path:
    env = os.environ.get("SECURE_CORE_BIN")
    if env:
        return Path(env).resolve()
    root = Path(__file__).resolve().parents[2]
    return root / "cpp_core" / "build" / "secure_core"


def keygen(public_pem: Path, private_pem: Path, bits: int = 2048) -> None:
    exe = _binary_path()
    subprocess.run(
        [
            str(exe),
            "keygen",
            "--pub",
            str(public_pem),
            "--priv",
            str(private_pem),
            "--bits",
            str(bits),
        ],
        check=True,
        capture_output=True,
        timeout=120,
    )


def encrypt_file(plaintext: Path, bundle: Path, recipient_public_pem: Path) -> None:
    exe = _binary_path()
    subprocess.run(
        [
            str(exe),
            "encrypt",
            "--in",
            str(plaintext),
            "--out",
            str(bundle),
            "--recipient-pub",
            str(recipient_public_pem),
        ],
        check=True,
        capture_output=True,
        timeout=600,
    )


def decrypt_file(bundle: Path, plaintext: Path, private_pem: Path) -> None:
    exe = _binary_path()
    subprocess.run(
        [
            str(exe),
            "decrypt",
            "--in",
            str(bundle),
            "--out",
            str(plaintext),
            "--private-key",
            str(private_pem),
        ],
        check=True,
        capture_output=True,
        timeout=600,
    )
