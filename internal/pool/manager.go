package pool

import (
	"fmt"

	"github.com/dxpb/dxpb/internal/bus"
)

type mergedChan struct {
	alias string
	val   buildUpdate
}

func managePool(msgbus *bus.Bus, builders map[string]builderInfo) {
	var inChan chan mergedChan
	var jobs chan BuildJob
	jobs = make(chan BuildJob)

	msgbus.Sub("job-to-do", func(val interface{}) {
		fmt.Println("New jobs!")
		jobs <- *new(BuildJob)
	})

	for alias, builder := range builders {
		go func(out chan<- mergedChan, alias string, from chan buildUpdate) {
			for {
				select {
				case in := <-from:
					out <- mergedChan{val: in, alias: alias}
				}
			}
		}(inChan, alias, builder.ret)
	}

	for {
		select {
		case job := <-jobs:
			for _, builder := range builders {
				builder.req <- job
			}
		case fromWorkers := <-inChan:
			_ = builders[fromWorkers.alias]
			_ = fromWorkers.val
		}
	}
}
