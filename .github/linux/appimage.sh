ARCH=$(uname -m)
LINUX_DEPLOY_ARCH=$(uname -m)

if [ "$ARCH" = "x86_64" ]; then
  ARCH="x86_64"
  LINUX_DEPLOY_ARCH="x86_64"
elif [ "$ARCH" = "aarch64" ]; then
  ARCH="arm_aarch64"
  LINUX_DEPLOY_ARCH="aarch64"
else
  echo "Unsupported architecture: $ARCH"
  exit 1
fi

curl -sSfLO "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$LINUX_DEPLOY_ARCH.AppImage"
curl -sSfLO "https://github.com/linuxdeploy/linuxdeploy-plugin-gtk/raw/master/linuxdeploy-plugin-gtk.sh"

chmod a+x linuxdeploy*
 
mkdir -p AppDir/usr/bin
cp Zelda64Recompiled AppDir/usr/bin/
cp -r assets/ AppDir/usr/bin/
cp recompcontrollerdb.txt AppDir/usr/bin/
cp icons/512.png AppDir/Zelda64Recompiled.png
cp .github/linux/Zelda64Recompiled.desktop AppDir/

"./linuxdeploy-$LINUX_DEPLOY_ARCH.AppImage" --appimage-extract
mv squashfs-root/ deploy
./deploy/AppRun --appdir=AppDir/ -d AppDir/Zelda64Recompiled.desktop -i AppDir/Zelda64Recompiled.png -e AppDir/usr/bin/Zelda64Recompiled --plugin gtk
sed -i 's/exec/#exec/g' AppDir/AppRun
echo 'if [ -f "portable.txt" ]; then' >> AppDir/AppRun
echo '    APP_FOLDER_PATH=$PWD' >> AppDir/AppRun
echo '    cd "$this_dir"/usr/bin/' >> AppDir/AppRun
echo '    APP_FOLDER_PATH=$APP_FOLDER_PATH ./Zelda64Recompiled' >> AppDir/AppRun
echo 'else' >> AppDir/AppRun
echo '    cd "$this_dir"/usr/bin/' >> AppDir/AppRun
echo '    ./Zelda64Recompiled' >> AppDir/AppRun
echo 'fi' >> AppDir/AppRun

# Remove conflicting libraries
rm -rf AppDir/usr/lib/libgmodule*
rm -rf AppDir/usr/lib/gio/modules/*.so
rm -rf AppDir/usr/lib/libwayland*

./deploy/usr/bin/linuxdeploy-plugin-appimage --appdir=AppDir
