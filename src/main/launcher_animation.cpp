#include "zelda_launcher.h"
#include <atomic>

struct TitleElement {
    recompui::Element *wrapper = nullptr;
    recompui::Label *title = nullptr;
    recompui::Label *subtitle = nullptr;
    recompui::Label *note = nullptr;
};

struct LauncherContext {
    TitleElement oot_title;
    TitleElement mm_title;
    recompui::Element *bg_mask = nullptr;
} launcher_context;

TitleElement create_title(recompui::ContextId context, recompui::Element *parent, const std::string &title, const std::string &subtitle, const std::string &note, bool left_alignment) {
    TitleElement e;

    e.wrapper = context.create_element<recompui::Element>(parent);
    e.wrapper->set_position(recompui::Position::Absolute);
    e.wrapper->set_top(25.0f, recompui::Unit::Percent);
    e.wrapper->set_left(left_alignment ? 0.0f : 60.0f, recompui::Unit::Percent); // TODO: Needs to use proper left alignment instead.
    e.wrapper->set_width_auto();
    e.wrapper->set_height_auto();

    e.title = context.create_element<recompui::Label>(e.wrapper, title, recompui::theme::Typography::Header2);
    e.title->set_color(recompui::theme::color::Text);

    e.subtitle = context.create_element<recompui::Label>(e.wrapper, subtitle, recompui::theme::Typography::Header1);
    e.subtitle->set_color(recompui::theme::color::Text);

    if (!note.empty()) {
        e.note = context.create_element<recompui::Label>(e.wrapper, note, recompui::theme::Typography::LabelMD);
        e.note->set_color(recompui::theme::color::Text);
    }

    return e;
}

void zelda64::launcher_animation_setup(recompui::LauncherMenu *menu) {
    auto context = recompui::get_current_context();
    recompui::Element *background_container = menu->get_background_container();
    background_container->set_background_color({ 0x08, 0x07, 0x0D, 0xFF });

    // TODO: Refactor into using the original div chain.
    launcher_context.bg_mask = context.create_element<recompui::Svg>(background_container, "mm-clipped.svg");
    launcher_context.bg_mask->set_position(recompui::Position::Absolute);
    launcher_context.bg_mask->set_width_auto();
    launcher_context.bg_mask->set_height(100.0f, recompui::Unit::Percent);
    launcher_context.bg_mask->set_opacity(0.1f); // TODO: Animate from 0 to 0.1 over 2.5s (with cubic-in-out possibly)
    launcher_context.bg_mask->set_translate_2D(100.0f, 0.0f); // TODO: Animated from 0 to 100 over 25s (with cubic-out possibly).

    launcher_context.oot_title = create_title(context, background_container, "Zelda 64: Recompiled", "Ocarina of Time", "Coming Soon\u2122", true);
    launcher_context.mm_title = create_title(context, background_container, "Zelda 64: Recompiled", "Majora's Mask", "", false);
}

void zelda64::launcher_animation_update(recompui::LauncherMenu *menu) {
}
