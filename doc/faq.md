# FAQ

#### What git remote will dxpb use when manipulating an existing checkout?

The reference `refs/remotes/dxpb-remote/master` is used. The remote is known as
`dxpb-remote`. The use of the master branch is hardcoded in one line of code
and one comment. `dxpb-remote` is hardcoded in two lines of code and two
comments and now in this document.

#### How do I specify socket endpoints?

We use zeromq for our connectivity. Zeromq defines a number of transports, but
we do not support datagram transports, only stream transports. Thus, endpoints
follow the following formulae:

```
ipc://$PATH
tcp://$HOST:$PORT
```
