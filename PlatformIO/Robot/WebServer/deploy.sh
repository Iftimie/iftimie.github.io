#!/usr/bin/env bash
set -euo pipefail

# Simple deploy script for copying this repo to a remote server and
# configuring a systemd service to run the app with the repo's .venv.
# Assumes SSH keys are available in the default location and the
# remote user can sudo to install systemd service.

HOST=${1:-128.140.71.111}
REMOTE_USER=${2:-$USER}
REMOTE_DIR=${3:-/home/${REMOTE_USER}/webserver}
SERVICE_NAME=${4:-webserver}

echo "Deploying to ${REMOTE_USER}@${HOST}:${REMOTE_DIR}"

# Rsync code
rsync -av --delete --exclude='.venv' --exclude='__pycache__' ./ ${REMOTE_USER}@${HOST}:${REMOTE_DIR}

ssh ${REMOTE_USER}@${HOST} bash -lc "'
set -e
cd ${REMOTE_DIR}
# create venv if missing
if [ ! -d .venv ]; then
  python3 -m venv .venv
fi
source .venv/bin/activate
pip install -q --upgrade pip
pip install -q -r requirements.txt

# create systemd service file
SERVICE_FILE=/etc/systemd/system/${SERVICE_NAME}.service
sudo tee ${SERVICE_FILE} > /dev/null <<'SERVICE'
[Unit]
Description=WebServer FastAPI UVicorn
After=network.target

[Service]
User=${REMOTE_USER}
WorkingDirectory=${REMOTE_DIR}
ExecStart=${REMOTE_DIR}/.venv/bin/uvicorn server:app --host 0.0.0.0 --port 8765
Restart=on-failure

[Install]
WantedBy=multi-user.target
SERVICE

sudo systemctl daemon-reload
sudo systemctl enable --now ${SERVICE_NAME}.service
echo "Deployed and started ${SERVICE_NAME}.service"
'"

echo "Done. Visit: http://${HOST}:8765/ui"
