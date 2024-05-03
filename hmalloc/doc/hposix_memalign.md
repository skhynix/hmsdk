% HPOSIX_MEMALIGN(3) HMSDK Programmer's Manuals
% Honggyu Kim <honggyu.kim@sk.com>, Yunjeong Mun <yunjeong.mun@sk.com>
% Apr, 2024

NAME
====
hposix_memalign, haligned_alloc - allocate aligned heterogeneous memory


SYNOPSIS
========
**#include <hmalloc.h>**

**int hposix_memalign(void \*\*_memptr_, size_t _alignment_, size_t _size_);** \
**void \*haligned_alloc(size_t _alignment_, size_t _size_);**


DESCRIPTION
===========
The function **hposix_memalign**() allocates _size_ bytes and places the address
of the allocated memory in _\*memptr_ from **hmalloc pool** that can be
optionally controlled by **hmctl**(8) tool.

The behaviors of **hposix_memalign** is same as **posix_memalign**(3) so the
address of the allocated memory will be a multiple of _alignment_,
which must be a power of two and a multiple of _sizeof(void \*)_.  This address
can later be successfully passed to **hfree**(3).  Even if _size_ is 0, then the
value placed in _\*memptr_ is a unique pointer value that should be deallocated
by **hfree**(3).

The function **haligned_alloc**() works same as **aligned_alloc**(3) so the
returned pointer will be a multiple of _alignment_, which must be a power of
two.  This pointer should be deallocated by **hfree**(3) later.


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
**hposix_memalign**() returns zero on success, or one of the error values listed
in the **ERRORS** section on failure.  The value of _errno_ is not set.

**haligned_alloc**() returns a pointer to the allocated memory on success.  On
error, NULL is returned, and _errno_ is set to indicate the cause of the error.


ERRORS
======
**EINVAL** The _alignment_ argument was not a power of two, or was not a
multiple of _sizeof(void \*)_.

**ENOMEM** There was insufficient memory to fulfill the allocation request from
**hmalloc pool**.


SEE ALSO
========
**hmctl**(8), **hfree**(3), **posix_memalign**(3), **aligned_alloc**(3)
