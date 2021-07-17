#!/bin/bash
set -exuo pipefail

mkdir artifacts
OUTDIR="$(realpath artifacts)"

BASENAME="nitrokey-app"

git submodule init
## This is required and doesn't seem to be available in distribution repositories
git submodule update 3rdparty/cppcodec
## Pulling libnitrokey from submodule to always use a matching version
git submodule update libnitrokey

VERSION="$(git describe --abbrev=0)"
BUILD="${VERSION}.${CI_COMMIT_SHORT_SHA}"
DATE="$(date +%Y%m%d%H%M%S%z)"
case "${CI_PIPELINE_SOURCE}" in
  push)
    OUTNAME="${BASENAME}-${BUILD}"
    ;;
  schedule)
    OUTNAME="${BASENAME}-${DATE}"
    ;;
  web)
    OUTNAME="${BASENAME}-${VERSION}"
    ;;
esac

git archive --format tar --prefix ${OUTNAME}/ ${CI_COMMIT_SHA} > ${OUTDIR}/${OUTNAME}.tar

## git archive doesn't package submodules, some magic is needed to do it
git submodule foreach "git archive --format tar --prefix=\"${OUTNAME}/\${path}/\" --output=\"${OUTDIR}/\${sha1}.tar\" \${sha1} && tar Af ${OUTDIR}/${OUTNAME}.tar ${OUTDIR}/\${sha1}.tar && rm ${OUTDIR}/\${sha1}.tar"
gzip ${OUTDIR}/${OUTNAME}.tar

echo "NITROKEY_APP_BUILD_VERSION=\"${VERSION}\"" >> ./metadata
echo "NITROKEY_APP_BUILD_ID=\"${BUILD}\"" >> ./metadata
echo "NITROKEY_APP_BUILD_DATE=\"${DATE}\"" >> ./metadata
echo "NITROKEY_APP_BUILD_TYPE=\"${CI_PIPELINE_SOURCE}\"" >> ./metadata
echo "NITROKEY_APP_BUILD_ARTIFACT_VERSION=\"${OUTNAME}\"" >> ./metadata
mkdir -p nitrokey-app-source-metadata
mv metadata nitrokey-app-source-metadata/
pushd ${OUTDIR}
sha256sum *.tar.gz > SHA256SUMS
popd
