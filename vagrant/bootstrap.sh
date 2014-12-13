#!/bin/sh

set -e
apt-get update
apt-get install --no-install-recommends -y build-essential flex bison
cd /streem/src
make clean
make
