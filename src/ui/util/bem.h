#ifndef RECOMPUI_ELEMENTS_BEM
#define RECOMPUI_ELEMENTS_BEM

#include <string>

namespace recompui {
    // BEM base class
    class BEM {
    public:
        std::string block;

        BEM(const std::string &block) : block(block) {}
        virtual ~BEM() = default;

        const std::string get_block() const {
            return block;
        }

        const std::string el(const std::string &element) const {
            return block + "__" + element;
        }

        const BEM bem_el(const std::string &element) const {
            return BEM(el(element));
        }

        const std::string mod(const std::string &modifier) const {
            return block + "--" + modifier;
        }
    };
} // namespace recompui

#endif
