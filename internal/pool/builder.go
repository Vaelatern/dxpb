package pool

import (
	"context"
	"log"
	"os"

	"github.com/prometheus/client_golang/prometheus"

	"github.com/dxpb/dxpb/internal/logger"
	"github.com/dxpb/dxpb/internal/spec"
)

func createOpts(p spec.Builder_build_Params, tag string) (spec.Builder_Opts, error) {
	what, err := p.NewOptions()
	if err != nil {
		return what, err
	}

	logee := spec.Logger_ServerToClient(&logger.Logger{To: os.Stdout})
	what.SetIgnorePkgSpec(true)
	what.SetLog(logee)
	return what, nil
}

func translateJob(p spec.Builder_build_Params, in BuildJob) (spec.Builder_What, error) {
	what, err := p.NewWhat()
	if err != nil {
		return what, err
	}
	what.SetName("")
	what.SetVer("")
	what.SetArch(spec.Arch_noarch)
	return what, nil
}

func runBuilds(ctx context.Context, drone spec.Builder, busyGauge prometheus.Gauge, trigBuild <-chan BuildJob, update chan<- buildUpdate) error {
	id := "Dummy"
	end_builds := false
	for !end_builds {
		select {
		case what := <-trigBuild:
			log.Println("Starting build")
			busyGauge.Inc()
			_, err := drone.Build(ctx, func(p spec.Builder_build_Params) error {
				setThis, err := translateJob(p, what)
				if err != nil {
					return err
				}
				p.SetWhat(setThis)

				opts, err := createOpts(p, id)
				if err != nil {
					return err
				}
				p.SetOptions(opts)
				return nil
			}).Struct()
			busyGauge.Dec()
			if err != nil {
				log.Println("Build erred: ", err)
				end_builds = true
			}
			log.Println("Build done")
		}
	}
	return nil
}
