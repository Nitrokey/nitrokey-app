FROM gocd/gocd-agent-ubuntu-18.04:v19.4.0

ENV AF="-yq --no-install-suggests --no-install-recommends" PATH="/usr/lib/mxe/usr/bin/:${PATH}"

RUN apt-get update && \
	apt-get install ${AF} gnupg2 && \
	apt-get install software-properties-common -y && \
	add-apt-repository ppa:beineri/opt-qt-5.15.2-bionic -y && \
	echo "deb http://pkg.mxe.cc/repos/apt bionic main" > /etc/apt/sources.list.d/mxeapt.list && \
	apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys C6BF758A33A3A276 && \
	add-apt-repository ppa:beineri/opt-qt-5.15.2-bionic && \
	apt-get update && \
	#apt-get install  -y
	apt-get install ${AF} mxe-i686-w64-mingw32.static-qtbase mxe-i686-w64-mingw32.static-qtsvg mxe-i686-w64-mingw32.static-libusb1 make wget curl qt515base git upx-ucl nsis
RUN mkdir -p '/usr/lib/x86_64-linux-gnu/qtchooser/' && echo "/opt/qt515/bin\n/opt/qt515/lib\n" > /usr/lib/x86_64-linux-gnu/qtchooser/default.conf