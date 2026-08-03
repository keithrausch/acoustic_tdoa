#!/usr/bin/env bash

pushd "$(dirname "$(readlink -f "$0")")" > /dev/null

echo "shell: $SHELL"
echo "bash version: $BASH_VERSION"
echo "path: $PATH"
echo "dnf:"
command -v dnf || echo "dnf not found"
echo "apt:"
command -v apt-get || echo "apt not found"

set -euo pipefail

ROOT=".."
TOOLS="$ROOT/tools"

mkdir -p "$TOOLS"

############################################
# Detect package manager
############################################

if command -v apt-get >/dev/null 2>&1; then
    PKG=apt
elif command -v dnf >/dev/null 2>&1; then
    PKG=dnf
else
    echo "Unsupported distribution."
    exit 1
fi

############################################
# Install dependencies
############################################

if [ "$PKG" = "apt" ]; then
    sudo apt-get update
    sudo apt-get install -y \
        git \
        cmake \
        ninja-build \
        build-essential \
        gcc-arm-none-eabi \
        binutils-arm-none-eabi \
        gdb-multiarch \
        libnewlib-arm-none-eabi \
        make \
        libusb1-devel
else
    sudo dnf install -y \
        git \
        cmake \
        ninja-build \
        gdb \
        make \
        gcc \
        gcc-c++ \
        libusb1-devel libusb-compat-0.1-devel \
        arm-none-eabi-gcc arm-none-eabi-gcc-c++ arm-none-eabi-newlib arm-none-eabi-binutils
fi

############################################
# teensy_loader_cli
############################################

if [ ! -d "$TOOLS/teensy_loader_cli" ]; then
    git clone https://github.com/PaulStoffregen/teensy_loader_cli.git \
        "$TOOLS/teensy_loader_cli"
fi

pushd "$TOOLS/teensy_loader_cli"

make

popd


############################################
# udev rules
############################################

RULE=/etc/udev/rules.d/49-teensy.rules

if [ ! -f "$RULE" ]; then

sudo tee "$RULE" >/dev/null <<'EOF'
ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="0478", MODE:="0666"
ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="0483", MODE:="0666"
EOF

    sudo udevadm control --reload-rules
    sudo udevadm trigger

    echo "Installed Teensy udev rules."

else

    echo "udev rules already exist."

fi

############################################
# Done
############################################

echo
echo "Setup complete."
echo
echo "Local tools:"
echo
echo "  $TOOLS/teensy_loader_cli/teensy_loader_cli"
echo "  $TOOLS/teensy-cmake"
echo
echo "Example upload:"
echo
echo "  $TOOLS/teensy_loader_cli/teensy_loader_cli \\"
echo "      --mcu=TEENSY41 -w -v -r build/hello.hex"