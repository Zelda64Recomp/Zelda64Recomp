#include <mutex>

#include "recomp_ui.h"

#include "elements/ui_element.h"
#include "elements/ui_label.h"
#include "elements/ui_button.h"

struct {
    recompui::ContextId ui_context;
    recompui::Label* prompt_header;
    recompui::Label* prompt_label;
    recompui::Element* prompt_controls;
    recompui::Button* confirm_button;
    recompui::Button* cancel_button;
    std::function<void()> confirm_action;
    std::function<void()> cancel_action;
    std::string return_element_id;
    std::mutex mutex;
} prompt_state;

void run_confirm_callback() {
    std::function<void()> confirm_action;
    {
        std::lock_guard lock{ prompt_state.mutex };
        confirm_action = std::move(prompt_state.confirm_action);
    }
    if (confirm_action) {
        confirm_action();
    }
    recompui::hide_context(prompt_state.ui_context);

    // TODO nav: focus on return_element_id
    // or just remove it as the usage of the prompt can change now
}

void run_cancel_callback() {
    std::function<void()> cancel_action;
    {
        std::lock_guard lock{ prompt_state.mutex };
        cancel_action = std::move(prompt_state.cancel_action);
    }
    if (cancel_action) {
        cancel_action();
    }
    recompui::hide_context(prompt_state.ui_context);

    // TODO nav: focus on return_element_id
    // or just remove it as the usage of the prompt can change now
}

void recompui::init_prompt_context() {
    ContextId context = create_context();

    std::lock_guard lock{ prompt_state.mutex };

    context.open();

    prompt_state.ui_context = context;

    Element* window = context.create_element<Element>(context.get_root_element());
    window->set_display(Display::Flex);
    window->set_flex_direction(FlexDirection::Column);
    window->set_background_color({0, 0, 0, 0});

    Element* prompt_overlay = context.create_element<Element>(window);
    prompt_overlay->set_background_color(Color{ 190, 184, 219, 25 });
    prompt_overlay->set_position(Position::Absolute);
    prompt_overlay->set_top(0);
    prompt_overlay->set_right(0);
    prompt_overlay->set_bottom(0);
    prompt_overlay->set_left(0);

    Element* prompt_content_wrapper = context.create_element<Element>(window);
    prompt_content_wrapper->set_display(Display::Flex);
    prompt_content_wrapper->set_position(Position::Absolute);
    prompt_content_wrapper->set_top(0);
    prompt_content_wrapper->set_right(0);
    prompt_content_wrapper->set_bottom(0);
    prompt_content_wrapper->set_left(0);
    prompt_content_wrapper->set_align_items(AlignItems::Center);
    prompt_content_wrapper->set_justify_content(JustifyContent::Center);

    Element* prompt_content = context.create_element<Element>(prompt_content_wrapper);
    prompt_content->set_display(Display::Flex);
    prompt_content->set_position(Position::Relative);
    prompt_content->set_flex(1.0f, 1.0f);
    prompt_content->set_flex_basis(100, Unit::Percent);
    prompt_content->set_flex_direction(FlexDirection::Column);
    prompt_content->set_width(100, Unit::Percent);
    prompt_content->set_max_width(700, Unit::Dp);
    prompt_content->set_height_auto();
    prompt_content->set_margin_auto();
    prompt_content->set_border_width(1.1, Unit::Dp);
    prompt_content->set_border_radius(16, Unit::Dp);
    prompt_content->set_border_color(Color{ 255, 255, 255, 51 });
    prompt_content->set_background_color(Color{ 8, 7, 13, 229 });
    
    prompt_state.prompt_header = context.create_element<Label>(prompt_content, "", LabelStyle::Large);
    prompt_state.prompt_header->set_margin(24, Unit::Dp);

    prompt_state.prompt_label = context.create_element<Label>(prompt_content, "", LabelStyle::Small);
    prompt_state.prompt_label->set_margin(24, Unit::Dp);
    prompt_state.prompt_label->set_margin_top(0);
    
    prompt_state.prompt_controls = context.create_element<Element>(prompt_content);
    
    prompt_state.prompt_controls->set_display(Display::Flex);
    prompt_state.prompt_controls->set_flex_direction(FlexDirection::Row);
    prompt_state.prompt_controls->set_justify_content(JustifyContent::Center);
    prompt_state.prompt_controls->set_padding_top(24, Unit::Dp);
    prompt_state.prompt_controls->set_padding_bottom(24, Unit::Dp);
    prompt_state.prompt_controls->set_padding_left(12, Unit::Dp);
    prompt_state.prompt_controls->set_padding_right(12, Unit::Dp);
    prompt_state.prompt_controls->set_border_top_width(1.1, Unit::Dp);
    prompt_state.prompt_controls->set_border_top_color({ 255, 255, 255, 25 });

    prompt_state.confirm_button = context.create_element<Button>(prompt_state.prompt_controls, "", ButtonStyle::Primary);
    prompt_state.confirm_button->set_min_width(185.0f, Unit::Dp);
    prompt_state.confirm_button->set_margin_top(0);
    prompt_state.confirm_button->set_margin_bottom(0);
    prompt_state.confirm_button->set_margin_left(12, Unit::Dp);
    prompt_state.confirm_button->set_margin_right(12, Unit::Dp);
    prompt_state.confirm_button->set_text_align(TextAlign::Center);
    prompt_state.confirm_button->set_color(Color{ 204, 204, 204, 255 });
    prompt_state.confirm_button->add_pressed_callback(run_confirm_callback);
    
    Style* confirm_hover_style = prompt_state.confirm_button->get_hover_style();
    confirm_hover_style->set_border_color(Color{ 69, 208, 67, 255 });
    confirm_hover_style->set_background_color(Color{ 69, 208, 67, 76 });
    confirm_hover_style->set_color(Color{ 242, 242, 242, 255 });
    
    Style* confirm_focus_style = prompt_state.confirm_button->get_focus_style();
    confirm_focus_style->set_border_color(Color{ 69, 208, 67, 255 });
    confirm_focus_style->set_background_color(Color{ 69, 208, 67, 76 });
    confirm_focus_style->set_color(Color{ 242, 242, 242, 255 });

    prompt_state.cancel_button = context.create_element<Button>(prompt_state.prompt_controls, "", ButtonStyle::Primary);
    prompt_state.cancel_button->set_min_width(185.0f, Unit::Dp);
    prompt_state.cancel_button->set_margin_top(0);
    prompt_state.cancel_button->set_margin_bottom(0);
    prompt_state.cancel_button->set_margin_left(12, Unit::Dp);
    prompt_state.cancel_button->set_margin_right(12, Unit::Dp);
    prompt_state.cancel_button->set_text_align(TextAlign::Center);
    prompt_state.cancel_button->set_color(Color{ 204, 204, 204, 255 });
    prompt_state.cancel_button->add_pressed_callback(run_cancel_callback);
    
    Style* cancel_hover_style = prompt_state.cancel_button->get_hover_style();
    cancel_hover_style->set_border_color(Color{ 248, 96, 57, 255 });
    cancel_hover_style->set_background_color(Color{ 248, 96, 57, 76 });
    cancel_hover_style->set_color(Color{ 242, 242, 242, 255 });

    Style* cancel_focus_style = prompt_state.cancel_button->get_focus_style();
    cancel_focus_style->set_border_color(Color{ 248, 96, 57, 255 });
    cancel_focus_style->set_background_color(Color{ 248, 96, 57, 76 });
    cancel_focus_style->set_color(Color{ 242, 242, 242, 255 });


    context.close();
}

