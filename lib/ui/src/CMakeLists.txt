SET(SOURCES ui_ScreenScreen1.c
    ui_ScreenScreen2.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    ui_img_rightturn_png.c
    ui_img_20241025_110411_0000_png.c
    ui_img_20241025_110411_0001_png.c
    ui_img_dog_0_png.c
    ui_img_dog_1_png.c
    ui_img_dog_2_png.c
    ui_img_h_20250228_131139_0000_png.c
    fonts/ui_font_Arial12.c
    fonts/ui_font_Arial14.c)

add_library(ui ${SOURCES})
