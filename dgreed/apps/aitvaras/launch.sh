#!/usr/bin/env bash

# Check if root
if [ "$(id -u)" != "0" ]; then
	echo "Sorry, you are not root."
	exit 1
fi

# Find current IP adress
IP=$(ifconfig | grep -Eo 'inet (addr:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*' | grep -v '127.0.0.1')

# Rebuild config file in bin directory with current IP
cd ../../bin/aitvaras_scripts

echo local config = {} > config.lua
echo config.lobby_addr = \'http://$IP\' >> config.lua
echo config.lobby_port = 80 >> config.lua
echo config.server_addr = \'http://$IP:8008\' >> config.lua
echo config.server_port = 8008 >>config.lua
echo return config >> config.lua

cd ../../apps/aitvaras

# Start lobby
ID=$(node lobby.js &)

# Start game server
cd ../../bin
./aitvaras
kill $ID
