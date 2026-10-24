/* GPL-3.0. Shared host options and large-bag navigation, no ROM required. */
#include "../core/pd_menu.h"
#include "../core/pd_ui_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t pixels[320 * 240], baseline[320 * 240];
static void tap(struct pd_ui *ui, int id) {
    for (int i = 0; i < ui->hitCount; i++) {
        struct pd_hit h = ui->hits[i];
        if (h.id != id) continue;
        pd_ui_touch(ui, h.x + h.w / 2, h.y + h.h / 2, true);
        pd_ui_touch(ui, h.x + h.w / 2, h.y + h.h / 2, false);
        return;
    }
    assert(!"missing hit");
}
static bool has(struct pd_ui *ui, int id) {
    for (int i = 0; i < ui->hitCount; i++)
        if (ui->hits[i].id == id) return true;
    return false;
}
int main(void) {
    struct pd_canvas c;
    pd_canvas_init(&c, 320, 240, pixels);
    struct pd_menu menu;
    pd_menu_init(&menu, "Test", NULL, 0);
    assert(!menu.emptyHint && !menu.emptyPath);
    pd_menu_draw(&menu, &c);
    memcpy(baseline, pixels, sizeof pixels);
    menu.emptyHint = "Custom host";
    menu.emptyPath = "";
    pd_menu_draw(&menu, &c);
    assert(memcmp(baseline, pixels, sizeof pixels));
    pd_menu_init(&menu, "Test", NULL, 0);
    pd_menu_draw(&menu, &c);
    assert(!memcmp(baseline, pixels, sizeof pixels));
    struct pd_settings settings = {.ffSpeed = 2};
    struct pd_ui ui;
    pd_ui_init(&ui, &settings);
    ui.tab = PD_TAB_SETTINGS;
    struct pd_game game = {0};
    struct pd_snapshot snap = {0};
    struct pd_host_info host = {0};
    pd_ui_draw(&ui, &c, &game, &snap, &host);
    assert(has(&ui, HIT_SETTING + SETTING_SCREEN));
    tap(&ui, HIT_SETTING + SETTING_SCREEN);
    assert(ui.overlay == PD_OVERLAY_PICK_SCREEN);
    pd_ui_back(&ui);
    host.hideScreenMode = true;
    pd_ui_draw(&ui, &c, &game, &snap, &host);
    assert(!has(&ui, HIT_SETTING + SETTING_SCREEN));
    tap(&ui, HIT_SETTING + SETTING_FF);
    assert(ui.overlay == PD_OVERLAY_PICK_FF);
    pd_ui_back(&ui);
    pd_ui_draw(&ui, &c, &game, &snap, &host);
    tap(&ui, HIT_SETTING + SETTING_SAVE_STATE);
    assert(ui.overlay == PD_OVERLAY_CONFIRM_SAVE);
    puts("PASS host defaults, menu hints, screen capability and large-bag/settings hit isolation");
    return 0;
}
