#ifndef RECOMPUI_ELEMENTS_BEM
#define RECOMPUI_ELEMENTS_BEM

#include <string>

#define EL(bem, element) bem "__" element
#define EL_DYN(bem, element) bem "__" + element
#define MOD(bem, modifier) bem "--" modifier
#define MOD_DYN(bem, modifier) bem "--" + modifier
#define BLOCK(bem) bem

#endif
