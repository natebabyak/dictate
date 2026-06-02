#include "platform/ptt.hpp"

#include "settings.hpp"
#include "state.hpp"

#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

#include <csignal>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace dictate {

namespace {

struct Hotkey {
  CGKeyCode key = kVK_ANSI_D;
  bool ctrl = true;
  bool shift = true;
  bool opt = false;
  bool cmd = false;
};

Hotkey g_hk;

static std::string lower(std::string s) {
  for (char &c : s)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

static CGKeyCode key_from_name(const std::string &name) {
  if (name.size() == 1) {
    const char c = lower(name)[0];
    if (c >= 'a' && c <= 'z')
      return static_cast<CGKeyCode>(kVK_ANSI_A + (c - 'a'));
    if (c >= '0' && c <= '9')
      return static_cast<CGKeyCode>(kVK_ANSI_0 + (c - '0'));
  }
  if (name == "space")
    return kVK_Space;
  if (name == "f5")
    return kVK_F5;
  return kVK_ANSI_D;
}

void parse_hotkey(const std::string &spec) {
  g_hk = {};
  std::stringstream ss(lower(spec));
  std::string part;
  std::string key_part = "d";
  while (std::getline(ss, part, '+')) {
    part.erase(0, part.find_first_not_of(" \t"));
    part.erase(part.find_last_not_of(" \t") + 1);
    if (part.empty())
      continue;
    if (part == "ctrl" || part == "control")
      g_hk.ctrl = true;
    else if (part == "shift")
      g_hk.shift = true;
    else if (part == "opt" || part == "option" || part == "alt")
      g_hk.opt = true;
    else if (part == "cmd" || part == "command")
      g_hk.cmd = true;
    else
      key_part = part;
  }
  g_hk.key = key_from_name(key_part);
}

bool mod_down(CGKeyCode vk) {
  return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, vk);
}

bool chord_live() {
  const bool ctrl = !g_hk.ctrl || mod_down(kVK_Control) || mod_down(kVK_RightControl);
  const bool shift = !g_hk.shift || mod_down(kVK_Shift) || mod_down(kVK_RightShift);
  const bool opt = !g_hk.opt || mod_down(kVK_Option) || mod_down(kVK_RightOption);
  const bool cmd = !g_hk.cmd || mod_down(kVK_Command) || mod_down(kVK_RightCommand);
  return ctrl && shift && opt && cmd;
}

bool chord_flags(NSEventModifierFlags f) {
  const bool ctrl = !g_hk.ctrl || (f & NSEventModifierFlagControl);
  const bool shift = !g_hk.shift || (f & NSEventModifierFlagShift);
  const bool opt = !g_hk.opt || (f & NSEventModifierFlagOption);
  const bool cmd = !g_hk.cmd || (f & NSEventModifierFlagCommand);
  return ctrl && shift && opt && cmd;
}

bool chord_ok(NSEventModifierFlags f) { return chord_flags(f) || chord_live(); }

bool chord_cg(CGEventFlags f) {
  const bool ctrl = !g_hk.ctrl || (f & kCGEventFlagMaskControl);
  const bool shift = !g_hk.shift || (f & kCGEventFlagMaskShift);
  const bool opt = !g_hk.opt || (f & kCGEventFlagMaskAlternate);
  const bool cmd = !g_hk.cmd || (f & kCGEventFlagMaskCommand);
  return (ctrl && shift && opt && cmd) || chord_live();
}

void on_hotkey(bool down) {
  const bool toggle = g_settings.mode == Settings::Mode::Toggle;
  if (toggle) {
    if (!down)
      return;
    if (g_session.recording.load())
      stop_recording();
    else
      start_recording();
    return;
  }
  if (down) {
    if (!g_session.recording.load())
      start_recording();
  } else if (g_session.recording.load()) {
    stop_recording();
  }
}

void handle_event(NSEvent *event) {
  const CGKeyCode code = static_cast<CGKeyCode>(event.keyCode);
  if (code != g_hk.key)
    return;

  if (event.type == NSEventTypeKeyDown)
    on_hotkey(true);
  else if (event.type == NSEventTypeKeyUp)
    on_hotkey(false);
}

CGEventRef tap_cb(CGEventTapProxy, CGEventType type, CGEventRef event, void *) {
  if (type == kCGEventTapDisabledByTimeout ||
      type == kCGEventTapDisabledByUserInput)
    return event;

  const CGKeyCode code = static_cast<CGKeyCode>(
      CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));

  if (code != g_hk.key)
    return event;

  if (type == kCGEventKeyDown && chord_cg(CGEventGetFlags(event)))
    on_hotkey(true);
  else if (type == kCGEventKeyUp)
    on_hotkey(false);

  return event;
}

} // namespace

void init_signals() {
  signal(SIGINT, [](int) { g_running = false; });
  signal(SIGTERM, [](int) { g_running = false; });
}

void run_ptt() {
  @autoreleasepool {
    parse_hotkey(g_settings.hotkey);

    if (!AXIsProcessTrusted())
      std::cerr << "Grant Accessibility to this app, then restart.\n";
    if (!CGPreflightListenEventAccess())
      CGRequestListenEventAccess();

    [NSApplication sharedApplication];

    const NSEventMask mask =
        NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskFlagsChanged;

    [NSEvent addGlobalMonitorForEventsMatchingMask:mask
                                           handler:^(NSEvent *e) {
                                             handle_event(e);
                                           }];

    [NSEvent addLocalMonitorForEventsMatchingMask:mask
                                        handler:^NSEvent *(NSEvent *e) {
                                          handle_event(e);
                                          return e;
                                        }];

    const CGEventMask cmask = CGEventMaskBit(kCGEventKeyDown) |
                              CGEventMaskBit(kCGEventKeyUp);
    CFMachPortRef tap = CGEventTapCreate(
        kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
        cmask, tap_cb, nullptr);
    if (tap) {
      CFRunLoopSourceRef src =
          CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
      CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopCommonModes);
      CGEventTapEnable(tap, true);
      CFRelease(src);
    }

    const char *mode =
        g_settings.mode == Settings::Mode::Toggle ? "toggle" : "hold";
    std::cerr << "Ready (" << mode << "): " << g_settings.hotkey << '\n';

    while (g_running) {
      [[NSRunLoop currentRunLoop]
          runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
    }
  }
}

} // namespace dictate
