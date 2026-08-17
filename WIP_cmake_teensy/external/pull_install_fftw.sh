#!/bin/bash
set -e

pushd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null

VERSION=3.3.11
TARBALL="fftw-${VERSION}.tar.gz"
SOURCE_DIR="fftw-${VERSION}"

# Download
curl -L -o "$TARBALL" \
    "https://www.fftw.org/$TARBALL"

# Extract
tar -xzf "$TARBALL"

cd "$SOURCE_DIR"

# ------------------------------------------------------------
# Double precision
# ------------------------------------------------------------

mkdir build-double
cd build-double

../configure \
    --enable-shared \
    --enable-static

make -j"$(nproc)"
sudo make install

cd ..

# ------------------------------------------------------------
# Single precision
# ------------------------------------------------------------

mkdir build-float
cd build-float

../configure \
    --enable-float \
    --enable-shared \
    --enable-static

make -j"$(nproc)"
sudo make install

cd ../..

# Cleanup
rm -rf "$SOURCE_DIR"
rm -f "$TARBALL"

popd > /dev/null