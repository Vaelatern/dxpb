using Go = import "/capnp/go.capnp";
@0x902d2bfc3bbf0d43;
$Go.package("spec");
$Go.import("github.com/dxpb/dxpb/internal/api/spec");

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

interface IrcBot {
	noteGhEvent @0 (gh :GithubEvent);
}
