package builder

import (
	"io"
	"time"

	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/shell"
	"github.com/dxpb/dxpb/internal/spec"
)

type builder struct {
	*builder_actual
}

type read_result struct {
	text []byte
	err  error
}

func watchReader(in io.Reader, out chan read_result) {
	var err error = nil
	log := make([]byte, 32*1024)
	for err == nil {
		limited_in := io.LimitedReader{R: in, N: 31 * 1024}
		n, err := limited_in.Read(log)
		if err == nil || err == io.EOF {
			out <- read_result{text: log[:n], err: err}
		}
		time.Sleep(1 * time.Second)
	}
}

type builder_actual struct{}

func reportLog(in chan read_result, supercall spec.Builder_build) (bool, error) {
	eof := false
	var log []byte
	var err error

	loopAgain := true

	for loopAgain {
		select {
		case res := <-in:
			log = res.text
			err = res.err
		default:
			log = nil
			err = nil
			loopAgain = false
		}

		if err != nil {
			if err == io.EOF {
				eof = true
			} else {
				return false, err
			}
		}

		opts, err := supercall.Params.Options()
		if err != nil {
			return false, err
		}
		opts.Log().Append(supercall.Ctx,
			func(call spec.Logger_append_Params) error {
				return call.SetLogs(log)
			})
	}
	return eof, nil
}

func (b *builder_actual) Build(call spec.Builder_build) error {
	cmd, pipeOut, err := shell.Exec(viper.GetString("cmd_to_run"), viper.GetStringSlice("cmd_args"))
	if err != nil {
		return err
	}
	eof := false

	logs := make(chan read_result, 100)
	go watchReader(pipeOut, logs)

	for !eof {
		select {
		case <-time.After(2 * time.Second):
			eof, err = reportLog(logs, call)
			if err != nil {
				return err
			}
		}
	}
	cmd.Wait()
	return nil
}

func (b *builder_actual) Capabilities(call spec.Builder_capabilities) error {
	capabilities, err := call.Results.NewResult(1)
	if err != nil {
		return err
	}
	capability := capabilities.At(0)
	capability.SetArch(spec.ArchFromString(viper.GetString("arch")))
	capability.SetHostarch(spec.ArchFromString(viper.GetString("hostarch")))
	capability.SetType(spec.BuildType_bulk) // This will be _individual later
	return nil
}
