/*  =========================================================================
    pkgimport_server_engine - pkgimport_server engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_server.xml, or
     * The code generation script that built this file: ../exec/zproto_server_c
    ************************************************************************
    =========================================================================
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#define ZPROTO_UNUSED(object) (void)object

//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    introduction_state = 1,
    pleasantries_state = 2,
    worker_ready_state = 3,
    worker_asked_state = 4,
    worker_working_state = 5,
    worker_worked_state = 6,
    grapher_state = 7,
    defaults_state = 8
} state_t;

typedef enum {
    NULL_event = 0,
    terminate_event = 1,
    hello_event = 2,
    rdy2wrk_event = 3,
    iamthegrapher_event = 4,
    turnonthenews_event = 5,
    ask_worker_to_read_event = 6,
    ruready_event = 7,
    pkginfo_event = 8,
    pkgdel_event = 9,
    didread_event = 10,
    roger_event = 11,
    process_pkgs_event = 12,
    send_myself_pkginfo_event = 13,
    send_myself_pkgdel_event = 14,
    plzreadall_event = 15,
    stablestatusplz_event = 16,
    send_stable_event = 17,
    send_unstable_event = 18,
    weseehash_event = 19,
    ping_event = 20,
    expired_event = 21
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "introduction",
    "pleasantries",
    "worker ready",
    "worker asked",
    "worker working",
    "worker worked",
    "grapher",
    "defaults"
};

static char *
s_event_name [] = {
    "(NONE)",
    "terminate",
    "HELLO",
    "RDY2WRK",
    "IAMTHEGRAPHER",
    "TURNONTHENEWS",
    "ask_worker_to_read",
    "RUREADY",
    "PKGINFO",
    "PKGDEL",
    "DIDREAD",
    "ROGER",
    "process_pkgs",
    "send_myself_pkginfo",
    "send_myself_pkgdel",
    "PLZREADALL",
    "STABLESTATUSPLZ",
    "send_stable",
    "send_unstable",
    "WESEEHASH",
    "PING",
    "expired"
};

//  ---------------------------------------------------------------------------
//  Context for the whole server task. This embeds the application-level
//  server context at its start (the entire structure, not a reference),
//  so we can cast a pointer between server_t and s_server_t arbitrarily.

typedef struct {
    server_t server;            //  Application-level server context
    zsock_t *pipe;              //  Socket to back to caller API
    zsock_t *router;            //  Socket to talk to clients
    zactor_t *auth;             //  Actor responsible for authentication
    int port;                   //  Server port bound to
    zloop_t *loop;              //  Reactor for server sockets
    pkgimport_msg_t *message;   //  Message received or sent
    zhash_t *clients;           //  Clients we're connected to
    zconfig_t *config;          //  Configuration tree
    uint client_id;             //  Client identifier counter
    size_t timeout;             //  Default client expiry timeout
    bool verbose;               //  Verbose logging enabled?
    char *log_prefix;           //  Default log prefix
} s_server_t;


//  ---------------------------------------------------------------------------
//  Context for each connected client. This embeds the application-level
//  client context at its start (the entire structure, not a reference),
//  so we can cast a pointer between client_t and s_client_t arbitrarily.

typedef struct {
    client_t client;            //  Application-level client context
    s_server_t *server;         //  Parent server context
    char *hashkey;              //  Key into server->clients hash
    zframe_t *routing_id;       //  Routing_id back to client
    uint unique_id;             //  Client identifier in server
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int wakeup;                 //  zloop timer for client alarms
    void *ticket;               //  zloop ticket for client timeouts
    event_t wakeup_event;       //  Wake up with this event
    char log_prefix [41];       //  Log prefix string
} s_client_t;

static int
    server_initialize (server_t *self);
static void
    server_terminate (server_t *self);
static zmsg_t *
    server_method (server_t *self, const char *method, zmsg_t *msg);
static void
    server_configuration (server_t *self, zconfig_t *config);
static int
    client_initialize (client_t *self);
static void
    client_terminate (client_t *self);
static void
    s_server_config_global (s_server_t *server);
static void
    s_client_execute (s_client_t *client, event_t event);
static int
    s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument);
static int
    s_client_handle_ticket (zloop_t *loop, int timer_id, void *argument);
static void
    ensure_all_configuration_is_complete (client_t *self);
static void
    set_grapher_bit (client_t *self);
static void
    get_new_pkgs_from_git (client_t *self);
static void
    assign_pending_work (client_t *self);
static void
    add_worker_to_queue (client_t *self);
static void
    route_pkginfo (client_t *self);
static void
    request_pkg_deletion (client_t *self);
static void
    pkg_read_finished (client_t *self);
static void
    remove_self_from_worker_list (client_t *self);
static void
    grab_tmppkg_and_process (client_t *self);
static void
    add_all_packages_to_the_queue (client_t *self);
static void
    respond_specifying_stability (client_t *self);
static void
    set_commithash (client_t *self);
static void
    ensure_git_consistency (client_t *self);
static void
    return_job_to_queue (client_t *self);

//  ---------------------------------------------------------------------------
//  These methods are an internal API for actions

//  Set the next event, needed in at least one action in an internal
//  state; otherwise the state machine will wait for a message on the
//  router socket and treat that as the event.

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

//  Set wakeup alarm after 'delay' msecs. The next state should
//  handle the wakeup event. The alarm is cancelled on any other
//  event.

static void
engine_set_wakeup_event (client_t *client, size_t delay, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (self->wakeup) {
            zloop_timer_end (self->server->loop, self->wakeup);
            self->wakeup = 0;
        }
        self->wakeup = zloop_timer (
            self->server->loop, delay, 1, s_client_handle_wakeup, self);
        self->wakeup_event = event;
    }
}

//  Execute 'event' on specified client. Use this to send events to
//  other clients. Cancels any wakeup alarm on that client.

static void
engine_send_event (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        s_client_execute (self, event);
    }
}

//  Execute 'event' on all clients known to the server. If you pass a
//  client argument, that client will not receive the broadcast. If you
//  want to pass any arguments, store them in the server context.

static void
engine_broadcast_event (server_t *server, client_t *client, event_t event)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        zlist_t *keys = zhash_keys (self->clients);
        char *key = (char *) zlist_first (keys);
        while (key) {
            s_client_t *target = (s_client_t *) zhash_lookup (self->clients, key);
            if (target != (s_client_t *) client)
                s_client_execute (target, event);
            key = (char *) zlist_next (keys);
        }
        zlist_destroy (&keys);
    }
}

//  Poll actor or zsock for activity, invoke handler on any received
//  message. Handler must be a CZMQ zloop_fn function; receives server
//  as arg.

static void
engine_handle_socket (server_t *server, void *sock, zloop_reader_fn handler)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        //  Resolve zactor_t -> zsock_t
        if (zactor_is (sock))
            sock = zactor_sock ((zactor_t *) sock);
        else
            assert (zsock_is (sock));
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, (zsock_t *) sock, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, (zsock_t *) sock);
        }
        else
            zloop_reader_end (self->loop, (zsock_t *) sock);
    }
}

//  Register monitor function that will be called at regular intervals
//  by the server engine. Returns an identifier that can be used to cancel it.

static int
engine_set_monitor (server_t *server, size_t interval, zloop_timer_fn monitor)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        int rc = zloop_timer (self->loop, interval, 0, monitor, self);
        assert (rc >= 0);
        return rc;
    }

    return -1;
}

//  Cancel the monitor function with the given identifier.
static void
engine_cancel_monitor (server_t *server, int identifier)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        int rc = zloop_timer_end (self->loop, identifier);
        assert (rc >= 0);
    }
}

//  Set log file prefix; this string will be added to log data, to make
//  log data more searchable. The string is truncated to ~20 chars.

static void
engine_set_log_prefix (client_t *client, const char *string)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        snprintf (self->log_prefix, sizeof (self->log_prefix),
            "%6d:%-33s", self->unique_id, string);
    }
}

//  Set a configuration value in the server's configuration tree. The
//  properties this engine uses are: server/verbose, server/timeout, and
//  server/background. You can also configure other abitrary properties.

static void
engine_configure (server_t *server, const char *path, const char *value)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        zconfig_put (self->config, path, value);
        s_server_config_global (self);
    }
}

//  Return true if server is running in verbose mode, else return false.

static bool
engine_verbose (server_t *server)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        return self->verbose;
    }
    return false;
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_send_event (NULL, NULL_event);
    engine_broadcast_event (NULL, NULL, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
    engine_set_monitor (NULL, 0, NULL);
    engine_cancel_monitor (NULL, 0);
    engine_set_log_prefix (NULL, NULL);
    engine_configure (NULL, NULL, NULL);
    engine_verbose (NULL);
}


//  ---------------------------------------------------------------------------
//  Generic methods on protocol messages
//  TODO: replace with lookup table, since ID is one byte

static event_t
s_protocol_event (pkgimport_msg_t *message)
{
    assert (message);
    switch (pkgimport_msg_id (message)) {
        case PKGIMPORT_MSG_HELLO:
            return hello_event;
            break;
        case PKGIMPORT_MSG_ROGER:
            return roger_event;
            break;
        case PKGIMPORT_MSG_RUREADY:
            return ruready_event;
            break;
        case PKGIMPORT_MSG_RDY2WRK:
            return rdy2wrk_event;
            break;
        case PKGIMPORT_MSG_PING:
            return ping_event;
            break;
        case PKGIMPORT_MSG_PLZREADALL:
            return plzreadall_event;
            break;
        case PKGIMPORT_MSG_IAMTHEGRAPHER:
            return iamthegrapher_event;
            break;
        case PKGIMPORT_MSG_STABLESTATUSPLZ:
            return stablestatusplz_event;
            break;
        case PKGIMPORT_MSG_TURNONTHENEWS:
            return turnonthenews_event;
            break;
        case PKGIMPORT_MSG_DIDREAD:
            return didread_event;
            break;
        case PKGIMPORT_MSG_PKGINFO:
            return pkginfo_event;
            break;
        case PKGIMPORT_MSG_PKGDEL:
            return pkgdel_event;
            break;
        case PKGIMPORT_MSG_WESEEHASH:
            return weseehash_event;
            break;
        default:
            //  Invalid pkgimport_msg_t
            return terminate_event;
    }
}


//  ---------------------------------------------------------------------------
//  Client methods

static s_client_t *
s_client_new (s_server_t *server, zframe_t *routing_id)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    assert (self);
    assert ((s_client_t *) &self->client == self);

    self->server = server;
    self->hashkey = zframe_strhex (routing_id);
    self->routing_id = zframe_dup (routing_id);
    self->unique_id = server->client_id++;
    engine_set_log_prefix (&self->client, server->log_prefix);

    self->client.server = (server_t *) server;
    self->client.message = server->message;

    //  If expiry timers are being used, create client ticket
    if (server->timeout)
        self->ticket = zloop_ticket (server->loop, s_client_handle_ticket, self);
    //  Give application chance to initialize and set next event
    self->state = introduction_state;
    self->event = NULL_event;
    client_initialize (&self->client);
    return self;
}

static void
s_client_destroy (s_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_client_t *self = *self_p;
        if (self->wakeup)
            zloop_timer_end (self->server->loop, self->wakeup);
        if (self->ticket)
            zloop_ticket_delete (self->server->loop, self->ticket);
        zframe_destroy (&self->routing_id);
        //  Provide visual clue if application misuses client reference
        engine_set_log_prefix (&self->client, "*** TERMINATED ***");
        client_terminate (&self->client);
        free (self->hashkey);
        free (self);
        *self_p = NULL;
    }
}

//  Callback when we remove client from 'clients' hash table
static void
s_client_free (void *argument)
{
    s_client_t *client = (s_client_t *) argument;
    s_client_destroy (&client);
}


//  Execute state machine as long as we have events

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup) {
        zloop_timer_end (self->server->loop, self->wakeup);
        self->wakeup = 0;
    }
    while (self->next_event > 0) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (self->server->verbose) {
            zsys_debug ("%s: %s:",
                self->log_prefix, s_state_name [self->state]);
            zsys_debug ("%s:     %s",
                self->log_prefix, s_event_name [self->event]);
        }
        switch (self->state) {
            case introduction_state:
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  ensure all configuration is complete
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ ensure all configuration is complete", self->log_prefix);
                        ensure_all_configuration_is_complete (&self->client);
                    }
                    if (!self->exception)
                        self->state = pleasantries_state;
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                break;

            case pleasantries_state:
                if (self->event == rdy2wrk_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = worker_ready_state;
                    if (!self->exception) {
                        //  add worker to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ add worker to queue", self->log_prefix);
                        add_worker_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                }
                else
                if (self->event == iamthegrapher_event) {
                    if (!self->exception) {
                        //  set grapher bit
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ set grapher bit", self->log_prefix);
                        set_grapher_bit (&self->client);
                    }
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = grapher_state;
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                }
                else
                if (self->event == turnonthenews_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  get new pkgs from git
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get new pkgs from git", self->log_prefix);
                        get_new_pkgs_from_git (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                }
                else
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = pleasantries_state;
                }
                else
                if (self->event == ping_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  remove self from worker list
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ remove self from worker list", self->log_prefix);
                        remove_self_from_worker_list (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception)
                        self->state = introduction_state;
                }
                break;

            case worker_ready_state:
                if (self->event == ask_worker_to_read_event) {
                    if (!self->exception) {
                        //  send PLZREAD
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send PLZREAD",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_PLZREAD);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = worker_asked_state;
                }
                else
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = pleasantries_state;
                }
                else
                if (self->event == ping_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  remove self from worker list
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ remove self from worker list", self->log_prefix);
                        remove_self_from_worker_list (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception)
                        self->state = introduction_state;
                }
                break;

            case worker_asked_state:
                if (self->event == ruready_event) {
                    if (!self->exception) {
                        //  send READY
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send READY",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_READY);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = worker_working_state;
                }
                else
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = pleasantries_state;
                }
                else
                if (self->event == ping_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  remove self from worker list
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ remove self from worker list", self->log_prefix);
                        remove_self_from_worker_list (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception)
                        self->state = introduction_state;
                }
                break;

            case worker_working_state:
                if (self->event == pkginfo_event) {
                    if (!self->exception) {
                        //  route pkginfo
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ route pkginfo", self->log_prefix);
                        route_pkginfo (&self->client);
                    }
                    if (!self->exception) {
                        //  send READY
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send READY",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_READY);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == pkgdel_event) {
                    if (!self->exception) {
                        //  request pkg deletion
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ request pkg deletion", self->log_prefix);
                        request_pkg_deletion (&self->client);
                    }
                }
                else
                if (self->event == didread_event) {
                    if (!self->exception) {
                        //  pkg read finished
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ pkg read finished", self->log_prefix);
                        pkg_read_finished (&self->client);
                    }
                    if (!self->exception) {
                        //  remove self from worker list
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ remove self from worker list", self->log_prefix);
                        remove_self_from_worker_list (&self->client);
                    }
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = worker_worked_state;
                }
                else
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = pleasantries_state;
                }
                else
                if (self->event == ping_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  remove self from worker list
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ remove self from worker list", self->log_prefix);
                        remove_self_from_worker_list (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception)
                        self->state = introduction_state;
                }
                break;

            case worker_worked_state:
                if (self->event == rdy2wrk_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = worker_ready_state;
                    if (!self->exception) {
                        //  add worker to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ add worker to queue", self->log_prefix);
                        add_worker_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                }
                else
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = pleasantries_state;
                }
                else
                if (self->event == ping_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  remove self from worker list
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ remove self from worker list", self->log_prefix);
                        remove_self_from_worker_list (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception)
                        self->state = introduction_state;
                }
                break;

            case grapher_state:
                if (self->event == roger_event) {
                }
                else
                if (self->event == process_pkgs_event) {
                    if (!self->exception) {
                        //  grab tmppkg and process
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ grab tmppkg and process", self->log_prefix);
                        grab_tmppkg_and_process (&self->client);
                    }
                }
                else
                if (self->event == send_myself_pkginfo_event) {
                    if (!self->exception) {
                        //  send PKGINFO
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send PKGINFO",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_PKGINFO);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  grab tmppkg and process
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ grab tmppkg and process", self->log_prefix);
                        grab_tmppkg_and_process (&self->client);
                    }
                }
                else
                if (self->event == send_myself_pkgdel_event) {
                    if (!self->exception) {
                        //  send PKGDEL
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send PKGDEL",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_PKGDEL);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  grab tmppkg and process
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ grab tmppkg and process", self->log_prefix);
                        grab_tmppkg_and_process (&self->client);
                    }
                }
                else
                if (self->event == plzreadall_event) {
                    if (!self->exception) {
                        //  add all packages to the queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ add all packages to the queue", self->log_prefix);
                        add_all_packages_to_the_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  get new pkgs from git
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get new pkgs from git", self->log_prefix);
                        get_new_pkgs_from_git (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                }
                else
                if (self->event == stablestatusplz_event) {
                    if (!self->exception) {
                        //  respond specifying stability
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ respond specifying stability", self->log_prefix);
                        respond_specifying_stability (&self->client);
                    }
                }
                else
                if (self->event == send_stable_event) {
                    if (!self->exception) {
                        //  set commithash
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ set commithash", self->log_prefix);
                        set_commithash (&self->client);
                    }
                    if (!self->exception) {
                        //  send STABLE
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send STABLE",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_STABLE);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == send_unstable_event) {
                    if (!self->exception) {
                        //  send UNSTABLE
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send UNSTABLE",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_UNSTABLE);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == weseehash_event) {
                    if (!self->exception) {
                        //  ensure git consistency
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ ensure git consistency", self->log_prefix);
                        ensure_git_consistency (&self->client);
                    }
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  get new pkgs from git
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get new pkgs from git", self->log_prefix);
                        get_new_pkgs_from_git (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                }
                else
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = pleasantries_state;
                }
                else
                if (self->event == ping_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  remove self from worker list
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ remove self from worker list", self->log_prefix);
                        remove_self_from_worker_list (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception)
                        self->state = introduction_state;
                }
                break;

            case defaults_state:
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = pleasantries_state;
                }
                else
                if (self->event == ping_event) {
                    if (!self->exception) {
                        //  send ROGER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ROGER",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_ROGER);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  remove self from worker list
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ remove self from worker list", self->log_prefix);
                        remove_self_from_worker_list (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send IFORGOTU
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send IFORGOTU",
                                self->log_prefix);
                        pkgimport_msg_set_id (self->server->message, PKGIMPORT_MSG_IFORGOTU);
                        zsys_debug ("%s: Send message to client", self->log_prefix);
                        pkgimport_msg_print (self->server->message);
                        pkgimport_msg_set_routing_id (self->server->message, self->routing_id);
                        pkgimport_msg_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  return job to queue
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ return job to queue", self->log_prefix);
                        return_job_to_queue (&self->client);
                    }
                    if (!self->exception) {
                        //  assign pending work
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ assign pending work", self->log_prefix);
                        assign_pending_work (&self->client);
                    }
                    if (!self->exception)
                        self->state = introduction_state;
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (self->server->verbose)
                zsys_debug ("%s:         ! %s",
                    self->log_prefix, s_event_name [self->exception]);

            self->next_event = self->exception;
        }
        if (self->next_event == terminate_event) {
            //  Automatically calls s_client_destroy
            zhash_delete (self->server->clients, self->hashkey);
            break;
        }
        else
        if (self->server->verbose)
            zsys_debug ("%s:         > %s",
                self->log_prefix, s_state_name [self->state]);
    }
}

//  zloop callback when client ticket expires

static int
s_client_handle_ticket (zloop_t *loop, int timer_id, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(timer_id);
    s_client_t *self = (s_client_t *) argument;
    self->ticket = NULL;        //  Ticket is now dead
    s_client_execute (self, expired_event);
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


//  Server methods

static void
s_server_config_global (s_server_t *self)
{
    //  Built-in server configuration options
    //
    //  If we didn't already set verbose, check if the config tree wants it
    if (!self->verbose
    && atoi (zconfig_get (self->config, "server/verbose", "0")))
        self->verbose = true;

    //  Default client timeout is 60 seconds
    self->timeout = atoi (
        zconfig_get (self->config, "server/timeout", "60000"));
    zloop_set_ticket_delay (self->loop, self->timeout);

    //  Do we want to run server in the background?
    int background = atoi (
        zconfig_get (self->config, "server/background", "0"));
    if (!background)
        zsys_set_logstream (stdout);
}

static s_server_t *
s_server_new (zsock_t *pipe)
{
    s_server_t *self = (s_server_t *) zmalloc (sizeof (s_server_t));
    assert (self);
    assert ((s_server_t *) &self->server == self);

    self->pipe = pipe;
    self->router = zsock_new (ZMQ_ROUTER);
    assert (self->router);
    //  By default the socket will discard outgoing messages above the
    //  HWM of 1,000. This isn't helpful for high-volume streaming. We
    //  will use a unbounded queue here. If applications need to guard
    //  against queue overflow, they should use a credit-based flow
    //  control scheme.
    zsock_set_unbounded (self->router);
    self->message = pkgimport_msg_new ();
    self->clients = zhash_new ();
    self->auth = NULL;
    self->config = zconfig_new ("root", NULL);
    self->loop = zloop_new ();
    srandom ((unsigned int) zclock_time ());
    self->client_id = randof (1000);
    zloop_set_verbose (self->loop, true);
    s_server_config_global (self);

    //  Initialize application server context
    self->server.pipe = self->pipe;
    self->server.config = self->config;
    server_initialize (&self->server);

    s_satisfy_pedantic_compilers ();
    return self;
}

static void
s_server_destroy (s_server_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_server_t *self = *self_p;
        pkgimport_msg_destroy (&self->message);
        //  Destroy clients before destroying the server
        zhash_destroy (&self->clients);
        server_terminate (&self->server);
        zsock_destroy (&self->router);
        zconfig_destroy (&self->config);
        zloop_destroy (&self->loop);
	zactor_destroy (&self->auth);
        free (self);
        *self_p = NULL;
    }
}

//  Apply service-specific configuration tree:
//   * apply server configuration
//   * print any echo items in top-level sections
//   * apply sections that match methods

static void
s_server_config_service (s_server_t *self)
{
    //  Apply echo commands and class methods
    zconfig_t *section = zconfig_locate (self->config, "pkgimport_server");
    if (section)
        section = zconfig_child (section);

    while (section) {
        if (streq (zconfig_name (section), "echo"))
            zsys_notice ("%s", zconfig_value (section));
        else
        if (streq (zconfig_name (section), "bind")) {
            char *endpoint = zconfig_get (section, "endpoint", "?");
            if (zsock_bind (self->router, "%s", endpoint) == -1)
                zsys_warning ("could not bind to %s (%s)", endpoint, zmq_strerror (zmq_errno ()));
        }
#if (ZMQ_VERSION_MAJOR >= 4)
        else
        if (streq (zconfig_name (section), "security")) {
            char *mechanism = zconfig_get (section, "mechanism", "null");
            char *domain = zconfig_get (section, "domain", NULL);
            if (streq (mechanism, "null")) {
                zsys_notice ("server is using NULL security");
                if (domain)
                    zsock_set_zap_domain (self->router, NULL);
            }
            else
            if (streq (mechanism, "plain")) {
                zsys_notice ("server is using PLAIN security");
                zsock_set_plain_server (self->router, 1);
            }
            else
                zsys_warning ("mechanism=%s is not supported", mechanism);
        }
#endif
        section = zconfig_next (section);
    }
    s_server_config_global (self);
}

//  Process message from pipe

static int
s_server_handle_pipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(reader);

    s_server_t *self = (s_server_t *) argument;
    zmsg_t *msg = zmsg_recv (self->pipe);
    if (!msg)
        return -1;              //  Interrupted; exit zloop
    zsys_debug ("API message:");
    zmsg_print (msg);

    char *method = zmsg_popstr (msg);
    if (self->verbose)
        zsys_debug ("%s:     API command=%s", self->log_prefix, method);

    if (streq (method, "VERBOSE"))
        self->verbose = true;
    else
    if (streq (method, "$TERM")) {
        //  Shutdown the engine
        zstr_free (&method);
        zmsg_destroy (&msg);
        return -1;
    }
    else
    if (streq (method, "BIND")) {
        //  Bind to a specified endpoint, which may use an ephemeral port
        char *endpoint = zmsg_popstr (msg);
        self->port = zsock_bind (self->router, "%s", endpoint);
        if (self->port == -1)
            zsys_warning ("could not bind to %s", endpoint);
        zstr_free (&endpoint);
    }
    else
    if (streq (method, "START AUTH")) {
        //  Begin the auth actor
	self->auth = zactor_new (zauth, NULL);
    }
    else
    if (streq (method, "AUTH")) {
        //  Send arguments to the auth actor
        zmsg_send (&msg, self->auth);
    }
    else
    if (streq (method, "CURVE")) {
        //  Given a private-public set of keys, set the server socket
	//  Auth needs to be created separately
        char *private_key = zmsg_popstr (msg);
        char *public_key = zmsg_popstr (msg);
	zsock_set_curve_secretkey (self->router, private_key);
	zsock_set_curve_publickey (self->router, public_key);
	zsock_set_curve_server (self->router, 1);
        zstr_free (&public_key);
        zstr_free (&private_key);
    }
    else
    if (streq (method, "PORT")) {
        //  Return PORT + port number from the last bind, if any
        zstr_sendm (self->pipe, "PORT");
        zstr_sendf (self->pipe, "%d", self->port);
    }
    else                       //  Deprecated method name
    if (streq (method, "LOAD") || streq (method, "CONFIGURE")) {
        char *filename = zmsg_popstr (msg);
        zconfig_destroy (&self->config);
        self->config = zconfig_load (filename);
        if (self->config) {
            s_server_config_service (self);
            self->server.config = self->config;
            server_configuration (&self->server, self->config);
        }
        else {
            zsys_warning ("cannot load config file '%s'", filename);
            self->config = zconfig_new ("root", NULL);
        }
        zstr_free (&filename);
    }
    else
    if (streq (method, "SET")) {
        char *path = zmsg_popstr (msg);
        char *value = zmsg_popstr (msg);
        zconfig_put (self->config, path, value);
        if (streq (path, "server/animate")) {
            zsys_warning ("'%s' is deprecated, use VERBOSE command instead", path);
            self->verbose = (atoi (value) == 1);
        }
        s_server_config_global (self);
        zstr_free (&value);
        zstr_free (&path);
    }
    else
    if (streq (method, "SAVE")) {
        char *filename = zmsg_popstr (msg);
        if (zconfig_save (self->config, filename))
            zsys_warning ("cannot save config file '%s'", filename);
        zstr_free (&filename);
    }
    else {
        //  Execute custom method
        zmsg_t *reply = server_method (&self->server, method, msg);
        //  If reply isn't null, send it to caller
        zmsg_send (&reply, self->pipe);
    }
    zstr_free (&method);
    zmsg_destroy (&msg);
    return 0;
}

//  Handle a protocol message from the client

static int
s_server_handle_protocol (zloop_t *loop, zsock_t *reader, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(reader);

    s_server_t *self = (s_server_t *) argument;
    //  We process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->router) & ZMQ_POLLIN) {
        int rc = pkgimport_msg_recv (self->message, self->router);
        if (rc == -1)
            return -1;      //  Interrupted; exit zloop


        //  TODO: use binary hashing on routing_id
        char *hashkey = zframe_strhex (pkgimport_msg_routing_id (self->message));
        s_client_t *client = (s_client_t *) zhash_lookup (self->clients, hashkey);
        if (client == NULL) {
            client = s_client_new (self, pkgimport_msg_routing_id (self->message));
            zhash_insert (self->clients, hashkey, client);
            zhash_freefn (self->clients, hashkey, s_client_free);
        }
        free (hashkey);
        zsys_debug ("%d: Client message", client->unique_id);
        pkgimport_msg_print (self->message);
        //  Any input from client counts as activity
        if (client->ticket)
            zloop_ticket_reset (self->loop, client->ticket);

        if (rc == -2) {
            continue;       //  Malformed, but malformed_event doesn't exist
                            //  -> discard the message
        }
        //  Pass to client state machine
        s_client_execute (client, s_protocol_event (self->message));
    }
    return 0;
}

//  Watch server config file and reload if changed

static int
s_watch_server_config (zloop_t *loop, int timer_id, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(timer_id);

    s_server_t *self = (s_server_t *) argument;
    if (zconfig_has_changed (self->config)
    &&  zconfig_reload (&self->config) == 0) {
        s_server_config_service (self);
        self->server.config = self->config;
        server_configuration (&self->server, self->config);
        zsys_notice ("reloaded configuration from %s",
            zconfig_filename (self->config));
    }
    return 0;
}


//  ---------------------------------------------------------------------------
//  This is the server actor, which polls its two sockets and processes
//  incoming messages

void
pkgimport_server (zsock_t *pipe, void *args)
{
    //  Initialize
    s_server_t *self = s_server_new (pipe);
    assert (self);
    zsock_signal (pipe, 0);
    //  Actor argument may be a string used for logging
    self->log_prefix = args? (char *) args: "";

    //  Set-up server monitor to watch for config file changes
    engine_set_monitor ((server_t *) self, 1000, s_watch_server_config);
    //  Set up handler for the two main sockets the server uses
    engine_handle_socket ((server_t *) self, self->pipe, s_server_handle_pipe);
    engine_handle_socket ((server_t *) self, self->router, s_server_handle_protocol);

    //  Run reactor until there's a termination signal
    zloop_start (self->loop);

    //  Reactor has ended
    s_server_destroy (&self);
}
