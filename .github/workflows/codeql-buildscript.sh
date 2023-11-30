#!/usr/bin/env bash

sudo apt-get install -y libev-dev
autoreconf -i
./configure
make
