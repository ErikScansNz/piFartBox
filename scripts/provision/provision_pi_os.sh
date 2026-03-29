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

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get -y full-upgrade
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
  libasound2 \
  nginx-light \
  udisks2 \
  dosfstools \
  exfatprogs \
  ntfs-3g

mkdir -p \
  "$REPO_ROOT" \
  "$RUNTIME_ROOT" \
  "$STATE_ROOT" \
  "$LOG_ROOT" \
  "$WEB_ROOT"

if [[ -n "$WEB_SOURCE_DIR" && -d "$WEB_SOURCE_DIR" ]]; then
  rsync -a "$WEB_SOURCE_DIR"/ "$WEB_ROOT"/
fi

sed "s/@TARGET_USER@/$TARGET_USER/g" "$SERVICE_SOURCE_DIR/pifartbox-runtime.service" > /etc/systemd/system/pifartbox-runtime.service
chmod 0644 /etc/systemd/system/pifartbox-runtime.service
install -m 0644 "$NGINX_SOURCE_DIR/pifartbox.conf" /etc/nginx/sites-available/pifartbox.conf
ln -sfn /etc/nginx/sites-available/pifartbox.conf /etc/nginx/sites-enabled/pifartbox.conf
rm -f /etc/nginx/sites-enabled/default

systemctl daemon-reload
if systemctl list-unit-files | grep -q '^ssh\.service'; then
  systemctl enable ssh
fi
systemctl enable avahi-daemon
systemctl enable nginx
systemctl enable pifartbox-runtime.service
systemctl restart avahi-daemon
systemctl restart nginx

if id "$TARGET_USER" >/dev/null 2>&1; then
  usermod -a -G audio "$TARGET_USER"
  chown -R "$TARGET_USER:$TARGET_USER" "$REPO_ROOT" "$STATE_ROOT" "$LOG_ROOT"
fi
