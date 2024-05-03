% HMMAP(3) HMSDK Programmer's Manuals
% Honggyu Kim <honggyu.kim@sk.com>, Yunjeong Mun <yunjeong.mun@sk.com>
% Apr, 2024

NAME
====
hmmap, hmunmap - map or unmap files or devices into heterogeneous memory


SYNOPSIS
========
**#include <hmalloc.h>**

**void \*hmmap(void \*_addr_, size_t _length_, int _prot_, int _flags_, int _fd_, off_t _offset_);** \
**int hmunmap(void \*_addr_, size_t _length_);**


DESCRIPTION
===========
**hmmap**() creates a new mapping in the virtual address space of the calling
process same as **mmap**(2) so the meaning of each argument can simply be refer
to the manual of **mmap**(2).

However, there are some differences between **hmmap** and **mmap(2)**.  The
**hmmap** allocates memory in **hmalloc pool** that can be optionally controlled
by **hmctl**(8) tool.


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
On success, **hmmap**() returns a pointer to the mapped area in **hmalloc pool**.

On error of **hmmap**() due to the internal **mmap**(2) fails, the value
**MAP_FAILED** (that is, _(void \*) -1_) is returned, and _errno_ is set by
**mmap**(2) to indicate the cause of the error.  If **mmap**(2) is successful
but **mbind**(2) fails, then it unmaps the area again and returns **NULL** with
_errno_ set by **mbind**(2).

On success, **hmunmap**() returns 0.  On failure, it returns -1, and _errno_ is
set to indicate the cause of the error (probably to **EINVAL**).


ERRORS
======
See **mmap**(2), **mbind**(2) and **munmap**(2)


SEE ALSO
========
**hmctl**(8), **mmap**(2), **mbind**(2), **munmap**(2)
