#!/usr/bin/env python3
import argparse
import json
import pathlib
import shlex
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

import paramiko


def read_output(cmd):
    completed = subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return completed.stdout.strip()


def ssh_exec(client, command):
    stdin, stdout, stderr = client.exec_command(command)
    out = stdout.read().decode("utf-8", "ignore")
    err = stderr.read().decode("utf-8", "ignore")
    code = stdout.channel.recv_exit_status()
    return code, out, err


def main():
    parser = argparse.ArgumentParser(description="Deploy a prebuilt runtime artifact to the Pi target.")
    parser.add_argument("--host", required=True)
    parser.add_argument("--user", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--deploy-root", default="/opt/piFartBox")
    parser.add_argument("--artifact-name", default="pi_fartbox_runtime_probe")
    parser.add_argument("--remote-staging-root", default="/home/pi/piFartBox-artifacts")
    args = parser.parse_args()

    binary_path = pathlib.Path(args.binary).resolve()
    if not binary_path.exists():
        raise SystemExit(f"missing binary: {binary_path}")

    commit = read_output(["git", "-C", str(pathlib.Path(".").resolve()), "rev-parse", "--short", "HEAD"])

    with tempfile.TemporaryDirectory(prefix="piFartBox-artifact-") as temp_dir:
        temp_root = pathlib.Path(temp_dir)
        package_root = temp_root / "piFartBox-runtime-artifact"
        bin_root = package_root / "bin"
        bin_root.mkdir(parents=True, exist_ok=True)
        staged_binary = bin_root / args.artifact_name
        staged_binary.write_bytes(binary_path.read_bytes())

        manifest = {
            "project": "piFartBox",
            "target": "armv6-rpi1-linux-gnueabihf",
            "commit": commit,
            "artifact_name": args.artifact_name,
            "created_at": datetime.now(timezone.utc).isoformat(),
        }
        (package_root / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

        archive_base = temp_root / f"piFartBox-runtime-{commit}"
        archive_path = shutil.make_archive(str(archive_base), "gztar", root_dir=str(package_root))

        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        client.connect(args.host, username=args.user, password=args.password, timeout=10, allow_agent=False, look_for_keys=False)
        try:
            quoted_password = shlex.quote(args.password)
            quoted_staging = shlex.quote(args.remote_staging_root)
            quoted_deploy = shlex.quote(args.deploy_root)
            quoted_user = shlex.quote(args.user)
            quoted_commit = shlex.quote(commit)
            quoted_artifact_name = shlex.quote(args.artifact_name)
            remote_archive = f"{args.remote_staging_root}/piFartBox-runtime-{commit}.tar.gz"

            code, out, err = ssh_exec(client, f"mkdir -p {quoted_staging}")
            if code != 0:
                sys.stderr.write(err)
                return code

            sftp = client.open_sftp()
            try:
                sftp.put(archive_path, remote_archive)
            finally:
                sftp.close()

            remote_script = f"""
set -e
echo {quoted_password} | sudo -S mkdir -p {quoted_deploy}/runtime/{quoted_commit}
echo {quoted_password} | sudo -S tar -xzf {shlex.quote(remote_archive)} -C {quoted_deploy}/runtime/{quoted_commit} --strip-components=1
echo {quoted_password} | sudo -S chmod 755 {quoted_deploy}/runtime/{quoted_commit}/bin/{quoted_artifact_name}
echo {quoted_password} | sudo -S ln -sfn {quoted_deploy}/runtime/{quoted_commit} {quoted_deploy}/runtime-current
echo {quoted_password} | sudo -S chown -R {quoted_user}:{quoted_user} {quoted_deploy}/runtime {quoted_deploy}/runtime-current
"""
            code, out, err = ssh_exec(client, remote_script)
            sys.stdout.write(out)
            if err:
                sys.stderr.write(err)
            return code
        finally:
            client.close()


if __name__ == "__main__":
    raise SystemExit(main())
