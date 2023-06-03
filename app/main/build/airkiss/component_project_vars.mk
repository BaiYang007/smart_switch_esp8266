# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(IDF_PATH)/app/components/airkiss/include $(IDF_PATH)/app/components/airkiss/airkiss/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/airkiss $(IDF_PATH)/app/components/airkiss/lib/libairkiss.a  
COMPONENT_LINKER_DEPS += $(IDF_PATH)/app/components/airkiss/lib/libairkiss.a
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += airkiss
COMPONENT_LDFRAGMENTS += 
component-airkiss-build: 
