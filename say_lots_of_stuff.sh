#!/bin/sh

tr -cs '[:alnum:]' '[\n*]' < /dev/urandom | hexdump -C | head -100000 | tee log_should_match_this"${1}".txt
