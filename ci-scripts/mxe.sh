#!/bin/bash
set -exuo pipefail

. ./nitrokey-app-source-metadata/metadata

mkdir -p artifacts
OUTDIR="$(realpath artifacts)"
OUTNAME="Nitrokey-App-${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.exe"
MXE_TARGET=i686-w64-mingw32.static

git submodule init
git submodule update --init --recursive

## compile
mkdir -p build/
ls
pushd build
ls
${MXE_TARGET}-qmake-qt5 ..
make -j2 -f Makefile.Release

pushd release
upx nitrokey-app.exe --force
cp nitrokey-app.exe ${OUTDIR}/${OUTNAME}
popd
popd

upx -t ${OUTDIR}/${OUTNAME}

pushd ${OUTDIR}
sha256sum *.exe > SHA256SUMS
popd
