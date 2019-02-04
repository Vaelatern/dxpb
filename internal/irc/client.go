package irc

import (
	"errors"
	"log"

	"github.com/spf13/viper"
	"github.com/thoj/go-ircevent"
)

// New returns a new IRC client that is initailized and ready for use.
func New() (Client, error) {
	viper.SetDefault("irc.nick", "dxpb")
	viper.SetDefault("irc.ssl", true)

	nick := viper.GetString("irc.nick")
	x := Client{
		Connection: irc.IRC(nick, nick),
		channel:    viper.GetString("irc.channel"),
	}

	x.UseTLS = viper.GetBool("irc.ssl")
	x.QuitMessage = "Server Restarting..."
	x.channel = viper.GetString("irc.channel")

	x.AddCallback("001", func(event *irc.Event) {
		log.Printf("Joining %s", x.channel)
		x.Join(x.channel)
	})

	return x, nil
}

// Connect connects to the IRC server, and only returns on fatal
// errors.
func (c *Client) Connect() error {
	server := viper.GetString("irc.server")
	log.Println("Attempting to connect to", server)
	if err := c.Connection.Connect(server); err != nil {
		return err
	}
	c.Loop()
	return errors.New("Impossible return, IRC connection loop exited")
}

// NoticeCommit sends a formatted message to the connected channel
// that includes the person, the hash itself, and an arbitrary message
// associated with the commit.
func (c *Client) NoticeCommit(who, hash, msg string) {
	c.Noticef(c.channel, "%s committed %s: %s", who, hash, msg)
}
