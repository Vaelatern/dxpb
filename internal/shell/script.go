package shell

import (
	"io"
	"os/exec"
)

func Exec(cmd string) (io.ReadCloser, error) {
	command := exec.Command(cmd)
	stdPipe, err := command.StdoutPipe()
	err = command.Start()
	return stdPipe, err
}
