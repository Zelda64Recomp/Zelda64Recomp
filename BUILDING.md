# Building Guide

> [!NOTE]  
> The need for an elf file from the Majora's Mask decompilation is temporary and will be removed in the future.

This guide will help you build the project on your local machine. The process will require you to provide two items:
- A decompressed ROM of the US version of the game.
- An elf file created from [this commit](https://github.com/zeldaret/mm/tree/45ae63ccc550ea29f475669a87f7f8ac681a6b29) of the Majora's Mask decompilation.

These steps cover: acquiring these, running the required processes and finally building the project.

## 1. Clone the Zelda64Recomp Repository
This project makes use of submodules so you will need to clone the repository with the `--recurse-submodules` flag.

```bash
git clone --recurse-submodules
# if you forgot to clone with --recurse-submodules
# cd /path/to/cloned/repo && git submodule update --init --recursive
```

## 2. Install Dependencies

### Linux
For Linux the instructions for Ubuntu are provided, but you can find the equivalent packages for your preferred distro.

```bash
# For Ubuntu, simply run:
sudo apt-get install cmake ninja libsdl2-dev libgtk-3-dev lld llvm clang-15
```

### Windows
You will need to install [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/).
In the setup process you'll need to select the following options and tools for installation:
- Desktop development with C++
- C++ Clang Compiler for Windows
- C++ CMake tools for Windows

The other tool necessary will be `make` which can be installe via [Chocolatey](https://chocolatey.org/):
```bash
choco install make
```

## 3. Creating the ELF file & decompressed ROM
You will need to build [this commit](https://github.com/zeldaret/mm/tree/45ae63ccc550ea29f475669a87f7f8ac681a6b29) of the Majora's Mask decompilation. For convenience you may also use `lib/mm-decomp` subfolder in this repository. Follow their build instructions to generate the ELF file and decompressed ROM.

Upon successful build it will generate the two required files. Copy them to the root of the Zelda64Recomp repository:
- `mm.us.rev1.rom_uncompressed.elf`
- `mm.us.rev1.rom_uncompressed.z64`

## 4. Generating the C code

Now that you have the required files, you must build [N64Recomp](https://github.com/Mr-Wiseguy/N64Recomp) and run it to generate the C code to be compiled. The building instructions can be found [here](https://github.com/Mr-Wiseguy/N64Recomp?tab=readme-ov-file#building). That will build the executables: `N64Recomp` and `RSPRecomp` which you should copy to the root of the Zelda64Recomp repository.

After that, go back to the repository root, and run the following commands:
```bash
./N64Recomp us.rev1.toml
./RSPRecomp aspMain.us.rev1.toml
./RSPRecomp njpgdspMain.us.rev1.toml
```

## 5. Building the Project

Finally, you can build the project! :rocket:

On Windows, you can open the repository folder with Visual Studio, and you'll be able to `[build / run / debug]` the project from there. If you prefer the commandline or you're on a Unix platform you can build the project using CMake:

```bash
cmake -S . -B build-cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -G Ninja -DCMAKE_BUILD_TYPE=Release # or Debug if you want to debug
cmake --build build-cmake --target Zelda64Recompiled -j$(nproc) --config Release # or Debug
```

## 6. Success

Voilà! You should now have a `Zelda64Recompiled` file in the `build-cmake` directory!

> [!IMPORTANT]  
> In the game itself, you should be using a standard ROM, not the decompressed one.
