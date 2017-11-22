/*  =========================================================================
    pkgimport_grapher_engine - Package Grapher engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_grapher.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    register_state = 2,
    unstable_state = 3,
    stable_state = 4,
    working_defaults_state = 5,
    die_state = 6,
    defaults_state = 7
} state_t;

typedef enum {
    NULL_event = 0,
    set_publish_endpoint_event = 1,
    set_db_path_event = 2,
    construct_event = 3,
    roger_event = 4,
    connect_error_event = 5,
    iforgotu_event = 6,
    unstable_event = 7,
    no_hash_found_event = 8,
    confirm_hash_event = 9,
    expired_event = 10,
    worker_available_event = 11,
    stable_event = 12,
    invalid_event = 13,
    kthnksbye_event = 14,
    pkginfo_event = 15,
    pkgdel_event = 16,
    worker_gone_event = 17,
    pkg_now_completed_event = 18,
    pkg_not_here_event = 19,
    pkg_here_event = 20,
    pkg_failed_to_build_event = 21,
    worker_not_appropiate_event = 22
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "register",
    "unstable",
    "stable",
    "working defaults",
    "die",
    "defaults"
};

static char *
s_event_name [] = {
    "(NONE)",
    "set_publish_endpoint",
    "set_db_path",
    "construct",
    "ROGER",
    "connect_error",
    "IFORGOTU",
    "UNSTABLE",
    "no_hash_found",
    "confirm_hash",
    "expired",
    "worker_available",
    "STABLE",
    "INVALID",
    "KTHNKSBYE",
    "PKGINFO",
    "PKGDEL",
    "worker_gone",
    "pkg_now_completed",
    "pkg_not_here",
    "pkg_here",
    "pkg_failed_to_build",
    "worker_not_appropiate"
};


//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

//  These are the different method arguments we manage automatically
struct _client_args_t {
    char *endpoint;
    char *pkgname;
    char *version;
    char *arch;
    int addr;
    int check;
    char *hostarch;
    char *targetarch;
    uint8_t iscross;
    uint16_t cost;
    char *dbpath;
    char *pubpoint;
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
    set_publish_endpoint_as_supplied (client_t *self);
static void
    set_db_path_as_supplied (client_t *self);
static void
    connect_to_server (client_t *self);
static void
    complain_about_connection_error (client_t *self);
static void
    read_all_from_db (client_t *self);
static void
    decide_next_action_from_hash (client_t *self);
static void
    add_worker_to_list (client_t *self);
static void
    store_hash (client_t *self);
static void
    set_expiry_low (client_t *self);
static void
    find_package_for_worker (client_t *self);
static void
    write_graph_to_db (client_t *self);
static void
    resolve_graph (client_t *self);
static void
    get_packages (client_t *self);
static void
    ask_around_for_wanted_pkgs (client_t *self);
static void
    set_expiry_high (client_t *self);
static void
    create_pkg_from_info (client_t *self);
static void
    mark_pkg_for_deletion (client_t *self);
static void
    return_pkg_to_pending (client_t *self);
static void
    unassign_worker (client_t *self);
static void
    remove_worker_from_list (client_t *self);
static void
    ask_around_for_this_pkg (client_t *self);
static void
    note_pkg_not_yet_around (client_t *self);
static void
    match_workers_to_this_pkg (client_t *self);
static void
    note_pkg_present (client_t *self);
static void
    get_package_list_for_arch (client_t *self);
static void
    mark_pkg_bad (client_t *self);
static void
    negative_pkg_affinity_for_wrkr (client_t *self);
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
            "%6d:%-33s", randof (1000000), "pkgimport_grapher");
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
        zstr_free (&self->args.pkgname);
        zstr_free (&self->args.version);
        zstr_free (&self->args.arch);
        zstr_free (&self->args.hostarch);
        zstr_free (&self->args.targetarch);
        zstr_free (&self->args.dbpath);
        zstr_free (&self->args.pubpoint);
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
        case PKGIMPORT_MSG_INVALID:
            return invalid_event;
            break;
        case PKGIMPORT_MSG_KTHNKSBYE:
            return kthnksbye_event;
            break;
        case PKGIMPORT_MSG_PKGINFO:
            return pkginfo_event;
            break;
        case PKGIMPORT_MSG_PKGDEL:
            return pkgdel_event;
            break;
        case PKGIMPORT_MSG_STABLE:
            return stable_event;
            break;
        case PKGIMPORT_MSG_UNSTABLE:
            return unstable_event;
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
                if (self->event == set_publish_endpoint_event) {
                    if (!self->exception) {
                        //  set publish endpoint as supplied
                        if (self->verbose)
                            zsys_debug ("%s:         $ set publish endpoint as supplied", self->log_prefix);
                        set_publish_endpoint_as_supplied (&self->client);
                    }
                }
                else
                if (self->event == set_db_path_event) {
                    if (!self->exception) {
                        //  set db path as supplied
                        if (self->verbose)
                            zsys_debug ("%s:         $ set db path as supplied", self->log_prefix);
                        set_db_path_as_supplied (&self->client);
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
                        self->state = register_state;
                    if (!self->exception) {
                        //  send IAMTHEGRAPHER
                        if (self->verbose)
                            zsys_debug ("%s:         $ send IAMTHEGRAPHER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_IAMTHEGRAPHER);
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
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
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
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
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
                        //  send INVALID
                        if (self->verbose)
                            zsys_debug ("%s:         $ send INVALID",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_INVALID);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                break;

            case register_state:
                if (self->event == roger_event) {
                    if (!self->exception) {
                        //  read all from db
                        if (self->verbose)
                            zsys_debug ("%s:         $ read all from db", self->log_prefix);
                        read_all_from_db (&self->client);
                    }
                    if (!self->exception) {
                        //  decide next action from hash
                        if (self->verbose)
                            zsys_debug ("%s:         $ decide next action from hash", self->log_prefix);
                        decide_next_action_from_hash (&self->client);
                    }
                    if (!self->exception)
                        self->state = unstable_state;
                    if (!self->exception) {
                        //  set expiry low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry low", self->log_prefix);
                        set_expiry_low (&self->client);
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
                        //  read all from db
                        if (self->verbose)
                            zsys_debug ("%s:         $ read all from db", self->log_prefix);
                        read_all_from_db (&self->client);
                    }
                    if (!self->exception) {
                        //  decide next action from hash
                        if (self->verbose)
                            zsys_debug ("%s:         $ decide next action from hash", self->log_prefix);
                        decide_next_action_from_hash (&self->client);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  read all from db
                        if (self->verbose)
                            zsys_debug ("%s:         $ read all from db", self->log_prefix);
                        read_all_from_db (&self->client);
                    }
                    if (!self->exception) {
                        //  decide next action from hash
                        if (self->verbose)
                            zsys_debug ("%s:         $ decide next action from hash", self->log_prefix);
                        decide_next_action_from_hash (&self->client);
                    }
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception) {
                        //  read all from db
                        if (self->verbose)
                            zsys_debug ("%s:         $ read all from db", self->log_prefix);
                        read_all_from_db (&self->client);
                    }
                    if (!self->exception) {
                        //  decide next action from hash
                        if (self->verbose)
                            zsys_debug ("%s:         $ decide next action from hash", self->log_prefix);
                        decide_next_action_from_hash (&self->client);
                    }
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
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
                        //  send INVALID
                        if (self->verbose)
                            zsys_debug ("%s:         $ send INVALID",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_INVALID);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  read all from db
                        if (self->verbose)
                            zsys_debug ("%s:         $ read all from db", self->log_prefix);
                        read_all_from_db (&self->client);
                    }
                    if (!self->exception) {
                        //  decide next action from hash
                        if (self->verbose)
                            zsys_debug ("%s:         $ decide next action from hash", self->log_prefix);
                        decide_next_action_from_hash (&self->client);
                    }
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                break;

            case unstable_state:
                if (self->event == unstable_event) {
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
                if (self->event == no_hash_found_event) {
                    if (!self->exception) {
                        //  send PLZREADALL
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PLZREADALL",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_PLZREADALL);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == confirm_hash_event) {
                    if (!self->exception) {
                        //  send WESEEHASH
                        if (self->verbose)
                            zsys_debug ("%s:         $ send WESEEHASH",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_WESEEHASH);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  send STABLESTATUSPLZ
                        if (self->verbose)
                            zsys_debug ("%s:         $ send STABLESTATUSPLZ",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_STABLESTATUSPLZ);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == worker_available_event) {
                    if (!self->exception) {
                        //  add worker to list
                        if (self->verbose)
                            zsys_debug ("%s:         $ add worker to list", self->log_prefix);
                        add_worker_to_list (&self->client);
                    }
                }
                else
                if (self->event == stable_event) {
                    if (!self->exception) {
                        //  store hash
                        if (self->verbose)
                            zsys_debug ("%s:         $ store hash", self->log_prefix);
                        store_hash (&self->client);
                    }
                    if (!self->exception)
                        self->state = stable_state;
                    if (!self->exception) {
                        //  write graph to db
                        if (self->verbose)
                            zsys_debug ("%s:         $ write graph to db", self->log_prefix);
                        write_graph_to_db (&self->client);
                    }
                    if (!self->exception) {
                        //  resolve graph
                        if (self->verbose)
                            zsys_debug ("%s:         $ resolve graph", self->log_prefix);
                        resolve_graph (&self->client);
                    }
                    if (!self->exception) {
                        //  get packages
                        if (self->verbose)
                            zsys_debug ("%s:         $ get packages", self->log_prefix);
                        get_packages (&self->client);
                    }
                    if (!self->exception) {
                        //  ask around for wanted pkgs
                        if (self->verbose)
                            zsys_debug ("%s:         $ ask around for wanted pkgs", self->log_prefix);
                        ask_around_for_wanted_pkgs (&self->client);
                    }
                    if (!self->exception) {
                        //  set expiry high
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry high", self->log_prefix);
                        set_expiry_high (&self->client);
                    }
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                else
                if (self->event == pkginfo_event) {
                    if (!self->exception) {
                        //  create pkg from info
                        if (self->verbose)
                            zsys_debug ("%s:         $ create pkg from info", self->log_prefix);
                        create_pkg_from_info (&self->client);
                    }
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
                    if (!self->exception)
                        self->state = unstable_state;
                    if (!self->exception) {
                        //  set expiry low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry low", self->log_prefix);
                        set_expiry_low (&self->client);
                    }
                }
                else
                if (self->event == pkgdel_event) {
                    if (!self->exception) {
                        //  mark pkg for deletion
                        if (self->verbose)
                            zsys_debug ("%s:         $ mark pkg for deletion", self->log_prefix);
                        mark_pkg_for_deletion (&self->client);
                    }
                    if (!self->exception)
                        self->state = unstable_state;
                    if (!self->exception) {
                        //  set expiry low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry low", self->log_prefix);
                        set_expiry_low (&self->client);
                    }
                }
                else
                if (self->event == worker_gone_event) {
                    if (!self->exception) {
                        //  return pkg to pending
                        if (self->verbose)
                            zsys_debug ("%s:         $ return pkg to pending", self->log_prefix);
                        return_pkg_to_pending (&self->client);
                    }
                    if (!self->exception) {
                        //  unassign worker
                        if (self->verbose)
                            zsys_debug ("%s:         $ unassign worker", self->log_prefix);
                        unassign_worker (&self->client);
                    }
                    if (!self->exception) {
                        //  remove worker from list
                        if (self->verbose)
                            zsys_debug ("%s:         $ remove worker from list", self->log_prefix);
                        remove_worker_from_list (&self->client);
                    }
                }
                else
                if (self->event == pkg_now_completed_event) {
                    if (!self->exception) {
                        //  ask around for this pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ ask around for this pkg", self->log_prefix);
                        ask_around_for_this_pkg (&self->client);
                    }
                    if (!self->exception) {
                        //  unassign worker
                        if (self->verbose)
                            zsys_debug ("%s:         $ unassign worker", self->log_prefix);
                        unassign_worker (&self->client);
                    }
                    if (!self->exception) {
                        //  remove worker from list
                        if (self->verbose)
                            zsys_debug ("%s:         $ remove worker from list", self->log_prefix);
                        remove_worker_from_list (&self->client);
                    }
                }
                else
                if (self->event == pkg_not_here_event) {
                    if (!self->exception) {
                        //  note pkg not yet around
                        if (self->verbose)
                            zsys_debug ("%s:         $ note pkg not yet around", self->log_prefix);
                        note_pkg_not_yet_around (&self->client);
                    }
                    if (!self->exception) {
                        //  match workers to this pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ match workers to this pkg", self->log_prefix);
                        match_workers_to_this_pkg (&self->client);
                    }
                }
                else
                if (self->event == pkg_here_event) {
                    if (!self->exception) {
                        //  note pkg present
                        if (self->verbose)
                            zsys_debug ("%s:         $ note pkg present", self->log_prefix);
                        note_pkg_present (&self->client);
                    }
                    if (!self->exception) {
                        //  get package list for arch
                        if (self->verbose)
                            zsys_debug ("%s:         $ get package list for arch", self->log_prefix);
                        get_package_list_for_arch (&self->client);
                    }
                    if (!self->exception) {
                        //  ask around for wanted pkgs
                        if (self->verbose)
                            zsys_debug ("%s:         $ ask around for wanted pkgs", self->log_prefix);
                        ask_around_for_wanted_pkgs (&self->client);
                    }
                }
                else
                if (self->event == pkg_failed_to_build_event) {
                    if (!self->exception) {
                        //  mark pkg bad
                        if (self->verbose)
                            zsys_debug ("%s:         $ mark pkg bad", self->log_prefix);
                        mark_pkg_bad (&self->client);
                    }
                }
                else
                if (self->event == worker_not_appropiate_event) {
                    if (!self->exception) {
                        //  return pkg to pending
                        if (self->verbose)
                            zsys_debug ("%s:         $ return pkg to pending", self->log_prefix);
                        return_pkg_to_pending (&self->client);
                    }
                    if (!self->exception) {
                        //  negative pkg affinity for wrkr
                        if (self->verbose)
                            zsys_debug ("%s:         $ negative pkg affinity for wrkr", self->log_prefix);
                        negative_pkg_affinity_for_wrkr (&self->client);
                    }
                }
                else {
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
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                break;

            case stable_state:
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
                if (self->event == worker_available_event) {
                    if (!self->exception) {
                        //  add worker to list
                        if (self->verbose)
                            zsys_debug ("%s:         $ add worker to list", self->log_prefix);
                        add_worker_to_list (&self->client);
                    }
                    if (!self->exception) {
                        //  find package for worker
                        if (self->verbose)
                            zsys_debug ("%s:         $ find package for worker", self->log_prefix);
                        find_package_for_worker (&self->client);
                    }
                }
                else
                if (self->event == unstable_event) {
                    if (!self->exception) {
                        //  store hash
                        if (self->verbose)
                            zsys_debug ("%s:         $ store hash", self->log_prefix);
                        store_hash (&self->client);
                    }
                    if (!self->exception)
                        self->state = unstable_state;
                    if (!self->exception) {
                        //  set expiry low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry low", self->log_prefix);
                        set_expiry_low (&self->client);
                    }
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
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
                else
                if (self->event == pkginfo_event) {
                    if (!self->exception) {
                        //  create pkg from info
                        if (self->verbose)
                            zsys_debug ("%s:         $ create pkg from info", self->log_prefix);
                        create_pkg_from_info (&self->client);
                    }
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
                    if (!self->exception)
                        self->state = unstable_state;
                    if (!self->exception) {
                        //  set expiry low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry low", self->log_prefix);
                        set_expiry_low (&self->client);
                    }
                }
                else
                if (self->event == pkgdel_event) {
                    if (!self->exception) {
                        //  mark pkg for deletion
                        if (self->verbose)
                            zsys_debug ("%s:         $ mark pkg for deletion", self->log_prefix);
                        mark_pkg_for_deletion (&self->client);
                    }
                    if (!self->exception)
                        self->state = unstable_state;
                    if (!self->exception) {
                        //  set expiry low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry low", self->log_prefix);
                        set_expiry_low (&self->client);
                    }
                }
                else
                if (self->event == worker_gone_event) {
                    if (!self->exception) {
                        //  return pkg to pending
                        if (self->verbose)
                            zsys_debug ("%s:         $ return pkg to pending", self->log_prefix);
                        return_pkg_to_pending (&self->client);
                    }
                    if (!self->exception) {
                        //  unassign worker
                        if (self->verbose)
                            zsys_debug ("%s:         $ unassign worker", self->log_prefix);
                        unassign_worker (&self->client);
                    }
                    if (!self->exception) {
                        //  remove worker from list
                        if (self->verbose)
                            zsys_debug ("%s:         $ remove worker from list", self->log_prefix);
                        remove_worker_from_list (&self->client);
                    }
                }
                else
                if (self->event == pkg_now_completed_event) {
                    if (!self->exception) {
                        //  ask around for this pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ ask around for this pkg", self->log_prefix);
                        ask_around_for_this_pkg (&self->client);
                    }
                    if (!self->exception) {
                        //  unassign worker
                        if (self->verbose)
                            zsys_debug ("%s:         $ unassign worker", self->log_prefix);
                        unassign_worker (&self->client);
                    }
                    if (!self->exception) {
                        //  remove worker from list
                        if (self->verbose)
                            zsys_debug ("%s:         $ remove worker from list", self->log_prefix);
                        remove_worker_from_list (&self->client);
                    }
                }
                else
                if (self->event == pkg_not_here_event) {
                    if (!self->exception) {
                        //  note pkg not yet around
                        if (self->verbose)
                            zsys_debug ("%s:         $ note pkg not yet around", self->log_prefix);
                        note_pkg_not_yet_around (&self->client);
                    }
                    if (!self->exception) {
                        //  match workers to this pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ match workers to this pkg", self->log_prefix);
                        match_workers_to_this_pkg (&self->client);
                    }
                }
                else
                if (self->event == pkg_here_event) {
                    if (!self->exception) {
                        //  note pkg present
                        if (self->verbose)
                            zsys_debug ("%s:         $ note pkg present", self->log_prefix);
                        note_pkg_present (&self->client);
                    }
                    if (!self->exception) {
                        //  get package list for arch
                        if (self->verbose)
                            zsys_debug ("%s:         $ get package list for arch", self->log_prefix);
                        get_package_list_for_arch (&self->client);
                    }
                    if (!self->exception) {
                        //  ask around for wanted pkgs
                        if (self->verbose)
                            zsys_debug ("%s:         $ ask around for wanted pkgs", self->log_prefix);
                        ask_around_for_wanted_pkgs (&self->client);
                    }
                }
                else
                if (self->event == pkg_failed_to_build_event) {
                    if (!self->exception) {
                        //  mark pkg bad
                        if (self->verbose)
                            zsys_debug ("%s:         $ mark pkg bad", self->log_prefix);
                        mark_pkg_bad (&self->client);
                    }
                }
                else
                if (self->event == worker_not_appropiate_event) {
                    if (!self->exception) {
                        //  return pkg to pending
                        if (self->verbose)
                            zsys_debug ("%s:         $ return pkg to pending", self->log_prefix);
                        return_pkg_to_pending (&self->client);
                    }
                    if (!self->exception) {
                        //  negative pkg affinity for wrkr
                        if (self->verbose)
                            zsys_debug ("%s:         $ negative pkg affinity for wrkr", self->log_prefix);
                        negative_pkg_affinity_for_wrkr (&self->client);
                    }
                }
                else {
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
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                break;

            case working_defaults_state:
                if (self->event == invalid_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
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
                else
                if (self->event == pkginfo_event) {
                    if (!self->exception) {
                        //  create pkg from info
                        if (self->verbose)
                            zsys_debug ("%s:         $ create pkg from info", self->log_prefix);
                        create_pkg_from_info (&self->client);
                    }
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
                    if (!self->exception)
                        self->state = unstable_state;
                    if (!self->exception) {
                        //  set expiry low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry low", self->log_prefix);
                        set_expiry_low (&self->client);
                    }
                }
                else
                if (self->event == pkgdel_event) {
                    if (!self->exception) {
                        //  mark pkg for deletion
                        if (self->verbose)
                            zsys_debug ("%s:         $ mark pkg for deletion", self->log_prefix);
                        mark_pkg_for_deletion (&self->client);
                    }
                    if (!self->exception)
                        self->state = unstable_state;
                    if (!self->exception) {
                        //  set expiry low
                        if (self->verbose)
                            zsys_debug ("%s:         $ set expiry low", self->log_prefix);
                        set_expiry_low (&self->client);
                    }
                }
                else
                if (self->event == worker_gone_event) {
                    if (!self->exception) {
                        //  return pkg to pending
                        if (self->verbose)
                            zsys_debug ("%s:         $ return pkg to pending", self->log_prefix);
                        return_pkg_to_pending (&self->client);
                    }
                    if (!self->exception) {
                        //  unassign worker
                        if (self->verbose)
                            zsys_debug ("%s:         $ unassign worker", self->log_prefix);
                        unassign_worker (&self->client);
                    }
                    if (!self->exception) {
                        //  remove worker from list
                        if (self->verbose)
                            zsys_debug ("%s:         $ remove worker from list", self->log_prefix);
                        remove_worker_from_list (&self->client);
                    }
                }
                else
                if (self->event == pkg_now_completed_event) {
                    if (!self->exception) {
                        //  ask around for this pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ ask around for this pkg", self->log_prefix);
                        ask_around_for_this_pkg (&self->client);
                    }
                    if (!self->exception) {
                        //  unassign worker
                        if (self->verbose)
                            zsys_debug ("%s:         $ unassign worker", self->log_prefix);
                        unassign_worker (&self->client);
                    }
                    if (!self->exception) {
                        //  remove worker from list
                        if (self->verbose)
                            zsys_debug ("%s:         $ remove worker from list", self->log_prefix);
                        remove_worker_from_list (&self->client);
                    }
                }
                else
                if (self->event == pkg_not_here_event) {
                    if (!self->exception) {
                        //  note pkg not yet around
                        if (self->verbose)
                            zsys_debug ("%s:         $ note pkg not yet around", self->log_prefix);
                        note_pkg_not_yet_around (&self->client);
                    }
                    if (!self->exception) {
                        //  match workers to this pkg
                        if (self->verbose)
                            zsys_debug ("%s:         $ match workers to this pkg", self->log_prefix);
                        match_workers_to_this_pkg (&self->client);
                    }
                }
                else
                if (self->event == pkg_here_event) {
                    if (!self->exception) {
                        //  note pkg present
                        if (self->verbose)
                            zsys_debug ("%s:         $ note pkg present", self->log_prefix);
                        note_pkg_present (&self->client);
                    }
                    if (!self->exception) {
                        //  get package list for arch
                        if (self->verbose)
                            zsys_debug ("%s:         $ get package list for arch", self->log_prefix);
                        get_package_list_for_arch (&self->client);
                    }
                    if (!self->exception) {
                        //  ask around for wanted pkgs
                        if (self->verbose)
                            zsys_debug ("%s:         $ ask around for wanted pkgs", self->log_prefix);
                        ask_around_for_wanted_pkgs (&self->client);
                    }
                }
                else
                if (self->event == pkg_failed_to_build_event) {
                    if (!self->exception) {
                        //  mark pkg bad
                        if (self->verbose)
                            zsys_debug ("%s:         $ mark pkg bad", self->log_prefix);
                        mark_pkg_bad (&self->client);
                    }
                }
                else
                if (self->event == worker_not_appropiate_event) {
                    if (!self->exception) {
                        //  return pkg to pending
                        if (self->verbose)
                            zsys_debug ("%s:         $ return pkg to pending", self->log_prefix);
                        return_pkg_to_pending (&self->client);
                    }
                    if (!self->exception) {
                        //  negative pkg affinity for wrkr
                        if (self->verbose)
                            zsys_debug ("%s:         $ negative pkg affinity for wrkr", self->log_prefix);
                        negative_pkg_affinity_for_wrkr (&self->client);
                    }
                }
                else {
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
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
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
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
                }
                else
                if (self->event == roger_event) {
                }
                else
                if (self->event == kthnksbye_event) {
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
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
                        //  send INVALID
                        if (self->verbose)
                            zsys_debug ("%s:         $ send INVALID",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->message, PKGIMPORT_MSG_INVALID);
                        zsys_debug ("%s: Send message to server", self->log_prefix);
                        pkgimport_msg_print (self->message);
                        pkgimport_msg_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = die_state;
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                    if (!self->exception) {
                        //  cease all operations
                        if (self->verbose)
                            zsys_debug ("%s:         $ cease all operations", self->log_prefix);
                        cease_all_operations (&self->client);
                    }
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
    if (streq (method, "PKG NOW COMPLETED")) {
        zstr_free (&self->args.pkgname);
        zstr_free (&self->args.version);
        zstr_free (&self->args.arch);
        zsock_recv (self->cmdpipe, "sss", &self->args.pkgname, &self->args.version, &self->args.arch);
        s_client_execute (self, pkg_now_completed_event);
    }
    else
    if (streq (method, "WORKER AVAILABLE")) {
        zstr_free (&self->args.hostarch);
        zstr_free (&self->args.targetarch);
        zsock_recv (self->cmdpipe, "iiss12", &self->args.addr, &self->args.check, &self->args.hostarch, &self->args.targetarch, &self->args.iscross, &self->args.cost);
        s_client_execute (self, worker_available_event);
    }
    else
    if (streq (method, "WORKER GONE")) {
        zsock_recv (self->cmdpipe, "ii", &self->args.addr, &self->args.check);
        s_client_execute (self, worker_gone_event);
    }
    else
    if (streq (method, "PKG HERE")) {
        zstr_free (&self->args.pkgname);
        zstr_free (&self->args.version);
        zstr_free (&self->args.arch);
        zsock_recv (self->cmdpipe, "sss", &self->args.pkgname, &self->args.version, &self->args.arch);
        s_client_execute (self, pkg_here_event);
    }
    else
    if (streq (method, "PKG NOT HERE")) {
        zstr_free (&self->args.pkgname);
        zstr_free (&self->args.version);
        zstr_free (&self->args.arch);
        zsock_recv (self->cmdpipe, "sss", &self->args.pkgname, &self->args.version, &self->args.arch);
        s_client_execute (self, pkg_not_here_event);
    }
    else
    if (streq (method, "PKG FAILED TO BUILD")) {
        zstr_free (&self->args.pkgname);
        zstr_free (&self->args.version);
        zstr_free (&self->args.arch);
        zsock_recv (self->cmdpipe, "iisss", &self->args.addr, &self->args.check, &self->args.pkgname, &self->args.version, &self->args.arch);
        s_client_execute (self, pkg_failed_to_build_event);
    }
    else
    if (streq (method, "WORKER NOT APPROPIATE")) {
        zsock_recv (self->cmdpipe, "ii", &self->args.addr, &self->args.check);
        s_client_execute (self, worker_not_appropiate_event);
    }
    else
    if (streq (method, "SET DB PATH")) {
        zstr_free (&self->args.dbpath);
        zsock_recv (self->cmdpipe, "s", &self->args.dbpath);
        s_client_execute (self, set_db_path_event);
    }
    else
    if (streq (method, "SET PUBLISH ENDPOINT")) {
        zstr_free (&self->args.pubpoint);
        zsock_recv (self->cmdpipe, "s", &self->args.pubpoint);
        s_client_execute (self, set_publish_endpoint_event);
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
pkgimport_grapher (zsock_t *cmdpipe, void *piperays)
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

struct _pkgimport_grapher_t {
    zactor_t *actor;            //  Client actor
    zsock_t *msgpipe;           //  Pipe for async message flow
    bool connected;             //  Client currently connected or not
};


//  ---------------------------------------------------------------------------
//  Create a new pkgimport_grapher

pkgimport_grapher_t *
pkgimport_grapher_new (void)
{
    zsock_t *provided_pipe = NULL;
    pkgimport_grapher_t *self = (pkgimport_grapher_t *) zmalloc (sizeof (pkgimport_grapher_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
	zsock_t *piperays[] = {backend, provided_pipe};
        if (self->msgpipe)
            self->actor = zactor_new (pkgimport_grapher, piperays);
        if (!self->actor)
            pkgimport_grapher_destroy (&self);
    }
    return self;
}


//  ---------------------------------------------------------------------------
//  Destroy the pkgimport_grapher

void
pkgimport_grapher_destroy (pkgimport_grapher_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pkgimport_grapher_t *self = *self_p;
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
pkgimport_grapher_actor (pkgimport_grapher_t *self)
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
pkgimport_grapher_msgpipe (pkgimport_grapher_t *self)
{
    assert (self);
    return self->msgpipe;
}


//  ---------------------------------------------------------------------------
//  Used to connect to the server.                                                  

int 
pkgimport_grapher_construct (pkgimport_grapher_t *self, const char *endpoint)
{
    assert (self);

    zsock_send (self->actor, "ss", "CONSTRUCT", endpoint);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_pkg_now_completed (pkgimport_grapher_t *self, const char *pkgname, const char *version, const char *arch)
{
    assert (self);

    zsock_send (self->actor, "ssss", "PKG NOW COMPLETED", pkgname, version, arch);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_worker_available (pkgimport_grapher_t *self, int addr, int check, const char *hostarch, const char *targetarch, uint8_t iscross, uint16_t cost)
{
    assert (self);

    zsock_send (self->actor, "siiss12", "WORKER AVAILABLE", addr, check, hostarch, targetarch, iscross, cost);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_worker_gone (pkgimport_grapher_t *self, int addr, int check)
{
    assert (self);

    zsock_send (self->actor, "sii", "WORKER GONE", addr, check);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_pkg_here (pkgimport_grapher_t *self, const char *pkgname, const char *version, const char *arch)
{
    assert (self);

    zsock_send (self->actor, "ssss", "PKG HERE", pkgname, version, arch);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_pkg_not_here (pkgimport_grapher_t *self, const char *pkgname, const char *version, const char *arch)
{
    assert (self);

    zsock_send (self->actor, "ssss", "PKG NOT HERE", pkgname, version, arch);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_pkg_failed_to_build (pkgimport_grapher_t *self, int addr, int check, const char *pkgname, const char *version, const char *arch)
{
    assert (self);

    zsock_send (self->actor, "siisss", "PKG FAILED TO BUILD", addr, check, pkgname, version, arch);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_worker_not_appropiate (pkgimport_grapher_t *self, int addr, int check)
{
    assert (self);

    zsock_send (self->actor, "sii", "WORKER NOT APPROPIATE", addr, check);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_set_db_path (pkgimport_grapher_t *self, const char *dbpath)
{
    assert (self);

    zsock_send (self->actor, "ss", "SET DB PATH", dbpath);
    return 0;
}


//  ---------------------------------------------------------------------------
//  No explanation                                                                  

int 
pkgimport_grapher_set_publish_endpoint (pkgimport_grapher_t *self, const char *pubpoint)
{
    assert (self);

    zsock_send (self->actor, "ss", "SET PUBLISH ENDPOINT", pubpoint);
    return 0;
}


//  ---------------------------------------------------------------------------
//  Enable verbose tracing (animation) of state machine activity.

void
pkgimport_grapher_set_verbose (pkgimport_grapher_t *self, bool verbose)
{
    assert (self);
    zsock_send (self->actor, "si", "SET VERBOSE", verbose);
}
