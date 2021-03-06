# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# definitions shared by host_executable.make and host_static_library.make
#

# the directory where we're going to place our object files
LOCAL_OBJS_DIR  := $(call intermediates-dir-for,EXECUTABLES,$(LOCAL_MODULE))
LOCAL_OBJECTS   :=
$(call local-host-define,CC)
$(call local-host-define,LD)
LOCAL_C_SOURCES := $(filter  %.c,$(LOCAL_SRC_FILES))
LOCAL_GENERATED_C_SOURCES := $(filter %.c,$(LOCAL_GENERATED_SOURCES))
LOCAL_GENERATED_CXX_SOURCES := $(filter %$(LOCAL_CPP_EXTENSION),$(LOCAL_GENERATED_SOURCES))
LOCAL_CXX_SOURCES := $(filter %$(LOCAL_CPP_EXTENSION),$(LOCAL_SRC_FILES))
LOCAL_OBJC_SOURCES := $(filter %.m,$(LOCAL_SRC_FILES))

LOCAL_CFLAGS := $(strip $(patsubst %,-I%,$(LOCAL_C_INCLUDES)) $(LOCAL_CFLAGS))

# HACK ATTACK: For the Darwin x86 build, we need to add
# '-read_only_relocs suppress' to the linker command to avoid errors.
# The only way to detect if we're building 32-bit or 64-bit binaries
# is to look at -m32 and -m64 statements in the final link line, and
# only keep the last one.
ifeq ($(HOST_OS),darwin)
  my_bitness := $(lastword $(filter -m32 -m64,$(LOCAL_LDFLAGS) $(LOCAL_LDLIBS) $(LOCAL_CFLAGS)))
  ifeq (-m32,$(my_bitness))
    LOCAL_LDLIBS += -Wl,-read_only_relocs,suppress
  endif
endif

$(foreach src,$(LOCAL_C_SOURCES), \
    $(eval $(call compile-c-source,$(src))) \
)

$(foreach src,$(LOCAL_GENERATED_C_SOURCES), \
    $(eval $(call compile-generated-c-source,$(src))) \
)

$(foreach src,$(LOCAL_GENERATED_CXX_SOURCES), \
    $(eval $(call compile-generated-cxx-source,$(src))) \
)

$(foreach src,$(LOCAL_CXX_SOURCES), \
    $(eval $(call compile-cxx-source,$(src))) \
)

$(foreach src,$(LOCAL_OBJC_SOURCES), \
    $(eval $(call compile-objc-source,$(src))) \
)

# Ensure that we build all generated sources before the objects
$(LOCAL_OBJECTS): | $(LOCAL_GENERATED_SOURCES) $(LOCAL_ADDITIONAL_DEPENDENCIES)

CLEAN_OBJS_DIRS += $(LOCAL_OBJS_DIR)
