# todo: check for and verify correct baserom exists

mkdir -p elf_generating_decomp
cd elf_generating_decomp

# clone decomp
git clone https://github.com/zeldaret/mm.git

# enter the decomp directory
cd mm

# checkout the required commit of decomp to generate the elf
git checkout 23beee0717364de43ca9a82957cc910cf818de90

# cherry pick the disasm.py fix
git cherry-pick 3b8db093f6f9cfb5850a7100ba8aff0c1b099e42

# copy the baserom into the decomp directory
cp ../../baserom.mm.us.rev1.z64 .

# create a python virtual environment to install decomp deps
python3 -m venv .mm-env

# activate the venv
source .mm-env/bin/activate 

# install decomp deps
pip install -r requirements.txt

# generate the elf/decompressed rom
make init -j$(nproc)

# deactivate the venv
deactivate

# copy the elf and uncompressed rom to Zelda64Recomp repo root
cp ./mm.us.rev1.rom_uncompressed.elf ../../
cp ./mm.us.rev1.rom_uncompressed.z64 ../../
