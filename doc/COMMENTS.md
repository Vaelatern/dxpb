# Distributed XBPS Package Builder

Yo.

# List of packages necessary for build

- zeromq-devel
- czmq-devel
- sqlite-devel
- libgit2-devel
- libbsd-devel
- libxbps-devel
- graphviz-devel

# Information for development

This space will be used for rambling, unordered comments for right now.

### pkg_archs

When transmitting over any protocol or saving to a database, architectures must
be of type string. When storing in memory for the purposes of working with the
data, it may be stored as enum pkg_archs. The pkg_archs_str character array
can be used to aid in translation from enum to string, and there is a helper
function under bpkg to help translate the other direction (string to enum).

The reasoning for this design is simple. In the future, there will be more
architectures supported. The enum values will then change. Any currently
existing data (database, etc.) may not know about the change right away,
but it would not be reasoanble to regenerate everything just because an
enumeration changed.  Thus, any data being sent out of memory must be a
string to make transitions like this neat.

The second reason is for interoperability, to make it simpler for alternate
tools, maybe a later Python web client or an IRC bot, to simply use the
architecture data as provided without a need for a dupicate of the map defined
in bxpkg.h. This may well save some development hassle down the road.

### Setting cancross on a package

The first step occurs when reading the package. Check if nocross is set, and
set cancross to !nocross. Then conditional on that being okay, read the package
as if doing a cross build and record values. But inside that, we check for
cancross one more time, to support broken setting inside a conditional on cross
building, as well as a contional on cross building and a particular (target)
architecture. That second step works according to this truth table:

```
B A | c
r n | a
o d | n
k   | c
e N   r
n o   o
? w   s
  ?   s
-------
F T | N
F F | Y
T T | Y
T F | Y
```

### Number of dxpb-pkgimport-agents to run

In the early days of this software, pkgimporting was done in a locking fashion,
so I ran 14 threads (one for each arch then some extra wiggle room for noarch).
It locked horribly. The current model doesn't lock at all, so the number of
agents is immaterial. However, in trials on a 2.6 GHz Xeon, a single package
(with all 10 architectures + noarch + host) could be read in a single thread in
just under 10 seconds total. Therefore, one can decide how many agents to run
based on that number, but probably should be capped at the 2x the number of
cores one can devote to the agents. In those same trials, each agent used 34%
of cpu time on a single core while actively reading. The number of agents
should not be just one, however, because that would make commits that touch
hundreds of templates take over an hour to resolve, as that ten second number
would be necessary for every package and subpackage. No processing can happen
while packages are being read from xbps-src, because the graph is not likely
able to be resolved until after the packages are all read, and is likely to be
wrong with obsolete packages in the meanwhile.

In the future it would be nice to have a daemon fork off agents to decide for
you how many agents to run. Not happening today.

It should be noted that in series, 700 packages would take at least 116 minutes
to read in from xbps-src. With 14 workers in parallel, this number would be cut
to about 9 minutes.

### ensure_sock()

On Linux, if you want to communicate over a socket on the filesystem, even just
to read, you must have read and write access. On BSD, permissions on sockets
are ignored, and access to the socket directory is the only control on access.

We at least target Linux, so we must chmod 777 the sockets. We can't use
group permissions on BSD, so we just use the _dxpb group on `/var/run/dxpb/`
for a portable permissioning scheme.

### The link that straddles pkgimport and pkggraph and pkgfiles

Going on the basic notion that zeromq is based on "multipart" messages, where
zeromq framing segregates message parts.

We implement that in our routing, but only as a "last stop." We are breaking
the zeromq mechanisms, and use a new frame to give extra information about what
follows. The logic is simple if one message type goes to one location. It isn't
so simple if not.

### The types of package dependencies

There are a limited number of possible package name formats.
1. `${pkgname}-${version}_${revision}`
1. `${pkgname}>=${version}_${revision}`
1. `${pkgname}<=${version}_${revision}`
1. `${pkgname}>${version}_${revision}`
1. `${pkgname}<${version}_${revision}`
1. `${pkgname}`

But the basic forms are just 2. `$pkgname{,-$version_$revision}}`, since xbps
refuses `$pkgname-$version` without `_$revision`.
Therefore to match a dependency statement to a package name, one simply tries
the whole string, or passes off the xbps pkgname checking functions.

### So how does this SSL thing work anyway?

Zeromq provides a curve implementation that uses Curve25519, which is an
implementation of DJB's protocol to provide perfect forward secrecy. This
involves server keys, where the public key must be available to every endpoint
which wants to connect. Private keys are generated on every endpoint. Each
builder is capable of choosing its own private key (based on argv0), but the
server does not enforce one key per connection. Thus, if desirable, every
remote on a single box may use a single private key. Due to the curve
implementation, and how permanent keys are never sent in the clear, this may be
an acceptable solution.

#### Yeah, but I looked at the code. It's just magic.

ZeroMQ provides the magic. For processes with multiple threads, the
special `inproc://` connection mechanism enables communication over named
sockets that are not visible to the filesystem or network stack.

When a zeromq socket option is set, setting the encryption to CURVE, the socket
is told to begin negotiations by passing the protocol through an actor. This
handles the implementation of the curve protocol to establish encryption, and
then the socket itself can take over the connection.

There is a special endpoint, `inproc://zeromq.zap.01`, where a `zauth` actor
listens and replies. That zauth actor is configured in the `dxpb-*` family
of binaries to be aware of the private and public keys it should use or accept.
Thus, the socket involved in the connection needs only to know it needs to use
CURVE authentication for it to be handled automatically.

The list of allowed public keys is not explicitly given to the server. Instead,
the server with a directory containing the acceptable client public keys.
