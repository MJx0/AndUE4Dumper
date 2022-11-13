LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := UE4Dump3r_$(TARGET_ARCH)

LOCAL_CPPFLAGS += -std=c++17 -I$(LOCAL_PATH)/includes -D_EXECUTABLE

LOCAL_SRC_FILES := src/executable.cpp src/KittyCmdln.cpp src/Core/GameProfiles/GameProfile.cpp \
$(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/src/Core/*.cpp)) \
$(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/includes/KittyMemory/*.cpp)) \
includes/fmt/format.cc

include $(BUILD_EXECUTABLE)