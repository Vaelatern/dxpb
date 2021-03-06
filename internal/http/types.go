package http

import (
	"net"

	"github.com/gin-gonic/gin"
	"gopkg.in/go-playground/webhooks.v5/github"

	"github.com/dxpb/dxpb/internal/bus"
)

// server encapsulates everything available to the httpd
type server struct {
	router *gin.Engine
	github *github.Webhook
	toIRC  net.Conn
	Msgbus *bus.Bus
}
