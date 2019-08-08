package builder

import (
	"log"
	"net/http"

	// "github.com/gorilla/websocket" //Can't because it won't be a io.ReaderWriter
	//	"golang.org/x/net/websocket" // Insufficient?
	"nhooyr.io/websocket"
	"zombiezen.com/go/capnproto2/rpc"

	"github.com/dxpb/dxpb/internal/spec"
)

type server struct {
	builder builder
}

func ListenAndWork(listen string) {
	s := server{}
	http.HandleFunc("/v1/ws", s.handleWebsocketV1())

	log.Println("Now listening on " + listen)
	log.Fatal(http.ListenAndServe(listen, nil))
}

func (s *server) handleWebsocketV1() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		conn, err := websocket.Accept(w, r, websocket.AcceptOptions{
			Subprotocols: []string{"capnproto-dxpb-v1"},
		})
		if err != nil {
			log.Println(err)
			return
		}
		defer conn.Close(websocket.StatusInternalError, "the sky is falling")

		// Don't ask me why. We must use the connection before RPC gets
		// a hold of it. Otherwise something weird breaks.
		websocket.NetConn(conn).Write([]byte("Hello"))

		transport := rpc.StreamTransport(websocket.NetConn(conn))
		iface := rpc.MainInterface(spec.Builder_ServerToClient(s.builder).Client)
		rpcconn := rpc.NewConn(transport, iface)
		log.Println("Now listening to the server's instructions.")
		err = rpcconn.Wait()
		log.Println("Server connection closed, err: ", err)
	}
}
