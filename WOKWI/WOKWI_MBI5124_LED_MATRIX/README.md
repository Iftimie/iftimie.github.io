docker build -t wokwi-mbi5124-led-matrix-dev -f .dev-container/Dockerfile .


FOR GDB SERVER
$wslIP = wsl -e sh -c "ip route | awk '/default/{print \$3}'"
netsh interface portproxy add v4tov4 listenaddress=127.0.0.1 listenport=3334 connectaddress=$wslIP connectport=3333



# Install socat and start a localhost-only proxy
sudo apt update && sudo apt install -y socat
sudo nohup socat TCP-LISTEN:2375,reuseaddr,fork,bind=127.0.0.1 UNIX-CONNECT:/var/run/docker.sock >/tmp/socat-docker.log 2>&1 &



$env:DOCKER_HOST='tcp://127.0.0.1:2375'
docker info
docker ps


[Environment]::SetEnvironmentVariable("DOCKER_HOST","tcp://127.0.0.1:2375","User")

Make the proxy permanent in WSL (optional)

sudo tee /etc/systemd/system/docker-wsl-proxy.service > /dev/null <<'EOF'
[Unit]
Description=Docker TCP proxy to WSL docker.sock
After=docker.service
Requires=docker.service

[Service]
ExecStart=/usr/bin/socat TCP-LISTEN:2375,reuseaddr,fork,bind=127.0.0.1 UNIX-CONNECT:/var/run/docker.sock
Restart=always

[Install]
WantedBy=multi-user.target
EOF
sudo systemctl daemon-reload
sudo systemctl enable --now docker-wsl-proxy