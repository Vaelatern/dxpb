package http

import (
	"context"
	"errors"
	"net/http"
	"sync"

	"github.com/spf13/viper"
	"gopkg.in/go-playground/webhooks.v5/github"
	"zombiezen.com/go/capnproto2/rpc"

	"github.com/dxpb/dxpb/internal/spec"
)

// GithubListener creates an http listener, configured by viper, which
// acts as a webhook target for github events, sending events to the
// out RPC connection
func (s server) githubListener() error {
	if viper.GetString("github.secret") == "" {
		return errors.New("Can't add a github webhook without a github secret")
	}

	var err error

	s.github, err = github.New(github.Options.Secret(viper.GetString("github.secret")))

	return err
}

func (s server) handleGithubWebhook() http.HandlerFunc {
	var (
		init sync.Once
		out  spec.IrcBot
		ctx  context.Context
	)
	return func(w http.ResponseWriter, r *http.Request) {
		init.Do(func() {
			ctx = context.Background()
			conn := rpc.NewConn(rpc.StreamTransport(s.toIRC))
			out = spec.IrcBot{Client: conn.Bootstrap(ctx)}
		})
		payload, err := s.github.Parse(r, github.PushEvent, github.IssuesEvent, github.PullRequestEvent)
		if err != nil {
			if err == github.ErrEventNotFound {
				// ok event wasn;t one of the ones asked to be parsed
			}
		}
		switch payload.(type) {
		case github.PushPayload:
			push := payload.(github.PushPayload)
			for _, commit := range push.Commits {
				out.NoteGhEvent(ctx, func(p spec.IrcBot_noteGhEvent_Params) error {
					gh, err := p.NewGh()
					if err != nil {
						return err
					}
					gh.SetWho(commit.Committer.Username)
					gh.SetCommit()
					asCommit := gh.Commit()
					asCommit.SetHash(commit.ID)
					asCommit.SetMsg(commit.Message)
					return nil
				})
			}
		case github.PullRequestPayload:
			prPayload := payload.(github.PullRequestPayload)
			pr := prPayload.PullRequest
			out.NoteGhEvent(ctx, func(p spec.IrcBot_noteGhEvent_Params) error {
				gh, err := p.NewGh()
				if err != nil {
					return err
				}
				gh.SetWho(prPayload.Sender.Login)
				gh.SetPullRequest()
				asPr := gh.PullRequest()
				asPr.SetNumber(pr.Number)
				asPr.SetMsg(pr.Title)
				asPr.SetAction(prPayload.Action)
				return nil
			})
		case github.IssuesPayload:
			iPayload := payload.(github.IssuesPayload)
			issue := iPayload.Issue
			out.NoteGhEvent(ctx, func(p spec.IrcBot_noteGhEvent_Params) error {
				gh, err := p.NewGh()
				if err != nil {
					return err
				}
				gh.SetWho(iPayload.Sender.Login)
				gh.SetIssue()
				asIssue := gh.Issue()
				asIssue.SetNumber(issue.Number)
				asIssue.SetMsg(issue.Title)
				asIssue.SetAction(iPayload.Action)
				return nil
			})
		}
	}
}
