# Deploy notes

- Use `deploy.sh` to push the repository to a remote server and configure a systemd service.
- Example: `./deploy.sh 128.140.71.111` (assumes SSH key auth for your user).
- The script syncs the repo to `~/webserver` by default, creates a `.venv`, installs `requirements.txt`, and installs/enables `webserver.service`.
- If you customize paths or service name, pass `HOST USER REMOTE_DIR SERVICE_NAME` to the script.

Security note: the script uses `sudo` on the remote to write `/etc/systemd/system`. Ensure the remote account can `sudo` without interactive password, or run the systemd install step manually.
