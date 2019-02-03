package main

import (
	"log"

	"github.com/dxpb/dxpb/internal/irc"
	"github.com/dxpb/dxpb/internal/util"
	"github.com/dxpb/dxpb/internal/webhook_target"
)

func main() {
	log.Println("hello!")
	go irc.Start()
	gitBind := ":8080"
	gitEvent := make(chan webhook_target.Event)
	go webhook_target.GithubListener(gitEvent, gitBind)
	util.Pause()
}
