package main

import (
	"log"

	"github.com/dxpb/dxpb/internal/app/webhook_target"
)

func main() {
	log.Println("hello!")
	gitBind := ":8080"
	gitEvent := make(chan webhook_target.Event)
	go webhook_target.GithubListener(gitEvent, gitBind)
}
