idf_component_register(
    SRCS "display_manager.c" "ssd1306.c"
    INCLUDE_DIRS "include"
    REQUIRES driver game_logic
)
