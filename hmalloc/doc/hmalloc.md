% HMALLOC(3) HMSDK Programmer's Manuals
% Honggyu Kim <honggyu.kim@sk.com>, Yunjeong Mun <yunjeong.mun@sk.com>
% Apr, 2024

NAME
====
hmalloc, hfree, hcalloc, hrealloc - allocate and free heterogeneous dynamic memory


SYNOPSIS
========
**#include <hmalloc.h>**

**void \*hmalloc(size_t _size_);** \
**void hfree(void _\*ptr_);** \
**void \*hcalloc(size_t _nmemb_, size_t _size_);** \
**void \*hrealloc(void _\*ptr_, size_t _size_);**


DESCRIPTION
===========
The **hmalloc**() function allocates dynamic memory managed in **hmalloc pool**
that can be optionally controlled by **hmctl**(8) tool and this can make those
dynamic memory be allocated on a specific NUMA node.  Except for this, hmalloc
works exactly same as **malloc**(3).

Likewise, **hcalloc**() and **hrealloc**() work exactly same as **calloc**(3)
and **realloc**(3), but the allocated memory can be optionally managed by
**hmctl**(8) tool just like **hmalloc**().

All those dynamic memory allocated by **hmalloc**(), **hcalloc**(), and
**hrealloc**() should be freed by **hfree**().


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


RETURN VALUE
============
The return values of **hmalloc**, **hcalloc**, and **hrealloc** are same as
**malloc**(3), **calloc**(3), and **hrealloc**(3).


ERRORS
======
Likewise, **hmalloc**(), **hcalloc**(), and **hrealloc**() can fail with the
following error:

**ENOMEM** Out of memory.  Possibly, the application hit the **RLIMIT_AS** or
**RLIMIT_DATA** limit described in **getrlimit**(2).


SEE ALSO
========
**hmctl**(8), **malloc**(3), **free**(3), **calloc**(3), **realloc**(3)
