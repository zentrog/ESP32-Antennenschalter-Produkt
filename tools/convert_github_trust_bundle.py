#!/usr/bin/env python3
"""Convert the two-entry legacy compact CA bundle to ESP-IDF's X.509 format."""

from pathlib import Path
import struct
import json


ROOT = Path(__file__).resolve().parents[1]
source = ROOT / "certs" / "github-roots.json"
output = ROOT / "include" / "GitHubTrustBundle.h"
definitions = json.loads(source.read_text(encoding="ascii"))["roots"]
entries: list[tuple[bytes, bytes]] = [
    (bytes.fromhex(item["subject_der"]), bytes.fromhex(item["public_key_spki_der"]))
    for item in definitions
]
if not entries or any(not subject or not key for subject, key in entries):
    raise SystemExit("CA bundle source contains an empty subject or public key")

entries.sort(key=lambda entry: entry[0])
payload = bytearray()
offsets: list[int] = []
table_size = 4 * len(entries)
for subject, public_key in entries:
    offsets.append(table_size + len(payload))
    payload.extend(struct.pack("<HH", len(subject), len(public_key)))
    payload.extend(subject)
    payload.extend(public_key)
bundle = struct.pack(f"<{len(offsets)}I", *offsets) + payload

lines = [
    "#pragma once",
    "#include <stddef.h>",
    "#include <stdint.h>",
    "// Generated from certs/github-roots.json in ESP-IDF x509_crt_bundle format.",
    "static const uint8_t GITHUB_TRUST_BUNDLE[] = {",
]
for i in range(0, len(bundle), 16):
    lines.append("  " + ",".join(f"0x{x:02X}" for x in bundle[i : i + 16]) + ",")
lines += ["};", f"static constexpr size_t GITHUB_TRUST_BUNDLE_SIZE = {len(bundle)};", ""]
output.write_text("\n".join(lines), encoding="ascii", newline="\n")
print(f"Converted {len(entries)} sorted CA entries to ESP-IDF bundle format ({len(bundle)} bytes)")
