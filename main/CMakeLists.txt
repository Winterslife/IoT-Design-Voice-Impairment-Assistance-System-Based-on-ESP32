idf_component_register(
    SRC_DIRS
        "."
        "app"
        "ui_zjm"
        "../components/bsp_extra/src"
    INCLUDE_DIRS
    "."
        "app"
        "../components/bsp_extra/include"
        "ui_zjm"
)

# spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
