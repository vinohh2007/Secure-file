# Share Your Local App with a Public Link (ngrok)

This guide shows how to run the project **on your computer** and give others a **temporary HTTPS link** using **ngrok**. The server is still **local**; ngrok only **forwards** traffic from the internet to your machine.

**Use case:** demos, viva, showing the UI to teammates. **Not** a replacement for real hosting on a VPS.

---

## What you need first

1. The app runs locally without ngrok:

   - C++ `secure_core` built.
   - `uvicorn` running on port **8000** (default in this project).

2. A free [ngrok](https://ngrok.com/) account (for the auth token).

---

## Step 1 — Install ngrok

### Windows

1. Download the ZIP from [ngrok download](https://ngrok.com/download).
2. Extract `ngrok.exe` to a folder that is on your **PATH** (e.g. `C:\ngrok\`), *or* run it with the full path.
3. Optional: add that folder to **Environment Variables → Path** so you can type `ngrok` anywhere.

### Linux / macOS

```bash
# Example: download official binary (check ngrok site for latest URL)
curl -s https://ngrok-agent.s3.amazonaws.com/ngrok.asc | sudo tee /etc/apt/trusted.gpg.d/ngrok.asc >/dev/null
# Or use package managers / direct zip from ngrok.com/download
```

Many users simply **download the archive**, unzip, and run `./ngrok` from that directory.

---

## Step 2 — Connect your ngrok account (one time)

1. Sign up at [dashboard.ngrok.com](https://dashboard.ngrok.com/).
2. Copy your **authtoken** from the dashboard.
3. Run:

```bash
ngrok config add-authtoken YOUR_TOKEN_HERE
```

On Windows (PowerShell or cmd):

```text
ngrok config add-authtoken YOUR_TOKEN_HERE
```

This stores the token so ngrok can start tunnels under your account.

---

## Step 3 — Start the project locally

From the **project root** (same as in the main README):

**Linux / macOS**

```bash
export PYTHONPATH=$PWD
export SECURE_CORE_BIN=$PWD/cpp_core/build/secure_core
source .venv/bin/activate   # if you use a venv
uvicorn backend.main:app --host 0.0.0.0 --port 8000
```

**Windows (PowerShell)**

```powershell
$env:PYTHONPATH = $PWD
$env:SECURE_CORE_BIN = "$PWD\cpp_core\build\secure_core.exe"
.\.venv\Scripts\Activate.ps1
uvicorn backend.main:app --host 0.0.0.0 --port 8000
```

Leave this terminal **open**. You should see Uvicorn listening on `8000`.

---

## Step 4 — Start ngrok (second terminal)

Open a **new** terminal and run:

```bash
ngrok http 8000
```

You will see a screen like:

```text
Forwarding   https://abcd-12-34-56-78.ngrok-free.app -> http://localhost:8000
```

**That `https://....ngrok-free.app` URL is what you share.**

- Opening the **base URL** (no path) sends people to the **web UI** (same as `/ui/`).
- **Swagger API docs** (if you need them): `https://YOUR-SUBDOMAIN.ngrok-free.app/docs`

---

## Step 5 — Browser and “API base” in the UI

1. **Free ngrok** may show an **interstitial warning page** the first time someone opens the link; they click **Visit Site** to continue.
2. In the web UI (**Connection → Backend URL**), set the base to your ngrok URL **without** a trailing slash, for example:

   `https://abcd-12-34-56-78.ngrok-free.app`

   So the browser calls the same host for `/users`, `/upload`, etc.

3. If you open the UI from `http://127.0.0.1:8000/`, the default API base is still **localhost**. For sharing with others, they should open the **ngrok base URL** (it redirects to `/ui/`). Then the page origin matches the API (or paste the ngrok URL in **Backend URL** if needed).

---

## Security notes (read this)

| Topic | What to know |
|--------|----------------|
| **Exposure** | While ngrok runs, **anyone with the link** can reach your API. Use only for **short demos**. |
| **Secrets** | Set a strong random `E2EE_APP_SECRET` in the environment if you use download tokens; default dev secret is weak. |
| **Private keys** | The demo UI sends private keys to **your** server for decrypt—acceptable for a course demo, not for real secrets. |
| **Stopping** | Press `Ctrl+C` in the ngrok terminal to close the tunnel; the public link stops working immediately. |

---

## Troubleshooting

| Problem | What to try |
|---------|----------------|
| `connection refused` in ngrok | Uvicorn not running, or wrong port (must be **8000** if you used `ngrok http 8000`). |
| `502 Bad Gateway` | App crashed; check the Uvicorn terminal for errors. |
| CORS errors | This project allows `*` for development; if you changed CORS, allow your ngrok origin. |
| URL changes every time | Normal on **free** ngrok: each new `ngrok http` run can get a **new** subdomain. Share the new URL each time. |
| Need a **fixed** URL | ngrok **paid** plans offer reserved domains; optional for stable demos. |

---

## One-line summary

**Terminal 1:** run `uvicorn ... --port 8000`. **Terminal 2:** run `ngrok http 8000`. Share the **https** forwarding URL — it opens the **web UI**; use **`/docs`** only if you want Swagger.