void style_button(recompui::Button* button, recompui::ButtonVariant variant) {
    recompui::Color button_color{};

    switch (variant) {
        case recompui::ButtonVariant::Primary:
            button_color = { 185, 125, 242, 255 };
            break;
        case recompui::ButtonVariant::Secondary:
            button_color = { 23, 214, 232, 255 };
            break;
        case recompui::ButtonVariant::Tertiary:
            button_color = { 242, 242, 242, 255 };
            break;
        case recompui::ButtonVariant::Success:
            button_color = { 69, 208, 67, 255 };
            break;
        case recompui::ButtonVariant::Error:
            button_color = { 248, 96, 57, 255 };
            break;
        case recompui::ButtonVariant::Warning:
            button_color = { 233, 205, 53, 255 };
            break;
        default:
            assert(false);
            break;
    }

    recompui::Color border_color = button_color;
    recompui::Color background_color = button_color;
    border_color.a = 0.8f * 255;
    background_color.a = 0.05f * 255;
    button->set_border_color(border_color);
    button->set_background_color(background_color);
    
    recompui::Color hover_border_color = button_color;
    recompui::Color hover_background_color = button_color;
    hover_border_color.a = 255;
    hover_background_color.a = 0.3f * 255;
    recompui::Style* hover_style = button->get_hover_style();
    hover_style->set_border_color(hover_border_color);
    hover_style->set_background_color(hover_background_color);
    
    recompui::Style* focus_style = button->get_focus_style();
    focus_style->set_border_color(hover_border_color);
    focus_style->set_background_color(hover_background_color);

    recompui::Color disabled_color { 255, 255, 255, 0.6f * 255 };
    recompui::Style* disabled_style = button->get_disabled_style();
    disabled_style->set_color(disabled_color);
}

