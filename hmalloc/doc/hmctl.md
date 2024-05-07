% HMCTL(8) Hmctl User Manuals
% Honggyu Kim <honggyu.kim@sk.com>
% Apr, 2024

NAME
====
hmctl - Control heterogeneous memory allocation policy


SYNOPSIS
========
hmctl [_options_] COMMAND [_command-options_]


DESCRIPTION
===========
The **hmctl** tool is to control heterogeneous memory allocation policy.  It
changes memory policy only for the area allocated by **hmalloc APIs**, which is
called as **hmalloc pool** then executes the given `COMMAND` program.  The rest
of memory regions other than **hmalloc pool** are not affected by this tool.

_nodes_ may be specified as N,N,N or N-N or N,N-N or N-N,N-N and so forth.
Please see its annotation details at **numactl**(8).


OPTIONS
=======
-m _nodes_, \--membind=_nodes_
:   Allocate memory only from _nodes_ for hmalloc family allocations.

-p _node_, \--preferred=_node_
:   Preferably allocate memory on _node_ for hmalloc family allocations.

-?, \--help
:   Print help message and list of options with description

\--usage
:   Print usage string


EXAMPLES
========
Let's say if the target test program allocates 512 MiB using **hmalloc**(3),
then this memory area can be allocated to the intended node as follows.

    # Allocate hmalloc area to node 2 with MPOL_BIND policy.
    $ hmctl -m 2 ./prog

    # Allocate hmalloc area to node 1 with MPOL_PREFERRED policy.
    $ hmctl -p 1 ./prog

If you want to use a different memory policy with global policy, then hmctl can
be used along with numactl as follows.

    # Set global policy to MPOL_PREFERRED for node 1,
    # but use MPOL_BIND for node 2 only to hmalloc area.
    $ numactl -p 1 hmctl -m 2 ./prog

    # Set global policy to MPOL_INTERLEAVE for node 1 and 2,
    # but use MPOL_PREFERRED for node 3 only to hmalloc area.
    $ numactl -i 1,2 hmctl -p 3 ./prog


GLOSSARY
========
HMALLOC APIS
------------
The **hmalloc APIs** are heterogeneous memory allocation APIs provided by
**libhmalloc.so** such as **hmalloc**(3), **hcalloc**(3),
**hposix_memalign**(3), **hmmap**(3), etc.  All the APIs defined in
**hmalloc.h** are **hmalloc APIs**.

HMALLOC POOL
------------
The **hmalloc pool** is specially managed memory areas that can be optionally
controlled by **hmctl**(8) tool.
If target programs allocate memory using **hmalloc APIs**, then this area is
mapped as **hmalloc pool**.  This **hmalloc pool** has no effect if the target
program runs without **hmctl**(8), but if it runs with **hmctl**(8) attached,
then the memory policy of this area can be changed based on the usage of
**hmctl**(8).

HMCTL
-----
The **hmctl**(8) is a tool that controls heterogeneous memory allocation policy.
That means it can change the memory policy of **hmalloc pool** allocated by
**hmalloc APIs** internally using **mmap**(2) and **mbind**(2).
If **hmctl**(8) is attached and **-m**/**--membind** or **-p**/**--preferred**
option is given with a valid NUMA node ID, then the **hmalloc pool** memory is
allocated from the target node with the given memory policy based on the usage
of **hmctl**(8).


SEE ALSO
========
**numactl**(8), **hmalloc**(3)
