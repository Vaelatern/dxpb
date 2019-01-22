#!/bin/sh

for cmd in ./cmd/*; do
	go build $cmd;
done
