LOCAL_PATH := $(call my-dir)

KITTYMEMORY_PATH = $(LOCAL_PATH)/../../deps/KittyMemoryEx/KittyMemoryEx
KITTYMEMORY_SRC  = $(subst $(LOCAL_PATH)/,,$(wildcard $(KITTYMEMORY_PATH)/*.cpp))

include $(CLEAR_VARS)

LOCAL_MODULE := UE4Dump3r_$(TARGET_ARCH)

LOCAL_CPPFLAGS += -std=c++17 -DkNO_KEYSTONE -D_EXECUTABLE

LOCAL_C_INCLUDES += $(KITTYMEMORY_PATH) $(LOCAL_PATH)/includes

LOCAL_SRC_FILES := src/executable.cpp src/KittyCmdln.cpp src/Core/GameProfiles/GameProfile.cpp \
$(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/src/Core/*.cpp)) \
$(KITTYMEMORY_SRC) \
includes/fmt/format.cc

LOCAL_LDLIBS += -llog

include $(BUILD_EXECUTABLE)