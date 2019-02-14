package irc

import (
	irc "github.com/thoj/go-ircevent"
)

// Client encapsulates the complexities of interacting with arbitrary
// IRC networks.
type Client struct {
	*irc.Connection
	channel string
}
