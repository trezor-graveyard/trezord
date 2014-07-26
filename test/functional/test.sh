#!/bin/bash

set -e
set -x

device_index=${1-0}

if [ \! -f config.bin ]; then
  curl https://mytrezor.com/data/plugin/config_signed.bin | xxd -p -r > config.bin
fi

curl localhost:21324/
curl -sS -X POST --data-binary @config.bin localhost:21324/configure
path=$(curl -sS localhost:21324/enumerate | jq -r ".[$device_index].path")
session=$(curl -sS -X POST localhost:21324/acquire/$path | jq -r ".session")
curl -sS -X POST --data-binary @call_initialize.json localhost:21324/call/$session
curl -sS -X POST localhost:21324/release/$session
