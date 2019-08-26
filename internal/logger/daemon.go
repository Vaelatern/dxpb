package logger

import (
	"io"
	"time"

	"github.com/dxpb/dxpb/internal/spec"
)

type Logger struct {
	To     io.WriteCloser
	Closer *time.Timer
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

	if !l.Closer.Stop() {
		<-l.Closer.C
	}
	l.Closer.Reset(10 * time.Second) // This is what closes the file on inactivity

	return nil
}
