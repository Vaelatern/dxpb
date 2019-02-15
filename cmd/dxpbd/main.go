package main

import (
	"log"
	"net"

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

	var ircpipe net.Conn
	var ircside net.Conn
	if !viper.GetBool("irc.disabled") {
		ircside, ircpipe = net.Pipe()
		go ircClient.Connect(ircside)
	}

	webhook_target.GithubListener(ircpipe)
}
