package irc

import (
	"log"

	"github.com/spf13/viper"
	"github.com/thoj/go-ircevent"
)

var obj *irc.Connection

func Start() {
	log.Println("Beginning IRC bot")
	obj = irc.IRC(viper.GetString("irc.nick"), viper.GetString("irc.nick"))
	obj.UseTLS = viper.GetBool("irc.ssl")
	obj.QuitMessage = "Server restart"

	obj.AddCallback("001", func(event *irc.Event) {
		log.Printf("Joining %s\n", viper.GetString("irc.channel"))
		obj.Join(viper.GetString("irc.channel"))
	})
	obj.AddCallback("PRIVMSG", func(event *irc.Event) {
		log.Println(event.Message())
	})

	err := obj.Connect(viper.GetString("irc.server"))
	if err != nil {
		log.Panicf("Failed to connect to IRC server %s\n", viper.GetString("irc.server"))
	}
	obj.Loop()
}

func CommitMade(who string, hash string, message string) {
	obj.Noticef(viper.GetString("irc.channel"), "%s committed %s: %s", who, hash, message)
}
