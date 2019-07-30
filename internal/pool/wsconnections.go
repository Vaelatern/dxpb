package pool

import (
	"context"
	"log"

	"nhooyr.io/websocket"
	"zombiezen.com/go/capnproto2/rpc"

	"github.com/dxpb/dxpb/internal/spec"
)

type drone struct {
	Url  string
	Conn *rpc.Conn
}

func connectDrone(ctx context.Context, url string, resChan chan<- drone) spec.Builder_capabilities_Results {
	rawconn, _, err := websocket.Dial(ctx, url, websocket.DialOptions{
		Subprotocols: []string{"capnproto-dxpb-v1"},
	})
	if err != nil {
		log.Fatal("TODO: Don't fatally crash on failed dials: err: ", err)
	}
	conn := rpc.NewConn(rpc.StreamTransport(websocket.NetConn(rawconn)))
	resChan <- drone{Url: url, Conn: conn}
	drone := spec.Builder{Client: conn.Bootstrap(ctx)}

	log.Println("Getting capabilities for " + url)
	rV, err := drone.Capabilities(ctx, func(p spec.Builder_capabilities_Params) error {
		return nil
	}).Struct()
	if err != nil {
		log.Fatal("TODO: Don't fatally crash when the capabilities can't be got due to err: ", err)
	}
	return rV
}

func RunPool(foreigners []string) {
	ctx := context.Background()
	resChan := make(chan drone, len(foreigners))
	for _, path := range foreigners {
		go connectDrone(ctx, "ws://"+path+"/ws", resChan)
	}

	for {
		select {
		case a := <-resChan:
			log.Println(a.Url)
		}
	}
}
