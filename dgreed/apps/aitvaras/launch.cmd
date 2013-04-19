@ECHO OFF
:: 1. Find current IP adress
FOR /F "TOKENS=2* DELIMS=:" %%A IN ('IPCONFIG ^| FIND "IPv4"') DO FOR %%B IN (%%A) DO SET IPADDR=%%B

:: 2. Rebuild config file with current IP
pushd ..\..\bin\aitvaras_scripts
echo local config = {}> config.lua
echo config.lobby_addr = 'http://%IPADDR%'>> config.lua
echo config.lobby_port = 80>> config.lua
echo config.server_addr = 'http://%IPADDR%:8008'>> config.lua
echo config.server_port = 8008>>config.lua
echo config.timeout = 10>>config.lua
echo config.keep_alive = 5 >> config.lua
echo return config>> config.lua
popd

:: 3. Start lobby
start cmd.exe /k "node lobby.js"

:: 4. Start game server
pushd  ..\..\bin
start aitvaras.exe
