#!/bin/python3

import irc.client
import sys
import zmq
from threading import Thread
from queue import Queue

class QCat(irc.client.SimpleIRCClient):
    def __init__(self, q, target):
        irc.client.SimpleIRCClient.__init__(self)
        self.target = target
        self.queue = q
    def on_welcome(self, connection, event):
        if irc.client.is_channel(self.target):
            connection.join(self.target)
        else:
            self.queue_spewer()
    def on_join(self, connection, event):
        self.queue_spewer()
    def on_disconnect(self, connection, event):
        sys.exit(0)
    def queue_spewer(self):
        while True:
            data = self.queue.get()
            if data == None:
                break
            line = ': '.join([a.decode('utf-8') for a in data])
            self.connection.privmsg(self.target, line)
        self.connection.quit("Using dxpb.irc.pybot")

def start_client(conn):
    c = QCat(conn["q"], conn["chan"])
    try:
        c.connect(conn["server"], conn["port"], conn["nick"])
    except irc.client.ServerConnectionError as x:
        print(x)
        sys.exit(1)
    c.start()

def procmsg(socket, target=[]):
    while True:
        data = socket.recv_multipart()
        if data:
            for q in target:
                q.put(data)

def zmq_listener(endpoints=[], listeners=[]):
    ctx = zmq.Context()
    subsocks = {}
    desires = {}
    subthreads = []
    levels = ["error", "normal", "debug", "verbose", "trace"]
    for desc in levels:
        subsocks[desc] = ctx.socket(zmq.SUB)
        desires[desc] = []
    for url in endpoints:
        for desc in levels:
            # each socket connects to multiple endpoints, or at least can.
            subsocks[desc].connect(url)
    for desc in levels:
        subsocks[desc].subscribe(desc.encode('utf-8').upper())
    for q, maxlevel in listeners:
        for i in range(0, maxlevel):
            desires[levels[i]].append(q)
    for desc in levels:
        newt = Thread(target=procmsg, args=(subsocks[desc], desires[desc]))
        subthreads.append(newt)
    for thread in subthreads:
        thread.start()
    for thread in subthreads:
        thread.join()


def start(config):
    threads = []
    pollster = Thread(target=zmq_listener,
            kwargs={"endpoints": config["endpoints"], "listeners": config["listeners"]})
    threads.append(pollster)
    for conn in config["servers"]:
        threads.append(Thread(target=start_client, args=(conn,)))
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()


def parseargs(argv):
    if "-h" in argv or len(argv) < 5 or "--" not in argv or argv.index("--") < 3 or argv.index("--") >= (len(argv) - 1):
        print("Usage: {} <nick> <chan>@<server[:port]>[+<loglevel>] -- <endpoint>".format(argv[0]))
        print("\t <chan> is either #channel or nick")
        print("\t <port> is numeric, and defaults to 6667")
        print("\t <endpoint> is a zeromq socket endpoint to which we SUBSCRIBE")
        print("\t <loglevel> is prefixed with a + and is numeric, in the range 1 - 5")
        print("\t\t Default is 2, 'Normal', and larger numbers are more verbose")
        print("\t channel@server:port+loglevel may be specified multiple times, space separated")
        print("\t endpoint may also be specified multiple times, space separated")
        sys.exit(1)

    nick = argv[1]
    servers = []
    endpoints = []
    for index in range(argv.index("--") + 1, len(argv)):
        endpoints.append(argv[index])
    for index in range(2, argv.index("--")):
        conn = {"nick": nick}
        tmp = argv[index].split("@")
        conn["chan"] = tmp[0]
        tmp = tmp[1].split("+")
        if len(tmp) == 2:
            try:
                conn["loglevel"] = int(tmp[1])
            except ValueError:
                print("Error: Erroneous loglevel, not integer.")
                sys.exit(1)
            if conn["loglevel"] < 1 or conn["loglevel"] > 5:
                print("Error: Erroneous loglevel, range 1-5 inclusive allowed.")
                sys.exit(1)
        elif len(tmp) == 1:
            conn["loglevel"] = 2
        else:
            print("Too many + in server connection string")
            sys.exit(1)
        tmp = tmp[0].split(":")
        if len(tmp) == 1:
            conn["port"] = 6667
        elif len(tmp) == 2:
            try:
                conn["port"] = int(tmp[1])
            except ValueError:
                print("Error: Erroneous port, not integer.")
                sys.exit(1)
            if conn["port"] < 0 or conn["port"] > 65535:
                print("Error: Erroneous port, must be in range 0-65535.")
                sys.exit(1)
        else:
            print("Too many : in server connection string")
            sys.exit(1)
        conn["server"] = tmp[0]
        servers.append(conn)
    return (servers, endpoints)

def main(argv):
    servers, endpoints = parseargs(argv)
    for server in servers:
        server["q"] = Queue()
    config = {}
    config["servers"] = servers
    config["endpoints"] = endpoints
    config["listeners"] = [[a["q"], a["loglevel"]] for a in servers]
    start(config)

if __name__ == "__main__":
    main(sys.argv)
