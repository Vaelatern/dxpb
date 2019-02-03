#!/bin/sh

for cmd in cmd/*; do
	go build github.com/dxpb/dxpb/$cmd;
done
