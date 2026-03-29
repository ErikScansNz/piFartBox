#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="${REPO_ROOT:-/opt/piFartBox}"
STATE_ROOT="${STATE_ROOT:-/var/lib/piFartBox}"
LOG_ROOT="${LOG_ROOT:-/var/log/piFartBox}"
WEB_ROOT="${WEB_ROOT:-$REPO_ROOT/web}"
RUNTIME_ROOT="${RUNTIME_ROOT:-$REPO_ROOT/runtime}"
SERVICE_SOURCE_DIR="${SERVICE_SOURCE_DIR:-$(pwd)/deploy/systemd}"
NGINX_SOURCE_DIR="${NGINX_SOURCE_DIR:-$(pwd)/deploy/nginx}"
WEB_SOURCE_DIR="${WEB_SOURCE_DIR:-}"
TARGET_USER="${TARGET_USER:-pi}"
APT_MODE="${APT_MODE:-install-only}"

export DEBIAN_FRONTEND=noninteractive

log_step() {
  printf '\n[%s] %s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" "$1"
}

package_exists() {
  local candidate
  candidate="$(apt-cache policy "$1" 2>/dev/null | awk '/Candidate:/ {print $2; exit}')"
  [[ -n "$candidate" && "$candidate" != "(none)" ]]
}

resolve_audio_runtime_package() {
  if package_exists libasound2; then
    printf 'libasound2\n'
    return
  fi
  if package_exists libasound2t64; then
    printf 'libasound2t64\n'
    return
  fi
  echo "Unable to find a supported ALSA runtime package (libasound2 or libasound2t64)" >&2
  exit 3
}

install_package_baseline() {
  log_step "Installing package baseline"
  local audio_runtime_package
  audio_runtime_package="$(resolve_audio_runtime_package)"
  apt-get install -y \
    ca-certificates \
    curl \
    git \
    jq \
    rsync \
    unzip \
    python3 \
    python3-venv \
    avahi-daemon \
    alsa-utils \
    "$audio_runtime_package" \
    nginx-light \
    udisks2 \
    dosfstools \
    exfatprogs \
    ntfs-3g
}

log_step "Provisioning piFartBox target"
log_step "Using apt mode: $APT_MODE"

case "$APT_MODE" in
  full-upgrade)
    log_step "Running apt-get update"
    apt-get update
    log_step "Running apt-get full-upgrade"
    apt-get -y full-upgrade
    install_package_baseline
    ;;
  install-only)
    log_step "Running apt-get update"
    apt-get update
    install_package_baseline
    ;;
  skip)
    log_step "Skipping apt operations"
    ;;
  *)
    echo "Unsupported APT_MODE: $APT_MODE" >&2
    exit 2
    ;;
esac

log_step "Creating install, runtime, state, and log directories"
mkdir -p \
  "$REPO_ROOT" \
  "$RUNTIME_ROOT" \
  "$STATE_ROOT" \
  "$LOG_ROOT" \
  "$WEB_ROOT"

if [[ -n "$WEB_SOURCE_DIR" && -d "$WEB_SOURCE_DIR" ]]; then
  log_step "Syncing web root scaffold"
  if command -v rsync >/dev/null 2>&1; then
    rsync -a "$WEB_SOURCE_DIR"/ "$WEB_ROOT"/
  else
    cp -a "$WEB_SOURCE_DIR"/. "$WEB_ROOT"/
  fi
fi

log_step "Installing systemd service template"
sed "s/@TARGET_USER@/$TARGET_USER/g" "$SERVICE_SOURCE_DIR/pifartbox-runtime.service" > /etc/systemd/system/pifartbox-runtime.service
chmod 0644 /etc/systemd/system/pifartbox-runtime.service

if [[ -d /etc/nginx/sites-available ]] && command -v nginx >/dev/null 2>&1; then
  log_step "Installing nginx site configuration"
  install -m 0644 "$NGINX_SOURCE_DIR/pifartbox.conf" /etc/nginx/sites-available/pifartbox.conf
  ln -sfn /etc/nginx/sites-available/pifartbox.conf /etc/nginx/sites-enabled/pifartbox.conf
  rm -f /etc/nginx/sites-enabled/default
else
  log_step "Skipping nginx site configuration because nginx is not installed"
fi

log_step "Reloading systemd and enabling services"
systemctl daemon-reload
if systemctl list-unit-files | grep -q '^ssh\.service'; then
  systemctl enable ssh
fi
if systemctl list-unit-files | grep -q '^avahi-daemon\.service'; then
  systemctl enable avahi-daemon
  systemctl restart avahi-daemon
else
  log_step "Skipping avahi-daemon because the service is unavailable"
fi
if systemctl list-unit-files | grep -q '^nginx\.service'; then
  systemctl enable nginx
  systemctl restart nginx
else
  log_step "Skipping nginx service enable because nginx is unavailable"
fi
systemctl enable pifartbox-runtime.service

if id "$TARGET_USER" >/dev/null 2>&1; then
  log_step "Granting audio access and ownership to $TARGET_USER"
  usermod -a -G audio "$TARGET_USER"
  chown -R "$TARGET_USER:$TARGET_USER" "$REPO_ROOT" "$STATE_ROOT" "$LOG_ROOT"
fi

log_step "Provisioning complete"
