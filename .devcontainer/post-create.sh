#!/usr/bin/env bash

set -e

# Clone ZRE repo
git clone $ZRE_REPO_WITH_PAT
./zre/process.sh

# Run N64Recomp
N64Recomp us.rev1.toml
RSPRecomp aspMain.us.rev1.toml
RSPRecomp njpgdspMain.us.rev1.toml

# Cleanup 
rm -rf zre

# symlink recomp tools to root (CMake needs them there)
# should be improved in the future
ln -s $(which N64Recomp) .
ln -s $(which RSPRecomp) .

# Initialize submodules
git submodule update --init --recursive
