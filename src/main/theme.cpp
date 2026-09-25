#include "elements/ui_theme.h"
#include "theme.h"

void recomptheme::set_custom_theme() {
    using namespace recompui;

    theme::set_theme_color(theme::color::Background1, Color{8, 7, 13, 255});
    theme::set_theme_color(theme::color::Background2, Color{18, 16, 24, 255});
    theme::set_theme_color(theme::color::Background3, Color{25, 22, 34, 255});
    theme::set_theme_color(theme::color::BGOverlay, Color{190, 184, 219, 25}); // actually the overlay for the modal
    theme::set_theme_color(theme::color::ModalOverlay, Color{8, 7, 13, 229}); // actually the background color of the modal itself
    theme::set_theme_color(theme::color::BGShadow, Color{0, 0, 0, 89});
    theme::set_theme_color(theme::color::BGShadow2, Color{8, 7, 13, 184});
    theme::set_theme_color(theme::color::Text, Color{242, 242, 242, 255});
    theme::set_theme_color(theme::color::TextActive, Color{245, 245, 245, 255});
    theme::set_theme_color(theme::color::TextDim, Color{204, 204, 204, 255});
    theme::set_theme_color(theme::color::TextInactive, Color{255, 255, 255, 153});
    theme::set_theme_color(theme::color::TextA5, Color{242, 242, 242, 13});
    theme::set_theme_color(theme::color::TextA20, Color{242, 242, 242, 51});
    theme::set_theme_color(theme::color::TextA30, Color{242, 242, 242, 77});
    theme::set_theme_color(theme::color::TextA50, Color{242, 242, 242, 128});
    theme::set_theme_color(theme::color::TextA80, Color{242, 242, 242, 204});
    theme::set_theme_color(theme::color::Primary, Color{0xB9, 0x7D, 0xF2, 255});
    theme::set_theme_color(theme::color::PrimaryL, Color{0xDA, 0xBA, 0xF7, 255});
    theme::set_theme_color(theme::color::PrimaryD, Color{0x7A, 0x2A, 0xC6, 255});
    theme::set_theme_color(theme::color::PrimaryA5, Color{185, 125, 242, 13});
    theme::set_theme_color(theme::color::PrimaryA20, Color{185, 125, 242, 51});
    theme::set_theme_color(theme::color::PrimaryA30, Color{185, 125, 242, 77});
    theme::set_theme_color(theme::color::PrimaryA50, Color{185, 125, 242, 128});
    theme::set_theme_color(theme::color::PrimaryA80, Color{185, 125, 242, 204});
    theme::set_theme_color(theme::color::Secondary, Color{0x17, 0xD6, 0xE8, 255});
    theme::set_theme_color(theme::color::SecondaryL, Color{0xA2, 0xEF, 0xF6, 255});
    theme::set_theme_color(theme::color::SecondaryD, Color{0x25, 0xA1, 0xAD, 255});
    theme::set_theme_color(theme::color::SecondaryA5, Color{23, 214, 232, 13});
    theme::set_theme_color(theme::color::SecondaryA20, Color{23, 214, 232, 51});
    theme::set_theme_color(theme::color::SecondaryA30, Color{23, 214, 232, 77});
    theme::set_theme_color(theme::color::SecondaryA50, Color{23, 214, 232, 128});
    theme::set_theme_color(theme::color::SecondaryA80, Color{23, 214, 232, 204});
    theme::set_theme_color(theme::color::Warning, Color{0xE9, 0xCD, 0x35, 255});
    theme::set_theme_color(theme::color::WarningL, Color{0xF9, 0xE5, 0x7C, 255});
    theme::set_theme_color(theme::color::WarningD, Color{0xC5, 0xAA, 0x16, 255});
    theme::set_theme_color(theme::color::WarningA5, Color{233, 205, 53, 13});
    theme::set_theme_color(theme::color::WarningA20, Color{233, 205, 53, 51});
    theme::set_theme_color(theme::color::WarningA30, Color{233, 205, 53, 77});
    theme::set_theme_color(theme::color::WarningA50, Color{233, 205, 53, 128});
    theme::set_theme_color(theme::color::WarningA80, Color{233, 205, 53, 204});
    theme::set_theme_color(theme::color::Danger, Color{0xF8, 0x60, 0x39, 255});
    theme::set_theme_color(theme::color::DangerL, Color{0xFE, 0x86, 0x67, 255});
    theme::set_theme_color(theme::color::DangerD, Color{0xB2, 0x39, 0x19, 255});
    theme::set_theme_color(theme::color::DangerA5, Color{248, 96, 57, 13});
    theme::set_theme_color(theme::color::DangerA20, Color{248, 96, 57, 51});
    theme::set_theme_color(theme::color::DangerA30, Color{248, 96, 57, 77});
    theme::set_theme_color(theme::color::DangerA50, Color{248, 96, 57, 128});
    theme::set_theme_color(theme::color::DangerA80, Color{248, 96, 57, 204});
    theme::set_theme_color(theme::color::Success, Color{0x45, 0xD0, 0x43, 255});
    theme::set_theme_color(theme::color::SuccessL, Color{0xAA, 0xEA, 0xA9, 255});
    theme::set_theme_color(theme::color::SuccessD, Color{0x2C, 0xA7, 0x2A, 255});
    theme::set_theme_color(theme::color::SuccessA5, Color{69, 208, 67, 13});
    theme::set_theme_color(theme::color::SuccessA20, Color{69, 208, 67, 51});
    theme::set_theme_color(theme::color::SuccessA30, Color{69, 208, 67, 77});
    theme::set_theme_color(theme::color::SuccessA50, Color{69, 208, 67, 128});
    theme::set_theme_color(theme::color::SuccessA80, Color{69, 208, 67, 204});
    theme::set_theme_color(theme::color::Border, Color{255, 255, 255, 51});
    theme::set_theme_color(theme::color::BorderSoft, Color{255, 255, 255, 26});
    theme::set_theme_color(theme::color::BorderHard, Color{255, 255, 255, 77});
    theme::set_theme_color(theme::color::BorderSolid, Color{255, 255, 255, 153});
    theme::set_theme_color(theme::color::Transparent, Color{0, 0, 0, 0});
    theme::set_theme_color(theme::color::A, Color{51, 51, 255, 255});
    theme::set_theme_color(theme::color::AL, Color{178, 178, 255, 255});
    theme::set_theme_color(theme::color::AD, Color{32, 32, 172, 255});
    theme::set_theme_color(theme::color::AA5, Color{51, 51, 255, 13});
    theme::set_theme_color(theme::color::AA20, Color{51, 51, 255, 51});
    theme::set_theme_color(theme::color::AA30, Color{51, 51, 255, 77});
    theme::set_theme_color(theme::color::AA50, Color{51, 51, 255, 128});
    theme::set_theme_color(theme::color::AA80, Color{51, 51, 255, 204});
    theme::set_theme_color(theme::color::White, Color{255, 255, 255, 255});
    theme::set_theme_color(theme::color::WhiteA5, Color{255, 255, 255, 13});
    theme::set_theme_color(theme::color::WhiteA20, Color{255, 255, 255, 51});
    theme::set_theme_color(theme::color::WhiteA30, Color{255, 255, 255, 77});
    theme::set_theme_color(theme::color::WhiteA50, Color{255, 255, 255, 128});
    theme::set_theme_color(theme::color::WhiteA80, Color{255, 255, 255, 204});
    theme::set_theme_color(theme::color::BW05, Color{13, 13, 13, 255});
    theme::set_theme_color(theme::color::BW10, Color{26, 26, 26, 255});
    theme::set_theme_color(theme::color::BW25, Color{64, 64, 64, 255});
    theme::set_theme_color(theme::color::BW50, Color{128, 128, 128, 255});
    theme::set_theme_color(theme::color::BW75, Color{191, 191, 191, 255});
    theme::set_theme_color(theme::color::BW90, Color{229, 229, 229, 255});

    theme::border::radius_sm = 8.0f;
    theme::border::radius_md = 12.0f;
    theme::border::radius_lg = 16.0f;

    const uint32_t header_weight = 700;
    const uint32_t label_weight = 700;
    const uint32_t base_weight = 400;
    theme::set_typography_preset(theme::Typography::Header1, 64.0f + 4.0f, 0.07f, header_weight);
    theme::set_typography_preset(theme::Typography::Header2, 48.0f + 4.0f, 0.07f, header_weight);
    theme::set_typography_preset(theme::Typography::Header3, 32.0f + 4.0f, 0.07f, header_weight);
    theme::set_typography_preset(theme::Typography::LabelLG, 32.0f + 4.0f, 0.11f, label_weight);
    theme::set_typography_preset(theme::Typography::LabelMD, 24.0f + 4.0f, 0.11f, label_weight);
    theme::set_typography_preset(theme::Typography::LabelSM, 16.0f + 4.0f, 0.14f, label_weight);
    theme::set_typography_preset(theme::Typography::LabelXS, 14.0f + 4.0f, 0.14f, base_weight);
    theme::set_typography_preset(theme::Typography::Body,    16.0f + 4.0f, 0,     base_weight);
};


