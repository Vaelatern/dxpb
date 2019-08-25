package shell

import (
	"io"
	"os/exec"
)

func Exec(cmd string) (*exec.Cmd, io.ReadCloser, error) {
	command := exec.Command(cmd)
	stdPipe, err := command.StdoutPipe()
	err = command.Start()
	return command, stdPipe, err
}
