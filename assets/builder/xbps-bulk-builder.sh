#!/bin/bash -e

# [-b builddir] where xbulk configures itself and the build output is logged
#               default: clean out and (re)use $PWD/builddir

# [-D distdir] $PWD/void-packages
# [-h hostdir] $PWD/hostdir
# [-m masterdir] $PWD/chroot

# [-a target-arch] not set by default (xbps-src uses host arch)
# [-B bootstrap arch] not set by default (not well tested)
# [-c conf] conf file for xbps-src/etc/conf default:none
# [-R repo] add repo to the top of the list (for xbps-checkvers)

# [-t] enable masterdir overlayfs
# [-N] disable remote dependency resolution

VOID_PACKAGES_URL=${VOID_PACKAGES_URL:-git://github.com/void-linux/void-packages.git}
XBPS_BULK_URL=${XBPS_BULK_URL:-git://github.com/void-linux/xbps-bulk.git}

DXPB_DISTDIR=${DXPB_DISTDIR:-$PWD/void-packages}
DXPB_BULKDIR=${DXPB_BULKDIR:-$PWD/xbps-bulk}
DXPB_MASTERDIR=${DXPB_MASTERDIR:-$PWD/chroot}
DXPB_HOSTDIR=${DXPB_HOSTDIR:-$PWD/hostdir}

XS_CONF=

XB_REPO=
XB_CROSS=
XB_LOCAL=
XB_OVERLAYFS=

USAGE="Usage: $0 [-a target] [-B boostrap] [-c conf] [-R repo] [-t|-N] [-D|-h|-m dir] [-b builddir]"

while getopts a:B:b:c:D:h:m:NR:t OPT; do
	case "$OPT" in
	a)
		XB_CROSS="-a $OPTARG"
		;;
	B)
		DXPB_BOOTSTRAP="$OPTARG"
		;;
	b)
		DXPB_BUILDDIR=$(realpath "$OPTARG")
		mkdir -p "$DXPB_BUILDDIR"
		;;
	D)
		[ -d "$OPTARG" ] || {
			printf "ERROR: Cannot find DISTDIR "
			printf "'%s': No such file or directory.\n" "$OPTARG"
			exit 1
		}
		DXPB_DISTDIR=$(realpath "$OPTARG")
		;;
	c)
		[ -f "$OPTARG" ] || {
			printf "ERROR: Cannot find DISTDIR "
			printf "'%s': No such file or directory.\n" "$OPTARG"
			exit 1
		}
		XS_CONF=$(realpath "$OPTARG")
		;;
	h)
		[ -d "$OPTARG" ] || {
			printf "ERROR: Cannot find HOSTDIR "
			printf "'%s': No such file or directory.\n" "$OPTARG"
			exit 1
		}
		DXPB_HOSTDIR=$(realpath "$OPTARG")
		;;
	m)
		[ -d "$OPTARG" ] || {
			printf "ERROR: Cannot find MASTERDIR "
			printf "'%s': No such file or directory.\n" "$OPTARG"
			exit 1
		}
		DXPB_MASTERDIR=$(realpath "$OPTARG")
		;;
	N)
		XB_LOCAL=-N
		;;
	R)
		XB_REPO="-R $OPTARG"
		;;
	t)
		XB_OVERLAYFS=-t
		;;
	\?)
		printf "%s\n" "$USAGE"
		exit 1
		;;
	esac
done
shift $(($OPTIND - 1))

# if using the default "builddir", clean it out
if [ -z "$DXPB_BUILDDIR" ]; then
	DXPB_BUILDDIR=$(realpath builddir)
	mkdir -p $DXPB_BUILDDIR
	rm -rf $DXPB_BUILDDIR/*
fi

echo "DXPB Configuration:"
echo "VOID_PACKAGES_URL: $VOID_PACKAGES_URL"
echo "DXPB_DISTDIR: ${DXPB_DISTDIR}"
echo "DXPB_MASTERDIR: ${DXPB_MASTERDIR}"
echo "DXPB_HOSTDIR: ${DXPB_HOSTDIR}"
echo "DXPB_BUILDDIR: ${DXPB_BUILDDIR}"
echo "XS_CONF: ${XS_CONF}"
echo


git_xbps_bulk() {
	mkdir -p ${DXPB_BULKDIR}
	cd ${DXPB_BULKDIR}
	if [ ! -d .git ] ; then
		echo DXPB: xbps-bulk not found, cloning ...
		git clone ${XBPS_BULK_URL} .
	else
		echo DXPB: found xbps-bulk, updating ...
		git pull 
	fi
}

git_void_packages() {
	mkdir -p ${DXPB_DISTDIR}
	cd ${DXPB_DISTDIR}
	if [ ! -d .git ] ; then
		echo DXPB: void-packages not found, cloning ...
		git clone ${VOID_PACKAGES_URL} .
	else
		echo DXPB: found void-packages, updating ...
		git pull
	fi
}

xbps_src_config() {
	if [ -n "$XS_CONF" ] ; then
		echo DXPB: installing xbps-src etc/conf
		cp "$XS_CONF" ${DXPB_DISTDIR}/etc/conf
	fi
}

bootstrap_install() {
	echo DXPB: Running binary-bootstrap ${DXPB_BOOTSTRAP}
	${DXPB_DISTDIR}/xbps-src -H ${DXPB_HOSTDIR} -m ${DXPB_MASTERDIR} binary-bootstrap ${DXPB_BOOTSTRAP}
}

bootstrap_update() {
	echo DXPB: Running bootstrap-update
	${DXPB_DISTDIR}/xbps-src -H ${DXPB_HOSTDIR} -m ${DXPB_MASTERDIR} bootstrap-update
}

bulk_conf() {
	echo DXPB: Running xbps-bulk/configure
	mkdir -p ${DXPB_BUILDDIR}
	cd ${DXPB_BUILDDIR}
	${DXPB_BULKDIR}/configure -C
	${DXPB_BULKDIR}/configure ${XB_CROSS} ${XB_REPO} ${XB_LOCAL} ${XB_OVERLAYFS} -h \
		${DXPB_HOSTDIR} -d ${DXPB_DISTDIR} -m ${DXPB_MASTERDIR} -t \
		| tee ${DXPB_BUILDDIR}/bulk-config.log
}

bulk_getpkgs() {
	echo DXPB: Running xbps-bulk make print_pkgs
	cd ${DXPB_BUILDDIR}
	make print_pkgs | tee "${DXPB_BUILDDIR}/available-packages.txt"
}

bulk_make() {
	echo DXPB: Running xbps-bulk make
	cd ${DXPB_BUILDDIR}
	make | tee ${DXPB_BUILDDIR}/bulk-make.log
}

bulk_builder() {
	git_xbps_bulk
	git_void_packages
	xbps_src_config
	bootstrap_install
	bootstrap_update
	bulk_conf
	bulk_getpkgs
	bulk_make
}

bulk_builder

