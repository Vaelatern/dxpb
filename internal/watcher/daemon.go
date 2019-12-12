package watcher

import (
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"

	"github.com/dxpb/dxpb/internal/bus"
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
	msgsToSend = promauto.NewCounter(prometheus.CounterOpts{
		Name: "dxpb_irc_msg_to_send",
		Help: "Total number of messages to send",
	})
	msgsSent = promauto.NewCounter(prometheus.CounterOpts{
		Name: "dxpb_irc_msg_sent",
		Help: "Total number of messages sent",
	})
)

const (
	worker_GONE int = 0
	worker_IDLE int = 1
	worker_BUSY int = 2
)

type state struct {
	workers map[string]int
}

func Start(msgbus *bus.Bus) {
	state := state{workers: make(map[string]int)}

	msgbus.Sub("worker-busy", func(val interface{}) {
		v := val.(bus.WorkerSpec)
		state.workers[v.Alias] = worker_BUSY
		workersBusy.With(prometheus.Labels{
			"alias":    v.Alias,
			"hostarch": v.Hostarch,
			"arch":     v.Arch,
		}).Inc()
	})
	msgbus.Sub("worker-idle", func(val interface{}) {
		v := val.(bus.WorkerSpec)
		state.workers[v.Alias] = worker_IDLE
		workersBusy.With(prometheus.Labels{
			"alias":    v.Alias,
			"hostarch": v.Hostarch,
			"arch":     v.Arch,
		}).Dec()
	})
	msgbus.Sub("worker-ready", func(val interface{}) {
		v := val.(bus.WorkerSpec)
		state.workers[v.Alias] = worker_IDLE
		numWorkers.With(prometheus.Labels{
			"alias":    v.Alias,
			"hostarch": v.Hostarch,
			"arch":     v.Arch,
		}).Inc()
	})
	msgbus.Sub("worker-gone", func(val interface{}) {
		v := val.(bus.WorkerSpec)
		state.workers[v.Alias] = worker_GONE
		numWorkers.With(prometheus.Labels{
			"alias":    v.Alias,
			"hostarch": v.Hostarch,
			"arch":     v.Arch,
		}).Dec()
	})
	msgbus.Sub("irc-poke", func(val interface{}) {
	})
	msgbus.Sub("line-at-console", func(val interface{}) {
		msgbus.Pub("job-to-do", 1)
	})
	msgbus.Sub("note-gh-event-commit", func(val interface{}) {
		msgsToSend.Inc()
	})
	msgbus.Sub("note-gh-event-pullRequest", func(val interface{}) {
		msgsToSend.Inc()
	})
	msgbus.Sub("note-gh-event-issue", func(val interface{}) {
		msgsToSend.Inc()
	})
	msgbus.Sub("sent-irc-msg", func(val interface{}) {
		msgsSent.Inc()
	})
}
