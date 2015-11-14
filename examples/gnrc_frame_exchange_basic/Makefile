# This is a very simple application to test the exchange (sending and
#  receiving) frames with the 'gnrc_802154_basic_mac' layer
APPLICATION = gnrc_frame_exchange_basic

# If no BOARD is found in the environment, use this default:
#  (we better use some actual mote with 802.15.4 radio transceiver)
BOARD ?= iotlab-m3
# Note: telosb, z1 and wsn430 would be nice too,
#  once the CC2420 driver for "gnrc" netstack is ready...

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Include the 'gnrc_802154_basic_mac' layer
USEMODULE += gnrc_802154_basic_mac
USEMODULE += gnrc_netif_default
# This application dumps received packets to STDIO using the pktdump module
USEMODULE += gnrc_pktdump
# Use the shell as user interface, with some basic shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
# Remove the 'nomac' module that may have been imported
ifneq (,$(filter gnrc_nomac,$(USEMODULE)))
  USEMODULE -= gnrc_nomac
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
