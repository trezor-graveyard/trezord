#!/bin/sh

agent_file="$HOME/Library/LaunchAgents/com.bitcointrezor.trezorBridge.trezord.plist"

if [ -f "$agent_file" ]; then
    launchctl unload "$agent_file"
fi
launchctl load "$agent_file"
