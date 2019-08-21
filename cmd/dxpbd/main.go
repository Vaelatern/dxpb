package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"os"

	"github.com/spf13/pflag"
	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/http"
	"github.com/dxpb/dxpb/internal/irc"
	"github.com/dxpb/dxpb/internal/pool"
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

func pokeOnRead(out chan<- pool.BuildJob) {
	reader := bufio.NewReader(os.Stdin)
	fmt.Println("Press enter to trigger build")
	for {
		_, _ = reader.ReadString('\n')
		fmt.Println("Build Triggered")
		out <- *new(pool.BuildJob)
	}
}

func main() {
	log.Println("hello!")
	startConfig()

	ircClient, err := irc.New()
	if err != nil {
		log.Panic(err)
	}

	var queueJobs chan pool.BuildJob
	queueJobs = make(chan pool.BuildJob, 10)

	go pool.RunPool(queueJobs, viper.GetStringMapString("drones"))

	var ircpipe net.Conn
	var ircside net.Conn
	if !viper.GetBool("irc.disabled") {
		ircside, ircpipe = net.Pipe()
		go ircClient.Connect(ircside)
	}

	go pokeOnRead(queueJobs)
	httpd, err := http.New(ircpipe)
	if err != nil {
		log.Fatal(err)
	}
	log.Fatal(httpd.Start())
}
