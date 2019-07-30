all: dxpbd dxpbuilder

internal/dxpbspec.capnp.go: api/dxpbspec.capnp
	scripts/build-capnp.sh

dxpbd: internal/dxpbspec.capnp.go
	go build github.com/dxpb/dxpb/cmd/dxpbd

dxpbuilder: internal/dxpbspec.capnp.go
	go build github.com/dxpb/dxpb/cmd/dxpbuilder

clean:
	rm -f dxpbd dxpbuilder
