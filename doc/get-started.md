# Quick Start

You need to obtain the source, build the source, and then spin up dxpb.

First step: `git clone https://gitlab.com/Vaelatern/dxpb.git`

Second step: `cd dxpb; cd src; make; cd -`

Third, you need to spin up dxpb. A small cluster operating on only your
computer can be spun up with `runsvdir -P ./runsv/`.

## Git setup

It is necessary to clone void-packages to several places to make things work
correctly. To ensure that any repo clones are meant to be used by dxpb, dxpb
will not pull from "origin", but rather from "dxpb-remote". Please adjust
installation scripts accordingly.

### Where to go from here

The scripts under ./runsv can be modified or removed to alter functionality as
needed in your use case.
