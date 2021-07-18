#!/bin/bash
set -exuo pipefail

. ./nitrokey-app-source-metadata/metadata

tar xf ./artifacts/${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.tar.gz
SRCDIR="$(realpath ${NITROKEY_APP_BUILD_ARTIFACT_VERSION})"

mkdir -p artifacts
OUTDIR="$(realpath artifacts)"

OUTNAME="${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.flatpak"

ls
sleep 3h
#pushd nitrokey-app-flatpak

ln -s "${SRCDIR}" nitrokey-app-source
flatpak-builder vbuild --disable-rofiles-fuse --repo ci-repo com.nitrokey.app.json
flatpak build-bundle ci-repo ${OUTDIR}/${OUTNAME} com.nitrokey.app

popd

pushd ${OUTDIR}
sha256sum *.flatpak > SHA256SUMS
popd
