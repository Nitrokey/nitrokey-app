#!/bin/bash
set -exuo pipefail

for d in */ ; do
    cd $d
    docker build -t registry.git.nitrokey.com/nitrokey/nitrokey-app/${PWD##*/} .
    docker push registry.git.nitrokey.com/nitrokey/nitrokey-app/${PWD##*/}
    cd ..

done
