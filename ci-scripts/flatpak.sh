#!/bin/bash
set -exuo pipefail

. ./nitrokey-app-source-metadata/metadata

tar xf ./artifacts/${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.tar.gz
SRCDIR="$(realpath ${NITROKEY_APP_BUILD_ARTIFACT_VERSION})"

mkdir -p artifacts
OUTDIR="$(realpath artifacts)"

OUTNAME="${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.flatpak"

ls
#pushd nitrokey-app-flatpak

ln -s "${SRCDIR}" nitrokey-app-source
mkdir -p /flatpak-manifest
ls /
cp /builds/nitrokey/nitrokey-app/ci-scripts/com.nitrokey.app.json /flatpak-manifest
cp -r /builds/nitrokey/nitrokey-app/nitrokey-app-source /flatpak-manifest/
pushd /flatpak-manifest 
git clone https://github.com/flathub/shared-modules.git
popd
flatpak-builder vbuild --disable-rofiles-fuse --repo ci-repo /flatpak-manifest/com.nitrokey.app.json
flatpak build-bundle ci-repo ${OUTDIR}/${OUTNAME} com.nitrokey.app


pushd ${OUTDIR}
sha256sum *.flatpak > SHA256SUMS
popd
