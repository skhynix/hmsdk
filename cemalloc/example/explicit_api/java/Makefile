# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

JC = javac

.SUFFIXES: .java .class
.java.class:
	$(JC) $*.java

CLASSES = Example.java
default: classes

classes: $(CLASSES:.java=.class)

clean:
	rm -f Example.class
