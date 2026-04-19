//go:build darwin

package keytap

/*
#cgo LDFLAGS: -framework CoreGraphics -framework Carbon -framework CoreFoundation
#include "keytap.h"
*/
import "C"
import (
	"unsafe"

	"github.com/wattata/keyremapper/internal/config"
)

// Goのガベージコレクタがポインタを回収しないよう、パッケージスコープで保持する
var activeKeymap []C.KeymapEntry

func Start(keymap []config.KeymapEntry, imeSwitching bool) bool {
	ime := C.int(0)
	if imeSwitching {
		ime = C.int(1)
	}

	if len(keymap) == 0 {
		activeKeymap = nil
		return C.StartEventTap(nil, 0, ime) == 1
	}

	activeKeymap = make([]C.KeymapEntry, len(keymap))
	for i, e := range keymap {
		activeKeymap[i] = C.KeymapEntry{
			from:      C.int(e.From),
			to:        C.int(e.To),
			modifiers: C.uint32_t(e.Modifiers),
		}
	}
	return C.StartEventTap(
		(*C.KeymapEntry)(unsafe.Pointer(&activeKeymap[0])),
		C.int(len(activeKeymap)),
		ime,
	) == 1
}

func Update(keymap []config.KeymapEntry, imeSwitching bool) {
	ime := C.int(0)
	if imeSwitching {
		ime = C.int(1)
	}

	if len(keymap) == 0 {
		activeKeymap = nil
		C.UpdateKeymap(nil, 0, ime)
		return
	}

	newKeymap := make([]C.KeymapEntry, len(keymap))
	for i, e := range keymap {
		newKeymap[i] = C.KeymapEntry{
			from:      C.int(e.From),
			to:        C.int(e.To),
			modifiers: C.uint32_t(e.Modifiers),
		}
	}
	activeKeymap = newKeymap
	C.UpdateKeymap(
		(*C.KeymapEntry)(unsafe.Pointer(&activeKeymap[0])),
		C.int(len(activeKeymap)),
		ime,
	)
}

func Stop() {
	C.StopEventTap()
}
