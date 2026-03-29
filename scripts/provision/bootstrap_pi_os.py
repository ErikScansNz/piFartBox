#!/usr/bin/env python3
import argparse
import pathlib
import posixpath
import sys

import paramiko


def ssh_exec(client, command):
    stdin, stdout, stderr = client.exec_command(command)
    out = stdout.read().decode("utf-8", "ignore")
    err = stderr.read().decode("utf-8", "ignore")
    code = stdout.channel.recv_exit_status()
    return code, out, err


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

        remote_command = f"""
set -e
echo '{args.password}' | sudo -S REPO_ROOT='{args.repo_root}' SERVICE_SOURCE_DIR='{args.remote_staging_root}/deploy/systemd' NGINX_SOURCE_DIR='{args.remote_staging_root}/deploy/nginx' WEB_SOURCE_DIR='{args.remote_staging_root}/web' TARGET_USER='{args.user}' bash '{args.remote_staging_root}/scripts/provision/provision_pi_os.sh'
"""
        code, out, err = ssh_exec(client, remote_command)
        sys.stdout.write(out)
        if err:
            sys.stderr.write(err)
        return code
    finally:
        client.close()


if __name__ == "__main__":
    raise SystemExit(main())
