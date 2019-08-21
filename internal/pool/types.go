package pool

import (
	"zombiezen.com/go/capnproto2/rpc"

	"github.com/dxpb/dxpb/internal/spec"
)

type drone struct {
	Url  string
	Conn *rpc.Conn
}

type BuildStatus uint8

const (
	BuildStatus_startup BuildStatus = 0
	BuildStatus_ready   BuildStatus = 1
	BuildStatus_working BuildStatus = 2
)

type buildUpdate struct {
	status BuildStatus
	res    spec.Results
}

type BuildJob struct {
	pkglist []string
}

type builderInfo struct {
	status   BuildStatus
	hostarch spec.Arch
	arch     spec.Arch
	req      chan BuildJob
	ret      chan buildUpdate
}