// Must be called while prompt_state.mutex is locked.
void show_prompt(std::function<void()>& prev_cancel_action, bool focus_on_cancel) {
    if (focus_on_cancel) {
        prompt_state.ui_context.set_autofocus_element(prompt_state.cancel_button);
    }
    else {
        prompt_state.ui_context.set_autofocus_element(prompt_state.confirm_button);
    }

    if (!recompui::is_context_shown(prompt_state.ui_context)) {
        recompui::show_context(prompt_state.ui_context, "");
    }
    else {
        // Call the previous cancel action to effectively close the previous prompt.
        if (prev_cancel_action) {
            prev_cancel_action();
        }
    }
}

void recompui::open_choice_prompt(
    const std::string& header_text,
    const std::string& content_text,
    const std::string& confirm_label_text,
    const std::string& cancel_label_text,
    std::function<void()> confirm_action,
    std::function<void()> cancel_action,
    ButtonVariant confirm_variant,
    ButtonVariant cancel_variant,
    bool focus_on_cancel,
    const std::string& return_element_id
) {
    std::lock_guard lock{ prompt_state.mutex };

    std::function<void()> prev_cancel_action = std::move(prompt_state.cancel_action);

    ContextId prev_context = try_close_current_context();

    prompt_state.ui_context.open();

    prompt_state.prompt_header->set_text(header_text);
    prompt_state.prompt_label->set_text(content_text);
    prompt_state.prompt_controls->set_display(Display::Flex);
    prompt_state.confirm_button->set_display(Display::Block);
    prompt_state.cancel_button->set_display(Display::Block);
    prompt_state.confirm_button->set_text(confirm_label_text);
    prompt_state.cancel_button->set_text(cancel_label_text);
    prompt_state.confirm_action = confirm_action;
    prompt_state.cancel_action = cancel_action;
    prompt_state.return_element_id = return_element_id;

    style_button(prompt_state.confirm_button, confirm_variant);
    style_button(prompt_state.cancel_button, cancel_variant);

    prompt_state.ui_context.close();

    if (prev_context != ContextId::null()) {
        prev_context.open();
    }

    show_prompt(prev_cancel_action, focus_on_cancel);
}

void recompui::open_info_prompt(
    const std::string& header_text,
    const std::string& content_text,
    const std::string& okay_label_text,
    std::function<void()> okay_action,
    ButtonVariant okay_variant,
    const std::string& return_element_id
) {
    std::lock_guard lock{ prompt_state.mutex };

    std::function<void()> prev_cancel_action = std::move(prompt_state.cancel_action);

    ContextId prev_context = try_close_current_context();

    prompt_state.ui_context.open();

    prompt_state.prompt_header->set_text(header_text);
    prompt_state.prompt_label->set_text(content_text);
    prompt_state.prompt_controls->set_display(Display::Flex);
    prompt_state.confirm_button->set_display(Display::None);
    prompt_state.cancel_button->set_display(Display::Block);
    prompt_state.cancel_button->set_text(okay_label_text);
    prompt_state.confirm_action = {};
    prompt_state.cancel_action = okay_action;
    prompt_state.return_element_id = return_element_id;

    style_button(prompt_state.cancel_button, okay_variant);

    prompt_state.ui_context.close();

    if (prev_context != ContextId::null()) {
        prev_context.open();
    }

    show_prompt(prev_cancel_action, true);
}

void recompui::open_notification(
    const std::string& header_text,
    const std::string& content_text,
    const std::string& return_element_id
) {
    std::lock_guard lock{ prompt_state.mutex };

    std::function<void()> prev_cancel_action = std::move(prompt_state.cancel_action);

    ContextId prev_context = try_close_current_context();

    prompt_state.ui_context.open();

    prompt_state.prompt_header->set_text(header_text);
    prompt_state.prompt_label->set_text(content_text);
    prompt_state.prompt_controls->set_display(Display::None);
    prompt_state.confirm_button->set_display(Display::None);
    prompt_state.cancel_button->set_display(Display::None);
    prompt_state.confirm_action = {};
    prompt_state.cancel_action = {};
    prompt_state.return_element_id = return_element_id;

    prompt_state.ui_context.close();

    if (prev_context != ContextId::null()) {
        prev_context.open();
    }

    show_prompt(prev_cancel_action, false);
}

void recompui::close_prompt() {
    std::lock_guard lock{ prompt_state.mutex };

    if (recompui::is_context_shown(prompt_state.ui_context)) {
        if (prompt_state.cancel_action) {
            prompt_state.cancel_action();
        }

        recompui::hide_context(prompt_state.ui_context);
    }
}

bool recompui::is_prompt_open() {
    std::lock_guard lock{ prompt_state.mutex };

	return recompui::is_context_shown(prompt_state.ui_context);
}
