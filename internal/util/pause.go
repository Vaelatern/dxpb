package util

import "log"

func Pause() {
	log.Print("Gothread pausing")
	in := make(chan bool)
	<-in
}
