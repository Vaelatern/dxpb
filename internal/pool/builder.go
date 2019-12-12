package pool

import (
	"context"
	"io"
	"log"
	"os"
	"strconv"
	"time"

	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/bus"
	"github.com/dxpb/dxpb/internal/logger"
	"github.com/dxpb/dxpb/internal/spec"
)

func logFile(base string, tag string) (io.WriteCloser, error) {
	fullDirName := base + "/" + strconv.FormatInt(time.Now().Unix(), 10)
	err := os.MkdirAll(fullDirName, 0755)
	if err != nil {
		return nil, err
	}
	fullFileName := fullDirName + "/" + tag + ".txt"
	rV, err := os.OpenFile(fullFileName, os.O_APPEND|os.O_CREATE|os.O_EXCL|os.O_WRONLY, 0644)
	if err != nil {
		return nil, err
	}
	return rV, nil
}

func createOpts(p spec.Builder_build_Params, tag string) (spec.Builder_Opts, error) {
	what, err := p.NewOptions()
	if err != nil {
		return what, err
	}

	outFile, err := logFile(viper.GetString("logroot"), tag)
	if err != nil {
		return what, err
	}
	logee := spec.Logger_ServerToClient(&logger.Logger{To: outFile,
		Closer: time.AfterFunc(time.Minute, func() { _ = outFile.Close() })})
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

func runBuilds(ctx context.Context, alias string, drone spec.Builder, msgbus *bus.Bus, trigBuild <-chan BuildJob, update chan<- buildUpdate, hostarch string, arch string) error {
	end_builds := false
	wrkrSpec := bus.WorkerSpec{Alias: alias, Hostarch: hostarch, Arch: arch}
	for !end_builds {
		select {
		case what := <-trigBuild:
			log.Println("Starting build")
			msgbus.Pub("worker-busy", wrkrSpec)
			_, err := drone.Build(ctx, func(p spec.Builder_build_Params) error {
				setThis, err := translateJob(p, what)
				if err != nil {
					return err
				}
				p.SetWhat(setThis)

				opts, err := createOpts(p, alias)
				if err != nil {
					return err
				}
				p.SetOptions(opts)
				return nil
			}).Struct()
			msgbus.Pub("worker-idle", wrkrSpec)
			if err != nil {
				log.Println("Build erred: ", err)
				end_builds = true
			}
			log.Println("Build done")
		default:
			x, err := drone.Keepalive(ctx, func(p spec.Builder_keepalive_Params) error {
				p.SetI(3)
				return nil
			}).Struct()
			if err != nil {
				end_builds = true
			} else if x.I() != 6 {
				log.Fatal("Drone misbehaving! ", alias)
			}
		}
	}
	return nil
}
