package builder

import (
	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/shell"
	"github.com/dxpb/dxpb/internal/spec"
)

type builder struct {
	*builder_actual
}

type builder_actual struct{}

func (b *builder_actual) Build(call spec.Builder_build) error {
	shell.Exec(viper.GetString("cmd_to_run"))
	return nil
}

func (b *builder_actual) Capabilities(call spec.Builder_capabilities) error {
	capabilities, err := call.Results.NewResult(1)
	if err != nil {
		return err
	}
	capability := capabilities.At(0)
	capability.SetArch(spec.ArchFromString(viper.GetString("arch")))
	capability.SetType(spec.BuildType_bulk) // This will be _individual later
	return nil
}
