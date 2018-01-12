### Setup

First, build the dxpb package as provided
[here](https://github.com/Vaelatern/void-packages/tree/dxpb/). Should be
version 0.0.6 or greater.

I will assume the following setup: all server processes on the same machine.
Build on the same machine as well.

Please install the following packages: dxpb-server-all, dxpb-remote. Users will
be created during this process, this will be necessary.

At any point, should you wish to have more logging (Please do if testing), take
note of this line: `LOGFILE=/tmp/dxpb-pkgimport-master`. Please put that line,
and variations therapon (changed per daemon please) into `conf` files for the
following runit services: `pkgimport-master`, `frontend`, `grapher`, and
`hostdir-master`. You will also want one in a single additional config file,
to be announced later.

Please have a packages repository (not the git, but the distributed binaries)
with at least the `bootstrap=yes` set of packages at `/var/lib/dxpb/pkgs/`.
That path is equivalent to the current `/current/` path on repositories.

Please have the following ports free: 5195 5196. The default configuration set
will use these for communication.

Please git-clone(1) a packages repository (git this time) under an arbitrary
path. I like `/var/cache/dxpb/repo`. This will need to be fully owned by the
`_dxpb_import` user and group. Please also touch
`$XBPS_MASTERDIR/.xbps_chroot_init`

Please put a `conf` file under `/etc/sv/dxpb-pkgimport-master/`, with at least
the following contents: `WRKDIR=/var/cache/dxpb/repo`. This directory will be
familiar to you, and you should change this directory if you chose a different
path than I.

Please put that same conf file, with `WRKDIR=/var/cache/dxpb/repo`, under
`/etc/sv/dxpb-pkgimport-agent-generic/`.

Two pieces of configuration remain. Please edit `cat /etc/dxpb/conf.yml` to
have the IRC configuration that will best suite you for an overview of logging.
Beware that TRACE on the import master, or on the grapher, will be very verbose
and beyond the permitted resources of most public IRC servers.

We will now configure a builder. This requires a git packages repo checkout,
for which I like using `/var/cache/dxpb/void-packages/`. It will also need
a hostdir directory where `hostdir/binpkgs` refers to the directory we previous
set for `/var/lib/dxpb/pkgs/`.
Whether by symlink or by mount shouldn't matter,
this is a detail for xbps-src to care about. Next, we need configuration for
our builder. Please `mkdir /etc/sv/dxpb-x86_64`, `ln -rs
/etc/sv/dxpb-builder/run /etc/sv/dxpb-x86_64/run`. Then please put the
following in `/etc/sv/dxpb-x86_64/conf`:
```
WRKDIR=/var/cache/dxpb/void-packages/
WRKSPEC=x86_64:x86_64:100
LOGFILE=/tmp/dxpb-x86_64
```
(remember that LOGFILE variable I asked for earlier? Here it is again).

The owner:group for `/var/cache/dxpb/void-packages/` should, recursively,
be `_dxpb_build:_dxpb`. Otherwise xbps-src will complain.

The only reason I do not use `dxpb-hostdir-remote` in this setup is
convenience. It's faster and involves fewer moving parts to have the hostdir
directory refer directly. This is obviously unsuitable for production use, but
first I'd like to see everything up until the package return path work before
I make sure that works perfectly.

### Go Time

Now, make the symlinks! You will want to symlink everything matching
`/etc/sv/dxpb-*` except for: `dxpb-pkgimport-agent-generic` and `dxpb-builder`.

If issues occur, save the problematic daemon's LOGFILE to a gist, send it to
Vaelatern, and `pkill dxpb` to kill everything and start over. This will not
remove files on disk, which is good. If you don't know which daemon is
problematic, just gist all of them and Vaelatern can sort it out. Or you can,
and tell Vaelatern what broke so he can fix it faster and we can carry on to
the next bug.
