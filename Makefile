CONTIKI_PROJECT = udp-node
all: $(CONTIKI_PROJECT)
APPS=servreg-hack
PROJECT_SOURCEFILES += uart-module.c
CONTIKI = ../../contiki
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
