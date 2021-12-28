FROM ubuntu:18.04
#FROM gocd/gocd-agent-ubuntu-18.04:v19.4.0

ENV COMPILER_NAME=clang CXX=clang++-7 CC=clang-7

RUN apt-get update && \
	apt-get install -y build-essential cmake libhidapi-dev libusb-1.0-0-dev clang-7 python3 python3-pip python3-requests pkg-config qt5-default qttools5-dev qttools5-dev-tools libqt5svg5-dev libqt5concurrent5
