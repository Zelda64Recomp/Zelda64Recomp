#include "zelda_support.h"
#import <Foundation/Foundation.h>

void zelda64::dispatch_on_main_thread(std::function<void()> func) {
    dispatch_async(dispatch_get_main_queue(), ^{
        func();
    });
}

const char* zelda64::get_bundle_resource_directory() {
    NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
    return strdup([bundlePath UTF8String]);
}
