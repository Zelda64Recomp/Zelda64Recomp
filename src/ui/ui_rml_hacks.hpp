#ifndef UI_RML_HACKS_H
#define UI_RML_HACKS_H

#include "RmlUi/Core.h"
namespace RecompRml {
    Rml::Element* FindNextTabElement(Rml::Element* current_element, bool forward);

    enum class CanFocus { Yes, No, NoAndNoChildren };

    CanFocus CanFocusElement(Rml::Element* element);
}

#endif
