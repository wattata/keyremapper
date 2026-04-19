//go:build darwin

package device

/*
#include <stdint.h>
void StartDeviceMonitor(void);
*/
import "C"

var deviceInputHandler func(vendorID, productID uint32)

func SetInputHandler(h func(vendorID, productID uint32)) {
	deviceInputHandler = h
}

func StartMonitor() {
	C.StartDeviceMonitor()
}

//export goDeviceInputCallback
func goDeviceInputCallback(vendorID C.uint32_t, productID C.uint32_t) {
	if deviceInputHandler != nil {
		deviceInputHandler(uint32(vendorID), uint32(productID))
	}
}
