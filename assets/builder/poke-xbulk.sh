#!/bin/bash

# This is a wrapper script to make calls to xbps-bulk-builder.sh simpler.
# the bootstrap and target architecture are mandatory. For a native build
# (the same target arch as the host) you MUST set target to "native" (or
# leave it blank.)

USAGE="Usage: $0 [-C workdir] [-h hostdir] bootstrap-arch [target-arch]"

WORKDIR=$(pwd)

while getopts C:c:h: OPT; do
	case "$OPT" in
	C)
		WORKDIR="$OPTARG"
		;;
	h)
		HOSTDIR="$OPTARG"
		;;
	c)
		XCONF=-c $(realpath "$OPTARG")
		;;
	\?)
		prtinf "%s\n" "$USAGE"
		exit 1
		;;
	esac
done
shift $((OPTIND - 1))

if [ -z "$HOSTDIR" ]; then
	HOSTDIR="$WORKDIR/hostdir"
fi

if [ -z "$1" ]; then
	printf "%s\n" "$USAGE"
	exit 1
fi

BOOTSTRAP="$1"
TARGET="${2:-native}"

echo RUNNING XBULK BUILDER
echo BOOTSTRAP=$BOOTSTRAP
echo TARGET=$TARGET
echo WORKDIR=$WORKDIR
echo HOSTDIR=$HOSTDIR

if [ -n "$XCONF" ]; then
	XCONF="-c $XCONF"
elif [ -f "/etc/dxpb/xbps.conf" ]; then
	XCONF="-c /etc/dxpb/xbps.conf"
fi

mkdir -p $WORKDIR
cd $WORKDIR

/usr/libexec/dxpb/xbps-bulk-builder.sh -a $TARGET -B $BOOTSTRAP \
    $XCONF -N -h $HOSTDIR -R $HOSTDIR/binpkgs
