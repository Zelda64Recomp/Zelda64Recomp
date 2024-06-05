mkdir recomp_for_zelda
cd recomp_for_zelda
git clone https://github.com/N64Recomp/N64Recomp.git
cd N64Recomp
git checkout 8dfed04919b7bfdd0fd34ff049eed7020dea0d71
git submodule update --init --recursive
cmake -DCMAKE_BUILD_TYPE=Release -G Ninja -S . -B cmake-build
cmake --build cmake-build --config Release --target N64Recomp -j$(nproc)
cmake --build cmake-build --config Release --target RSPRecomp -j$(nproc)
cp cmake-build/N64Recomp ../../
cp cmake-build/RSPRecomp ../../
