#!/usr/bin/env bash
# End-to-end smoke: build C++ CLI, encrypt fixture, decrypt, cmp.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "== Build C++ core"
cmake -S cpp_core -B cpp_core/build
cmake --build cpp_core/build -j

BIN="${SECURE_CORE_BIN:-$ROOT/cpp_core/build/secure_core}"
if [[ ! -x "$BIN" ]]; then
  echo "Missing binary: $BIN"
  exit 1
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

PUB="$TMP/pub.pem"
PRIV="$TMP/priv.pem"
PLAIN="$ROOT/cpp_core/tests/fixtures/hello.bin"
OUT_ENC="$TMP/out.bin"
OUT_DEC="$TMP/hello.out"

echo "== keygen"
"$BIN" keygen --pub "$PUB" --priv "$PRIV" --bits 2048

echo "== encrypt fixture"
"$BIN" encrypt --in "$PLAIN" --out "$OUT_ENC" --recipient-pub "$PUB"

echo "== decrypt"
"$BIN" decrypt --in "$OUT_ENC" --out "$OUT_DEC" --private-key "$PRIV"

echo "== cmp"
cmp "$PLAIN" "$OUT_DEC"
echo "OK: plaintext matches fixture."

echo "== CTest"
ctest --test-dir cpp_core/build --output-on-failure

echo "All smoke checks passed."
