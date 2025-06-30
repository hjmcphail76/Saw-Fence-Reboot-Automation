SET(SOURCES components/ui_comp_imgbutton1.c
    components/ui_comp.c
    screens/ui_SplashScreen.c
    screens/ui_MainControlScreen.c
    screens/ui_SettingsScreen.c
    screens/ui_ParameterEditScreen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_988892695.c)

add_library(ui ${SOURCES})
