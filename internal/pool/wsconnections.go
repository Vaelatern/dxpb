package pool

import (
	"bytes"
	"context"
	"log"
	"time"

	"nhooyr.io/websocket"
	"zombiezen.com/go/capnproto2/rpc"

	"github.com/dxpb/dxpb/internal/spec"
)

type drone struct {
	Url  string
	Conn *rpc.Conn
}

func connectDrone(ctx context.Context, url string) spec.Builder_capabilities_Results {
	var backoff time.Duration = 0

	for {
		time.Sleep(backoff * time.Second) // Prevent hammering.
		if backoff > 120 {
			backoff = backoff
		} else {
			backoff = (backoff + 1) * 3
		}

		ctx, cancelCtx := context.WithCancel(ctx)
		rawconn, _, err := websocket.Dial(ctx, url, websocket.DialOptions{
			Subprotocols: []string{"capnproto-dxpb-v1"},
		})
		if err != nil {
			log.Println("Dial failed due to: err: ", err)
			cancelCtx()
			continue
		}

		// I apologize for this. I didn't want to pollute the namespace.
		if !func() bool {
			// Don't ask me why. We need to use the connection before the
			// RPC layer gets a go, or calls to Write() will never be made.
			// I really have no idea why.
			in := make([]byte, 6)
			toMatch := []byte{'H', 'e', 'l', 'l', 'o', 0}
			websocket.NetConn(rawconn).Read(in)
			if !bytes.Equal(in, toMatch) {
				log.Println("Comparison not equal: ", in, " != ", toMatch)
				return false
			}
			return true
		}() {
			cancelCtx()
			continue
		}

		conn := rpc.NewConn(rpc.StreamTransport(websocket.NetConn(rawconn)))
		log.Println("Connected to drone: " + url)
		drone := spec.Builder{Client: conn.Bootstrap(ctx)}

		capabilities, err := drone.Capabilities(ctx, func(p spec.Builder_capabilities_Params) error {
			return nil
		}).Struct()
		if err != nil {
			log.Println("Capabilities erred: ", err)
			cancelCtx()
			continue // Whoops! Inadequate
		}

		backoff = 0 // No reason to backoff any more
		if !capabilities.HasResult() {
			log.Println("Remote didn't return a result, resetting connection")
			cancelCtx()
			continue // Whoops! Inadequate
		}

		capList, err := capabilities.Result()
		if err != nil {
			log.Println("Can't get results due to err: ", err)
			cancelCtx()
			continue
		}

		numCaps := capList.Len()
		caps := make([]spec.Builder_Capability, numCaps)
		for i, _ := range caps {
			caps[i] = capList.At(i)
			log.Println(caps[i])
		}

		log.Println("Starting build")
		_, err = drone.Build(ctx, func(p spec.Builder_build_Params) error {
			return nil
		}).Struct()
		if err != nil {
			log.Println("Build erred: ", err)
			cancelCtx()
			continue // Whoops! Inadequate
		}
		log.Println("Build done")

		time.Sleep(3 * time.Minute)
	}
}

func RunPool(foreigners []string) {
	ctx := context.Background()
	resChan := make(chan drone, len(foreigners))
	for _, path := range foreigners {
		go connectDrone(ctx, "ws://"+path+"/ws")
	}

	for {
		select {
		case a := <-resChan:
			log.Println(a.Url)
		}
	}
}
