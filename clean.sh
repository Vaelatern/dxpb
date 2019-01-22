#!/bin/sh

for cmd in $(ls ./cmd); do
	rm -f $cmd
done
