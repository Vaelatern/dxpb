package shell

import (
	"os/exec"
)

func Exec(cmd string) {
	cmd := Command(cmd)
	stdPipe, err := cmd.StdoutPipe()
}
