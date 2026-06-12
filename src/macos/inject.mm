#include "platform/inject.hpp"

#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

namespace dictate {

void inject_text(const std::string &text) {
  if (text.empty())
    return;

  @autoreleasepool {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    [pb clearContents];
    NSString *s = [NSString stringWithUTF8String:text.c_str()];
    if (s)
      [pb setString:s forType:NSPasteboardTypeString];
  }

  CGEventRef down = CGEventCreateKeyboardEvent(nullptr, kVK_ANSI_V, true);
  CGEventSetFlags(down, kCGEventFlagMaskCommand);
  CGEventPost(kCGHIDEventTap, down);
  CFRelease(down);

  CGEventRef up = CGEventCreateKeyboardEvent(nullptr, kVK_ANSI_V, false);
  CGEventSetFlags(up, kCGEventFlagMaskCommand);
  CGEventPost(kCGHIDEventTap, up);
  CFRelease(up);
}

} // namespace dictate
