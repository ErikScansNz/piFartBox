#!/usr/bin/env python3
import argparse
import json
import pathlib
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
from datetime import datetime, timezone

import paramiko


EXCLUDED_TOP_LEVEL = {
    ".git",
    ".idea",
    ".vscode",
    ".cache",
    "build",
    "build-armv6",
    "build-native",
    "sysroots",
}

EXCLUDED_NAMES = {
    "__pycache__",
}

EXCLUDED_SUFFIXES = {
    ".pyc",
    ".pyo",
    ".tar",
    ".tar.gz",
    ".tgz",
}


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


def should_exclude(relative_path):
    parts = relative_path.parts
    if not parts:
        return False
    if parts[0] in EXCLUDED_TOP_LEVEL:
        return True
    if any(part in EXCLUDED_NAMES for part in parts):
        return True
    if relative_path.name in EXCLUDED_TOP_LEVEL:
        return True
    if relative_path.suffix.lower() in EXCLUDED_SUFFIXES:
        return True
    return False


def create_source_archive(repo_root, commit):
    with tempfile.TemporaryDirectory(prefix="pifartbox-ondevice-") as temp_dir:
        archive_path = pathlib.Path(temp_dir) / f"piFartBox-source-{commit}.tar.gz"
        with tarfile.open(archive_path, "w:gz") as archive:
            for path in repo_root.rglob("*"):
                relative = path.relative_to(repo_root)
                if should_exclude(relative):
                    continue
                archive.add(path, arcname=pathlib.Path("piFartBox-source") / relative, recursive=False)
        final_archive = pathlib.Path(tempfile.gettempdir()) / f"piFartBox-source-{commit}.tar.gz"
        shutil.copy2(archive_path, final_archive)
        return final_archive


def upload_progress(transferred, total):
    if total <= 0:
        return
    percent = int((transferred / total) * 100)
    write_text("stdout", f"\ruploading source archive: {transferred // (1024 * 1024)} MiB / {total // (1024 * 1024)} MiB ({percent}%)")


