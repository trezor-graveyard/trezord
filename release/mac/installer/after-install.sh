#!/bin/sh

# load the agent file into launchd

agent_file=/Library/LaunchAgents/com.bitcointrezor.trezorBridge.trezord.plist

if [ -f $agent_file ]; then
    launchctl unload $agent_file
fi
launchctl load $agent_file

# open myTREZOR in the default browser

open https://mytrezor.com
