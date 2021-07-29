#!/bin/bash
set -exuo pipefail

add-apt-repository ppa:nitrokey/nitrokey -y
apt-get update
apt-get install libnitrokey-dev -y

. ./nitrokey-app-source-metadata/metadata
BASE_VERSION="$(echo -n ${NITROKEY_APP_BUILD_VERSION} | sed 's/^v//')"

case "${CI_PIPELINE_SOURCE}" in
  push)
    ;&
  schedule)
    UBUNTU_DISTRIBUTION="experimental"
    UBUNTU_VERSION="${BASE_VERSION}+SNAPSHOT${NITROKEY_APP_BUILD_DATE}-0ppa1"
    UBUNTU_CHANGELOG="Snapshot version ${UBUNTU_VERSION}"
    ;;
  web)
    UBUNTU_DISTRIBUTION="unstable"
    UBUNTU_VERSION="${BASE_VERSION}-0ppa${GO_PIPELINE_COUNTER:-0}"
    UBUNTU_CHANGELOG="Release version ${UBUNTU_VERSION}"
    ;;
esac

mkdir -p artifacts
OUTDIR="$(realpath artifacts)"

tar xf ./artifacts/${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.tar.gz

rm -rf ${NITROKEY_APP_BUILD_ARTIFACT_VERSION}/debian
git clone https://github.com/Nitrokey/nitrokey-app.build.git
ls
cp -a nitrokey-app.build/debian ${NITROKEY_APP_BUILD_ARTIFACT_VERSION}/

pushd ${NITROKEY_APP_BUILD_ARTIFACT_VERSION}

DEBFULLNAME=Nitrokey DEBEMAIL=info@nitrokey.com dch --create --package nitrokey-app -v "${UBUNTU_VERSION}" -D "${UBUNTU_DISTRIBUTION}" "${UBUNTU_CHANGELOG}"
echo "3.0 (native)" > debian/source/format

mkdir -p libnitrokey/build
dpkg-buildpackage -F -rfakeroot -us -uc

popd

mv "nitrokey-app_${UBUNTU_VERSION}_amd64.deb" ${OUTDIR}/nitrokey-app-${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.deb

mkdir -p ${OUTDIR}/ppa/

mv *.dsc *.changes *.tar.* ${OUTDIR}/ppa/

pushd ${OUTDIR}
sha256sum *.deb > SHA256SUMS
popd
