#pragma once

#include <cstdint>

namespace recompui {
    class Style;
    class Element;
    struct ResourceId {
        uint32_t slot_id;

        bool operator==(const ResourceId& rhs) const = default;

        const Style* operator*() const;
        Style* operator*();
        
        const Style* operator->() const { return *(*this); }
        Style* operator->() { return *(*this); }

        const Element* as_element() const;
        Element* as_element();

        static constexpr ResourceId null() { return ResourceId{ uint32_t(-1) }; }
    };
} // namespace recompui
