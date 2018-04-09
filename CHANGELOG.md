# Changelog

## 0.0.13.3 - 2018-03-21 - The Next One
### Overview
- Builders now expect what the frontend will tell them.
- The frontend now properly propegates addresses to and from the grapher.

## 0.0.13 - 2018-03-13 - The one where build logs don't cause faults
### Overview
- A late-joining grapher now gets told about worker state. Does not yet get
  a dump of present worker state.
- The pkgimport-master now also binary-bootstraps its own masterdir, needed for
  correctness of certain templates.
- The package deletion case no longer causes the pkgimport-master to crash.
- The bworker library supports knowledge of subgroup addresses.
- Log routing to the logging agent is now possible without crashing. Logs are
  placed in $pkgname/$version/$arch format.

## 0.0.12 - 2018-03-03 - The one which probably works now
### Overview
- Flags have been changed for consistency of purpose across all daemons.
- There is now a manpage for dxpb, which will eventually be joined by more.
- Encrypted communications are now used if server keys are present.
- Attempting to assign workers with none available doesn't crash the grapher.
- The graph now doesn't leak memory when packages change to noarch.
- Now only one ARCH_TARGET package is created, improving quality of pkgimport.
- Job assignment is greatly improved, works now, and now any builder can be
  assigned a noarch package if there are no ready packages to be built in their
  native architecture.
- Workers requiring git repos will now do the clone themselves. This means
  a new flag was introduced.
- dxpb-graph-to-pkg now can output .dot to standard output. No news yet on
  reading sqlite3 databases from standard input, to use dxpb-graph-to-pkg as
  a filter in a pipeline.

## 0.0.11 - 2018-02-08 - The one where we are not a cookie monster
### Overview
- ALL BINARIES ARE INCOMPATIBLE WITH PREVIOUS VERSIONS - message signatures
  changed to make sure a mistyped port number is obvious at runtime.
- Binaries will now actually print 0.0.11 as their version, instead of 0.0.9
- Documentation was fixed, and now is copied with a `make install`.
- Grapher no longer has a failure mode which involves replicating and eating up
  memory.
- Git fast forwards got better and less-crashy on freshly cloned repos.

## 0.0.10 - 2018-01-30 - The one where the builder works more than once
### Overview
- Builder now properly resets after a build, and can be instructed to build
  again.
- Files are now checksummed by code, to ensure files have been sent faithfully.

## 0.0.9 - 2018-01-29 - The one where remote operations are possible
### Overview
- Hostdir-remote now works with hostdir-master to faithfully send files.
- Most instances of communication with ipc:// sockets have been changed to
  tcp:// endpoints by default, avoiding permissions issues
- Add harness for and fix build remote (including bootstrap functionality)

## 0.0.8 - 2018-01-19 - The one with better repository watching
### Overview
- Improve speed of confirming if a file is present in a directory.
- Fixed repository watcher state to actually send PKGNOTHERE notices when
  applicable.
- Disable a particular testing-only library by default.
- Slight state machine, code, and documentation cleanups.
- Grapher now won't endlessly order rebuilds of packages.

## 0.0.7 - 2018-01-13 - The one without unhandled states
### Overview
- State machines now handle every possible message.
- pkgimport-agent configuration is now centralized by default.
- sv-common now sets permissions correctly for dirs.
- Switched to localhost-bound connections by default instead of ipc sockets.

## 0.0.6 - 2018-01-12 - The one that doesn't infinite loop by design
### Overview
- The frontend doesn't infinite loop any longer
- Build with `make CC=gcc` works again.

## 0.0.5 - 2018-01-10 - The one that handles grapher timeouts better
### Overview
- It kills daemons less often due to timeouts and properly unassigns graphers.

## 0.0.4 - 2018-01-10 - The one that can be tested easily
### Overview
- Just shell fixes to make testing and spinning up new instances a lot easier.

## 0.0.3 - 2018-01-09 - The one that can be tested
### Overview
- Fixed runit scripts with more bells and whistles, making them more useful in
  testing.

## 0.0.2 - 2018-01-08 - The one that can use old package databases
### Overview
- Now with repository rewinding working, for when your pkgs.db is older than
  your git checkout
- More resilient package import server (pkgimport-master)
### Added
- Now pkgimport-master will survive the death of an agent, where previously
  agents remained as zombies, accepting jobs but never completing them,
  preventing legitimate workers from doing jobs.
- Packages repo rewinding
- Reading all pipes to the end at a cost of at least 6 syscalls per package
  import.

## 0.0.1 - 2018-01-01 - The one that runs
### Overview
- It runs. It's some lines of code. It might work, but we have not fully
  tested that functionality just yet.
