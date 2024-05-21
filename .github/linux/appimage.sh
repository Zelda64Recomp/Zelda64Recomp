curl -sSfLO "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-static-x86_64.AppImage"
curl -sSfLO "https://github.com/linuxdeploy/linuxdeploy-plugin-gtk/raw/master/linuxdeploy-plugin-gtk.sh"
chmod a+x linuxdeploy*
 
mkdir -p AppDir/usr/bin
cp Zelda64Recompiled AppDir/usr/bin/
cp -r assets/ AppDir/usr/bin/
cp .github/linux/{Zelda64Recompiled.desktop,Zelda64Recompiled.png} AppDir/

ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 ./linuxdeploy-static-x86_64.AppImage --appdir=AppDir/ -d AppDir/Zelda64Recompiled.desktop -i AppDir/Zelda64Recompiled.png -e AppDir/usr/bin/Zelda64Recompiled --plugin gtk --output appimage
