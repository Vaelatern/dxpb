dxpb(7) - Miscellaneous Information Manual

# NAME

**dxpb** - Builders and organization for running xbps-src builds well

# SYNOPSIS

## GRAPH BINARIES

**dxpb-builder**
\[**-hL**]
\[**-g**&nbsp;*graph-endpoint*]
\[**-k**&nbsp;*ssl-dir*]
\[**-H**&nbsp;*hostdir*]
\[**-m**&nbsp;*masterdir*]
\[**-p**&nbsp;*packages-git-repo*]
\[**-P**&nbsp;*git-repo-remote-url*]
\[**-T**&nbsp;*mkdtemp-template*]
**-W**&nbsp;*hostarch:targetarch:cost\[:iscross]*  
**dxpb-frontend**
\[**-hL**]
\[**-g**&nbsp;*graph-endpoint*]
\[**-G**&nbsp;*publish-socket*]
\[**-k**&nbsp;*ssl-dir*]  
**dxpb-grapher**
See:
*IMPORT&nbsp;BINARIES*  
**dxpb-hostdir-master**
See:
*FILE&nbsp;BINARIES*

## IMPORT BINARIES

**dxpb-grapher**
\[**-hL**]
\[**-i**&nbsp;*import-endpoint*]
\[**-g**&nbsp;*graph-endpoint*]
\[**-f**&nbsp;*file-endpoint*]
\[**-d**&nbsp;*package-db*]
\[**-I**&nbsp;*publish-socket*]
\[**-k**&nbsp;*ssl-dir*]  
**dxpb-pkgimport-agent**
\[**-hL**]
\[**-i**&nbsp;*import-endpoint*]
\[**-x**&nbsp;*path-to-xbps-src*]  
**dxpb-pkgimport-master**
\[**-hL**]
\[**-i**&nbsp;*import-endpoint*]
\[**-I**&nbsp;*publish-socket*]
\[**-p**&nbsp;*packages-git-repo*]
\[**-P**&nbsp;*git-repo-remote-url*]
\[**-x**&nbsp;*path-to-xbps-src*]  
**dxpb-poke**
\[**-hL**]
\[**-i**&nbsp;*import-endpoint*]

## FILE BINARIES

**dxpb-hostdir-master**
\[**-hL**]
\[**-r**&nbsp;*package-directory*]
\[**-l**&nbsp;*log-directory*]
\[**-s**&nbsp;*staging-directory*]
\[**-g**&nbsp;*graph-endpoint*]
\[**-f**&nbsp;*file-endpoint*]
\[**-G**&nbsp;*publish-socket-1*]
\[**-F**&nbsp;*publish-socket-2*]
\[**-k**&nbsp;*ssl-dir*]  
**dxpb-hostdir-remote**
\[**-hL**]
\[**-r**&nbsp;*local-package-directory*]
\[**-f**&nbsp;*file-endpoint*]
\[**-k**&nbsp;*ssl-dir*]  
**dxpb-grapher**
See:
*IMPORT&nbsp;BINARIES*

## UTILITIES

**dxpb-graph-to-dot**
\[**-hL**]
\[**-a**&nbsp;*arch*]
\[**-f**&nbsp;*package-db*]
\[**-t**&nbsp;*target-file*]

## SSL UTILITIES

**dxpb-certs-server**
\[**-hL**]
\[**-k**&nbsp;*ssl-dir*]
\[**-R**]
\[**-F**]
\[**-G**]  
**dxpb-certs-server**
\[**-hL**]
\[**-k**&nbsp;*ssl-dir*]
\[**-n**&nbsp;*daemon-name*]

# DESCRIPTION

The
**dxpb**
tool is a monolith program split into many binaries. These binaries can, and
probably should, be run under separate users. These binaries internally follow
certain "chains". Only binaries in the same "chain" can be connected to the
same endpoints.
**dxpb**
also requires certain resources.

# OPTIONS

The dxpb family of binaries accepts certain flags, and aims for consistency
among all daemons (though not necessarily for utilities).

**-a**

> Specific arch in a format xbps-src will accept

**-h**

> Print out help text for the specific binary and quit.

**-L**

> Print out the license and quit.

**-i**

> Endpoint in the form tcp://host:port for the import chain. Host can be \* but
> only on dxpb-pkgimport-master.

**-g**

> Endpoint in the form tcp://host:port for the graph chain. Host can be \* but
> only on dxpb-frontend.

**-f**

> Endpoint in the form tcp://host:port for the file chain. Host can be \* but
> only on dxpb-hostdir-master.

**-d**

> Path which will be a sqlite db, if it is not already, for storing packages.
> This database is not extremely important, but keeping it around will result in
> building sooner than if the database needs to be regenerated.

**-W** *hostarch:targetarch:cost\[:iscross]*

> Worker Specification. If the \[:iscross] component is present, the specification
> is defined as a crossbuilding setup. It is up to the caller to get this
> specification correct.
> The architectures are any architecture that xbps-src accepts.
> The hostarch should match the host machine's architecture.
> The targetarch is whatever that builder should be building. It can be the same
> as the hostarch, or it can be anything else xbps-src will accept. Remember to
> set iscross as relevant.
> Cost should default to 100, and is an unsigned integer between 0 and 255.

**-s**

> A staging directory. Just needs to not be tiny and needs to be read/writeable.
> Shouldn't have anything else in there.

**-I** **-G** **-F**

