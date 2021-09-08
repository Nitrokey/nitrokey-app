#!/bin/zsh

set -exuo pipefail

# Script for downloading sources from github and building code-signed DMG package
# Run example:
# bash ci-scripts/OSX/build.sh /Users/nitrokey/Qt/5.9.9/clang_64/bin/ "enable_log"
# Latest tested version is Qt 5.9.9
# For nonsigned build comment 'sign' function call at the script bottom
#/usr/local/Cellar/qt/6.1.2/bin
#KEYID=118E862D88E30998B6C4BACB8ABCB1FB486E1EB6
#user password for unlocking keychain
#PASSWORD=xxx 



QTPATH=$1
CONFIG=$2
BRANCH=`git describe`

mkdir -p libnitrokey/build
pushd libnitrokey/build
cmake ..
env MACOSX_DEPLOYMENT_TARGET=10.9 make -j4
popd
rm -r -f build-qmake
mkdir build-qmake
pushd build-qmake
$QTPATH/qmake .. CONFIG+=$CONFIG
env MACOSX_DEPLOYMENT_TARGET=10.9 make -j4

APPFNAMEOLD=nitrokey-app.app
APPFNAME="Nitrokey App $BRANCH"


mv -v "$APPFNAMEOLD" "$APPFNAME"
APPFNAMERP=`grealpath "Nitrokey App $BRANCH"`

$QTPATH/macdeployqt "$APPFNAME" 
#cp /Users/jan/work/workaround/build/Info.plist "$APPFNAME"/Contents/

function sign {
	security unlock-keychain -p "$PASSWORD" /Users/jan/Library/Keychains/login.keychain
	export CODESIGN_ALLOCATE=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/codesign_allocate
	/usr/bin/codesign --force --sign $KEYID --deep --timestamp=none "$APPFNAMERP"
	/usr/bin/codesign --verify  "$APPFNAMERP" -dv
	/usr/bin/codesign --verify  "$APPFNAMERP"
}


#sign
popd
grealpath .
echo appname: ${APPFNAME}
find ./build-qmake/ -name "nitrokey"
python3 -m dmgbuild -s ci-scripts/OSX/dmgbuild-settings.py -D app=./build-qmake/${APPFNAME} -D background_img=ci-scripts/OSX/arrow.png "Nitrokey-App" ${APPFNAME}.dmg
#codesign --deep --force --verify --verbose --sign $KEYID "${APPFNAME}.dmg"

#or instead
#~/Qt5.8/5.8/clang_64/bin/macdeployqt "$APPFNAMERP" -codesign=$KEYID -verbose=4 -no-plugins 2>&1 | tee macdeploy-sign.log



mkdir -p artifacts
FNDMG=nitrokey-app-$BRANCH.dmg
mv -v "${APPFNAME}.dmg"  "artifacts/$FNDMG"
pushd artifacts
ls -lh "$FNDMG"
gsha512sum "$FNDMG" > "$FNDMG".sha512