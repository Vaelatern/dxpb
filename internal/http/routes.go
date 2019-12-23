package http

import (
	"github.com/gin-gonic/gin"
	"github.com/prometheus/client_golang/prometheus/promhttp"
	"github.com/spf13/viper"
)

func (s *server) routes() {
	s.router.LoadHTMLGlob("web/templates/*")
	s.router.GET("/", s.statusPage())
	s.router.POST(viper.GetString("http.github"), gin.WrapF(s.handleGithubWebhook()))
	s.router.GET(viper.GetString("http.prometheus"), gin.WrapH(promhttp.Handler()))
}
