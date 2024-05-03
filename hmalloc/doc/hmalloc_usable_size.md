% HMALLOC_USABLE_SIZE(3) HMSDK Programmer's Manuals
% Honggyu Kim <honggyu.kim@sk.com>, Yunjeong Mun <yunjeong.mun@sk.com>
% Apr, 2024

NAME
====
hmalloc_usable_size - obtain size of block of memory allocated from hmalloc pool


SYNOPSIS
========
**#include <hmalloc.h>**

**size_t hmalloc_usable_size (void \*_ptr_);**


DESCRIPTION
===========
The **hmalloc_usable_size**() function returns the number of usable bytes in the
block pointed to by _ptr_, a pointer to a block of memory allocated by
**hmalloc APIs** such as **hmalloc**(3).


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
**hmalloc_usable_size**() returns the number of usable bytes in the block of
allocated memory pointed to by _ptr_.  If _ptr_ is NULL, 0 is returned.


SEE ALSO
========
**hmctl**(8), **hmalloc**(3), **malloc_usable_size**(3)
