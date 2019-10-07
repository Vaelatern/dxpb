package pool

import (
	"bytes"
	"context"
	"log"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
	"nhooyr.io/websocket"
	"zombiezen.com/go/capnproto2/rpc"

	"github.com/dxpb/dxpb/internal/spec"
)

var (
	numWorkers = promauto.NewGaugeVec(prometheus.GaugeOpts{
		Name: "dxpb_wrkrs_connected",
		Help: "Workers connected",
	}, []string{"alias", "hostarch", "arch"})
	workersBusy = promauto.NewGaugeVec(prometheus.GaugeOpts{
		Name: "dxpb_wrkrs_busy",
		Help: "Workers connected",
	}, []string{"alias", "hostarch", "arch"})
)

func connectDrone(ctx context.Context, url string, alias string, info builderInfo) spec.Builder_capabilities_Results {
	var backoff time.Duration = 0

	for {
		time.Sleep(backoff * time.Second) // Prevent hammering.
		if backoff >= 120 {
			backoff = 120
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
		if numCaps != 1 {
			log.Println("ERR: Can't currently use multiple capabilities")
			cancelCtx()
			continue
		}

		cap := capList.At(0)
		workerGauge := numWorkers.With(prometheus.Labels{
			"alias":    alias,
			"hostarch": cap.Hostarch().String(),
			"arch":     cap.Arch().String(),
		})
		workerBusyGauge := workersBusy.With(prometheus.Labels{
			"alias":    alias,
			"hostarch": cap.Hostarch().String(),
			"arch":     cap.Arch().String(),
		})

		// Blocks in an infinite loop
		workerGauge.Inc()
		err = runBuilds(ctx, alias, drone, workerBusyGauge, info.req, info.ret)
		workerGauge.Dec()
		if err != nil {
			log.Println("Builds failed due to err: ", err)
			cancelCtx()
			continue
		}
	}
}

func RunPool(jobqueue <-chan BuildJob, foreigners map[string]string) {
	ctx := context.Background()
	builders := make(map[string]builderInfo)
	for alias := range foreigners {
		b := *new(builderInfo)
		b.ret = make(chan buildUpdate)
		b.req = make(chan BuildJob)
		builders[alias] = b
	}
	for alias, path := range foreigners {
		go connectDrone(ctx, "ws://"+path+"/ws", alias, builders[alias])
	}

	managePool(jobqueue, builders)
}
