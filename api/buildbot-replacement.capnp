using go = import "go.capnp";

@0x902d2bfc3bbf0d43;

interface IrcBot {
	builderDone @0 (pkgs List(:Text)) -> ();
}

interface GraphServer {
	poke @0 () -> ();
}

interface JobServer {
	register @0 () -> ();
	writeLog @1 (log :Text) -> ();
	keepAlive @2 () -> ();
	startBulkBuild @3 (pkgs List(:Text)) -> ();
}

interface Worker {
	startBulkBuild @0 () -> ();
}
