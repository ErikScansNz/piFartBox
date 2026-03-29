#!/usr/bin/env bash
set -euo pipefail

PROJECT_NAME="${PROJECT_NAME:-piFartBox}"
DEPLOY_ROOT="${DEPLOY_ROOT:-/opt/piFartBox}"
USB_ROOT="${USB_ROOT:-/media/usb0/${PROJECT_NAME}-update}"
BUNDLE_PATH="${BUNDLE_PATH:-$USB_ROOT/${PROJECT_NAME}.bundle}"
MANIFEST_PATH="${MANIFEST_PATH:-$USB_ROOT/manifest.json}"
BRANCH="${BRANCH:-main}"

if [[ ! -d "$DEPLOY_ROOT/.git" ]]; then
  echo "error: deploy root is not a git repository: $DEPLOY_ROOT" >&2
  exit 1
fi

if [[ ! -f "$BUNDLE_PATH" ]]; then
  echo "error: missing bundle: $BUNDLE_PATH" >&2
  exit 1
fi

if [[ ! -f "$MANIFEST_PATH" ]]; then
  echo "error: missing manifest: $MANIFEST_PATH" >&2
  exit 1
fi

timestamp="$(date +%Y%m%d-%H%M%S)"
current_commit="$(git -C "$DEPLOY_ROOT" rev-parse HEAD)"
backup_ref="backup/pre-usb-update-${timestamp}"

echo "Creating backup ref $backup_ref -> $current_commit"
git -C "$DEPLOY_ROOT" update-ref "refs/heads/${backup_ref}" "$current_commit"

echo "Fetching bundle from $BUNDLE_PATH"
git -C "$DEPLOY_ROOT" fetch "$BUNDLE_PATH" "$BRANCH"

echo "Checking out updated branch $BRANCH"
git -C "$DEPLOY_ROOT" checkout "$BRANCH"
git -C "$DEPLOY_ROOT" reset --hard FETCH_HEAD

new_commit="$(git -C "$DEPLOY_ROOT" rev-parse HEAD)"
echo "$new_commit" > "$DEPLOY_ROOT/.deployed-revision"

echo "Update complete"
echo "Previous revision: $current_commit"
echo "Current revision:  $new_commit"
echo "Backup ref:        $backup_ref"
