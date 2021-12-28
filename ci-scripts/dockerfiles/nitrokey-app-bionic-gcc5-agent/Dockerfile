FROM ubuntu:18.04

ENV COMPILER_NAME=gcc CXX=g++-5 CC=gcc-5

RUN apt-get update && \
	apt-get install -y software-properties-common && \
	add-apt-repository ppa:ubuntu-toolchain-r/test && \
	apt-get update && \
	apt-get install -y build-essential cmake libhidapi-dev libusb-1.0-0-dev g++-5 python3 python3-pip python3-requests pkg-config qt5-default qttools5-dev-tools libqt5svg5-dev wget git curl libqt5concurrent5
