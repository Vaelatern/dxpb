# Consistency

Always one of the hardest things when one thread becomes two, and then when one
program/machine becomes two. How do we make sure that:

1) No builder works on a task without the master's knowledge?
2) The master knows when any builder leaves?

For that we do two things.
1) Each builder will os.Exit(0) when the one and only websocket connection is
closed. This ensures only one server for any builder.
2) The server will idle, insted of by waiting for a build trigger, by waiting
for the slave to respond to a ping. This ensures that the moment the slave
vanishes (dies), the server gets a cancelled context and can handle the loss.
