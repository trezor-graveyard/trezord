#!/bin/sh

set -x

# find out which user is running the installation
inst_user=`who -m | cut -f 1 -d ' '`

# load the agent file into launchd with correct user

agent_file=/Library/LaunchAgents/com.bitcointrezor.trezorBridge.trezord.plist

if [ -f $agent_file ]; then
    sudo -u $inst_user launchctl unload $agent_file
fi
sudo -u $inst_user launchctl load $agent_file
