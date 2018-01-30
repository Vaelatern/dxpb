/*  =========================================================================
    pkggraph_worker_engine - Package Builder - Where Work Gets Done engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkggraph_worker.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    negotiate_state = 2,
    hesitate_state = 3,
    preparing_state = 4,
    ready_state = 5,
    bootstrap_regular_state = 6,
    bootstrap_build_wanted_state = 7,
    working_state = 8,
    die_state = 9,
    defaults_state = 10
} state_t;

typedef enum {
    NULL_event = 0,
    set_build_params_event = 1,
    construct_event = 2,
    connect_error_event = 3,
    roger_event = 4,
    bootstrap_done_event = 5,
    update_bootstrap_event = 6,
    needpkg_event = 7,
    expired_event = 8,
    job_ended_event = 9,
    iforgotu_event = 10,
    invalid_event = 11,
    kthnksbye_event = 12,
    set_repopath_event = 13
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "negotiate",
    "hesitate",
    "preparing",
    "ready",
    "bootstrap regular",
    "bootstrap build wanted",
    "working",
    "die",
    "defaults"
};

static char *
s_event_name [] = {
    "(NONE)",
    "set_build_params",
    "construct",
    "connect_error",
    "ROGER",
    "bootstrap_done",
    "UPDATE_BOOTSTRAP",
    "NEEDPKG",
    "expired",
    "JOB_ENDED",
    "IFORGOTU",
    "INVALID",
    "kthnksbye",
    "set_repopath"
};


//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

//  These are the different method arguments we manage automatically
struct _client_args_t {
    char *endpoint;
    char *repopath;
    char *hostarch;
    char *targetarch;
    uint8_t iscross;
    uint16_t cost;
};

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *cmdpipe;           //  Get/send commands from caller API
    zsock_t *msgpipe;           //  Get/send messages from caller API
    zsock_t *provided_pipe;     //  Do whatever the caller defines
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
    pkggraph_msg_t *message;    //  Message received or sent
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
    set_build_params (client_t *self);
static void
    connect_to_server (client_t *self);
static void
    complain_about_connection_error (client_t *self);
static void
    flag_bootstrap_wanted (client_t *self);
static void
    prepare_icanhelp (client_t *self);
static void
    set_timeout_high (client_t *self);
static void
    act_if_bootstrap_is_wanted (client_t *self);
static void
    do_bootstrap_update (client_t *self);
static void
    begin_building_pkg (client_t *self);
static void
    get_log_data (client_t *self);
static void
    set_timeout_low (client_t *self);
static void
    do_git_ff (client_t *self);
static void
    cease_all_operations (client_t *self);
static void
    set_repopath_to_provided (client_t *self);

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
            "%6d:%-33s", randof (1000000), "pkggraph_worker");
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->message = pkggraph_msg_new ();
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
        zstr_free (&self->args.repopath);
        zstr_free (&self->args.hostarch);
        zstr_free (&self->args.targetarch);
        client_terminate (&self->client);
        pkggraph_msg_destroy (&self->message);
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
s_protocol_event (s_client_t *self, pkggraph_msg_t *message)
{
    assert (message);
    switch (pkggraph_msg_id (message)) {
        case PKGGRAPH_MSG_ROGER:
            return roger_event;
            break;
        case PKGGRAPH_MSG_IFORGOTU:
            return iforgotu_event;
            break;
        case PKGGRAPH_MSG_INVALID:
            return invalid_event;
            break;
        case PKGGRAPH_MSG_NEEDPKG:
            return needpkg_event;
            break;
        case PKGGRAPH_MSG_UPDATE_BOOTSTRAP:
            return update_bootstrap_event;
            break;
        case PKGGRAPH_MSG_JOB_ENDED:
            return job_ended_event;
            break;
        default:
            zsys_error ("%s: unknown command %s, halting",
                        self->log_prefix, pkggraph_msg_command (message));
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
                if (self->event == set_build_params_event) {
                    if (!self->exception) {
                        //  set build params
                        if (self->verbose)
                            zsys_debug ("%s:         $ set build params", self->log_prefix);
                        set_build_params (&self->client);
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  flag bootstrap wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ flag bootstrap wanted", self->log_prefix);
                        flag_bootstrap_wanted (&self->client);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                else
                if (self->event == connect_error_event) {
                    if (!self->exception) {
                        //  complain about connection error
                        if (self->verbose)
                            zsys_debug ("%s:         $ complain about connection error", self->log_prefix);
                        complain_about_connection_error (&self->client);
                    }
                    if (!self->exception) {
                        //  flag bootstrap wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ flag bootstrap wanted", self->log_prefix);
                        flag_bootstrap_wanted (&self->client);
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  flag bootstrap wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ flag bootstrap wanted", self->log_prefix);
                        flag_bootstrap_wanted (&self->client);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  flag bootstrap wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ flag bootstrap wanted", self->log_prefix);
                        flag_bootstrap_wanted (&self->client);
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
                if (self->event == kthnksbye_event) {
                    if (!self->exception) {
                        //  flag bootstrap wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ flag bootstrap wanted", self->log_prefix);
                        flag_bootstrap_wanted (&self->client);
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  flag bootstrap wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ flag bootstrap wanted", self->log_prefix);
                        flag_bootstrap_wanted (&self->client);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                break;

            case negotiate_state:
                if (self->event == roger_event) {
                    if (!self->exception) {
                        //  send IMAWORKER
                        if (self->verbose)
                            zsys_debug ("%s:         $ send IMAWORKER",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_IMAWORKER);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = hesitate_state;
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                break;

            case hesitate_state:
                if (self->event == roger_event) {
                    if (!self->exception)
                        self->state = preparing_state;
                    if (!self->exception) {
                        //  set timeout high
                        if (self->verbose)
                            zsys_debug ("%s:         $ set timeout high", self->log_prefix);
                        set_timeout_high (&self->client);
                    }
                    if (!self->exception) {
                        //  act if bootstrap is wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ act if bootstrap is wanted", self->log_prefix);
                        act_if_bootstrap_is_wanted (&self->client);
                    }
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                break;

            case preparing_state:
                if (self->event == bootstrap_done_event) {
                    if (!self->exception) {
                        //  prepare icanhelp
                        if (self->verbose)
                            zsys_debug ("%s:         $ prepare icanhelp", self->log_prefix);
                        prepare_icanhelp (&self->client);
                    }
                    if (!self->exception) {
                        //  send ICANHELP
                        if (self->verbose)
                            zsys_debug ("%s:         $ send ICANHELP",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_ICANHELP);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = ready_state;
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                break;

            case ready_state:
                if (self->event == update_bootstrap_event) {
                    if (!self->exception) {
                        //  do bootstrap update
                        if (self->verbose)
                            zsys_debug ("%s:         $ do bootstrap update", self->log_prefix);
                        do_bootstrap_update (&self->client);
                    }
                    if (!self->exception)
                        self->state = bootstrap_regular_state;
                }
                else
                if (self->event == needpkg_event) {
                    if (!self->exception)
                        self->state = working_state;
                    if (!self->exception) {
                        //  set timeout low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set timeout low", self->log_prefix);
                        set_timeout_low (&self->client);
                    }
                    if (!self->exception) {
                        //  do git ff
                        if (self->verbose)
                            zsys_debug ("%s:         $ do git ff", self->log_prefix);
                        do_git_ff (&self->client);
                    }
                    if (!self->exception) {
                        //  begin building pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ begin building pkg", self->log_prefix);
                        begin_building_pkg (&self->client);
                    }
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                break;

            case bootstrap_regular_state:
                if (self->event == needpkg_event) {
                    if (!self->exception)
                        self->state = bootstrap_build_wanted_state;
                }
                else
                if (self->event == bootstrap_done_event) {
                    if (!self->exception)
                        self->state = ready_state;
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                break;

            case bootstrap_build_wanted_state:
                if (self->event == bootstrap_done_event) {
                    if (!self->exception) {
                        //  begin building pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ begin building pkg", self->log_prefix);
                        begin_building_pkg (&self->client);
                    }
                    if (!self->exception)
                        self->state = working_state;
                    if (!self->exception) {
                        //  set timeout low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set timeout low", self->log_prefix);
                        set_timeout_low (&self->client);
                    }
                    if (!self->exception) {
                        //  do git ff
                        if (self->verbose)
                            zsys_debug ("%s:         $ do git ff", self->log_prefix);
                        do_git_ff (&self->client);
                    }
                    if (!self->exception) {
                        //  begin building pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ begin building pkg", self->log_prefix);
                        begin_building_pkg (&self->client);
                    }
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
                }
                break;

            case working_state:
                if (self->event == update_bootstrap_event) {
                    if (!self->exception) {
                        //  flag bootstrap wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ flag bootstrap wanted", self->log_prefix);
                        flag_bootstrap_wanted (&self->client);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  get log data
                        if (self->verbose)
                            zsys_debug ("%s:         $ get log data", self->log_prefix);
                        get_log_data (&self->client);
                    }
                    if (!self->exception) {
                        //  send LOGHERE
                        if (self->verbose)
                            zsys_debug ("%s:         $ send LOGHERE",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_LOGHERE);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == job_ended_event) {
                    if (!self->exception) {
                        //  send JOB_ENDED
                        if (self->verbose)
                            zsys_debug ("%s:         $ send JOB_ENDED",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_JOB_ENDED);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = preparing_state;
                    if (!self->exception) {
                        //  set timeout high
                        if (self->verbose)
                            zsys_debug ("%s:         $ set timeout high", self->log_prefix);
                        set_timeout_high (&self->client);
                    }
                    if (!self->exception) {
                        //  act if bootstrap is wanted
                        if (self->verbose)
                            zsys_debug ("%s:         $ act if bootstrap is wanted", self->log_prefix);
                        act_if_bootstrap_is_wanted (&self->client);
                    }
                }
                else
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                if (self->event == roger_event) {
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_INVALID);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                break;

            case defaults_state:
                if (self->event == iforgotu_event) {
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_PING);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == set_repopath_event) {
                    if (!self->exception) {
                        //  set repopath to provided
                        if (self->verbose)
                            zsys_debug ("%s:         $ set repopath to provided", self->log_prefix);
                        set_repopath_to_provided (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("%s:         $ send HELLO",
                                self->log_prefix);
                        pkggraph_msg_set_id (self->message, PKGGRAPH_MSG_HELLO);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkggraph_msg_print (self->message);
                        pkggraph_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = negotiate_state;
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
    if (streq (method, "SET REPOPATH")) {
        zstr_free (&self->args.repopath);
        zsock_recv (self->cmdpipe, "s", &self->args.repopath);
        s_client_execute (self, set_repopath_event);
    }
    else
    if (streq (method, "SET BUILD PARAMS")) {
        zstr_free (&self->args.hostarch);
        zstr_free (&self->args.targetarch);
        zsock_recv (self->cmdpipe, "ss12", &self->args.hostarch, &self->args.targetarch, &self->args.iscross, &self->args.cost);
        s_client_execute (self, set_build_params_event);
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
        if (pkggraph_msg_recv (self->message, self->dealer))
            return -1;              //  Interrupted; exit zloop

        zsys_debug ("Server message");
        pkggraph_msg_print (self->message);
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
pkggraph_worker (zsock_t *cmdpipe, void *piperays)
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

struct _pkggraph_worker_t {
    zactor_t *actor;            //  Client actor
    zsock_t *msgpipe;           //  Pipe for async message flow
    bool connected;             //  Client currently connected or not
    uint32_t ok;                //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new pkggraph_worker

pkggraph_worker_t *
pkggraph_worker_new (zsock_t *provided_pipe)
{
    pkggraph_worker_t *self = (pkggraph_worker_t *) zmalloc (sizeof (pkggraph_worker_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
	zsock_t *piperays[] = {backend, provided_pipe};
        if (self->msgpipe)
            self->actor = zactor_new (pkggraph_worker, piperays);
        if (!self->actor)
            pkggraph_worker_destroy (&self);
    }
    return self;
}


//  ---------------------------------------------------------------------------
//  Destroy the pkggraph_worker

void
pkggraph_worker_destroy (pkggraph_worker_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pkggraph_worker_t *self = *self_p;
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
pkggraph_worker_actor (pkggraph_worker_t *self)
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
pkggraph_worker_msgpipe (pkggraph_worker_t *self)
{
    assert (self);
    return self->msgpipe;
}


//  ---------------------------------------------------------------------------
//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.

bool
pkggraph_worker_connected (pkggraph_worker_t *self)
{
    assert (self);
    int connected;
    zsock_send (self->actor, "s", "$CONNECTED");
    zsock_recv (self->actor, "i", &connected);
    return connected == 1;
}


//  ---------------------------------------------------------------------------
//  Get valid reply from actor; discard replies that does not match. Current
//  implementation filters on first frame of message. Blocks until a valid
//  reply is received, and properties can be loaded from it. Returns 0 if
//  matched, -1 if interrupted.

static int
s_accept_reply (pkggraph_worker_t *self, ...)
{
    assert (self);
    while (!zsys_interrupted) {
        char *reply = zstr_recv (self->actor);
        if (!reply)
            break;              //  Interrupted

        va_list args;
        va_start (args, self);
        char *filter = va_arg (args, char *);
        while (filter) {
            if (streq (reply, filter)) {
                if (streq (reply, "STATUS")) {
                    zsock_recv (self->actor, "4", &self->ok);
                }
                break;
            }
            filter = va_arg (args, char *);
        }
        va_end (args);
        //  If anything was remaining on pipe, flush it
        zsock_flush (self->actor);
        zstr_free (&reply);
        if (filter) {
            return 0;           //  We matched one of the filters
        }
    }
    return -1;          //  Interrupted
}


//  ---------------------------------------------------------------------------
//  Used to connect to the server.

int
pkggraph_worker_construct (pkggraph_worker_t *self, const char *endpoint)
{
    assert (self);

    zsock_send (self->actor, "ss", "CONSTRUCT", endpoint);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation

int
pkggraph_worker_set_repopath (pkggraph_worker_t *self, const char *repopath)
{
    assert (self);

    zsock_send (self->actor, "ss", "SET REPOPATH", repopath);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation
//  Returns >= 0 if successful, -1 if interrupted.

uint32_t
pkggraph_worker_set_build_params (pkggraph_worker_t *self, const char *hostarch, const char *targetarch, uint8_t iscross, uint16_t cost)
{
    assert (self);

    zsock_send (self->actor, "sss12", "SET BUILD PARAMS", hostarch, targetarch, iscross, cost);
    if (s_accept_reply (self, "STATUS", NULL))
        return -1;              //  Interrupted or timed-out
    return self->ok;
}


//  ---------------------------------------------------------------------------
//  Return last received ok

uint32_t
pkggraph_worker_ok (pkggraph_worker_t *self)
{
    assert (self);
    return self->ok;
}


//  ---------------------------------------------------------------------------
//  Enable verbose tracing (animation) of state machine activity.

void
pkggraph_worker_set_verbose (pkggraph_worker_t *self, bool verbose)
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

