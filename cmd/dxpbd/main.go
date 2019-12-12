package main

import (
	"bufio"
	"fmt"
	"log"
	"os"

	"github.com/spf13/pflag"
	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/bus"
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
	viper.SetDefault("logroot", "./")
}

func pokeOnRead(bus *bus.Bus) {
	reader := bufio.NewReader(os.Stdin)
	fmt.Println("Press enter to trigger build")
	for {
		_, _ = reader.ReadString('\n')
		fmt.Println("Build Triggered")
		bus.Pub("line-at-console", 1)
	}
}

func main() {
	log.Println("hello!")
	startConfig()

	httpd, err := http.New()
	if err != nil {
		log.Fatal(err)
	}

	ircClient, err := irc.New()
	if err != nil {
		log.Panic(err)
	}

	if !viper.GetBool("irc.disabled") {
		go ircClient.Connect(httpd.Msgbus)
	}

	go pool.RunPool(httpd.Msgbus, viper.GetStringMapString("drones"))

	go pokeOnRead(httpd.Msgbus)
	log.Fatal(httpd.Start())
}
