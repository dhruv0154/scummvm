MODULE := engines/amber

MODULE_OBJS = \
	amber.o \
	decoders.o \
	archive.o \
	console.o \
	amiga.o \
	font.o \
	ui.o \
	cursor.o \
	character_creater.o \
	metaengine.o

# This module can be built as a plugin
ifeq ($(ENABLE_AMBER), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
