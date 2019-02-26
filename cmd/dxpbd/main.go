package main

import (
	"log"
	"net"

	"github.com/spf13/pflag"
	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/http"
	"github.com/dxpb/dxpb/internal/irc"
)

func startConfig() {
	pflag.BoolP("debug", "d", false, "Debug mode")
	pflag.Parse()
	viper.BindPFlags(pflag.CommandLine)
	viper.SetConfigName("dxpbd")
	viper.AddConfigPath("/etc/dxpb")
	viper.AddConfigPath(".")
	err := viper.ReadInConfig()
	if err != nil {
		log.Panicf("Could not read in configuration file: %s\n", err)
	}
}

func main() {
	log.Println("hello!")
	startConfig()

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
