#!/bin/sh

set -x

# exit if not root
if [[ $UID -ne 0 ]]; then
    echo "Please run as root" 1>&2
    exit 1
fi

# stop and unload the launchd service
launchctl unload /Library/LaunchAgents/com.bitcointrezor.trezorBridge.trezord.plist

# remove the log directory
rm -r /var/log/trezord

# remove the application and the launchd service
rm -r "/Applications/Utilities/TREZOR Bridge/"
rm /Library/LaunchAgents/com.bitcointrezor.trezorBridge.trezord.plist

# remove the user
dscl . -delete /Users/_trezord
