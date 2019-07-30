using Go = import "/capnp/go.capnp";
@0x902d2bfc3bbf0d43;
$Go.package("spec");
$Go.import("github.com/dxpb/dxpb/internal/api/spec");

enum Arch {
	noarch @0;
	x8664 @1;
	x8664Musl @2;
	i686 @3;
	i686Musl @4;
	armV6 @5;
	armV6Musl @6;
}

struct GithubEvent {
	who @0 :Text;
	union {
		commit :group {
			hash @1 :Text;
			branch @2 :Text;
			msg @3 :Text;
		}
		pullRequest :group {
			number @4 :Int64;
			msg @5 :Text;
			action @6 :Text;
		}
		issue :group {
			number @7 :Int64;
			msg @8 :Text;
			action @9 :Text;
		}
	}
}

enum Results {
	ok @0;
	err @1;
}

enum BuildType {
	bulk @0;
	individual @1;
}

interface Logger {
	append @0 (logs :Data);
}

interface IrcBot {
	noteGhEvent @0 (gh :GithubEvent);
}

interface Builder {
	capabilities @0 () -> (result :List(Capability));
	build @1 (what :What, options :Opts) -> (done :Results);
	struct What {
		name @0 :Text;
		ver @1 :Text;
		arch @2 :Arch;
	}
	struct Capability {
		arch @0 :Arch = noarch;
		type @1 :BuildType = individual;
	}
	struct Opts {
		ignorePkgSpec @0 :Bool;
		log @1 :Logger;
	}
}
