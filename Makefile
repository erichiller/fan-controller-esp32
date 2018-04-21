#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
# see: http://esp-idf.readthedocs.io/en/latest/api-guides/build-system.html
# 

PROJECT_NAME := fan-controller-esp32


include $(IDF_PATH)/make/project.mk
CFLAGS += -D U8X8_USE_PINS
