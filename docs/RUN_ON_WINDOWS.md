# How to Run This Project on Windows

Step-by-step guide: install tools, build the C++ program, install Python packages, run the server, open the UI.

> **Folder names:** These steps assume you extracted the project to something like `C:\dev\secure-file-sharing`. Adjust paths to match your machine.

---

## Part A — What you need (dependencies)

| Dependency | What it is | Why we need it |
|------------|------------|----------------|
| **Git** (optional) | Clone/download the project | To get the source code |
| **CMake** | Build system | Configures and builds the C++ `secure_core` program |
| **A C++ compiler** | MSVC **or** MinGW-w64 | Compiles `.cpp` files |
| **OpenSSL** | Crypto library | AES-GCM, RSA; CMake must find *development* files (headers + libs) |
| **Python 3.10+** | Language runtime | Runs FastAPI |
| **pip** | Python package installer | Installs FastAPI, Uvicorn, etc. |

You do **not** need to “install OpenSSL” twice for Python—Python uses its own libraries for HTTPS. The **C++** part needs OpenSSL **for the compiler** (headers like `openssl/evp.h`).

---

## Part B — Easy paths on Windows

### Option 1 — MSYS2 + MinGW (often simplest for OpenSSL + CMake)

1. Install **MSYS2** from [msys2.org](https://www.msys2.org/).
2. Open **MSYS2 UCRT64** (or MINGW64) terminal and run:

```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-openssl
```

3. Add the MinGW `bin` folder to your **PATH** (example):

`C:\msys64\ucrt64\bin`

4. Open **PowerShell** or **cmd**, `cd` to your project folder, then build (see Part D).

### Option 2 — Visual Studio + OpenSSL (Win64)

1. Install **Visual Studio 2022** with workload **“Desktop development with C++”**.
2. Install **CMake** (cmake.org) and add it to PATH.
3. Install **OpenSSL for Windows** (prebuilt) from [Shining Light Productions](https://slproweb.com/products/Win32OpenSSL.html) — choose **Win64 OpenSSL** (full installer).
4. Note the install folder (e.g. `C:\Program Files\OpenSSL-Win64`). You may need to tell CMake where to find it:

```powershell
cmake -S cpp_core -B cpp_core/build -DOPENSSL_ROOT_DIR="C:/Program Files/OpenSSL-Win64"
```

(Use forward slashes or escaped backslashes in CMake.)

---

## Part C — Python virtual environment

In **PowerShell**, from the **project root** (folder that contains `backend/` and `cpp_core/`):

```powershell
cd C:\path\to\your\project
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r backend\requirements.txt
```

If `python` is not found, try `py -3` instead of `python`.

---

## Part D — Build the C++ `secure_core` program

Still in the project root, with compiler + CMake + OpenSSL available:

```powershell
cmake -S cpp_core -B cpp_core\build
cmake --build cpp_core\build --config Release
```

After a successful build you should have:

```text
cpp_core\build\Release\secure_core.exe
```

or, with MinGW (single-config generators):

```text
cpp_core\build\secure_core.exe
```

Tell the backend where the EXE is (use **your real path**):

```powershell
$env:SECURE_CORE_BIN = "$PWD\cpp_core\build\Release\secure_core.exe"
# OR for MinGW default:
# $env:SECURE_CORE_BIN = "$PWD\cpp_core\build\secure_core.exe"
```

---

## Part E — Run the FastAPI server

From the **project root**, with venv **activated**:

```powershell
$env:PYTHONPATH = $PWD
uvicorn backend.main:app --reload --host 127.0.0.1 --port 8000
```

- API docs: [http://127.0.0.1:8000/docs](http://127.0.0.1:8000/docs)
- Web UI: [http://127.0.0.1:8000/ui/](http://127.0.0.1:8000/ui/)

If Windows Firewall asks, allow access for **private** networks when developing locally.

---

## Part F — Quick checks if something fails

| Problem | What to try |
|---------|-------------|
| `cmake` not found | Install CMake; restart terminal; check PATH |
| OpenSSL not found | Set `-DOPENSSL_ROOT_DIR=...` to your OpenSSL install; use matching **64-bit** toolchain |
| `secure_core.exe` not created | Open `cpp_core\build` and read CMake error; fix compiler/OpenSSL |
| Python `ModuleNotFoundError: fastapi` | Activate `.venv` and run `pip install -r backend\requirements.txt` |
| Upload fails / keygen fails | Ensure `SECURE_CORE_BIN` points to the **actual** `.exe` path |

---

## Part G — Run only the C++ CLI (no Python)

Useful to prove crypto works without the server:

```powershell
.\cpp_core\build\secure_core.exe version
.\cpp_core\build\secure_core.exe keygen --pub alice.pub.pem --priv alice.priv.pem --bits 2048
```

(Generate a second keypair for “Bob”, then `encrypt` with Bob’s public key and `decrypt` with Bob’s private key—see project README.)

---

## Summary checklist

1. Install **CMake**, **C++ compiler**, **OpenSSL dev**, **Python 3.10+**.  
2. `python -m venv .venv` → `pip install -r backend\requirements.txt`.  
3. `cmake -S cpp_core -B cpp_core\build` → `cmake --build cpp_core\build`.  
4. Set `PYTHONPATH` and `SECURE_CORE_BIN` to `secure_core.exe`.  
5. `uvicorn backend.main:app --host 127.0.0.1 --port 8000`.  
6. Open `/ui/` in the browser.
