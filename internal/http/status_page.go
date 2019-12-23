package http

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

func (s *server) statusPage() gin.HandlerFunc {
	var workers map[string]int
	s.Msgbus.Sub("state-of-workers", func(val interface{}) {
		workers = val.(map[string]int)
	})
	s.Msgbus.Pub("state-of-workers?", nil)
	return func(c *gin.Context) {
		c.HTML(http.StatusOK, "status-page.tmpl", gin.H{

			"workers": workers,
		})
	}
}
