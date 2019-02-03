package webhook_target

import (
	"log"
	"net/http"
)

type Event struct {
}

func GithubListener(outchan chan Event, bindto string) {
	log.Printf("Beginning Github listener, listening on %s\n", bindto)
	http.HandleFunc("/", handleAll(outchan))
	log.Fatal(http.ListenAndServe(bindto, nil))
}

func handleAll(out chan Event) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		out<-Event{}
	}
}
