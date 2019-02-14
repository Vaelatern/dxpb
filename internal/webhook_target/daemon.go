package webhook_target

import (
	"log"
	"net/http"

	"github.com/spf13/viper"
	"gopkg.in/go-playground/webhooks.v5/github"
)

// EventType declares the type of an Event
type EventType int

// Constant enumeration of EventType values
const (
	Commit EventType = iota
	PullRequest
	Issue
)

// Event type is sent to describe github events recieved as webhooks
type Event struct {
	Type      EventType // Type of the event
	Committer string    // who created the event
	Hash      string    // hash of commit (when applicable)
	Msg       string    // message associated with event
	Number    int64     // Number (when applicable)
}

// GithubListener creates an http listener, configured by viper, which
// acts as a webhook target for github events, sending events to the
// out Event channel
func GithubListener(out chan Event) {
	if viper.GetString("githubhook.secret") == "" {
		log.Panic("Githubhook secret is empty")
	}

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
				send := Event{
					Type:      Commit,
					Committer: commit.Committer.Username,
					Hash:      commit.ID[:12],
					Msg:       commit.Message,
				}
				out <- send
			}
		case github.PullRequestPayload:
			prPayload := payload.(github.PullRequestPayload)
			if prPayload.Action == "opened" {
				pr := prPayload.PullRequest
				send := Event{
					Type:      PullRequest,
					Committer: pr.User.GravatarID,
					Number:    pr.Number,
					Msg:       pr.Title,
				}
				out <- send
			}
		case github.IssuesPayload:
			iPayload := payload.(github.IssuesPayload)
			if iPayload.Action == "opened" {
				issue := iPayload.Issue
				send := Event{
					Type:      Issue,
					Committer: issue.User.GravatarID,
					Number:    issue.Number,
					Msg:       issue.Title,
				}
				out <- send
			}
		}
	}
}
