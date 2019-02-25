package main

import (
	"log"
	"net"

	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/http"
	"github.com/dxpb/dxpb/internal/irc"
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

	httpd, err := http.New(ircpipe)
	if err != nil {
		log.Fatal(err)
	}
	log.Fatal(httpd.Start())
}
