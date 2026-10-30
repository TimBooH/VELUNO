#!/usr/bin/env python3
"""Create a local signed update manifest. Does not upload or publish anything."""
import argparse
import base64
import hashlib
import json
import pathlib
import subprocess
import tempfile
from datetime import datetime, timezone

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("appimage", type=pathlib.Path)
parser.add_argument("--private-key", type=pathlib.Path, required=True)
parser.add_argument("--url", required=True)
parser.add_argument("--version", required=True)
parser.add_argument("--channel", choices=["stable", "unstable"], required=True)
parser.add_argument("--schema", type=int, required=True)
parser.add_argument("--min-schema", type=int, default=1)
parser.add_argument("--min-glibc", default="2.39")
parser.add_argument("--build", required=True, help="UTC build time as embedded in the build, ISO 8601")
parser.add_argument("--notes", default="")
parser.add_argument("--output", type=pathlib.Path, required=True)
args = parser.parse_args()
if not args.url.startswith("https://"):
    parser.error("--url must use HTTPS")
if args.channel == "stable" and "-" in args.version:
    parser.error("development versions cannot use the stable channel")
# Private key is passed to OpenSSL by filename and never printed or uploaded.
digest = hashlib.sha256()
with args.appimage.open("rb") as source:
    while chunk := source.read(1024 * 1024):
        digest.update(chunk)
payload = json.dumps({"format": 1, "version": args.version, "build": args.build,
    "channel": args.channel, "platform": "linux", "arch": "x86_64",
    "schema": args.schema, "minSchema": args.min_schema, "minGlibc": args.min_glibc,
    "url": args.url, "size": args.appimage.stat().st_size,
    "sha256": digest.hexdigest(), "notes": args.notes}, ensure_ascii=False,
    separators=(",", ":")).encode()
with tempfile.TemporaryDirectory(prefix="veluno-sign-") as temporary:
    message = pathlib.Path(temporary) / "payload.json"
    signature = pathlib.Path(temporary) / "signature"
    message.write_bytes(payload)
    subprocess.run(["openssl", "pkeyutl", "-sign", "-rawin", "-inkey", str(args.private_key),
        "-in", str(message), "-out", str(signature)], check=True)
    envelope = {"payload": base64.b64encode(payload).decode(),
        "signature": base64.b64encode(signature.read_bytes()).decode()}
args.output.write_text(json.dumps(envelope, indent=2) + "\n")
