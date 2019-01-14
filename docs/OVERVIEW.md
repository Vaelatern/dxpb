There are 3 systems.

- Graph
	- Aware of the current state of the packages repository.
	- Uses this awareness to build an in-state graph.
	- Uses this graph to decide what needs to be built.
- Build
	- Takes responsibility for making sure packages get built.
	- Has a controller and many workers.
	- Is responsible for logging builds and putting those logs in good
	  places.
- Files
	- Has a high security and very-high-security part.
	- Very High Security:
		- Signs packages after being told to do so.
	- High Security
		- Gets packages and puts them in place.
		- Responds to the Graph to let the Graph know what packages
		  exist on disk already. In this way, the Graph can let the
		  Files know what is probably ready to be brought back, in a
		  nice and consistent way.
