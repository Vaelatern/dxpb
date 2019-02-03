package irc

import (
	"log"

	"github.com/thoj/go-ircevent"
)

var obj *irc.Connection

func Start() {
	channame := "#dxpb3"
	servername := "irc.freenode.net:6697"

	log.Println("Beginning IRC bot")
	obj = irc.IRC("dxpb", "dxpb")
	obj.UseTLS = true
	obj.VerboseCallbackHandler = false
	obj.QuitMessage = "Server restart"

	obj.AddCallback("001", func (event *irc.Event) {
		log.Printf("Joining %s\n", channame)
		obj.Join(channame)
	})
	obj.AddCallback("PRIVMSG", func (event *irc.Event) {
		log.Println(event.Message())
	})

	err := obj.Connect(servername)
	if err != nil {
		log.Fatalf("Failed to connect to IRC server %s\n", servername)
	}
	obj.Loop()
}

func CommitMade(who string, hash string, message string) {
	obj.Noticef("%s committed %s: %s", who, hash, message)
}
