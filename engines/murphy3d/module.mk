MODULE := engines/murphy3d

MODULE_OBJS = \
	murphy3d.o \
	console.o \
	archive.o \
	item.o \
	font.o \
	ptf_decoder.o \
	renderer.o \
	shader.o \
	sqz.o \
	texture.o \
	uakm_map.o \
	map.o \
	location.o \
	metaengine.o

# This module can be built as a plugin
ifeq ($(ENABLE_MURPHY3D), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
