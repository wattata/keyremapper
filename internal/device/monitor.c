#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdint.h>

extern void goDeviceInputCallback(uint32_t vendorID, uint32_t productID);

static IOHIDManagerRef s_hidMonitor = NULL;

static uint32_t monitorVendorID(IOHIDDeviceRef d) {
    CFNumberRef r = IOHIDDeviceGetProperty(d, CFSTR(kIOHIDVendorIDKey));
    if (!r) return 0;
    uint32_t v = 0;
    CFNumberGetValue(r, kCFNumberSInt32Type, &v);
    return v;
}

static uint32_t monitorProductID(IOHIDDeviceRef d) {
    CFNumberRef r = IOHIDDeviceGetProperty(d, CFSTR(kIOHIDProductIDKey));
    if (!r) return 0;
    uint32_t v = 0;
    CFNumberGetValue(r, kCFNumberSInt32Type, &v);
    return v;
}

static void inputValueCallback(void *ctx, IOReturn result, void *sender, IOHIDValueRef value) {
    (void)ctx; (void)result; (void)value;
    IOHIDDeviceRef device = (IOHIDDeviceRef)sender;
    goDeviceInputCallback(monitorVendorID(device), monitorProductID(device));
}

void StartDeviceMonitor(void) {
    int usagePage = kHIDPage_GenericDesktop;
    int usage     = kHIDUsage_GD_Keyboard;

    CFNumberRef pageRef  = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
    CFNumberRef usageRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);

    CFStringRef keys[]   = { CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey) };
    CFTypeRef   values[] = { pageRef, usageRef };

    CFDictionaryRef matching = CFDictionaryCreate(
        kCFAllocatorDefault,
        (const void **)keys,
        (const void **)values,
        2,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    CFRelease(pageRef);
    CFRelease(usageRef);

    s_hidMonitor = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    IOHIDManagerSetDeviceMatching(s_hidMonitor, matching);
    CFRelease(matching);

    IOHIDManagerRegisterInputValueCallback(s_hidMonitor, inputValueCallback, NULL);
    IOHIDManagerScheduleWithRunLoop(s_hidMonitor, CFRunLoopGetMain(), kCFRunLoopCommonModes);
    IOHIDManagerOpen(s_hidMonitor, kIOHIDOptionsTypeNone);
}
