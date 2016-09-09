# Architecture

DXPB is made up of a great many parts. Originally developed in a single
monolithic program with threads and mutexes, the bottlenecks of mutexes became
too much and a message oriented architecture was adopted. This lead to three
overall message groups: Import, Files, and Graph. These message groups are
the basis of all communications between dxpb's components.

### The components

In no particular order, there are:
- dxpb-poke
- dxpb-pkgimport-agent
- dxpb-pkgimport-master
- dxpb-grapher
- dxpb-buildmaster
- dxpb-builder
- dxpb-hostdir-master
- dxpb-hostdir-remote

### The groups

#### Import

The first group written. This is the part that talks to xbps-src and turns
a package into a well structured form. It is also aware of a git repository,
and uses the information derived from fast forwarding that repository to
determine what packages need to be read in. This group is also where the full
package information stops. No other group is aware of a package's details
beyond its name, version, architecture, and possibly repository.

#### Files

Responsible for transferring files and keeping track of which files are already
present in the hostdir. Also responsible for storing build logs, though this
might be a mistake.

#### Graph

The magic has already happened. This group exists only to take the next layer
of the dependency graph and build it. End worker management is done here, as
are keepalives and ensuring no worker gets stalled for too long. However this
is ultimately just a protocol for making sure things that the boss wants get
built.

### The components in their groups

Sorted into message groups, we have the following:

| Import           | Files          | Graph        |
|------------------|----------------|--------------|
| pkgimport-master | hostdir-master | buildmaster  |
| pkgimport-agent  | hostdir-remote | builder      |
| grapher          | grapher        | grapher      |
| poke             | buildmaster    |              |

Of these, only one is short-lived: dxpb-poke. The rest are daemonized, and are
expected to be working for the system to be working.

As you can see, the grapher is the one daemon that spans all the message
groups. This is due only to the complicated nature of the grapher's role. It
has to be aware of the repository, has to be on the import group for learning
about new packages, and has to be able to instruct the builders. It is also
supposed to be the only one of its kind on the system.

Besides the grapher, we have other processes that expect to be the only one
relevant. We also have processes which, for a variety of reasons, we should
have more than just one instance.

| Group | Singleton        | Many to one    | Oneshot      |
|-------|------------------|----------------|--------------|
| Import| pkgimport-master | pkgimport-agent| poke         |
| Graph | buildmaster      | builder        |              |
| Files | hostdir-master   | hostdir-remote |              |

Any of the above daemons can be killed at any time, and life should resume
after a brief catchup process. If it does not, it's a bug.

It should be noted that the buildmaster is in the files group as well as the
graph group, for routing build logs to the file master. It does not actually
transfer any packages at any point, and is considered most firmly in the
graphing group.

### The connections

##### (Or: the hip daemon is connected to the leg daemon)

pkgimport-master <-> pkgimport-agent

pkgimport-master <- poke

pkgimport-master <-> grapher

hostdir-master <-> grapher

hostdir-master <-> hostdir-remote

grapher <-> buildmaster

builder <-> buildmaster

###### Or, as expressed in Makefile:

grapher: pkgimport-master hostdir-master buildmaster

buildmaster: grapher builder

builder: buildmaster

poke:

pkgimport-master: pkgimport-agent poke

pkgimport-agent: pkgimport-master

hostdir-master: hostdir-remote grapher

hostdir-remote: hostdir-master
