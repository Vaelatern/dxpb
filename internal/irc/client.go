package irc

import (
	"log"

	"github.com/spf13/viper"
	irc "github.com/thoj/go-ircevent"

	"github.com/dxpb/dxpb/internal/bus"
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

// NoteGhEvent returns functions that send a formatted message to the
// connected channel that includes a summary of the github event.
func (c *Client) NoteGhEvent(msgbus *bus.Bus, msgtype string) func(interface{}) {
	switch msgtype {
	case "commit":
		return func(val interface{}) {
			v := val.(bus.GhEvent)
			msgbus.Pub("sent-irc-msg", 1)
			c.Noticef(c.channel, "%s committed %s: %s", v.Who, v.Hash[:8], v.Msg)
		}
	case "pullRequest":
		return func(val interface{}) {
			v := val.(bus.GhEvent)
			msgbus.Pub("sent-irc-msg", 1)
			c.Noticef(c.channel, "%s %s PR %d: %s", v.Who, v.Action, v.Number, v.Msg)
		}
	case "issue":
		return func(val interface{}) {
			v := val.(bus.GhEvent)
			msgbus.Pub("sent-irc-msg", 1)
			c.Noticef(c.channel, "%s %s Issue #%d: %s", v.Who, v.Action, v.Number, v.Msg)
		}
	}
	return nil
}

// Connect connects to the IRC server, and only returns on fatal
// errors.
func (c *Client) Connect(bus *bus.Bus) error {
	server := viper.GetString("irc.server")
	log.Println("Attempting to connect to", server)
	if err := c.Connection.Connect(server); err != nil {
		return err
	}
	bus.Sub("note-gh-event-commit", c.NoteGhEvent(bus, "commit"))
	bus.Sub("note-gh-event-pullRequest", c.NoteGhEvent(bus, "pullRequest"))
	bus.Sub("note-gh-event-issue", c.NoteGhEvent(bus, "issue"))
	return nil
}
