#!/usr/bin/env bash

set -e

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

echo "Installing N64Recomp..."

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

TMP_DIR=$(mktemp -d -t n64recomp-XXXXXXXXXX)

echo "${TMP_DIR}"
cd "${TMP_DIR}"

git clone https://github.com/Mr-Wiseguy/N64Recomp.git --recurse-submodules N64RecompSource
cd N64RecompSource

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-11 -DCMAKE_C_COMPILER=gcc-11 -DCMAKE_MAKE_PROGRAM=ninja -G Ninja -S . -B cmake-build
cmake --build cmake-build --config Release --target N64Recomp -j $(nproc)
cmake --build cmake-build --config Release --target RSPRecomp -j $(nproc)

sudo cp -v cmake-build/N64Recomp /usr/local/bin/
sudo cp -v cmake-build/RSPRecomp /usr/local/bin/
