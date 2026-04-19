//go:build darwin

package device

/*
#cgo LDFLAGS: -framework IOKit -framework CoreFoundation
#include <stdint.h>

typedef struct {
    uint32_t vendor_id;
    uint32_t product_id;
} DeviceInfo;

void ListDevices(void);
int GetConnectedKeyboards(DeviceInfo *out, int maxCount);
*/
import "C"
import "unsafe"

type Info struct {
	VendorID  uint32
	ProductID uint32
}

func List() {
	C.ListDevices()
}

func ListConnected() []Info {
	const maxDevices = 32
	cDevices := make([]C.DeviceInfo, maxDevices)
	count := int(C.GetConnectedKeyboards(
		(*C.DeviceInfo)(unsafe.Pointer(&cDevices[0])),
		C.int(maxDevices),
	))
	result := make([]Info, count)
	for i := 0; i < count; i++ {
		result[i] = Info{
			VendorID:  uint32(cDevices[i].vendor_id),
			ProductID: uint32(cDevices[i].product_id),
		}
	}
	return result
}
