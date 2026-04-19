#include <stdio.h>
#include <stdlib.h>
#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>

typedef struct {
    uint32_t vendor_id;
    uint32_t product_id;
} DeviceInfo;

static CFDictionaryRef createKeyboardMatchingDict(void) {
    int usagePage = kHIDPage_GenericDesktop;
    int usage     = kHIDUsage_GD_Keyboard;

    CFNumberRef pageRef  = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
    CFNumberRef usageRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);

    CFStringRef keys[]   = { CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey) };
    CFTypeRef   values[] = { pageRef, usageRef };

    CFDictionaryRef dict = CFDictionaryCreate(
        kCFAllocatorDefault,
        (const void **)keys,
        (const void **)values,
        2,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    CFRelease(pageRef);
    CFRelease(usageRef);
    return dict;
}

static uint32_t getVendorID(IOHIDDeviceRef device) {
    CFNumberRef ref = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
    if (!ref) return 0;
    uint32_t val = 0;
    CFNumberGetValue(ref, kCFNumberSInt32Type, &val);
    return val;
}

static uint32_t getProductID(IOHIDDeviceRef device) {
    CFNumberRef ref = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
    if (!ref) return 0;
    uint32_t val = 0;
    CFNumberGetValue(ref, kCFNumberSInt32Type, &val);
    return val;
}

static const char *getProductName(IOHIDDeviceRef device) {
    CFStringRef ref = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
    if (!ref) return "(unknown)";
    static char buf[256];
    CFStringGetCString(ref, buf, sizeof(buf), kCFStringEncodingUTF8);
    return buf;
}

static CFSetRef openKeyboards(void) {
    IOHIDManagerRef mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    CFDictionaryRef matching = createKeyboardMatchingDict();
    IOHIDManagerSetDeviceMatching(mgr, matching);
    CFRelease(matching);
    IOHIDManagerOpen(mgr, kIOHIDOptionsTypeNone);
    CFSetRef devices = IOHIDManagerCopyDevices(mgr);
    CFRelease(mgr);
    return devices;
}

void ListDevices(void) {
    CFSetRef devices = openKeyboards();
    if (!devices) {
        printf("No keyboards detected.\n");
        return;
    }

    CFIndex count = CFSetGetCount(devices);
    IOHIDDeviceRef *list = calloc(count, sizeof(IOHIDDeviceRef));
    if (!list) { CFRelease(devices); return; }
    CFSetGetValues(devices, (const void **)list);

    printf("Detected keyboards:\n");
    for (CFIndex i = 0; i < count; i++) {
        uint32_t vid    = getVendorID(list[i]);
        uint32_t pid    = getProductID(list[i]);
        const char *name = getProductName(list[i]);
        printf("  [0x%04X:0x%04X] %s\n", vid, pid, name);
    }

    free(list);
    CFRelease(devices);
}

int GetConnectedKeyboards(DeviceInfo *out, int maxCount) {
    CFSetRef devices = openKeyboards();
    if (!devices) return 0;

    CFIndex count = CFSetGetCount(devices);
    if (count > maxCount) count = maxCount;

    IOHIDDeviceRef *list = calloc(count, sizeof(IOHIDDeviceRef));
    if (!list) { CFRelease(devices); return 0; }
    CFSetGetValues(devices, (const void **)list);

    for (CFIndex i = 0; i < count; i++) {
        out[i].vendor_id  = getVendorID(list[i]);
        out[i].product_id = getProductID(list[i]);
    }

    free(list);
    CFRelease(devices);
    return (int)count;
}
