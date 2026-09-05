/**
 * Secure File Sharing — E2EE demo UI.
 * Session fields persist in localStorage.
 */

(function () {
  const LS = {
    user: "sfs_user_id",
    key: "sfs_private_key_pem",
    recipient: "sfs_recipient_id",
    api: "sfs_api_base",
    decryptFile: "sfs_decrypt_file_id",
  };

  const $ = (id) => document.getElementById(id);
  const logEl = $("log");

  function apiBase() {
    const v = $("apiBase").value.trim();
    if (v) return v.replace(/\/$/, "");
    return window.location.origin;
  }

  function log(msg, cls) {
    const line = document.createElement("div");
    if (cls) line.className = cls;
    line.textContent = new Date().toISOString() + " " + msg;
    logEl.appendChild(line);
    logEl.scrollTop = logEl.scrollHeight;
  }

  function formatApiError(data) {
    if (data == null) return "Unknown error";
    const d = data.detail;
    if (typeof d === "string") return d;
    if (Array.isArray(d)) {
      return d
        .map((x) => (x.msg ? x.loc + ": " + x.msg : JSON.stringify(x)))
        .join("; ");
    }
    return JSON.stringify(data);
  }

  function saveSession() {
    try {
      localStorage.setItem(LS.user, $("userId").value);
      localStorage.setItem(LS.key, $("privateKey").value);
      localStorage.setItem(LS.recipient, $("recipientId").value);
      localStorage.setItem(LS.api, $("apiBase").value);
      localStorage.setItem(LS.decryptFile, $("decryptFileId").value);
    } catch (e) {
      /* ignore */
    }
  }

  function loadSession() {
    const api = localStorage.getItem(LS.api);
    if (api) $("apiBase").value = api;
    else $("apiBase").value = window.location.origin;
    const uid = localStorage.getItem(LS.user);
    if (uid) $("userId").value = uid;
    const pk = localStorage.getItem(LS.key);
    if (pk) $("privateKey").value = pk;
    const rec = localStorage.getItem(LS.recipient);
    if (rec) $("recipientId").value = rec;
    const df = localStorage.getItem(LS.decryptFile);
    if (df) $("decryptFileId").value = df;
  }

  ["userId", "privateKey", "recipientId", "apiBase", "decryptFileId"].forEach((id) => {
    $(id).addEventListener("change", saveSession);
    $(id).addEventListener("blur", saveSession);
  });

  loadSession();

  $("btnRegister").onclick = async () => {
    const name = $("regName").value.trim() || "user";
    const fd = new FormData();
    fd.append("display_name", name);
    try {
      const res = await fetch(apiBase() + "/users", { method: "POST", body: fd });
      const data = await res.json().catch(() => ({}));
      if (!res.ok) {
        log("Register failed: " + formatApiError(data), "err");
        return;
      }
      $("userId").value = data.user_id;
      $("privateKey").value = data.private_key_pem || "";
      saveSession();
      log("Registered. user_id=" + data.user_id, "ok");
      log("Save your private key — shown once (also saved in this browser).", "ok");
    } catch (e) {
      log("Register error: " + e, "err");
    }
  };

  $("btnRefresh").onclick = async () => {
    const uid = $("userId").value.trim();
    if (!uid) {
      log("Set user_id first (Register or paste saved id).", "err");
      return;
    }
    try {
      const res = await fetch(apiBase() + "/files?user_id=" + encodeURIComponent(uid));
      const data = await res.json();
      if (!res.ok) {
        log("List failed: " + formatApiError(data), "err");
        return;
      }
      const ul = $("fileList");
      ul.innerHTML = "";
      (data.files || []).forEach((f) => {
        const li = document.createElement("li");
        li.title = "Click to use this file_id for decrypt";
        li.innerHTML =
          "<span>" +
          escapeHtml(f.original_name) +
          '</span><span class="mono">' +
          escapeHtml(f.id.slice(0, 8)) +
          "…</span>";
        li.dataset.id = f.id;
        li.onclick = () => {
          $("decryptFileId").value = f.id;
          saveSession();
          log("Selected file_id for decrypt: " + f.id, "ok");
        };
        ul.appendChild(li);
      });
      log("Listed " + (data.files || []).length + " file(s).", "ok");
    } catch (e) {
      log("List error: " + e, "err");
    }
  };

  $("btnUpload").onclick = async () => {
    const sender = $("userId").value.trim();
    const recipient = $("recipientId").value.trim();
    const f = $("fileInput").files[0];
    const exp = $("expires").value.trim();
    if (!sender || !recipient || !f) {
      log("Need user_id (sender), recipient user_id, and a file.", "err");
      return;
    }
    if (sender === recipient) {
      log("Sender and recipient must differ. Register twice and paste the other user_id.", "err");
      return;
    }
    const fd = new FormData();
    fd.append("sender_id", sender);
    fd.append("recipient_id", recipient);
    fd.append("file", f);
    if (exp) fd.append("expires_in_hours", exp);
    try {
      const res = await fetch(apiBase() + "/upload", { method: "POST", body: fd });
      const data = await res.json().catch(() => ({}));
      if (!res.ok) {
        log("Upload failed: " + formatApiError(data), "err");
        return;
      }
      log("Uploaded. file_id=" + data.file_id, "ok");
      $("btnRefresh").click();
    } catch (e) {
      log("Upload error: " + e, "err");
    }
  };

  $("btnDecrypt").onclick = async () => {
    const fileId = $("decryptFileId").value.trim();
    const uid = $("userId").value.trim();
    const pk = $("privateKey").value.trim();
    if (!fileId || !uid || !pk) {
      log("Fill file_id (click a file in the list), user_id, and private key PEM. Receiver only.", "err");
      return;
    }
    const fd = new FormData();
    fd.append("user_id", uid);
    fd.append("private_key_pem", pk);
    try {
      const res = await fetch(apiBase() + "/files/" + encodeURIComponent(fileId) + "/decrypt", {
        method: "POST",
        body: fd,
      });
      if (!res.ok) {
        const err = await res.json().catch(() => ({}));
        log("Decrypt failed: " + formatApiError(err), "err");
        return;
      }
      const blob = await res.blob();
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download =
        res.headers.get("content-disposition")?.match(/filename="?([^";]+)"?/)?.[1] || "download.bin";
      a.click();
      URL.revokeObjectURL(url);
      log("Download started.", "ok");
    } catch (e) {
      log("Decrypt error: " + e, "err");
    }
  };

  function escapeHtml(s) {
    return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  log(
    "Ready. Tab A: register sender. Tab B: register receiver, copy receiver user_id → Recipient field on Tab A, then upload.",
    "ok"
  );
})();
