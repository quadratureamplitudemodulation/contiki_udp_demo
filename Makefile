CONTIKI_PROJECT = udp-node
CONTIKI_PROJECT = udp-root
all: $(CONTIKI_PROJECT)
APPS=servreg-hack
PROJECT_SOURCEFILES += uart-module.c
CONTIKI = ../../contiki
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
