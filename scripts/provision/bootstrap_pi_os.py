#!/usr/bin/env python3
import argparse
import pathlib
import posixpath
import sys
import time

import paramiko


def write_text(stream_name, text):
    data = text.encode("utf-8", "replace")
    stream = sys.stderr.buffer if stream_name == "stderr" else sys.stdout.buffer
    stream.write(data)
    stream.flush()


def ssh_exec(client, command):
    stdin, stdout, stderr = client.exec_command(command)
    out = stdout.read().decode("utf-8", "ignore")
    err = stderr.read().decode("utf-8", "ignore")
    code = stdout.channel.recv_exit_status()
    return code, out, err


def ssh_exec_stream(client, command, password=None):
    stdin, stdout, stderr = client.exec_command(command, get_pty=True)
    if password is not None:
        time.sleep(0.5)
        stdin.write(password + "\n")
        stdin.flush()
    channel = stdout.channel

    while True:
        while channel.recv_ready():
            write_text("stdout", channel.recv(4096).decode("utf-8", "ignore"))
        while channel.recv_stderr_ready():
            write_text("stderr", channel.recv_stderr(4096).decode("utf-8", "ignore"))
        if channel.exit_status_ready() and not channel.recv_ready() and not channel.recv_stderr_ready():
            break
        time.sleep(0.1)

    return channel.recv_exit_status()


def upload_dir(sftp, local_root, remote_root):
    local_root = pathlib.Path(local_root)
    for path in local_root.rglob("*"):
        relative = path.relative_to(local_root).as_posix()
        remote_path = posixpath.join(remote_root, relative)
        if path.is_dir():
            try:
                sftp.mkdir(remote_path)
            except OSError:
                pass
        else:
            parent = posixpath.dirname(remote_path)
            try:
                sftp.mkdir(parent)
            except OSError:
                pass
            sftp.put(str(path), remote_path)


def main():
    parser = argparse.ArgumentParser(description="Upload and run the Raspberry Pi OS provisioning scaffold.")
    parser.add_argument("--host", required=True)
    parser.add_argument("--user", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--remote-staging-root", default="/home/pi/pifartbox-provision")
    parser.add_argument("--repo-root", default="/opt/piFartBox")
    parser.add_argument(
        "--apt-mode",
        choices=["full-upgrade", "install-only", "skip"],
        default="install-only",
        help="Choose whether provisioning performs a full upgrade, installs only the baseline packages, or skips apt work.",
    )
    args = parser.parse_args()

    local_script_root = pathlib.Path(__file__).resolve().parent
    local_repo_root = local_script_root.parent.parent
    local_deploy_root = local_repo_root / "deploy"

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(args.host, username=args.user, password=args.password, timeout=10, allow_agent=False, look_for_keys=False)
    try:
        code, out, err = ssh_exec(
            client,
            f"mkdir -p {args.remote_staging_root}/scripts/provision {args.remote_staging_root}/deploy/systemd {args.remote_staging_root}/deploy/nginx {args.remote_staging_root}/web",
        )
        if code != 0:
            sys.stderr.write(err)
            return code

        sftp = client.open_sftp()
        try:
            upload_dir(sftp, local_script_root, f"{args.remote_staging_root}/scripts/provision")
            upload_dir(sftp, local_deploy_root / "systemd", f"{args.remote_staging_root}/deploy/systemd")
            upload_dir(sftp, local_deploy_root / "nginx", f"{args.remote_staging_root}/deploy/nginx")
            upload_dir(sftp, local_repo_root / "web", f"{args.remote_staging_root}/web")
        finally:
            sftp.close()

        write_text(
            "stdout",
            f"[bootstrap] starting remote provisioning on {args.user}@{args.host} "
            f"(apt-mode={args.apt_mode}, staging={args.remote_staging_root})\n",
        )

        remote_command = f"""
set -e
cleanup() {{
  stty echo >/dev/null 2>&1 || true
}}
trap cleanup EXIT
stty -echo >/dev/null 2>&1 || true
sudo -S -p '' env REPO_ROOT='{args.repo_root}' SERVICE_SOURCE_DIR='{args.remote_staging_root}/deploy/systemd' NGINX_SOURCE_DIR='{args.remote_staging_root}/deploy/nginx' WEB_SOURCE_DIR='{args.remote_staging_root}/web' TARGET_USER='{args.user}' APT_MODE='{args.apt_mode}' bash '{args.remote_staging_root}/scripts/provision/provision_pi_os.sh'
"""
        code = ssh_exec_stream(client, remote_command, password=args.password)
        write_text("stdout", f"\n[bootstrap] remote provisioning finished with exit code {code}\n")
        return code
    finally:
        client.close()


if __name__ == "__main__":
    raise SystemExit(main())
