LOCAL_PATH := $(call my-dir)

KITTYMEMORY_PATH = $(LOCAL_PATH)/../../deps/KittyMemoryEx/KittyMemoryEx
KITTYMEMORY_SRC  = $(subst $(LOCAL_PATH)/,,$(wildcard $(KITTYMEMORY_PATH)/*.cpp))

DEPS_PATH = $(LOCAL_PATH)/../../deps
DEPS_SRC  = $(DEPS_PATH)/fmt/format.cc $(KITTYMEMORY_SRC)

include $(CLEAR_VARS)

LOCAL_MODULE := shared_UEDump3r_$(TARGET_ARCH)

LOCAL_CPPFLAGS += -fexceptions -std=c++17 -DkNO_KEYSTONE

LOCAL_C_INCLUDES += $(KITTYMEMORY_PATH) $(DEPS_PATH)

LOCAL_SRC_FILES := library.cpp \
$(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/Utils/*.cpp)) \
$(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/Core/*.cpp)) \
$(subst $(LOCAL_PATH)/,,$(DEPS_SRC))

LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)