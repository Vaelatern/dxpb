package webhook_target

import (
	"log"
	"net/http"

	"gopkg.in/go-playground/webhooks.v5/github"
	"github.com/spf13/viper"
)

type Event struct {
	Committer string
	Hash string
	Msg string
}

func GithubListener(out chan Event) {
	hook, err := github.New(github.Options.Secret(viper.GetString("githubhook.secret")))

	if err != nil {
		log.Panic("Couldn't initiate github structure")
	}

	log.Printf("Beginning Github listener, listening on %s\n", viper.GetString("githubhook.bind"))
	http.HandleFunc(viper.GetString("githubhook.path"), handleAll(hook, out))
	log.Fatal(http.ListenAndServe(viper.GetString("githubhook.bind"), nil))
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
