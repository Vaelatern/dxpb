package irc

import (
	"errors"
)

var (
	// ErrInitialization is returned during early initialization
	// if the connection to the remote IRCd fails, or if required
	// information is not available from the configuration.
	ErrInitialization = errors.New("An error occured during initialization")
)
