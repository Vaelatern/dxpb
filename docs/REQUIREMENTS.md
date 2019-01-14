The dxpb system needs to do the following:
- Given a git repository, ensure all packages herein specified are in the
  package respository.
- Make sure all packages in the package repository are appropiately signed.
- If a package is lacking, request that it be built.
- Ensure a package is only built once its dependencies are all in the package
  repository.
- A package does not need all possible dependencies filled, just the
  dependencies necessary to build on a particular builder.
- Must inform interested parties about how the system is working.
- Needs to allow builders to be spun up on low resource hardware, as well as
  take into account limited resources when allocating builds.
- Must have all working traffic (non-public) encrypted.
- Must have a webpage to inform interested parties of the current state of all
  builds, including packages that will be built.
- Must expose to prometheus, statistics of build including, but not limited to:
  	- Time for a package to actually build
  	- Time between package getting into the git repository and into actual
	  repo.
	- Time between build and inclusion into repository.
	- Current packages per arch in repository
	- Packages per arch possible
