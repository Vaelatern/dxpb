/*  =========================================================================
    pkgimport_client_engine - Package Importer engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_client.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    pleasantries_state = 2,
    ready_state = 3,
    finished_reading_now_what_state = 4,
    die_state = 5,
    defaults_state = 6
} state_t;

typedef enum {
    NULL_event = 0,
    set_xbps_src_path_event = 1,
    construct_event = 2,
    roger_event = 3,
    connect_error_event = 4,
    plzread_event = 5,
    ready_event = 6,
    no_pkg_found_event = 7,
    move_on_event = 8,
    invalid_event = 9,
    iforgotu_event = 10,
    reset_event = 11,
    stop_event = 12,
    kthnksbye_event = 13,
    expired_event = 14
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "pleasantries",
    "ready",
    "finished reading now what",
    "die",
    "defaults"
};

static char *
s_event_name [] = {
    "(NONE)",
    "set_xbps_src_path",
    "construct",
    "ROGER",
    "connect_error",
    "PLZREAD",
    "READY",
    "no_pkg_found",
    "move_on",
    "INVALID",
    "IFORGOTU",
    "RESET",
    "STOP",
    "KTHNKSBYE",
    "expired"
};


//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

//  These are the different method arguments we manage automatically
struct _client_args_t {
    char *endpoint;
    char *xbps_src_path;
};

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *cmdpipe;           //  Get/send commands from caller API
    zsock_t *msgpipe;           //  Get/send messages from caller API
    zsock_t *provided_pipe;     //  Do whatever the caller defines
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
    pkgimport_msg_t *message;   //  Message received or sent
    client_args_t args;         //  Method arguments structure
    bool verbose;               //  True if we're tracing state machine
    bool connected;             //  True if client is connected
    bool terminated;            //  True if client is shutdown
    bool fsm_stopped;           //  "terminate" action called
    size_t expiry;              //  Expiry timer, msecs
    size_t heartbeat;           //  Heartbeat timer, msecs
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int expiry_timer;           //  zloop timer for expiry
    int wakeup_timer;           //  zloop timer for alarms
    int heartbeat_timer;        //  zloop timer for heartbeat
    event_t wakeup_event;       //  Wake up with this event
    char log_prefix [41];       //  Log prefix string
} s_client_t;

static int
    client_initialize (client_t *self);
static void
    client_terminate (client_t *self);
static void
    s_client_destroy (s_client_t **self_p);
static void
    s_client_execute (s_client_t *self, event_t event);
static int
    s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument);
static int
    s_client_handle_expiry (zloop_t *loop, int timer_id, void *argument);
static void
    s_satisfy_pedantic_compilers (void);
static void
    set_xbps_src_path (client_t *self);
static void
    connect_to_server (client_t *self);
static void
    complain_about_connection_error (client_t *self);
static void
    prepare_pkg_reading (client_t *self);
static void
    prepare_next_pkg (client_t *self);
static void
    cease_all_operations (client_t *self);

//  Create a new client connection

static s_client_t *
s_client_new (zsock_t *cmdpipe, zsock_t *msgpipe, zsock_t *provided_pipe)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    if (self) {
        assert ((s_client_t *) &self->client == self);
        self->cmdpipe = cmdpipe;
        self->msgpipe = msgpipe;
        self->provided_pipe = provided_pipe;
        self->state = start_state;
        self->event = NULL_event;
        snprintf (self->log_prefix, sizeof (self->log_prefix),
            "%6d:%-33s", randof (1000000), "pkgimport_client");
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->message = pkgimport_msg_new ();
        if (self->message)
            self->loop = zloop_new ();
        if (self->loop) {
            //  Give application chance to initialize and set next event
            self->client.cmdpipe = self->cmdpipe;
            self->client.msgpipe = self->msgpipe;
            self->client.provided_pipe = self->provided_pipe;
            self->client.dealer = self->dealer;
            self->client.message = self->message;
            self->client.args = &self->args;
            if (client_initialize (&self->client))
                s_client_destroy (&self);
        }
        else
            s_client_destroy (&self);
    }
    s_satisfy_pedantic_compilers ();
    return self;
}

//  Destroy the client connection

static void
s_client_destroy (s_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_client_t *self = *self_p;
        zstr_free (&self->args.endpoint);
        zstr_free (&self->args.xbps_src_path);
        client_terminate (&self->client);
        pkgimport_msg_destroy (&self->message);
        zsock_destroy (&self->msgpipe);
        zsock_destroy (&self->dealer);
        zloop_destroy (&self->loop);
        free (self);
        *self_p = NULL;
    }
}

//  ---------------------------------------------------------------------------
//  These methods are an internal API for actions

//  Set the next event, needed in at least one action in an internal
//  state; otherwise the state machine will wait for a message on the
//  dealer socket and treat that as the event.

static void
engine_set_next_event (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->next_event = event;
    }
}

//  Raise an exception with 'event', halting any actions in progress.
//  Continues execution of actions defined for the exception event.

static void
engine_set_exception (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->exception = event;
    }
}

//  Set wakeup alarm after 'delay' msecs. The next state should handle the
//  wakeup event. The alarm is cancelled on any other event.

static void
engine_set_wakeup_event (client_t *client, size_t delay, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (self->wakeup_timer) {
            zloop_timer_end (self->loop, self->wakeup_timer);
            self->wakeup_timer = 0;
        }
        self->wakeup_timer = zloop_timer (
            self->loop, delay, 1, s_client_handle_wakeup, self);
        self->wakeup_event = event;
    }
}

//  Set a heartbeat timer. The interval is in msecs and must be
//  non-zero. The state machine must handle the "heartbeat" event.
//  The heartbeat happens every interval no matter what traffic the
//  client is sending or receiving.

static void
engine_set_heartbeat (client_t *client, size_t heartbeat)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->heartbeat = heartbeat;
    }
}


//  Set expiry timer. Setting a non-zero expiry causes the state machine
//  to receive an "expired" event if there is no incoming traffic for that many
//  milliseconds. This cycles over and over until/unless the code sets a
//  zero expiry. The state machine must handle the "expired" event.

//  Macro to support deprecated name: remove after 2016-07-31
#define engine_set_timeout engine_set_expiry

static void
engine_set_expiry (client_t *client, size_t expiry)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->expiry = expiry;
        if (self->expiry_timer) {
            zloop_timer_end (self->loop, self->expiry_timer);
            self->expiry_timer = 0;
        }
        if (self->expiry)
            self->expiry_timer = zloop_timer (
                self->loop, self->expiry, 1, s_client_handle_expiry, self);
    }
}

//  Poll socket for activity, invoke handler on any received message.
//  Handler must be a CZMQ zloop_fn function; receives client as arg.

static void
engine_handle_socket (client_t *client, zsock_t *sock, zloop_reader_fn handler)
{
    if (client && sock) {
        s_client_t *self = (s_client_t *) client;
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, sock, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, sock);
        }
        else
            zloop_reader_end (self->loop, sock);
    }
}

//  Set connected to true/false. The client must call this if it wants
//  to provide the API with the connected status.

static void
engine_set_connected (client_t *client, bool connected)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->connected = connected;
    }
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_heartbeat (NULL, 0);
    engine_set_expiry (NULL, 0);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
    engine_set_connected (NULL, 0);
}


//  ---------------------------------------------------------------------------
//  Generic methods on protocol messages
//  TODO: replace with lookup table, since ID is one byte

static event_t
s_protocol_event (s_client_t *self, pkgimport_msg_t *message)
{
    assert (message);
    switch (pkgimport_msg_id (message)) {
        case PKGIMPORT_MSG_ROGER:
            return roger_event;
            break;
        case PKGIMPORT_MSG_IFORGOTU:
            return iforgotu_event;
            break;
        case PKGIMPORT_MSG_READY:
            return ready_event;
            break;
        case PKGIMPORT_MSG_RESET:
            return reset_event;
            break;
        case PKGIMPORT_MSG_INVALID:
            return invalid_event;
            break;
        case PKGIMPORT_MSG_KTHNKSBYE:
            return kthnksbye_event;
            break;
        case PKGIMPORT_MSG_STOP:
            return stop_event;
            break;
        case PKGIMPORT_MSG_PLZREAD:
            return plzread_event;
            break;
        default:
            zsys_error ("%s: unknown command %s, halting",
                        self->log_prefix, pkgimport_msg_command (message));
            self->terminated = true;
            return NULL_event;
    }
}


//  Execute state machine as long as we have events; if event is NULL_event,
//  or state machine is stopped, do nothing.

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup_timer) {
        zloop_timer_end (self->loop, self->wakeup_timer);
        self->wakeup_timer = 0;
    }
    while (!self->terminated                    //  Actor is dying
        && !self->fsm_stopped                   //  FSM has finished
        && self->next_event != NULL_event) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (self->verbose) {
            zsys_debug ("%s: %s:",
                self->log_prefix, s_state_name [self->state]);
            zsys_debug ("%s:     %s",
                self->log_prefix, s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == set_xbps_src_path_event) {
                    if (!self->exception) {
                        //  set xbps_src path
                        if (self->verbose)
                            zsys_debug ("%s:         $ set xbps_src path", self->log_prefix);
                        set_xbps_src_path (&self->client);
                    }
                }
                else
                if (self->event == construct_event) {
                    if (!self->exception) {
                        //  connect to server
                        if (self->verbose)
                            zsys_debug ("%s:         $ connect to server", self->log_prefix);
                        connect_to_server (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == roger_event) {
                    if (!self->exception)
                        self->state = pleasantries_state;
                    if (!self->exception) {
                        //  send RDY2WRK
                        if (self->verbose)
                            zsys_debug ("%s:         $ send RDY2WRK",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_RDY2WRK);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == connect_error_event) {
                    if (!self->exception) {
                        //  complain about connection error
                        if (self->verbose)
                            zsys_debug ("%s:         $ complain about connection error", self->log_prefix);
                        complain_about_connection_error (&self->client);
                    }
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                break;

            case pleasantries_state:
                if (self->event == roger_event) {
                    if (!self->exception)
                        self->state = ready_state;
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == reset_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == stop_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  send PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PING",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                break;

            case ready_state:
                if (self->event == plzread_event) {
                    if (!self->exception) {
                        //  prepare pkg reading
                        if (self->verbose)
                            zsys_debug ("%s:         $ prepare pkg reading", self->log_prefix);
                        prepare_pkg_reading (&self->client);
                    }
                    if (!self->exception) {
                        //  send RUREADY
                        if (self->verbose)
                            zsys_debug ("%s:         $ send RUREADY",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_RUREADY);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == ready_event) {
                    if (!self->exception) {
                        //  prepare next pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ prepare next pkg", self->log_prefix);
                        prepare_next_pkg (&self->client);
                    }
                    if (!self->exception) {
                        //  send PKGINFO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PKGINFO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_PKGINFO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == no_pkg_found_event) {
                    if (!self->exception) {
                        //  send PKGDEL
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PKGDEL",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_PKGDEL);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  send DIDREAD
                        if (self->verbose)
                            zsys_debug ("%s:         $ send DIDREAD",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_DIDREAD);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = finished_reading_now_what_state;
                }
                else
                if (self->event == move_on_event) {
                    if (!self->exception) {
                        //  send DIDREAD
                        if (self->verbose)
                            zsys_debug ("%s:         $ send DIDREAD",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_DIDREAD);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = finished_reading_now_what_state;
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  send DIDREAD
                        if (self->verbose)
                            zsys_debug ("%s:         $ send DIDREAD",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_DIDREAD);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  send DIDREAD
                        if (self->verbose)
                            zsys_debug ("%s:         $ send DIDREAD",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_DIDREAD);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == reset_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  send DIDREAD
                        if (self->verbose)
                            zsys_debug ("%s:         $ send DIDREAD",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_DIDREAD);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == stop_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception) {
                        //  send DIDREAD
                        if (self->verbose)
                            zsys_debug ("%s:         $ send DIDREAD",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_DIDREAD);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  send PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PING",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  send DIDREAD
                        if (self->verbose)
                            zsys_debug ("%s:         $ send DIDREAD",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_DIDREAD);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                break;

            case finished_reading_now_what_state:
                if (self->event == roger_event) {
                    if (!self->exception)
                        self->state = pleasantries_state;
                    if (!self->exception) {
                        //  send RDY2WRK
                        if (self->verbose)
                            zsys_debug ("%s:         $ send RDY2WRK",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_RDY2WRK);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == reset_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == stop_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  send PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PING",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                break;

            case die_state:
                {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send INVALID
                        if (self->verbose)
                            zsys_debug ("%s:         $ send INVALID",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_INVALID);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                break;

            case defaults_state:
                if (self->event == invalid_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == reset_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == stop_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  send PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PING",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (self->verbose)
                zsys_debug ("%s:         ! %s",
                    self->log_prefix, s_event_name [self->exception]);
            self->next_event = self->exception;
        }
        else
        if (self->verbose)
            zsys_debug ("%s:         > %s",
                    self->log_prefix, s_state_name [self->state]);
    }
}

//  zloop callback when client expiry timeout expires

static int
s_client_handle_expiry (zloop_t *loop, int timer_id, void *argument)
{
    ZPROTO_UNUSED(timer_id);
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, expired_event);
    if (self->terminated)
        return -1;

    if (self->expiry > 0)
        self->expiry_timer = zloop_timer (
            loop, self->expiry, 1, s_client_handle_expiry, self);
    return 0;
}

//  zloop callback when client wakeup timer expires

static int
s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(timer_id);
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, self->wakeup_event);
    return 0;
}


//  Handle command pipe to/from calling API

static int
s_client_handle_cmdpipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(reader);
    s_client_t *self = (s_client_t *) argument;
    char *method = zstr_recv (self->cmdpipe);
    if (!method)
        return -1;                  //  Interrupted; exit zloop
    if (self->verbose)
        zsys_debug ("%s:     API command=%s", self->log_prefix, method);

    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "$CONNECTED"))
        zsock_send (self->cmdpipe, "i", self->connected);
    else
    if (streq (method, "SET VERBOSE"))
        zsock_recv (self->cmdpipe, "i", &self->verbose);
    else
    if (streq (method, "CONSTRUCT")) {
        zstr_free (&self->args.endpoint);
        zsock_recv (self->cmdpipe, "s", &self->args.endpoint);
        s_client_execute (self, construct_event);
    }
    else
    if (streq (method, "SET XBPS_SRC PATH")) {
        zstr_free (&self->args.xbps_src_path);
        zsock_recv (self->cmdpipe, "s", &self->args.xbps_src_path);
        s_client_execute (self, set_xbps_src_path_event);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->cmdpipe)) {
        zsys_error ("%s: trailing API command frames (%s)",
            self->log_prefix, method);
        zmsg_t *more = zmsg_recv (self->cmdpipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}


//  Handle message pipe to/from calling API

static int
s_client_handle_msgpipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(reader);
    s_client_t *self = (s_client_t *) argument;

    //  We will process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->msgpipe) & ZMQ_POLLIN) {
        char *method = zstr_recv (self->msgpipe);
        if (!method)
            return -1;              //  Interrupted; exit zloop
        if (self->verbose)
            zsys_debug ("%s:     API message=%s", self->log_prefix, method);

        //  Front-end shuts down msgpipe before cmdpipe, this little
        //  handshake just ensures all traffic on the msgpipe has been
        //  flushed before the calling thread continues with destroying
        //  the actor.
        if (streq (method, "$FLUSH"))
            zsock_signal (self->cmdpipe, 0);
        //  Cleanup pipe if any argument frames are still waiting to be eaten
        if (zsock_rcvmore (self->msgpipe)) {
            zsys_error ("%s: trailing API message frames (%s)", self->log_prefix, method);
            zmsg_t *more = zmsg_recv (self->msgpipe);
            zmsg_print (more);
            zmsg_destroy (&more);
        }
        zstr_free (&method);
    }
    return 0;
}


//  Handle a message (a protocol reply) from the server

static int
s_client_handle_protocol (zloop_t *loop, zsock_t *reader, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(reader);
    s_client_t *self = (s_client_t *) argument;

    //  We will process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->dealer) & ZMQ_POLLIN) {
        if (pkgimport_msg_recv (self->message, self->dealer))
            return -1;              //  Interrupted; exit zloop

        zsys_debug ("Server message");
        pkgimport_msg_print (self->message);
        //  Any input from server counts as activity
        if (self->expiry_timer) {
            zloop_timer_end (self->loop, self->expiry_timer);
            self->expiry_timer = 0;
        }
        //  Reset expiry timer if expiry timeout not zero
        if (self->expiry)
            self->expiry_timer = zloop_timer (
                self->loop, self->expiry, 1, s_client_handle_expiry, self);
        s_client_execute (self, s_protocol_event (self, self->message));
        if (self->terminated)
            return -1;
    }
    return 0;
}


//  ---------------------------------------------------------------------------
//  This is the client actor, which polls its two sockets and processes
//  incoming messages

void
pkgimport_client (zsock_t *cmdpipe, void *piperays)
{
    //  Initialize
    zsock_t **pipes = piperays;
    s_client_t *self = s_client_new (cmdpipe, pipes[0], pipes[1]);
    if (self) {
        zsock_signal (cmdpipe, 0);

        //  Set up handler for the sockets the client uses
        engine_handle_socket ((client_t *) self, self->cmdpipe, s_client_handle_cmdpipe);
        engine_handle_socket ((client_t *) self, self->msgpipe, s_client_handle_msgpipe);
        engine_handle_socket ((client_t *) self, self->dealer, s_client_handle_protocol);

        //  Run reactor until there's a termination signal
        zloop_start (self->loop);

        //  Reactor has ended
        s_client_destroy (&self);
    }
    else
        zsock_signal (cmdpipe, -1);
}


//  ---------------------------------------------------------------------------
//  Class interface

struct _pkgimport_client_t {
    zactor_t *actor;            //  Client actor
    zsock_t *msgpipe;           //  Pipe for async message flow
    bool connected;             //  Client currently connected or not
};


//  ---------------------------------------------------------------------------
//  Create a new pkgimport_client

pkgimport_client_t *
pkgimport_client_new (void)
{
    zsock_t *provided_pipe = NULL;
    pkgimport_client_t *self = (pkgimport_client_t *) zmalloc (sizeof (pkgimport_client_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
	zsock_t *piperays[] = {backend, provided_pipe};
        if (self->msgpipe)
            self->actor = zactor_new (pkgimport_client, piperays);
        if (!self->actor)
            pkgimport_client_destroy (&self);
    }
    return self;
}


//  ---------------------------------------------------------------------------
//  Destroy the pkgimport_client

void
pkgimport_client_destroy (pkgimport_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pkgimport_client_t *self = *self_p;
        if (self->actor && !zsys_interrupted) {
            //  Before destroying the actor we have to flush any pending
            //  traffic on the msgpipe, otherwise it gets lost in a fire and
            //  forget scenario. We do this by sending $FLUSH to the msgpipe
            //  and waiting for a signal back on the cmdpipe.
            if (zstr_send (self->msgpipe, "$FLUSH") == 0)
                zsock_wait (self->actor);
        }
        zactor_destroy (&self->actor);
        zsock_destroy (&self->msgpipe);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------------
//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.

zactor_t *
pkgimport_client_actor (pkgimport_client_t *self)
{
    assert (self);
    return self->actor;
}


//  ---------------------------------------------------------------------------
//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.

zsock_t *
pkgimport_client_msgpipe (pkgimport_client_t *self)
{
    assert (self);
    return self->msgpipe;
}


//  ---------------------------------------------------------------------------
//  Used to connect to the server.

int
pkgimport_client_construct (pkgimport_client_t *self, const char *endpoint)
{
    assert (self);

    zsock_send (self->actor, "ss", "CONSTRUCT", endpoint);
    return 0;
}


//  ---------------------------------------------------------------------------
//  Used to connect to the server.

int
pkgimport_client_set_xbps_src_path (pkgimport_client_t *self, const char *xbps_src_path)
{
    assert (self);

    zsock_send (self->actor, "ss", "SET XBPS_SRC PATH", xbps_src_path);
    return 0;
}


//  ---------------------------------------------------------------------------
//  Enable verbose tracing (animation) of state machine activity.

void
pkgimport_client_set_verbose (pkgimport_client_t *self, bool verbose)
{
    assert (self);
    zsock_send (self->actor, "si", "SET VERBOSE", verbose);
}


//  -------------------------------------------------------------------
//  Takes privkey, pubkey, and serverkey, and sets them on the socket

void
pkgall_client_ssl_setcurve(void *client, const char *privkey,
                        const char *pubkey, const char *serverkey)
{
        client_t *self = client;
        zsock_set_curve_secretkey(self->dealer, privkey);
        zsock_set_curve_publickey(self->dealer, pubkey);
        zsock_set_curve_serverkey(self->dealer, serverkey);
        zsock_set_curve_server(self->dealer, 0);
}

