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
	main_menu.o \
	text.o \
	control.o \
	button.o \
	label.o \
	animated_cursor.o \
	elevation.o \
	ambient_audio.o \
    anim_base.o \
    animation_controller.o \
    caption.o \
    game_controller.o \
	game_base.o \
	hint.o \
    location_module.o \
	module_base.o \
    module_controller.o \
    sound.o \
	script_base.o \
	script_state.o \
    uakm_game.o \
    uakm_script.o \
    wave.o
	metaengine.o

# This module can be built as a plugin
ifeq ($(ENABLE_MURPHY3D), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
