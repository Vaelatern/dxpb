package main

import (
	"log"

	"github.com/dxpb/dxpb/internal/irc"
	"github.com/dxpb/dxpb/internal/webhook_target"
)

func main() {
	log.Println("hello!")
	Start()

	ircClient, err := irc.New()
	if err != nil {
		log.Panic(err)
	}

	log.Println("Connecting to IRC")
	go ircClient.Connect()

	gitEvent := make(chan webhook_target.Event)
	go webhook_target.GithubListener(gitEvent)
	for {
		select {
		case event := <-gitEvent:
			ircClient.NoticeCommit(event.Committer, event.Hash, event.Msg)
		}
	}
}
