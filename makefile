# ----------------------------
# Makefile Options
# ----------------------------

NAME = EZASM
ICON = icon.png
DESCRIPTION = "CE C Toolchain Demo"
COMPRESSED = YES
COMPRESSED_MODE = zx7

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
