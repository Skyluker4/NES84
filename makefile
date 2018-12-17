# ----------------------------
# Set NAME to the program name
# Set ICON to the png icon file name
# Set DESCRIPTION to display within a compatible shell
# Set COMPRESSED to "YES" to create a compressed program
# ----------------------------

NAME        ?= NCES84
COMPRESSED  ?= YES
ICON        ?= icon.png
DESCRIPTION ?= "NES emulator for TI-84+ CE"

# ----------------------------

include $(CEDEV)/include/.makefile
