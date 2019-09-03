package shell

import (
	"io"
	"os/exec"
)

func Exec(cmd string, args []string) (*exec.Cmd, io.ReadCloser, error) {
	command := exec.Command(cmd)
	stdPipe, err := command.StdoutPipe()
	command.Args = args
	err = command.Start()
	return command, stdPipe, err
}
