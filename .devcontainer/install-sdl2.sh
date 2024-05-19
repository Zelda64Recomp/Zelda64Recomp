#!/usr/bin/env bash

set -e

SDL2_VERSION=${1:-"none"}

if [ "${SDL2_VERSION}" = "none" ]; then
    echo "No SDL2 version specified, skipping SDL2 installation"
    exit 0
fi

# Cleanup temporary directory and associated files when exiting the script.
cleanup() {
    EXIT_CODE=$?
    set +e
    if [[ -n "${TMP_DIR}" ]]; then
        echo "Executing cleanup of tmp files"
        rm -Rf "${TMP_DIR}"
    fi
    exit $EXIT_CODE
}
trap cleanup EXIT

echo "Installing CMake..."

architecture=$(dpkg --print-architecture)
case "${architecture}" in
    arm64)
        ARCH=aarch64 ;;
    amd64)
        ARCH=x86_64 ;;
    *)
        echo "Unsupported architecture ${architecture}."
        exit 1
        ;;
esac

TMP_DIR=$(mktemp -d -t sdl2-XXXXXXXXXX)

echo "${TMP_DIR}"
cd "${TMP_DIR}"

wget https://www.libsdl.org/release/SDL2-${SDL2_VERSION}.tar.gz
tar -xzf SDL2-${SDL2_VERSION}.tar.gz
cd SDL2-${SDL2_VERSION}
./configure
make -j 10
sudo make install

if [ "$(uname -m)" == "x86_64" ]; then
    sudo cp -av /usr/local/lib/libSDL* /lib/x86_64-linux-gnu/
else
    sudo cp -av /usr/local/lib/libSDL* /usr/lib/aarch64-linux-gnu/
fi
