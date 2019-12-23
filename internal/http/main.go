package http

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/bus"
	"github.com/dxpb/dxpb/internal/watcher"
)

// New returns a ready-to-serve httpd
// By default, the routes are as follows:
// For the github webhook: /webhook
// For the prometheus endpoint: /metrics
func New() (*server, error) {
	viper.SetDefault("http.bind", ":8080")
	viper.SetDefault("http.github", "/webhook")
	viper.SetDefault("http.prometheus", "/metrics")

	rV := server{}
	rV.router = gin.Default()
	rV.Msgbus = bus.New()
	watcher.Start(rV.Msgbus, viper.GetStringMapString("drones"))
	err := rV.githubListener()
	if err != nil {
		return nil, err
	}
	rV.routes()
	if !viper.GetBool("debug") {
		gin.SetMode(gin.ReleaseMode)
	} else {
		gin.SetMode(gin.DebugMode)
	}
	return &rV, nil
}

// Start blocks until the httpd stops. It takes only the viper string
// "http.bind" to decide where to listen. By default, http.bind is set to ":8080"
func (s *server) Start() error {
	return http.ListenAndServe(viper.GetString("http.bind"), s.router)
}
