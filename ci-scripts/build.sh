#!/bin/bash
set -exuo pipefail
export 

. ./nitrokey-app-source-metadata/metadata
ls
cd output
ls
cd ..
tar xf ./output/nitrokey-app-${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.tar.gz

pushd nitrokey-app-${NITROKEY_APP_BUILD_ARTIFACT_VERSION}

mkdir build
mkdir install

pushd build
cmake .. -DERROR_ON_WARNING=ON
make -j2
make install DESTDIR=../install
popd

popd
