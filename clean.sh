#!/bin/sh

for cmd in cmd/*; do rm -f -- "${cmd#cmd/}"; done
