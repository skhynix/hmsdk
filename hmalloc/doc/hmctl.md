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
The hmctl tool is to control heterogeneous memory allocation policy for hmalloc
family allocation APIs provided by libhmalloc.so library.  For simplicity, the
hmalloc family allocation APIs will be called as hmalloc APIs below.

The hmctl changes memory policy only for the area allocated by hmalloc APIs then
executes the given `COMMAND` program, which is linked with libhmalloc.so at
build time.  The rest of memory region other than hmalloc area is not affected
by this tool.


OPTIONS
=======
-m _node_, \--membind=_node_
:   Allocate memory only from _node_ for hmalloc family allocations.

-p _node_, \--preferred=_node_
:   Preferably allocate memory on _node_ for hmalloc family allocations.

-?, \--help
:   Print help message and list of options with description

\--usage
:   Print usage string


EXAMPLES
========
Let's say if the target test program allocates 512 MiB using hmalloc(), then
this memory area can be allocated to the intended node as follows.

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


SEE ALSO
========
`numactl`(8)
