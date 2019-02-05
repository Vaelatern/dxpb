package main

import (
	"log"
	"os"

	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/irc"
	"github.com/dxpb/dxpb/internal/webhook_target"
)

func main() {
	configGood := true
	err := Start()
	writeConfigFunc := func() error { return viper.SafeWriteConfigAs(viper.GetString("output-conf")) }
	if err != nil && viper.GetString("output-conf") == "" {
		configGood = false
		writeConfigFunc = func() error { return viper.SafeWriteConfigAs("dxpbd.yaml") }
	}

	ircClient, err := irc.New()
	if err != nil {
		log.Panic(err)
	}

	if viper.GetString("output-conf") != "" || !configGood {
		if err := writeConfigFunc(); err != nil {
			log.Fatal("Could not write sample config: ", err)
		}
		if configGood {
			log.Printf("Wrote sample config with defaults to %s\n",
				viper.GetString("output-conf"))
		} else {
			log.Printf("Wrote sample config with defaults to dxpbd.yaml\n")
		}
		log.Println("Quitting cleanly on a job well done")
		os.Exit(0)
	}

	if !viper.GetBool("irc.disabled") {
		go ircClient.Connect()
	}

	gitEvent := make(chan webhook_target.Event)
	go webhook_target.GithubListener(gitEvent)
	for {
		select {
		case event := <-gitEvent:
			ircClient.NoticeCommit(event.Committer, event.Hash, event.Msg)
		}
	}
}
