package webhook_target

import (
	"log"
	"net/http"

	"gopkg.in/go-playground/webhooks.v5/github"
)

type Event struct {
	Committer string
	Hash string
	Msg string
}

func GithubListener(out chan Event, bindto string) {
	path := "/github"
	secret := "FF"

	hook, err := github.New(github.Options.Secret(secret))

	if err != nil {
		log.Fatal("Couldn't initiate github structure")
	}

	log.Printf("Beginning Github listener, listening on %s\n", bindto)
	http.HandleFunc(path, handleAll(hook, out))
	log.Fatal(http.ListenAndServe(bindto, nil))
}

func handleAll(hook *github.Webhook, out chan Event) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		payload, err := hook.Parse(r, github.PushEvent)
		if err != nil {
			if err == github.ErrEventNotFound {
				// ok event wasn;t one of the ones asked to be parsed
			}
		}
		switch payload.(type) {
		case github.PushPayload:
			push := payload.(github.PushPayload)
			for _, commit := range push.Commits {
				send := Event {
					Committer: commit.Committer.Username,
					Hash: commit.ID[:12],
					Msg: commit.Message,
				}
				out<-send
			}
		}
	}
}
