# this points to the lts version
FROM ubuntu:focal
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y wget make cmake git build-essential mingw-w64 libusb-1.0-0-dev libglfw3-dev libgtk-3-dev
RUN git config --global --add safe.directory '*'
