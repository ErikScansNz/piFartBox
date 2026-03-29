#!/usr/bin/env python3
import argparse
import json
import os
import pathlib
import shlex
import shutil
import subprocess
import sys
import tempfile
from typing import Tuple

import paramiko


def run_local(cmd, cwd=None):
    subprocess.run(cmd, cwd=cwd, check=True)


def read_local_output(cmd, cwd=None) -> str:
    completed = subprocess.run(cmd, cwd=cwd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return completed.stdout.strip()


def ssh_exec(client: paramiko.SSHClient, command: str) -> Tuple[int, str, str]:
    stdin, stdout, stderr = client.exec_command(command)
    output = stdout.read().decode("utf-8", "ignore")
    error = stderr.read().decode("utf-8", "ignore")
    exit_code = stdout.channel.recv_exit_status()
    return exit_code, output, error


def main() -> int:
    parser = argparse.ArgumentParser(description="Bootstrap a remote piFartBox checkout over SSH using a git bundle.")
    parser.add_argument("--host", required=True)
    parser.add_argument("--user", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--repo-root", default=os.getcwd())
    parser.add_argument("--branch", default="main")
    parser.add_argument("--deploy-root", default="/opt/piFartBox")
    parser.add_argument("--remote-staging-root", default="/home/pi/piFartBox-update")
    args = parser.parse_args()

    repo_root = pathlib.Path(args.repo_root).resolve()
    temp_dir = pathlib.Path(tempfile.mkdtemp(prefix="piFartBox-bootstrap-"))
    try:
        bundle_path = temp_dir / "piFartBox.bundle"
        manifest_path = temp_dir / "manifest.json"

        short_commit = read_local_output(["git", "-C", str(repo_root), "rev-parse", "--short", args.branch])
        full_commit = read_local_output(["git", "-C", str(repo_root), "rev-parse", args.branch])
        run_local(["git", "-C", str(repo_root), "bundle", "create", str(bundle_path), args.branch])

        manifest = {
            "project": "piFartBox",
            "branch": args.branch,
            "commit": short_commit,
            "full_commit": full_commit,
            "bundle": "piFartBox.bundle",
        }
        manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        client.connect(args.host, username=args.user, password=args.password, timeout=10, allow_agent=False, look_for_keys=False)
        try:
            quoted_password = shlex.quote(args.password)
            quoted_deploy_root = shlex.quote(args.deploy_root)
            quoted_remote_staging = shlex.quote(args.remote_staging_root)
            quoted_branch = shlex.quote(args.branch)
            quoted_user = shlex.quote(args.user)

            code, out, err = ssh_exec(client, f"mkdir -p {quoted_remote_staging}")
            if out:
                sys.stdout.write(out)
            if err:
                sys.stderr.write(err)
            if code != 0:
                return code

            sftp = client.open_sftp()
            try:
                sftp.put(str(bundle_path), f"{args.remote_staging_root}/piFartBox.bundle")
                sftp.put(str(manifest_path), f"{args.remote_staging_root}/manifest.json")
            finally:
                sftp.close()

            remote_script = f"""
set -e
echo {quoted_password} | sudo -S mkdir -p {quoted_deploy_root}
if [ ! -d {quoted_deploy_root}/.git ]; then
  echo {quoted_password} | sudo -S git init {quoted_deploy_root}
fi
echo {quoted_password} | sudo -S git -C {quoted_deploy_root} fetch {quoted_remote_staging}/piFartBox.bundle {quoted_branch}
echo {quoted_password} | sudo -S git -C {quoted_deploy_root} checkout -B {quoted_branch} FETCH_HEAD
echo {quoted_password} | sudo -S sh -c "git -C {quoted_deploy_root} rev-parse HEAD > {quoted_deploy_root}/.deployed-revision"
echo {quoted_password} | sudo -S chown -R {quoted_user}:{quoted_user} {quoted_deploy_root}
"""
            code, out, err = ssh_exec(client, remote_script)
            sys.stdout.write(out)
            if err:
                sys.stderr.write(err)
            if code != 0:
                return code

            verify_cmd = f"git -C {quoted_deploy_root} rev-parse HEAD && ls {quoted_deploy_root}"
            code, out, err = ssh_exec(client, verify_cmd)
            sys.stdout.write(out)
            if err:
                sys.stderr.write(err)
            return code
        finally:
            client.close()
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
