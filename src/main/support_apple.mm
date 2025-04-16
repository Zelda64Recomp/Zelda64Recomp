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

    std::optional<std::filesystem::path> get_application_support_directory() {
        NSArray *dirs = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        if ([dirs count] > 0) {
            NSString *dir = [dirs firstObject];
            return std::filesystem::path([dir UTF8String]);
        }
        return std::nullopt;
    }

    std::filesystem::path get_bundle_resource_directory() {
        NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
        return std::filesystem::path([bundlePath UTF8String]);
    }

    std::filesystem::path get_bundle_directory() {
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        return std::filesystem::path([bundlePath UTF8String]);
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
