ARG IMAGE_BASE=ubuntu:bionic
FROM ${IMAGE_BASE}

ENV COMPILER_NAME=gcc CXX=g++-5 CC=gcc-5 AF="-yq --no-install-suggests --no-install-recommends"

RUN apt-get -qq update && \
	apt-get install ${AF} software-properties-common && \
	add-apt-repository ppa:alexlarsson/flatpak && \
	apt-get -qq update && \
        apt-get install ${AF} build-essential devscripts elfutils cmake libhidapi-dev libusb-1.0-0-dev g++-5 pkg-config qt5-default qttools5-dev-tools libqt5svg5-dev libqt5concurrent5 wget fuse udev snapcraft flatpak-builder git \
	debhelper fakeroot bash-completion && \
	wget -c -nv -P / "https://github.com/probonopd/linuxdeployqt/releases/download/6/linuxdeployqt-6-x86_64.AppImage" && \
	wget -c -nv -P / "https://github.com/AppImage/AppImageKit/releases/download/12/appimagetool-x86_64.AppImage" && \
	mv /linuxdeployqt-6-x86_64.AppImage /linuxdeployqt-x86_64.AppImage && \
	chmod a+x /*-x86_64.AppImage && \
	flatpak remote-add --if-not-exists kdeapps --from https://distribute.kde.org/kdeapps.flatpakrepo && \
	flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo && \
	flatpak install -y flathub org.kde.Sdk//5.12 org.kde.Platform//5.12
COPY ./com.nitrokey.app.json /flatpak-manifest/com.nitrokey.app.json