> Publishing sockets for an ircbot or other scraper to listen and glean state.
> Every publishing socket needs to be unique, otherwise zeromq can't handle it.

**-x**

> A path to xbps-src. This is most often irrelevant, since the working directory
> of those binaries coupled with the default option for this flag generally is
> all a user needs.

**-m**

> Masterdir as used by xbps-src. Generally a chroot-ready environment, managed
> completely by xbps-src (and thus not by the users of dxpb). Any daemon requiring
> this flag must be able to read and write to this.

**-H**

> A path to a /hostdir. /hostdir is the path from the root of a git checkout of
> the upstream -packages repo. This is most often irrelevant, since the working
> directory of those binaries coupled with the default option for this flag
> generally is all a user needs.

**-n**

> Name to be used as daemon. This is relevant for ssl keys, because this name is
> used to name ssl keys. Clients even have the names for their servers hardcoded.

**-r**

> A path to a directory to store xbps packages. hostdir/binpkgs from the root of
> a package git repository checkout.

**-p**

> Path to a git-clone of the upstream -packages repository.

**-P**

> The url to use as remote, as in with a git-clone.

**-T**

> A template pattern for mkdtemp. Should be a string where the last 6 characters
> are X - please see the mkdtemp manpage for more information. Any caller needs
> to create directories with these patterns at whim.

**-k**

> Directory for storing public keys and at least the private key for the named
> daemon. When empty, ssl is disabled.

# CHAINS

## Import

This chain is responsible for reading in xbps-src templates, understanding
what is set in every template, and getting the information needed to track
which packages should be build before building which others.

This set of programs can be aware of the full set of variables available in
an xbps-src template. Here there are the workers who import packages. These are
simple binaries, but are split out into separate binaries to prevent perceived
thread-unsafe file descriptor manipulations when forking().

This chain is where packages are read in for the grapher's sake, and where
the dxpb system is alerted to new packages.

Binaries are named dxpb-poke,
dxpb-pkgimport-agent, dxpb-pkgimport-master, and dxpb-grapher.

## File

This chain is responsible for xbps packages.
Here, files are identified by a triplet of pkgname, version, and arch.
There is support for transporting large binary files (far larger than 2
gigabytes) from remote workers to the main repository. This chain
exists to keep track of where files are.

Binaries are named dxpb-hostdir-master, dxpb-hostdir-remote and dxpb-grapher.

## Graph

On this chain, the graph of all packages is already known, and work is done to
realize the packages on that graph (do the actual building). Here the atom
being communicated is a worker which can help with a pair of target and host
architectures.

Binaries are named dxpb-hostdir-master, dxpb-frontend, dxpb-grapher, and
dxpb-builder.

# RESOURCES

There are a variety of resources needed by dxpb, and they are listed below.

## Import chain

*	The package database, owned and handled by the dxpb-grapher.

*	A git clone of the packages repository, owned and handled by the
	dxpb-pkgimport-master, but read from by the dxpb-pkgimport-agents.

*	An endpoint over which to communicate. See dxpb-grapher -h for the default
	endpoint.

## File chain

*	A directory which is the master repository. This will be owned and managed by
	the dxpb-hostdir-master daemon.

*	A directory for being owned and managed by the dxpb-hostdir-master daemon, for
	use as a staging directory, so as not to pollute the master repository with
	unfinished transfers.

*	A hostdir repository to be read from by any given dxpb-hostdir-remote. There
	should be a one-to-one mapping of these directories and daemons.

*	An endpoint over which to communicate. See dxpb-grapher -h for the default
	endpoint.

## Graph chain

*	A directory for package logs. This will be owned and managed by the
	dxpb-hostdir-master daemon. Build output per architecture/pkgname/version will
	be stored here.

*	A git-clone of a packages repository to be owned and managed by a single
	dxpb-builder process. It will do its job in this directory.

*	An endpoint over which to communicate. See dxpb-grapher -h for the default
	endpoint.

# SSL

Zeromq provides a curve implementation that uses Curve25519, which is an
implementation of DJB's protocol to provide perfect forward secrecy. This
involves server keys, where the public key must be available to every endpoint
which wants to connect. Private keys are generated on every endpoint. Each
client is theoretically capable of choosing its own private key (based on
argv0), but the server does not enforce one key per connection. Thus, if
desirable, every remote on a single box may use a single private key. Due to
the curve implementation, and how permanent keys are never sent in the clear,
this may be an acceptable solution.

The list of allowed public keys is not explicitly given to the server. Instead,
the server with a directory containing the acceptable client public keys.
Public keys and private keys are just flat files, and there is no technological
rule enforcing naming, for dxpb's purposes, argv0 needs to be the pubkey, and
argv0\_secret must be the private key.

# AUTHORS

Toyam Cox &lt;Vaelatern@gmail.com&gt;

# BUGS

Plenty. We just haven't found them all yet.

# SECURITY CONSIDERATIONS

The dxpb-frontend is a rather dumb component. Almost everything goes directly
to the grapher, but is processed by the frontend first. The only reason for
this is to avoid exposing the grapher directly to the internet, since the
grapher actually is capable of ordering builds.

The hostdir-master is NOT a dumb endpoint. Exposing a vulnerability in this
program means exposing the entire repository to an attacker. In the future this
might be fixed.

# SEE ALSO

zmq\_tcp(7)
zmq\_curve(7)

Void Linux - March 20, 2018