def main():
    parser = argparse.ArgumentParser(description="Upload the repo, preflight native dependencies, and build piFartBox on-device.")
    parser.add_argument("--host", required=True)
    parser.add_argument("--user", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--repo-root", default="/opt/piFartBox")
    parser.add_argument("--remote-staging-root", default="")
    parser.add_argument("--source-root", default="")
    parser.add_argument("--build-root", default="")
    parser.add_argument("--parallel", type=int, default=1)
    parser.add_argument("--build-type", default="Release")
    parser.add_argument("--generator", default="Ninja")
    parser.add_argument("--dependency-mode", choices=["install", "check", "skip"], default="install")
    parser.add_argument("--preflight-only", action="store_true")
    parser.add_argument("--restart-service", action="store_true")
    args = parser.parse_args()

    repo_root = pathlib.Path(__file__).resolve().parents[2]
    commit = (
        pathlib.Path(repo_root / ".git").exists()
        and subprocess.run(
            ["git", "-C", str(repo_root), "rev-parse", "--short", "HEAD"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        ).stdout.strip()
        or "unknown"
    )
    created_at = datetime.now(timezone.utc).isoformat()

    remote_staging_root = args.remote_staging_root or f"/home/{args.user}/pifartbox-native-build"
    source_root = args.source_root or f"{args.repo_root}/source-native"
    build_root = args.build_root or f"{args.repo_root}/build-native"
    runtime_revision = f"{commit}-native"
    runtime_root = f"{args.repo_root}/runtime/{runtime_revision}"
    remote_archive = f"{remote_staging_root}/piFartBox-source-{commit}.tar.gz"

    archive_path = create_source_archive(repo_root, commit)

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(args.host, username=args.user, password=args.password, timeout=10, allow_agent=False, look_for_keys=False)
    try:
        code, _, err = ssh_exec(client, f"mkdir -p {remote_staging_root}")
        if code != 0:
            sys.stderr.write(err)
            return code

        sftp = client.open_sftp()
        try:
            sftp.put(str(archive_path), remote_archive, callback=upload_progress)
        finally:
            sftp.close()
        write_text("stdout", "\n")

        manifest_json = json.dumps(
            {
                "project": "piFartBox",
                "commit": commit,
                "built_on_target": True,
                "build_type": args.build_type,
                "generator": args.generator,
                "created_at": created_at,
            },
            indent=2,
        ).replace("'", "'\"'\"'")

        restart_clause = ""
        if args.restart_service:
            restart_clause = """
log_step "Restarting pifartbox-runtime.service"
systemctl restart pifartbox-runtime.service
"""
        remote_command = f"""
set -e
cleanup() {{
  stty echo >/dev/null 2>&1 || true
}}
trap cleanup EXIT
stty -echo >/dev/null 2>&1 || true
sudo -S -p '' env TARGET_USER='{args.user}' SOURCE_ROOT='{source_root}' BUILD_ROOT='{build_root}' REPO_ROOT='{args.repo_root}' RUNTIME_ROOT='{runtime_root}' RUNTIME_REVISION='{runtime_revision}' REMOTE_ARCHIVE='{remote_archive}' BUILD_TYPE='{args.build_type}' BUILD_GENERATOR='{args.generator}' BUILD_PARALLEL='{args.parallel}' MANIFEST_JSON='{manifest_json}' DEPENDENCY_MODE='{args.dependency_mode}' PREFLIGHT_ONLY='{"1" if args.preflight_only else "0"}' bash <<'ROOTSCRIPT'
set -euo pipefail
log_step() {{
  printf '\\n[%s] %s\\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" "$1"
}}
preflight_packages() {{
  missing=()
  dpkg -s cmake >/dev/null 2>&1 || missing+=("cmake")
  dpkg -s ninja-build >/dev/null 2>&1 || missing+=("ninja-build")
  dpkg -s build-essential >/dev/null 2>&1 || missing+=("build-essential")
  dpkg -s pkgconf >/dev/null 2>&1 || missing+=("pkgconf")
  dpkg -s git >/dev/null 2>&1 || missing+=("git")
  dpkg -s python3 >/dev/null 2>&1 || missing+=("python3")
  if [ "${{#missing[@]}}" -eq 0 ]; then
    log_step "Native build dependencies already present"
    return 0
  fi
  case "$DEPENDENCY_MODE" in
    install)
      log_step "Installing native build dependencies: ${{missing[*]}}"
      apt-get update
      apt-get install -y "${{missing[@]}}"
      ;;
    check)
      echo "Missing native build dependencies: ${{missing[*]}}" >&2
      exit 3
      ;;
    skip)
      log_step "Skipping dependency install despite missing packages: ${{missing[*]}}"
      ;;
  esac
}}
preflight_packages
log_step "Preparing native source and build roots"
mkdir -p "$REPO_ROOT" "$REPO_ROOT/runtime"
rm -rf "$SOURCE_ROOT" "$BUILD_ROOT"
mkdir -p "$SOURCE_ROOT" "$BUILD_ROOT"
log_step "Extracting uploaded source archive"
tar -xzf "$REMOTE_ARCHIVE" -C "$SOURCE_ROOT" --strip-components=1
chown -R "$TARGET_USER:$TARGET_USER" "$SOURCE_ROOT" "$BUILD_ROOT" "$REPO_ROOT/runtime"
if [ "$PREFLIGHT_ONLY" = "1" ]; then
  log_step "Preflight complete"
  exit 0
fi
log_step "Configuring native build"
su - "$TARGET_USER" -c "cmake -S '$SOURCE_ROOT' -B '$BUILD_ROOT' -G '$BUILD_GENERATOR' -DCMAKE_BUILD_TYPE='$BUILD_TYPE'"
log_step "Building pi_fartbox_runtime on-device"
su - "$TARGET_USER" -c "cmake --build '$BUILD_ROOT' --parallel '$BUILD_PARALLEL'"
log_step "Publishing native runtime artifact"
rm -rf "$RUNTIME_ROOT"
mkdir -p "$RUNTIME_ROOT/bin"
install -m 0755 "$BUILD_ROOT/apps/runtime/pi_fartbox_runtime" "$RUNTIME_ROOT/bin/pi_fartbox_runtime"
printf '%s\n' "$MANIFEST_JSON" > "$RUNTIME_ROOT/manifest.json"
ln -sfn "$RUNTIME_ROOT" "$REPO_ROOT/runtime-current"
chown -R "$TARGET_USER:$TARGET_USER" "$REPO_ROOT/runtime" "$REPO_ROOT/runtime-current"
{restart_clause}
log_step "Native on-device build pipeline complete"
ROOTSCRIPT
"""
        code = ssh_exec_stream(client, remote_command, password=args.password)
        return code
    finally:
        client.close()
        try:
            archive_path.unlink(missing_ok=True)
        except Exception:
            pass


if __name__ == "__main__":
    raise SystemExit(main())
