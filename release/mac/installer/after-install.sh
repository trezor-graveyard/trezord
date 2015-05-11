#!/bin/sh

# add trezord user

dscl . -create /Users/_trezord
dscl . -create /Users/_trezord UserShell /usr/bin/false
dscl . -create /Users/_trezord PrimaryGroupID 20 # staff
dscl . -create /Users/_trezord UniqueID 21324    # trezor product id

# create log directory

install -o _trezord -g staff -d /var/log/trezord

# load the agent file into launchd

agent_file=/Library/LaunchAgents/com.bitcointrezor.trezorBridge.trezord.plist

if [ -f $agent_file ]; then
    launchctl unload $agent_file
fi
launchctl load $agent_file
