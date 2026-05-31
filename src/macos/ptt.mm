#include "platform/ptt.hpp"

#include "common/state.hpp"

#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CoreFoundation.h>

#include <cstdlib>
#include <iostream>

namespace dictate::platform {

namespace {

constexpr CGKeyCode kPttKey = kVK_ANSI_D; // 0x02

bool g_debug = false;
CFMachPortRef g_event_tap = nullptr;

bool debug_enabled() {
  const char *env = std::getenv("DICTATE_DEBUG");
  return env != nullptr && env[0] != '\0' && env[0] != '0';
}

void load_ptt_config() { g_debug = debug_enabled(); }

bool key_down(CGKeyCode key) {
  return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, key) ||
         CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, key);
}

bool modifiers_chord_live() {
  const bool ctrl = key_down(static_cast<CGKeyCode>(kVK_Control)) ||
                    key_down(static_cast<CGKeyCode>(kVK_RightControl));
  const bool shift = key_down(static_cast<CGKeyCode>(kVK_Shift)) ||
                     key_down(static_cast<CGKeyCode>(kVK_RightShift));
  return ctrl && shift;
}

bool modifiers_chord_flags(NSEventModifierFlags flags) {
  return (flags & NSEventModifierFlagControl) &&
         (flags & NSEventModifierFlagShift);
}

bool modifiers_chord_cg(CGEventFlags flags) {
  return (flags & kCGEventFlagMaskControl) && (flags & kCGEventFlagMaskShift);
}

const char *key_name() { return "Control+Shift+D"; }

void log_key_event(const char *source, const char *kind, CGKeyCode keycode,
                   NSEventModifierFlags flags) {
  if (!g_debug && keycode != kPttKey)
    return;
  std::cerr << "[ptt] " << source << " " << kind << " keycode=" << keycode
            << " event-ctrl="
            << ((flags & NSEventModifierFlagControl) ? "1" : "0")
            << " event-shift="
            << ((flags & NSEventModifierFlagShift) ? "1" : "0")
            << " live-ctrl-shift="
            << (modifiers_chord_live() ? "1" : "0") << '\n';
}

void on_chord_pressed() {
  std::cerr << "[ptt] Control+Shift+D pressed — recording started\n";
  begin_session();
}

void on_chord_released() {
  std::cerr << "[ptt] Control+Shift+D released — finalizing\n";
  end_session();
}

bool chord_triggered(NSEventModifierFlags flags) {
  return modifiers_chord_flags(flags) || modifiers_chord_live();
}

void handle_flags_changed(NSEvent *event, const char *source) {
  log_key_event(source, "flagsChanged", static_cast<CGKeyCode>(event.keyCode),
                event.modifierFlags);

  if (g_ptt.active.load() && !modifiers_chord_live())
    on_chord_released();
}

void handle_key_down_up(NSEvent *event, const char *source) {
  const CGKeyCode keycode = static_cast<CGKeyCode>(event.keyCode);

  if (g_debug || keycode == kPttKey) {
    log_key_event(source,
                  event.type == NSEventTypeKeyDown ? "keyDown" : "keyUp",
                  keycode, event.modifierFlags);
  }

  if (keycode != kPttKey)
    return;

  if (event.type == NSEventTypeKeyDown) {
    if (chord_triggered(event.modifierFlags) && !g_ptt.active.load())
      on_chord_pressed();
  } else if (g_ptt.active.load()) {
    on_chord_released();
  }
}

void handle_ns_event(NSEvent *event, const char *source) {
  if (event.type == NSEventTypeFlagsChanged)
    handle_flags_changed(event, source);
  else if (event.type == NSEventTypeKeyDown ||
           event.type == NSEventTypeKeyUp)
    handle_key_down_up(event, source);
}

void request_permissions() {
  const bool ax = AXIsProcessTrusted();
  const bool listen = CGPreflightListenEventAccess();

  std::cerr << "[ptt] Accessibility: " << (ax ? "granted" : "NOT granted")
            << '\n';
  std::cerr << "[ptt] Input Monitoring: " << (listen ? "granted" : "NOT granted")
            << '\n';

  if (!ax) {
    std::cerr << "  → Enable Cursor/Terminal/dictate under Privacy > "
                 "Accessibility, then restart.\n";
    const void *keys[] = {kAXTrustedCheckOptionPrompt};
    const void *values[] = {kCFBooleanTrue};
    CFDictionaryRef opts = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, 1, &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);
    AXIsProcessTrustedWithOptions(opts);
    CFRelease(opts);
  }

  if (!listen) {
    std::cerr << "  → Enable the same app under Privacy > Input Monitoring, "
                 "then restart.\n";
    CGRequestListenEventAccess();
  }
}

