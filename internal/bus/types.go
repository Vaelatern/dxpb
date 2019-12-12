package bus

type WorkerSpec struct {
	Alias    string
	Hostarch string
	Arch     string
}

type GhEvent struct {
	Who    string
	Hash   string
	Branch string
	Msg    string
	Action string
	Number int64
}
