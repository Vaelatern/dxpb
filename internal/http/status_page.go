package http

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

func statusPage(c *gin.Context) {
	c.HTML(http.StatusOK, "status-page.tmpl", gin.H{
		"n": "5",
	})
}
