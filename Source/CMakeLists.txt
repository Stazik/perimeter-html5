ADD_SUBDIRECTORY("XPrm")
ADD_SUBDIRECTORY("Scripts")
ADD_SUBDIRECTORY("XTool")
ADD_SUBDIRECTORY("PluginMAX")
ADD_SUBDIRECTORY("Terra")
ADD_SUBDIRECTORY("tx3d")
ADD_SUBDIRECTORY("Render")
ADD_SUBDIRECTORY("Sound")
ADD_SUBDIRECTORY("Network")
ADD_SUBDIRECTORY("Util")
ADD_SUBDIRECTORY("Units")
ADD_SUBDIRECTORY("UserInterface")
ADD_SUBDIRECTORY("Physics")
ADD_SUBDIRECTORY("AI")
ADD_SUBDIRECTORY("GeoLogic")
ADD_SUBDIRECTORY("HT")

IF(PERIMETER_EXODUS)
    ADD_SUBDIRECTORY("Exodus")
ENDIF()

if(NOT OPTION_FINAL_VERSION)
    ADD_SUBDIRECTORY("TriggerEditor")
ENDIF()

add_executable(perimeter ${PERIMETER_EXE_FLAGS})
ADD_SUBDIRECTORY("Game")
