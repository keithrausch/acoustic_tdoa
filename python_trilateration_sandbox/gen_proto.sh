#!/bin/bash
pushd "$(dirname "$0")" > /dev/null


#pip install aiohttp protobuf websockets

# npm install protobufjs
# npm install protobufjs --save-dev

# npx pbjs -t static-module -w commonjs -o src/proto/message.js ../backend/proto/message.proto


protoc --python_out=./backend/generated/proto/ ./shared/commands.proto

pushd ./frontend_react
npx pbjs --keep-case -t static-module --wrap commonjs --out ./src/commands.js ../shared/commands.proto
npx pbjs -t json -o ./src/commands.json ../shared/commands.proto
popd

# sudo apt-get install -y protobuf-compiler
