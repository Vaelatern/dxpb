package main

import (
	"log"

	"github.com/dxpb/dxpb/internal/irc"
	"github.com/dxpb/dxpb/internal/webhook_target"
)

func main() {
	log.Println("hello!")
	Start()
	go irc.Start()
	gitEvent := make(chan webhook_target.Event)
	go webhook_target.GithubListener(gitEvent)
	for {
		select {
		case event := <-gitEvent:
			irc.CommitMade(event.Committer, event.Hash, event.Msg)
		}
	}
}
