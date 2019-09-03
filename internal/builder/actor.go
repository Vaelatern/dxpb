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

type builder_actual struct{}

func reportLog(in io.Reader, supercall spec.Builder_build) (bool, error) {
	eof := false
	log := make([]byte, 64*1024)
	n, err := in.Read(log)
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
			return call.SetLogs(log[:n])
		})
	return eof, nil
}

func (b *builder_actual) Build(call spec.Builder_build) error {
	cmd, pipeOut, err := shell.Exec(viper.GetString("cmd_to_run"), viper.GetStringSlice("cmd_args"))
	if err != nil {
		return err
	}
	eof := false
	for !eof {
		select {
		case <-time.After(2 * time.Second):
			eof, err = reportLog(pipeOut, call)
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
