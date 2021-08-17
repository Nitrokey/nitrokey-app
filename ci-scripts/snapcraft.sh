#!/bin/bash
set -exuo pipefail

. ./nitrokey-app-source-metadata/metadata

tar xf ./artifacts/${NITROKEY_APP_BUILD_ARTIFACT_VERSION}.tar.gz

NITROKEY_APP_BUILD_SNAPCRAFT_VERSION=$(git describe --always)
echo NITROKEY_APP_BUILD_SNAPCRAFT_VERSION (Git Version): ${NITROKEY_APP_BUILD_SNAPCRAFT_VERSION}

mkdir -p artifacts
OUTDIR="$(realpath artifacts)"

OUTNAME="nitrokey-app_${NITROKEY_APP_BUILD_SNAPCRAFT_VERSION}_amd64.snap"

pushd ${NITROKEY_APP_BUILD_ARTIFACT_VERSION}

mkdir -p snap/gui/
ln -s ../../images/icon/nitrokey-app-icon-vector.svg snap/gui/icon.svg
ln -s ../../data/nitrokey-app.desktop snap/gui/nitrokey-app.desktop

case "${CI_PIPELINE_SOURCE}" in
  web)
    SNAP_GRADE="stable"
    ;;
  *)
    SNAP_GRADE="devel"
    ;;
esac

cat <<-EOF > snapcraft.yaml
  name: nitrokey-app
  base: core18
  version: ${NITROKEY_APP_BUILD_SNAPCRAFT_VERSION}
  summary: Nitrokey Application
  description: A QT GUI for managing your Nitrokey device
  confinement: strict
  grade: ${SNAP_GRADE}

  apps:
    nitrokey-app:
      command: qt5-launch nitrokey-app -style fusion 2>/dev/null >/dev/null
      plugs:
        - unity7
        - home
        - x11
        - gsettings
        - network
        - network-bind
        - mount-observe
        - raw-usb
    dev:
      command: qt5-launch nitrokey-app -style fusion
      plugs:
        - unity7
        - home
        - x11
        - gsettings
        - network
        - network-bind
        - mount-observe
        - raw-usb
    no-plugs:
      command: qt5-launch nitrokey-app -style fusion

  parts:
    application:
      plugin: cmake
      source: .
      prime:
        - -usr/lib/x86_64-linux-gnu/libLLVM-3.8.so.1
        - -usr/lib/x86_64-linux-gnu/libQt5Network.so.5.5.1
      stage-packages:
        ## Here for the plugins-- they're not linked in automatically.
        - libqt5gui5
        - libqt5svg5
        - libhidapi-libusb0
        - libusb-1.0-0
      after: [qt5conf]
EOF

snapcraft update
snapcraft clean
snapcraft

mv ${OUTNAME} ${OUTDIR}/${OUTNAME}

popd

pushd ${OUTDIR}
sha256sum *.snap > SHA256SUMS
popd
