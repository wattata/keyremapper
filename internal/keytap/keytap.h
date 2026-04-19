#ifndef KEYTAP_H
#define KEYTAP_H

#include <stdint.h>
#include <CoreGraphics/CoreGraphics.h>

// CGEventFlags のデバイス固有ビット + 汎用マスクビット
// 上位16bitは config.go modifierMap の汎用マスク (kCGEventFlagMask*) と対応
#define KEYTAP_FLAGS_LEFT_CMD    ((CGEventFlags)0x00100008)
#define KEYTAP_FLAGS_RIGHT_CMD   ((CGEventFlags)0x00100010)
#define KEYTAP_FLAGS_LEFT_SHIFT  ((CGEventFlags)0x00020002)
#define KEYTAP_FLAGS_RIGHT_SHIFT ((CGEventFlags)0x00020004)
#define KEYTAP_FLAGS_LEFT_OPT    ((CGEventFlags)0x00080020)
#define KEYTAP_FLAGS_RIGHT_OPT   ((CGEventFlags)0x00080040)
#define KEYTAP_FLAGS_LEFT_CTRL   ((CGEventFlags)0x00040001)
#define KEYTAP_FLAGS_RIGHT_CTRL  ((CGEventFlags)0x00042000)

typedef struct {
    int      from;
    int      to;
    uint32_t modifiers;
} KeymapEntry;

int  StartEventTap(KeymapEntry *keymap, int count, int imeSwitching);
void UpdateKeymap(KeymapEntry *keymap, int count, int imeSwitching);
void StopEventTap(void);

#endif
