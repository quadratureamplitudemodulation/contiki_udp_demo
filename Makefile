CONTIKI_PROJECT = udp-demo
all: $(CONTIKI_PROJECT)
APPS=servreg-hack
CONTIKI = ../../contiki
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
