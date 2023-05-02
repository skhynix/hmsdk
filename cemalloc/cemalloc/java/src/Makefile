# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

JC = javac

.SUFFIXES: .java .class
.java.class:
	$(JC) -d ../ $*.java

CLASSES = Cemalloc.java
default: classes

classes: $(CLASSES:.java=.class)

clean:
	rm -rf ../cemalloc
