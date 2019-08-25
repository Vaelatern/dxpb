package logger

import (
	"io"

	"github.com/dxpb/dxpb/internal/spec"
)

type Logger struct {
	To io.Writer
}

func (l *Logger) Append(call spec.Logger_append) error {
	logs, err := call.Params.Logs()
	if err != nil {
		return err
	}
	_, err = l.To.Write(logs)
	if err != nil {
		return err
	}
	return nil
}