CGEventRef cg_event_tap_callback(CGEventTapProxy, CGEventType type,
                                 CGEventRef event, void *) {
  if (type == kCGEventTapDisabledByTimeout ||
      type == kCGEventTapDisabledByUserInput) {
    if (g_event_tap != nullptr)
      CGEventTapEnable(g_event_tap, true);
    return event;
  }

  const CGKeyCode keycode = static_cast<CGKeyCode>(
      CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
  const CGEventFlags flags = CGEventGetFlags(event);

  if (type == kCGEventFlagsChanged) {
    if (g_ptt.active.load() && !modifiers_chord_live())
      on_chord_released();
    return event;
  }

  if (keycode != kPttKey)
    return event;

  if (g_debug) {
    std::cerr << "[ptt] tap " << (type == kCGEventKeyDown ? "keyDown" : "keyUp")
              << " keycode=" << keycode << '\n';
  }

  if (type == kCGEventKeyDown) {
    if ((modifiers_chord_cg(flags) || modifiers_chord_live()) &&
        !g_ptt.active.load())
      on_chord_pressed();
  } else if (g_ptt.active.load()) {
    on_chord_released();
  }

  return event;
}

bool install_event_tap() {
  const CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) |
                           CGEventMaskBit(kCGEventKeyUp) |
                           CGEventMaskBit(kCGEventFlagsChanged);

  const CGEventTapLocation locations[] = {kCGSessionEventTap, kCGHIDEventTap,
                                          kCGAnnotatedSessionEventTap};

  for (CGEventTapLocation loc : locations) {
    g_event_tap = CGEventTapCreate(loc, kCGHeadInsertEventTap,
                                   kCGEventTapOptionListenOnly, mask,
                                   cg_event_tap_callback, nullptr);
    if (g_event_tap != nullptr) {
      std::cerr << "[ptt] CGEventTap installed.\n";
      break;
    }
  }

  if (g_event_tap == nullptr) {
    std::cerr << "[ptt] CGEventTap failed to install.\n";
    return false;
  }

  CFRunLoopSourceRef source =
      CFMachPortCreateRunLoopSource(kCFAllocatorDefault, g_event_tap, 0);
  if (source == nullptr) {
    CFRelease(g_event_tap);
    g_event_tap = nullptr;
    return false;
  }

  CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
  CGEventTapEnable(g_event_tap, true);
  CFRelease(source);
  return true;
}

} // namespace

const char *ptt_key_name() { return key_name(); }

void run_ptt_loop() {
  @autoreleasepool {
    load_ptt_config();
    request_permissions();

    [NSApplication sharedApplication];

    const NSEventMask mask = NSEventMaskKeyDown | NSEventMaskKeyUp |
                             NSEventMaskFlagsChanged;

    id global_monitor = [NSEvent addGlobalMonitorForEventsMatchingMask:mask
                                                               handler:^(NSEvent *e) {
                                                                 handle_ns_event(
                                                                     e, "global");
                                                               }];

    id local_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:mask
                                                             handler:^NSEvent *(
                                                                 NSEvent *e) {
                                                               handle_ns_event(
                                                                   e, "local");
                                                               return e;
                                                             }];

    std::cerr << "[ptt] Global monitor: "
              << (global_monitor ? "ok" : "FAILED") << '\n';
    std::cerr << "[ptt] Local monitor: "
              << (local_monitor ? "ok" : "FAILED") << '\n';

    install_event_tap();

    std::cerr << "[ptt] Hold " << key_name()
              << " (D keycode=" << static_cast<unsigned>(kPttKey) << ").\n";
    std::cerr << "[ptt] DICTATE_DEBUG=1 logs every key event.\n";

    while (g_running) {
      [[NSRunLoop currentRunLoop]
          runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
    }

    if (g_event_tap != nullptr) {
      CGEventTapEnable(g_event_tap, false);
      CFRelease(g_event_tap);
      g_event_tap = nullptr;
    }
  }
}

} // namespace dictate::platform
