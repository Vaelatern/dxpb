package main

import (
	"log"

	"github.com/spf13/viper"

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

	if !viper.GetBool("irc.disabled") {
		go ircClient.Connect()
	}

	gitEvent := make(chan webhook_target.Event)
	go webhook_target.GithubListener(gitEvent)
	for {
		select {
		case event := <-gitEvent:
			ircClient.NoticeEvent(event)
		}
	}
}
