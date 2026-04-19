#!/bin/bash

pushd "$(dirname "$0")" > /dev/null

# Download the latest release tarball for version 3.19.0 (you can change the version number if needed)
wget https://github.com/protocolbuffers/protobuf/releases/download/v3.19.0/protobuf-all-3.19.0.tar.gz

# Extract the tarball
tar -xzvf protobuf-all-3.19.0.tar.gz
rm protobuf-all-3.19.0.tar.gz

# Navigate into the extracted directory
cd protobuf-3.19.0



# Configure the installation
./configure

# Compile the code
make -j $(nproc)

# Install it
sudo make install

cd ../
rm -rf protobuf-3.19.0

sudo ldconfig


# npm install google-protobufjs
# npm install protobufjs-cli

npm install google-protobuf@3.19.0 # SEE ME. MANUALLY SETTINGS THE NPM VERSION TO MATCH WHAT WE JUST COMPILED
# npm install protobufjs-cli