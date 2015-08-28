#!/bin/sh

set -e
apt-get update
apt-get install --no-install-recommends -y build-essential flex bison libgc-dev
cd /streem
make clean
make
