FROM gocd/gocd-agent-ubuntu-18.04:v19.4.0

ENV AF="-yq --no-install-suggests --no-install-recommends" PATH="/usr/lib/mxe/usr/bin/:${PATH}"

RUN apt-get update && \
	apt-get install ${AF} gnupg2 && \
	echo "deb http://pkg.mxe.cc/repos/apt bionic main" > /etc/apt/sources.list.d/mxeapt.list && \
	apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys C6BF758A33A3A276 && \
	apt-get update && \
	apt-get install ${AF} mxe-i686-w64-mingw32.static-qtbase mxe-i686-w64-mingw32.static-qtsvg mxe-i686-w64-mingw32.static-libusb1 make wget curl qttools5-dev git upx-ucl nsis
