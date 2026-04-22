#include "keytap.h"
#include <CoreGraphics/CoreGraphics.h>
#include <Carbon/Carbon.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define kVK_JIS_Eisu 102
#define kVK_JIS_Kana 104
#define kVK_LeftCmd  55
#define kVK_RightCmd 54
#define IME_SENTINEL 0xDEADBEEFL

static CFMachPortRef      s_eventTap      = NULL;
static CFRunLoopSourceRef s_runLoopSrc    = NULL;
static KeymapEntry       *s_keymap        = NULL;
static int                s_keymapCount   = 0;
static int                s_imeSwitching  = 0;

static _Atomic int leftCmdAlone  = 0;
static _Atomic int rightCmdAlone = 0;

// modifier キーの押下状態をトグル方式で追跡
static uint8_t s_modKeyState[256];

static KeymapEntry *findEntry(int keyCode) {
    for (int i = 0; i < s_keymapCount; i++) {
        if (s_keymap[i].from == keyCode) return &s_keymap[i];
    }
    return NULL;
}

static void postIMEKey(CGKeyCode kc) {
    CGEventRef d = CGEventCreateKeyboardEvent(NULL, kc, true);
    CGEventRef u = CGEventCreateKeyboardEvent(NULL, kc, false);
    // 直前の Cmd 等のフラグが残らないよう明示的にクリアする
    CGEventSetFlags(d, 0);
    CGEventSetFlags(u, 0);
    CGEventSetIntegerValueField(d, kCGEventSourceUserData, IME_SENTINEL);
    CGEventSetIntegerValueField(u, kCGEventSourceUserData, IME_SENTINEL);
    CGEventPost(kCGSessionEventTap, d);
    CGEventPost(kCGSessionEventTap, u);
    CFRelease(d);
    CFRelease(u);
}

static void postKeyWithModifiers(int keyCode, uint32_t modifiers, bool keyDown) {
    CGEventRef ev = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, keyDown);
    CGEventSetFlags(ev, (CGEventFlags)modifiers);
    CGEventSetIntegerValueField(ev, kCGEventSourceUserData, IME_SENTINEL);
    CGEventPost(kCGSessionEventTap, ev);
    CFRelease(ev);
}

// キーコードに対応する modifier フラグビット（デバイス固有 + 汎用）を返す
static CGEventFlags keycodeToFlags(int kc) {
    switch (kc) {
        case 55: return KEYTAP_FLAGS_LEFT_CMD;
        case 54: return KEYTAP_FLAGS_RIGHT_CMD;
        case 56: return KEYTAP_FLAGS_LEFT_SHIFT;
        case 60: return KEYTAP_FLAGS_RIGHT_SHIFT;
        case 58: return KEYTAP_FLAGS_LEFT_OPT;
        case 61: return KEYTAP_FLAGS_RIGHT_OPT;
        case 59: return KEYTAP_FLAGS_LEFT_CTRL;
        case 62: return KEYTAP_FLAGS_RIGHT_CTRL;
        default: return (CGEventFlags)0;
    }
}

// 現在押されている remapped modifier を反映するよう flags を書き換える
static void transformFlags(CGEventRef event) {
    CGEventFlags flags         = CGEventGetFlags(event);
    CGEventFlags flagsToRemove = 0;
    CGEventFlags flagsToAdd    = 0;

    for (int i = 0; i < s_keymapCount; i++) {
        KeymapEntry *e = &s_keymap[i];
        if (e->modifiers != 0) continue;
        CGEventFlags srcBits = keycodeToFlags(e->from);
        CGEventFlags dstBits = keycodeToFlags(e->to);
        if (srcBits == 0 || dstBits == 0) continue;

        if (s_modKeyState[e->from & 0xFF]) {
            flagsToRemove |= srcBits;
            flagsToAdd    |= dstBits;
        } else {
            // 押されていない src キーのビットが残っていたら除去
            flagsToRemove |= srcBits;
        }
    }

    CGEventFlags newFlags = (flags & ~flagsToRemove) | flagsToAdd;
    if (newFlags != flags) {
        CGEventSetFlags(event, newFlags);
    }
}

