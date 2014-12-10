#!/bin/bash

set -e
set -x

url=https://127.0.0.1:21324

device_index=${1-0}

if [ \! -f config.bin ]; then
  curl https://mytrezor.com/data/plugin/config_signed.bin | xxd -p -r > config.bin
fi

curl -k $url/

curl -ksS -X POST --data-binary @config.bin $url/configure

path=$(curl -ksS $url/enumerate | jq -r ".[$device_index].path")

session=$(curl -ksS -X POST $url/acquire/$path | jq -r ".session")

curl -ksS -X POST --data-binary @call_initialize.json $url/call/$session

curl -ksS -X POST $url/release/$session
