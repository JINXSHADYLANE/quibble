LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

# LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -DTRACK_MEMORY -DNO_DEVMODE -DSTBI_NO_STDIO -DSTBI_NO_HDR -DVFONT_STBTT -DNDEBUG -Wall -std=c99
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/dgreed

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.cpp \
	app/ai.c \
	app/characters.c \
	app/devmode.c \
	app/fadeout.c \
	app/game.c \
	app/game_over.c \
	app/hud.c \
	app/in_app.c \
	app/item_unlock.c \
	app/level_select.c \
	app/levels.c \
	app/mchains.c \
	app/minimap.c \
	app/morka.c \
	app/obj_bomb.c \
	app/obj_cactus.c \
	app/obj_deco.c \
	app/obj_eraser.c \
	app/obj_fall_trigger.c \
	app/obj_floater.c \
	app/obj_ground.c \
	app/obj_mushroom.c \
	app/obj_particle_anchor.c \
	app/obj_powerup.c \
	app/obj_rabbit.c \
	app/obj_speed_trigger.c \
	app/obj_trampoline.c \
	app/objects.c \
	app/pause.c \
	app/placement.c \
	app/shop.c \
	app/tutorials.c \
	app/worldgen.c \
	dgreed/lib9-utf/rune.c \
	dgreed/lib9-utf/runetype.c \
	dgreed/lib9-utf/utfecpy.c \
	dgreed/lib9-utf/utflen.c \
	dgreed/lib9-utf/utfnlen.c \
	dgreed/lib9-utf/utfrrune.c \
	dgreed/lib9-utf/utfrune.c \
	dgreed/lib9-utf/utfutf.c \
	dgreed/malka/lua/lapi.c  \
	dgreed/malka/lua/lcode.c \
	dgreed/malka/lua/ldebug.c \
	dgreed/malka/lua/ldo.c \
	dgreed/malka/lua/ldump.c \
	dgreed/malka/lua/lfunc.c \
	dgreed/malka/lua/lgc.c \
	dgreed/malka/lua/llex.c \
	dgreed/malka/lua/lmem.c \
	dgreed/malka/lua/lobject.c \
	dgreed/malka/lua/lopcodes.c \
	dgreed/malka/lua/lparser.c \
	dgreed/malka/lua/lstate.c \
	dgreed/malka/lua/lstring.c \
	dgreed/malka/lua/ltable.c \
	dgreed/malka/lua/ltm.c \
	dgreed/malka/lua/lundump.c \
	dgreed/malka/lua/lvm.c \
	dgreed/malka/lua/lzio.c \
	dgreed/malka/lua/lauxlib.c \
	dgreed/malka/lua/lbaselib.c \
	dgreed/malka/lua/ldblib.c \
	dgreed/malka/lua/liolib.c \
	dgreed/malka/lua/lmathlib.c \
	dgreed/malka/lua/loslib.c \
	dgreed/malka/lua/lstrlib.c \
	dgreed/malka/lua/ltablib.c \
	dgreed/malka/lua/loadlib.c \
	dgreed/malka/lua/linit.c \
	dgreed/malka/lua/bit.c \
	dgreed/malka/malka.c \
	dgreed/malka/profiler.c \
	dgreed/malka/ml_utils.c \
	dgreed/malka/ml_system.c \
	dgreed/malka/ml_states.c \
	dgreed/malka/ml_sprsheet.c \
	dgreed/malka/ml_os.c \
	dgreed/malka/ml_mml.c \
	dgreed/malka/ml_mfx.c \
	dgreed/malka/ml_localization.c \
	dgreed/malka/ml_keyval.c \
	dgreed/malka/ml_iap.c \
	dgreed/malka/ml_gamecenter.c \
	dgreed/malka/ml_coldet.c \
	dgreed/malka/ml_anim.c \
	dgreed/malka/lua_utf8.c \
	dgreed/sys_misc_android.c \
	dgreed/sys_snd_android.c \
	dgreed/sys_gfx_gles2.c \
	dgreed/stb_image.c \
	dgreed/utils.c \
	dgreed/darray.c \
	dgreed/datastruct.c \
	dgreed/async.c \
	dgreed/mml.c \
	dgreed/keyval.c \
	dgreed/memory.c \
	dgreed/mempool.c \
	dgreed/memlin.c \
	dgreed/wav.c \
	dgreed/coldet.c \
	dgreed/anim.c \
	dgreed/font.c \
	dgreed/gfx_utils.c \
	dgreed/gui.c \
	dgreed/image.c \
	dgreed/localization.c \
	dgreed/lz4.c \
	dgreed/lz4hc.c \
	dgreed/mfx.c \
	dgreed/miniz.c \
	dgreed/particles.c \
	dgreed/sprsheet.c \
	dgreed/tilemap.c \
	dgreed/touch_ctrls.c \
	dgreed/tweakables.c \
	dgreed/uidesc.c \
	dgreed/vfont.c \
	dgreed/vfont_stbtt.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
