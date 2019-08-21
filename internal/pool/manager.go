package pool

import ()

type mergedChan struct {
	alias string
	val   buildUpdate
}

func managePool(jobs <-chan BuildJob, builders map[string]builderInfo) {
	var inChan chan mergedChan
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
