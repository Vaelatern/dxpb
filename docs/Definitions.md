### Sane mechanism for file transfer

The current problem:
- Repositories must be present, completely, on each builder
- The individual xbps-src hostdirs are the sources of truth for the repo
	- Which requires them to be complete and
	- Forbids multiple xbps-src instances on different computers from
	  helping build for the same repo.

What a sane mechanism must do:
- Allow the server to fetch individual packages from the builders.
- Copy those packages into the repo, and arrange for them to be signed.
- Arrange for the repoindex to be regenerated, with the new package added.
