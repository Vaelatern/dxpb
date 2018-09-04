# FAQ

### What IS dxpb?

A big pile of code to orchestrate xbps-src across a lot of machines to create
Void Linux's binary packages in a trustworthy and reliable manner

### So, is this dxpb like some sort of a distcc-like distributed build thing? Or what does it do exactly to achieve this making across many machines?

Not distcc, distcc is distributed compilation of a package, which xbps-src
supports. We are not stupid enough to replicate the functionality of xbps-src
in another system. Instead we manage the running of those xbps-src processes.

### Why do you have a C program for managing running a shell script?

We have to manage builds of N packages over M arches, in such a way that all
dependencies of a package are built before a given package. The current system
generates a Makefile, but does so somewhat poorly and in a hacked-up manner. It
fails particularly spectacurlarly when it comes to host dependencies. The
current system also was built around a single builder per architecture, which
does not scale as Void gets bigger.

### What other benefits might such orchestration have?

Currently we cannot add native arm builders for a platform without removing
cross build. If we support having cross builders alongside native builders, we
greatly expand the number of packages we can reasonably build, since certain
packages have legitimate reasons to not support cross compilation, but the
packages that depend on them do not.

### Is there any advantage to using C over another language?

The author was most comfortable in C when he began, and we needed a compiled
language to give dxpb a better chance of being stable over system upgrades.
C also has decent integration with basically any library relevant to solving
out problems.
