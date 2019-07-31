package pool

import (
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

		rawconn, _, err := websocket.Dial(ctx, url, websocket.DialOptions{
			Subprotocols: []string{"capnproto-dxpb-v1"},
		})
		if err != nil {
			log.Println("TODO: Don't fatally crash on failed dials: err: ", err)
			continue
		}

		conn := rpc.NewConn(rpc.StreamTransport(websocket.NetConn(rawconn)))
		log.Println("Connected to drone: " + url)
		drone := spec.Builder{Client: conn.Bootstrap(ctx)}

		log.Println("Getting capabilities for " + url)
		capabilities, err := drone.Capabilities(ctx, func(p spec.Builder_capabilities_Params) error {
			return nil
		}).Struct()
		if err != nil {
			log.Println("TODO: Don't fatally crash when the capabilities can't be got due to err: ", err)
			continue
		}

		backoff = 0      // No reason to backoff any more
		_ = capabilities // TODO
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
