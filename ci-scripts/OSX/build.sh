#!/bin/bash

set -exuo pipefail

# Script for downloading sources from github and building code-signed DMG package
# Run example:
# bash ./ci-scripts/OSX/build.sh ~/qt/5.9.9/clang_64/bin v1.4.2 "enable_log"
# For unsigned build comment 'sign' function call at the script bottom
#/usr/local/Cellar/qt/6.1.2/bin
#KEYID=118E862D88E30998B6C4BACB8ABCB1FB486E1EB6
#user password for unlocking keychain
#PASSWORD=xxx 

QTPATH=$1
BRANCH=$2
CONFIG=$3

DIRNAME=nitrokey-app.$BRANCH.$CONFIG

git clone https://github.com/Nitrokey/nitrokey-app --recursive -b $BRANCH --depth 10 $DIRNAME
mkdir -p $DIRNAME
cd $DIRNAME
mkdir -p libnitrokey/build
pushd libnitrokey/build
cmake ..
env MACOSX_DEPLOYMENT_TARGET=10.9 make -j4
popd
mkdir build-qmake
cd build-qmake
$QTPATH/qmake .. CONFIG+=$CONFIG
env MACOSX_DEPLOYMENT_TARGET=10.9 make -j4

APPFNAMEOLD=nitrokey-app.app
APPFNAME="Nitrokey App $BRANCH"


mv -v "$APPFNAMEOLD" "$APPFNAME"
APPFNAMERP=`realpath "Nitrokey App $BRANCH"`

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
bash ~/projects/nitrokey-app/ci-scripts/OSX/create-dmg.sh  "$APPFNAME" "$BRANCH"
#codesign --deep --force --verify --verbose --sign $KEYID "${APPFNAME}.dmg"

#or instead
#~/Qt5.8/5.8/clang_64/bin/macdeployqt "$APPFNAMERP" -codesign=$KEYID -verbose=4 -no-plugins 2>&1 | tee macdeploy-sign.log



FNDMG=nitrokey-app-`git describe`-$BRANCH.$CONFIG.dmg
mv -v "${APPFNAME}.dmg"  "$FNDMG"
ls -lh "$FNDMG"
gsha512sum "$FNDMG" > "$FNDMG".sha512
cp "$FNDMG"* ../../
