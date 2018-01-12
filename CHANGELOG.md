# Changelog

## 0.0.6 - 2018-01-12 - The one that doesn't infinite loop by design.
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


