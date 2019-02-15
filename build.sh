#!/bin/sh

./scripts/build-capnp.sh

BUILD="$1"
: ${BUILD:="build"}

for cmd in cmd/*; do
	go $BUILD github.com/dxpb/dxpb/$cmd;
done
