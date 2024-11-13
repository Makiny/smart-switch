file(GLOB_RECURSE SRC_FILES "../device/*/*.c" "../device/*.c")

idf_component_register(SRCS ./app_main.c ${SRC_FILES}
                        INCLUDE_DIRS "." "../device/drivers/include" "../device/wifi_manage/include" "../device/platform/include"
                            "../device/hal/include" "../device/device_manage/include" "../device")
