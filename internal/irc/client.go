package irc

import (
	"log"
	"net"

	"github.com/spf13/viper"
	irc "github.com/thoj/go-ircevent"
	"zombiezen.com/go/capnproto2/rpc"

	"github.com/dxpb/dxpb/internal/spec"
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
func (c *Client) Connect(in net.Conn) error {
	server := viper.GetString("irc.server")
	log.Println("Attempting to connect to", server)
	if err := c.Connection.Connect(server); err != nil {
		return err
	}
	conn := rpc.NewConn(rpc.StreamTransport(in), rpc.MainInterface(spec.IrcBot_ServerToClient(c).Client))
	c.Loop()
	return conn.Wait() // Should not ever get here, but if it does we should see this error.
}

// NoticeEvent sends a formatted message to the connected channel
// that includes a summary of the github event.
func (c *Client) NoteGhEvent(call spec.IrcBot_noteGhEvent) error {
	event, err := call.Params.Gh()
	if err != nil {
		return err
	}

	switch event.Which() {
	case spec.GithubEvent_Which_commit:
		thing := event.Commit()
		committer, err := event.Who()
		if err != nil {
			return err
		}
		hash, err := thing.Hash()
		if err != nil {
			return err
		}
		msg, err := thing.Msg()
		if err != nil {
			return err
		}
		c.Noticef(c.channel, "%s committed %s: %s", committer, hash, msg)
	case spec.GithubEvent_Which_pullRequest:
		thing := event.PullRequest()
		committer, err := event.Who()
		if err != nil {
			return err
		}
		action, err := thing.Action()
		if err != nil {
			return err
		}
		msg, err := thing.Msg()
		if err != nil {
			return err
		}
		c.Noticef(c.channel, "%s %s PR %d: %s", committer, action, thing.Number(), msg)
	case spec.GithubEvent_Which_issue:
		thing := event.Issue()
		committer, err := event.Who()
		if err != nil {
			return err
		}
		action, err := thing.Action()
		if err != nil {
			return err
		}
		msg, err := thing.Msg()
		if err != nil {
			return err
		}
		c.Noticef(c.channel, "%s %s Issue #%d: %s", committer, action, thing.Number(), msg)
	}
	return nil
}
