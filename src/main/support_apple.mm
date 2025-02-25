#include "zelda_support.h"
#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <SDL.h>
#include "nfd.h"

namespace zelda64 {
    void dispatch_on_ui_thread(std::function<void()> func) {
        dispatch_async(dispatch_get_main_queue(), ^{
            func();
        });
    }

    const char* get_bundle_resource_directory() {
        NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
        return strdup([bundlePath UTF8String]);
    }
}

// Used to swizzle the updateDrawableSize method in SDL_cocoametalview to not
// automatically resize the underlying CAMetalLayer when the window size changes.
static void MySwizzleSDLMetalView(void) {
    Class cls = objc_getClass("SDL_cocoametalview");
    if (!cls) {
        // Probably means SDL is using a different name, or the symbol is still hidden.
        return;
    }

    SEL originalSelector = sel_registerName("updateDrawableSize");
    SEL swizzledSelector = sel_registerName("my_updateDrawableSize");

    Method originalMethod = class_getInstanceMethod(cls, originalSelector);
    if (!originalMethod) {
        // The method might not exist or might get inlined in some SDL builds.
        return;
    }

    // Implementation of our replacement method
    IMP swizzledIMP = imp_implementationWithBlock(^void(id selfObj) {
        // (no-op)
    });

    // Swizzle method
    class_addMethod(cls, swizzledSelector, swizzledIMP, method_getTypeEncoding(originalMethod));
    Method swizzledMethod = class_getInstanceMethod(cls, swizzledSelector);
    method_exchangeImplementations(originalMethod, swizzledMethod);
}

__attribute__((constructor))
static void PatchSDLMetalViewConstructor() {
    // This runs as soon as the dynamic library/executable is loaded, before main().
    MySwizzleSDLMetalView();
}
