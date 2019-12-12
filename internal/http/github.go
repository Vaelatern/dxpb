package http

import (
	"errors"
	"io"
	"log"
	"net/http"

	"github.com/spf13/viper"
	"gopkg.in/go-playground/webhooks.v5/github"

	"github.com/dxpb/dxpb/internal/bus"
)

// GithubListener creates an http listener, configured by viper, which
// acts as a webhook target for github events, sending events to the
// messagebus
func (s *server) githubListener() error {
	if viper.GetString("github.secret") == "" {
		return errors.New("Can't add a github webhook without a github secret")
	}

	var err error

	s.github, err = github.New(github.Options.Secret(viper.GetString("github.secret")))
	if err != nil {
		s.github = nil
	}
	return err // either nil, or not.
}

func (s server) handleGithubWebhook() http.HandlerFunc {
	if s.github == nil {
		log.Println("Github Hook nil! Can't serve github requests!")
		return func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(500)
			io.WriteString(w, "Github webhook inadequately initialized")
		}
	}

	return func(w http.ResponseWriter, r *http.Request) {
		payload, err := s.github.Parse(r, github.PushEvent, github.IssuesEvent, github.PullRequestEvent)
		if err != nil {
			if err == github.ErrEventNotFound {
				// ok event wasn;t one of the ones asked to be parsed
			} else {
				s.Msgbus.Pub("gh-event-parse-fail", 1)
			}
		}
		switch payload.(type) {
		case github.PushPayload:
			push := payload.(github.PushPayload)
			for _, commit := range push.Commits {
				s.Msgbus.Pub("note-gh-event-commit", bus.GhEvent{
					Who:  commit.Committer.Username,
					Hash: commit.ID,
					Msg:  commit.Message})
			}
		case github.PullRequestPayload:
			prPayload := payload.(github.PullRequestPayload)
			pr := prPayload.PullRequest
			s.Msgbus.Pub("note-gh-event-pullRequest", bus.GhEvent{
				Who:    prPayload.Sender.Login,
				Number: pr.Number,
				Msg:    pr.Title,
				Action: prPayload.Action})
		case github.IssuesPayload:
			iPayload := payload.(github.IssuesPayload)
			issue := iPayload.Issue
			s.Msgbus.Pub("note-gh-event-issue", bus.GhEvent{
				Who:    iPayload.Sender.Login,
				Number: issue.Number,
				Msg:    issue.Title,
				Action: iPayload.Action})
		}
	}
}
