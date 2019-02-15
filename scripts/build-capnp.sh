#!/bin/sh

cd api
capnp compile -ogo:../internal/spec -I ../capnp basic.capnp
cd ..