static CGEventRef eventCallback(CGEventTapProxy proxy, CGEventType type,
                                CGEventRef event, void *refcon) {
    if (CGEventGetIntegerValueField(event, kCGEventSourceUserData) == (int64_t)IME_SENTINEL) {
        return event;
    }

    if (type == kCGEventTapDisabledByTimeout) {
        CGEventTapEnable(s_eventTap, true);
        return event;
    }

    if (type == kCGEventLeftMouseDown  || type == kCGEventLeftMouseUp  ||
        type == kCGEventRightMouseDown || type == kCGEventRightMouseUp ||
        type == kCGEventOtherMouseDown || type == kCGEventOtherMouseUp) {
        transformFlags(event);
        return event;
    }

    int keyCode = (int)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    if (keyCode < 0 || keyCode > 255) return event;

    if (type == kCGEventFlagsChanged) {
        // FlagsChanged は modifier キー1回押下ごとに1回、解放ごとに1回発火するため、
        // トグル方式で押下状態を判定する
        int isDown = !s_modKeyState[keyCode & 0xFF];
        s_modKeyState[keyCode & 0xFF] = isDown ? 1 : 0;

        KeymapEntry *e = findEntry(keyCode);
        int effectiveKey = e ? e->to : keyCode;

        if (s_imeSwitching) {
            if (effectiveKey == kVK_LeftCmd) {
                if (isDown) {
                    atomic_store(&leftCmdAlone, 1);
                } else {
                    if (atomic_load(&leftCmdAlone)) postIMEKey(kVK_JIS_Eisu);
                    atomic_store(&leftCmdAlone, 0);
                }
            } else if (effectiveKey == kVK_RightCmd) {
                if (isDown) {
                    atomic_store(&rightCmdAlone, 1);
                } else {
                    if (atomic_load(&rightCmdAlone)) postIMEKey(kVK_JIS_Kana);
                    atomic_store(&rightCmdAlone, 0);
                }
            } else if (isDown) {
                atomic_store(&leftCmdAlone, 0);
                atomic_store(&rightCmdAlone, 0);
            }
        }

        transformFlags(event);

        if (e && e->modifiers == 0) {
            CGEventSetIntegerValueField(event, kCGKeyboardEventKeycode, e->to);
        }

        return event;
    }

    if (type == kCGEventKeyDown) {
        if (s_imeSwitching) {
            atomic_store(&leftCmdAlone, 0);
            atomic_store(&rightCmdAlone, 0);
        }

        transformFlags(event);

        KeymapEntry *e = findEntry(keyCode);
        if (e) {
            if (e->modifiers == 0) {
                CGEventSetIntegerValueField(event, kCGKeyboardEventKeycode, e->to);
                return event;
            }
            postKeyWithModifiers(e->to, e->modifiers, true);
            return NULL;
        }
    } else if (type == kCGEventKeyUp) {
        transformFlags(event);

        KeymapEntry *e = findEntry(keyCode);
        if (e) {
            if (e->modifiers == 0) {
                CGEventSetIntegerValueField(event, kCGKeyboardEventKeycode, e->to);
                return event;
            }
            postKeyWithModifiers(e->to, e->modifiers, false);
            return NULL;
        }
    }

    return event;
}

int StartEventTap(KeymapEntry *keymap, int count, int imeSwitching) {
    memset(s_modKeyState, 0, sizeof(s_modKeyState));
    s_keymap       = keymap;
    s_keymapCount  = count;
    s_imeSwitching = imeSwitching;

    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown)
                     | CGEventMaskBit(kCGEventKeyUp)
                     | CGEventMaskBit(kCGEventFlagsChanged)
                     | CGEventMaskBit(kCGEventLeftMouseDown)
                     | CGEventMaskBit(kCGEventLeftMouseUp)
                     | CGEventMaskBit(kCGEventRightMouseDown)
                     | CGEventMaskBit(kCGEventRightMouseUp)
                     | CGEventMaskBit(kCGEventOtherMouseDown)
                     | CGEventMaskBit(kCGEventOtherMouseUp);

    s_eventTap = CGEventTapCreate(
        kCGHIDEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        eventCallback,
        NULL
    );
    if (!s_eventTap) return 0;

    s_runLoopSrc = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, s_eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), s_runLoopSrc, kCFRunLoopCommonModes);
    CGEventTapEnable(s_eventTap, true);
    return 1;
}

void UpdateKeymap(KeymapEntry *keymap, int count, int imeSwitching) {
    s_keymap       = keymap;
    s_keymapCount  = count;
    s_imeSwitching = imeSwitching;
    memset(s_modKeyState, 0, sizeof(s_modKeyState));
    atomic_store(&leftCmdAlone, 0);
    atomic_store(&rightCmdAlone, 0);
}

void StopEventTap(void) {
    if (!s_eventTap) return;
    CGEventTapEnable(s_eventTap, false);
    CFRunLoopRemoveSource(CFRunLoopGetMain(), s_runLoopSrc, kCFRunLoopCommonModes);
    CFRelease(s_runLoopSrc);
    CFRelease(s_eventTap);
    s_eventTap   = NULL;
    s_runLoopSrc = NULL;
}
