#!/usr/bin/env python3
import argparse
import json
import pathlib
import shutil
import sys
import tarfile
import tempfile
from datetime import datetime, timezone

import paramiko


def ssh_exec(client, command):
    stdin, stdout, stderr = client.exec_command(command)
    out = stdout.read().decode("utf-8", "ignore")
    err = stderr.read().decode("utf-8", "ignore")
    code = stdout.channel.recv_exit_status()
    return code, out, err


def main():
    parser = argparse.ArgumentParser(description="Capture a Pi target sysroot for cross-build compatibility.")
    parser.add_argument("--host", required=True)
    parser.add_argument("--user", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--output-dir", default="sysroots/live-pi")
    parser.add_argument("--remote-archive", default="/home/pi/piFartBox-sysroot.tar.gz")
    args = parser.parse_args()

    output_dir = pathlib.Path(args.output_dir).resolve()
    temp_dir = pathlib.Path(tempfile.mkdtemp(prefix="piFartBox-sysroot-"))
    local_archive = temp_dir / "piFartBox-sysroot.tar.gz"

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(args.host, username=args.user, password=args.password, timeout=10, allow_agent=False, look_for_keys=False)
    try:
        remote_script = f"""
set -e
echo '{args.password}' | sudo -S rm -f '{args.remote_archive}'
echo '{args.password}' | sudo -S tar -C / -czf '{args.remote_archive}' \\
  lib \\
  usr/include \\
  usr/lib \\
  usr/arm-linux-gnueabihf
"""
        code, out, err = ssh_exec(client, remote_script)
        if code != 0:
            sys.stderr.write(err)
            return code

        sftp = client.open_sftp()
        try:
            sftp.get(args.remote_archive, str(local_archive))
        finally:
            sftp.close()
    finally:
        client.close()

    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    with tarfile.open(local_archive, "r:gz") as archive:
        archive.extractall(output_dir)

    metadata = {
        "host": args.host,
        "captured_at": datetime.now(timezone.utc).isoformat(),
        "source_archive": args.remote_archive,
    }
    (output_dir / "sysroot-metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(output_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
