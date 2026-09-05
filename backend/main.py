"""Thin FastAPI layer: HTTP, SQLite, subprocess calls into C++ `secure_core`."""

from __future__ import annotations

import logging
import os
import subprocess
import tempfile
import uuid
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any, Optional

from fastapi import FastAPI, File, Form, HTTPException, Query, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse, JSONResponse, RedirectResponse
from fastapi.staticfiles import StaticFiles
from itsdangerous import BadSignature, SignatureExpired, URLSafeTimedSerializer
from starlette.background import BackgroundTask

from backend.db import get_connection
from backend.services import crypto_cli

ROOT = Path(__file__).resolve().parents[1]
STORAGE = ROOT / "storage"
FILES_DIR = STORAGE / "files"
KEYS_DIR = STORAGE / "keys"
LOGS_DIR = STORAGE / "logs"
DB_PATH = STORAGE / "app.db"

FILES_DIR.mkdir(parents=True, exist_ok=True)
KEYS_DIR.mkdir(parents=True, exist_ok=True)
LOGS_DIR.mkdir(parents=True, exist_ok=True)

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler(LOGS_DIR / "app.log"),
    ],
)
log = logging.getLogger("secure_file_sharing")

SECRET_KEY = os.environ.get(
    "E2EE_APP_SECRET",
    os.environ.get("SARAVANA_SECRET", "dev-change-me-in-production"),
)
serializer = URLSafeTimedSerializer(SECRET_KEY, salt="e2ee-fs-download")

app = FastAPI(
    title="Secure File Sharing (E2EE, OOP core)",
    description="Hybrid encryption via C++ OpenSSL CLI; metadata in SQLite.",
    version="1.0.0",
)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


@app.get("/")
def root() -> RedirectResponse:
    """Landing page: web UI. API docs remain at /docs."""
    return RedirectResponse(url="/ui/")


@app.on_event("startup")
def startup() -> None:
    with get_connection(DB_PATH):
        pass
    log.info("Database ready at %s", DB_PATH)


@app.post("/users")
def create_user(display_name: str = Form(...)) -> JSONResponse:
    user_id = str(uuid.uuid4())
    pub_path = KEYS_DIR / f"{user_id}_public.pem"
    priv_path = KEYS_DIR / f"{user_id}_private.pem"
    try:
        crypto_cli.keygen(pub_path, priv_path, bits=2048)
    except subprocess.CalledProcessError as exc:
        log.exception("keygen failed")
        err = exc.stderr.decode() if exc.stderr else str(exc)
        raise HTTPException(status_code=500, detail=err) from exc

    public_pem = pub_path.read_text(encoding="utf-8")
    private_pem = priv_path.read_text(encoding="utf-8")
    # E2EE: do not retain private keys on the server after registration.
    try:
        priv_path.unlink()
    except OSError:
        pass

    with get_connection(DB_PATH) as conn:
        conn.execute(
            "INSERT INTO users (id, display_name, public_key_pem, created_at) VALUES (?, ?, ?, ?)",
            (user_id, display_name, public_pem, _now_iso()),
        )
        conn.commit()

    log.info("Registered user %s", user_id)
    return JSONResponse(
        {
            "user_id": user_id,
            "display_name": display_name,
            "public_key_pem": public_pem,
            "private_key_pem": private_pem,
            "warning": "Save private_key_pem now; the server does not store it.",
        },
        status_code=201,
    )


