package main

import (
	"log"
	"strings"
	"sync"
	"time"

	"github.com/thoj/go-ircevent"
	"github.com/zeromq/goczmq"
)

type ircChan struct {
	name     string
	loglevel int
}

type ircMsg struct {
	txt      string
	loglevel int
}

type ircServer struct {
	irc     *irc.Connection
	nick    string
	connstr string
	chans   []*ircChan
	pipe    <-chan *ircMsg
}

var types []string = []string{"ERROR", "NORMAL", "DEBUG", "VERBOSE", "TRACE"}

func tellchans(server *ircServer, wg sync.WaitGroup) {
	go server.irc.Loop()
	for msg := range server.pipe {
		if msg == nil {
			break
		}
		for _, channel := range server.chans {
			if channel.loglevel >= msg.loglevel {
				server.irc.Notice(channel.name, msg.txt)
				time.Sleep(2 * time.Second)
			}
		}
	}
	server.irc.Disconnect()
	wg.Done()
}

func initIRC(servers []*ircServer, done chan<- int) ([]*ircServer, []chan<- *ircMsg) {
	var pipe chan *ircMsg
	var retPipes []chan<- *ircMsg
	var wg sync.WaitGroup
	wg.Add(len(servers))
	for _, server := range servers {
		server.irc = irc.IRC(server.nick, "Lobotomy")
		server.irc.UseTLS = true
		server.irc.Connect(server.connstr)
		for _, curchan := range server.chans {
			server.irc.Join(curchan.name)
			if curchan.loglevel > 2 {
				server.irc.Notice(curchan.name, "Bot ready")
			}
			log.Printf("Bot attached to channel %s\n", curchan.name)
		}
		pipe = make(chan *ircMsg, 64)
		server.pipe = pipe
		retPipes = append(retPipes, pipe)
		go tellchans(server, wg)
	}
	go func() {
		wg.Wait()
		done <- 1
	}()
	return servers, retPipes
}

func init0mq(endpoints []string) map[string]*goczmq.Channeler {
	socks := map[string]*goczmq.Channeler{}
	endpointstr := strings.Join(endpoints, ",")
	log.Printf("Connecting to endpoints: %s\n", endpointstr)
	for _, mode := range types {
		log.Printf("Adding subscriber for mode: %s\n", mode)
		sock := goczmq.NewSubChanneler(endpointstr, mode)
		socks[mode] = sock
	}
	return socks
}

func prepMsgTxt(prefix byte, in [][]byte) []string {
	var ret []string = make([]string, len(in)+1)
	ret[0] = string(prefix)
	for i, bytes := range in {
		ret[i+1] = string(bytes)
	}
	return ret
}

func sockToPipes(sock *goczmq.Channeler, pipes []chan<- *ircMsg, loglevel int) {
	defer sock.Destroy()
	for {
		msg := <-sock.RecvChan
		out := new(ircMsg)
		out.loglevel = loglevel
		switch len(msg) {
		case 0:
			out.txt = "Got channel error"
		case 1:
			out.txt = string(msg[0])
		default:
			out.txt = strings.Join(prepMsgTxt(msg[0][0], msg[1:]), ": ")
		}
		for _, pipe := range pipes {
			select {
			case pipe <- out:
			default:
			}
		}
	}
	log.Printf("Done reading socks for loglevel %d\n", loglevel)
}

func startSocks(socks map[string]*goczmq.Channeler, pipes []chan<- *ircMsg) {
	for loglevel, mode := range types {
		go sockToPipes(socks[mode], pipes, loglevel)
	}
}

func defaultServers() []*ircServer {
	var retVal []*ircServer
	var chn *ircChan = new(ircChan)
	var srvr *ircServer = new(ircServer)
	srvr.nick = "dxpb"
	srvr.connstr = "irc.freenode.net:6697"
	chn = new(ircChan)
	chn.name = "Vaelatern"
	chn.loglevel = 5
	srvr.chans = append(srvr.chans, chn)
	chn = new(ircChan)
	chn.name = "#dxpb"
	chn.loglevel = 5
	srvr.chans = append(srvr.chans, chn)
	retVal = append(retVal, srvr)
	return retVal
}

func main() {
	ch := make(chan int)
	log.Println("Preparing default servers")
	var servers []*ircServer = defaultServers()
	endpoints := []string{"ipc:///var/run/dxpb/log-dxpb-frontend.sock",
		"ipc:///var/run/dxpb/log-dxpb-grapher.sock",
		"ipc:///var/run/dxpb/log-dxpb-hostdir-master-graph.sock",
		"ipc:///var/run/dxpb/log-dxpb-hostdir-master-files.sock",
		"ipc:///var/run/dxpb/log-pkgimport-master.sock"}
	log.Println("Preparing IRC")
	servers, pipes := initIRC(servers, ch)
	log.Println("Preparing 0mq")
	socks := init0mq(endpoints)
	log.Println("Starting socket work")
	startSocks(socks, pipes)
	<-ch
	<-ch
}
