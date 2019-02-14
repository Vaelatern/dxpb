package irc

import (
	"errors"
	"log"

	"github.com/dxpb/dxpb/internal/webhook_target"
	"github.com/spf13/viper"
	irc "github.com/thoj/go-ircevent"
)

// New returns a new IRC client that is initailized and ready for use.
func New() (Client, error) {
	viper.SetDefault("irc.disabled", false)
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

// NoticeEvent sends a formatted message to the connected channel
// that includes a summary of the github event.
func (c *Client) NoticeEvent(event webhook_target.Event) {
	switch event.Type {
	case webhook_target.Commit:
		c.Noticef(c.channel, "%s committed %s: %s", event.Committer, event.Hash, event.Msg)
	case webhook_target.PullRequest:
		c.Noticef(c.channel, "%s created PR %i: %s", event.Committer, event.Number, event.Msg)
	case webhook_target.Issue:
		c.Noticef(c.channel, "%s opened Issue #%i: %s", event.Committer, event.Number, event.Msg)
	}
}
