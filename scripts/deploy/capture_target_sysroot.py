#!/usr/bin/env python3
import argparse
import pathlib
import shutil
import sys
import tarfile
import tempfile

import paramiko


def ssh_exec(client, command):
    stdin, stdout, stderr = client.exec_command(command)
    out = stdout.read().decode("utf-8", "ignore")
    err = stderr.read().decode("utf-8", "ignore")
    code = stdout.channel.recv_exit_status()
    return code, out, err


def print_progress(transferred, total):
    if total <= 0:
        return
    percent = int((transferred / total) * 100)
    sys.stdout.write(f"\rdownloading sysroot archive: {transferred // (1024 * 1024)} MiB / {total // (1024 * 1024)} MiB ({percent}%)")
    sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser(description="Capture a target-compatible sysroot snapshot from a live Pi.")
    parser.add_argument("--host", required=True)
    parser.add_argument("--user", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--name", default="rpi1-trixie")
    parser.add_argument("--remote-archive", default="/tmp/pifartbox-sysroot.tar.gz")
    parser.add_argument("--output-root", default="sysroots")
    args = parser.parse_args()

    output_root = pathlib.Path(args.output_root).resolve()
    sysroot_root = output_root / args.name
    if sysroot_root.exists():
        shutil.rmtree(sysroot_root)
    sysroot_root.mkdir(parents=True, exist_ok=True)

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(args.host, username=args.user, password=args.password, timeout=10, allow_agent=False, look_for_keys=False)
    try:
        archive_cmd = f"""
set -e
rm -f {args.remote_archive}
tar -C / -czf {args.remote_archive} \
  lib/arm-linux-gnueabihf \
  usr/lib/arm-linux-gnueabihf \
  usr/include \
  usr/lib/gcc/arm-linux-gnueabihf \
  lib/ld-linux-armhf.so.3
"""
        code, _, err = ssh_exec(client, archive_cmd)
        if code != 0:
            sys.stderr.write(err)
            return code

        with tempfile.TemporaryDirectory(prefix="pifartbox-sysroot-") as temp_dir:
            local_archive = pathlib.Path(temp_dir) / "sysroot.tar.gz"
            sftp = client.open_sftp()
            try:
                sftp.get(args.remote_archive, str(local_archive), callback=print_progress)
            finally:
                sftp.close()
            sys.stdout.write("\n")
            sys.stdout.flush()

            with tarfile.open(local_archive, "r:gz") as archive:
                archive.extractall(sysroot_root)

        cleanup_code, _, cleanup_err = ssh_exec(client, f"rm -f {args.remote_archive}")
        if cleanup_code != 0 and cleanup_err:
            sys.stderr.write(cleanup_err)

        sys.stdout.write(f"captured sysroot to {sysroot_root}\n")
        return 0
    finally:
        client.close()


if __name__ == "__main__":
    raise SystemExit(main())