@app.post("/upload")
async def upload(
    sender_id: str = Form(...),
    recipient_id: str = Form(...),
    file: UploadFile = File(...),
    expires_in_hours: Optional[int] = Form(None),
) -> JSONResponse:
    sender_id = sender_id.strip()
    recipient_id = recipient_id.strip()
    if sender_id == recipient_id:
        raise HTTPException(status_code=400, detail="Sender and recipient must differ")

    with get_connection(DB_PATH) as conn:
        sender = conn.execute("SELECT id FROM users WHERE id = ?", (sender_id,)).fetchone()
        recipient = conn.execute(
            "SELECT id, public_key_pem FROM users WHERE id = ?", (recipient_id,)
        ).fetchone()
        if not sender:
            raise HTTPException(
                status_code=404,
                detail="Unknown sender_id — not registered on this server. Paste your user_id from Register.",
            )
        if not recipient:
            raise HTTPException(
                status_code=404,
                detail="Unknown recipient_id — that user must register first (other tab). Paste their full UUID.",
            )

    file_id = str(uuid.uuid4())
    bundle_path = FILES_DIR / f"{file_id}.bin"

    recipient_pub_tmp = tempfile.NamedTemporaryFile(
        mode="w", suffix=".pem", delete=False, encoding="utf-8"
    )
    try:
        recipient_pub_tmp.write(recipient["public_key_pem"])
        recipient_pub_tmp.close()
        plaintext_tmp = tempfile.NamedTemporaryFile(delete=False)
        try:
            content = await file.read()
            plaintext_tmp.write(content)
            plaintext_tmp.close()
            crypto_cli.encrypt_file(
                Path(plaintext_tmp.name), bundle_path, Path(recipient_pub_tmp.name)
            )
        finally:
            Path(plaintext_tmp.name).unlink(missing_ok=True)
    except Exception as exc:
        bundle_path.unlink(missing_ok=True)
        log.exception("encrypt failed")
        raise HTTPException(status_code=500, detail=str(exc)) from exc
    finally:
        Path(recipient_pub_tmp.name).unlink(missing_ok=True)

    expires_at: Optional[str] = None
    if expires_in_hours is not None and expires_in_hours > 0:
        expires_at = (datetime.now(timezone.utc) + timedelta(hours=expires_in_hours)).isoformat()

    with get_connection(DB_PATH) as conn:
        conn.execute(
            """
            INSERT INTO files (id, sender_id, receiver_id, storage_path, original_name, created_at, expires_at)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
            (
                file_id,
                sender_id,
                recipient_id,
                str(bundle_path),
                file.filename or "unnamed",
                _now_iso(),
                expires_at,
            ),
        )
        conn.commit()

    log.info("Stored encrypted file %s for receiver %s", file_id, recipient_id)
    return JSONResponse({"file_id": file_id, "expires_at": expires_at}, status_code=201)


@app.get("/files")
def list_files(user_id: str = Query(..., description="Current user id")) -> dict[str, Any]:
    with get_connection(DB_PATH) as conn:
        rows = conn.execute(
            """
            SELECT id, sender_id, receiver_id, original_name, created_at, expires_at
            FROM files
            WHERE sender_id = ? OR receiver_id = ?
            ORDER BY created_at DESC
            """,
            (user_id, user_id),
        ).fetchall()
    return {
        "files": [
            {
                "id": r["id"],
                "sender_id": r["sender_id"],
                "receiver_id": r["receiver_id"],
                "original_name": r["original_name"],
                "created_at": r["created_at"],
                "expires_at": r["expires_at"],
            }
            for r in rows
        ]
    }


def _file_not_expired(expires_at: Optional[str]) -> bool:
    if not expires_at:
        return True
    try:
        exp = datetime.fromisoformat(expires_at)
    except ValueError:
        return False
    return datetime.now(timezone.utc) <= exp.replace(tzinfo=timezone.utc)


@app.post("/files/{file_id}/decrypt")
def decrypt_file_endpoint(
    file_id: str,
    user_id: str = Form(...),
    private_key_pem: str = Form(...),
) -> FileResponse:
    with get_connection(DB_PATH) as conn:
        row = conn.execute(
            "SELECT * FROM files WHERE id = ?",
            (file_id,),
        ).fetchone()
    if not row:
        raise HTTPException(status_code=404, detail="File not found")
    if not _file_not_expired(row["expires_at"]):
        raise HTTPException(status_code=410, detail="File expired")

    if user_id != row["receiver_id"]:
        raise HTTPException(status_code=403, detail="Only the receiver can decrypt with a private key")

    priv_tmp = tempfile.NamedTemporaryFile(mode="w", suffix=".pem", delete=False, encoding="utf-8")
    out_tmp = tempfile.NamedTemporaryFile(delete=False)
    try:
        priv_tmp.write(private_key_pem)
        priv_tmp.close()
        out_tmp.close()
        crypto_cli.decrypt_file(Path(row["storage_path"]), Path(out_tmp.name), Path(priv_tmp.name))
    except Exception as exc:
        log.exception("decrypt failed")
        raise HTTPException(status_code=500, detail=str(exc)) from exc
    finally:
        Path(priv_tmp.name).unlink(missing_ok=True)

    log.info("Decrypted file %s for user %s", file_id, user_id)

    out_path = Path(out_tmp.name)

    def _cleanup() -> None:
        out_path.unlink(missing_ok=True)

    return FileResponse(
        path=str(out_path),
        filename=row["original_name"],
        media_type="application/octet-stream",
        background=BackgroundTask(_cleanup),
    )


@app.post("/files/{file_id}/download-token")
def issue_download_token(file_id: str, user_id: str = Form(...)) -> dict[str, str]:
    with get_connection(DB_PATH) as conn:
        row = conn.execute("SELECT * FROM files WHERE id = ?", (file_id,)).fetchone()
    if not row:
        raise HTTPException(status_code=404, detail="File not found")
    if user_id not in (row["sender_id"], row["receiver_id"]):
        raise HTTPException(status_code=403, detail="Forbidden")
    if not _file_not_expired(row["expires_at"]):
        raise HTTPException(status_code=410, detail="File expired")

    token = serializer.dumps({"file_id": file_id, "user_id": user_id})
    return {"token": token, "expires_in_seconds": 900}


@app.post("/download-by-token")
def download_by_token(
    token: str = Form(...),
    private_key_pem: str = Form(...),
) -> FileResponse:
    """Redeem a short-lived token; private key is never placed in query strings."""
    try:
        payload = serializer.loads(token, max_age=900)
    except SignatureExpired as exc:
        raise HTTPException(status_code=401, detail="Token expired") from exc
    except BadSignature as exc:
        raise HTTPException(status_code=401, detail="Invalid token") from exc

    file_id = payload["file_id"]
    user_id = payload["user_id"]
    with get_connection(DB_PATH) as conn:
        row = conn.execute("SELECT * FROM files WHERE id = ?", (file_id,)).fetchone()
    if not row:
        raise HTTPException(status_code=404, detail="File not found")
    if not _file_not_expired(row["expires_at"]):
        raise HTTPException(status_code=410, detail="File expired")
    if user_id != row["receiver_id"]:
        raise HTTPException(status_code=403, detail="Token not valid for decrypt")

    priv_tmp = tempfile.NamedTemporaryFile(mode="w", suffix=".pem", delete=False, encoding="utf-8")
    out_tmp = tempfile.NamedTemporaryFile(delete=False)
    try:
        priv_tmp.write(private_key_pem)
        priv_tmp.close()
        out_tmp.close()
        crypto_cli.decrypt_file(Path(row["storage_path"]), Path(out_tmp.name), Path(priv_tmp.name))
    except Exception as exc:
        Path(out_tmp.name).unlink(missing_ok=True)
        raise HTTPException(status_code=500, detail=str(exc)) from exc
    finally:
        Path(priv_tmp.name).unlink(missing_ok=True)

    out_path = Path(out_tmp.name)

    def _cleanup() -> None:
        out_path.unlink(missing_ok=True)

    return FileResponse(
        path=str(out_path),
        filename=row["original_name"],
        media_type="application/octet-stream",
        background=BackgroundTask(_cleanup),
    )


frontend_dir = ROOT / "frontend"
if frontend_dir.is_dir():
    app.mount("/ui", StaticFiles(directory=str(frontend_dir), html=True), name="ui")
