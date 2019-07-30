using Go = import "/capnp/go.capnp";
@0x902d2bfc3bbf0d43;
$Go.package("spec");
$Go.import("internal/api/spec");

enum BuildEndResult {
	ok @0;
	err @1;
}

enum Arch {
	noarch @0;
	aarch64 @1;
	aarch64Musl @2;
	armV6 @3;
	armV6Musl @4;
	armV7 @5;
	armV7Musl @6;
	i686 @7;
	i686Musl @8;
	x8664 @9;
	x8664Musl @10;
}

struct Repo {
	name @0 :Text;
	arch @1 :Arch;
}

struct Wrkr {
	arch @0 :Arch;
	hostArch @1 :Arch;
	isCross @2 :Bool;
	speed @3 :UInt32;
}

struct Pkg {
	name @0 :Text;
	arch @1 :Arch;
	ver @2  :Text;
}

interface Worker {
	details @0 () -> (wrkr :Wrkr);
	build @1 (pkg :Pkg) -> (status :BuildEndResult);
	listBuiltPkgs @2 () -> (pkg :List(Pkg));
	canBuildPkg @3 (pkg :Pkg) -> (canBuild :Bool);
}

interface IrcBot {
	builderDone @0 (pkg :Pkg, status :BuildEndResult);
}

interface GraphServer {
	checkForNewPackagesInGit @0 ();
	canPkgBuild @1 (pkg :Pkg) -> (canBuild :Bool);
}

interface JobServer {
	keepAlive @0 ();
	register @1 (worker :Worker);
	writeLog @2 (log :Data);
	startBulkBuild @3 (pkgs :Text);
}
