//go:build darwin

package main

/*
#cgo LDFLAGS: -framework ApplicationServices -framework CoreFoundation
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>

void CFRunLoopRun(void);
void CFRunLoopStop(CFRunLoopRef rl);
CFRunLoopRef CFRunLoopGetMain(void);

static int requestAccessibility(void) {
    CFStringRef keys[]   = { kAXTrustedCheckOptionPrompt };
    CFBooleanRef values[] = { kCFBooleanTrue };
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault,
        (const void **)keys,
        (const void **)values,
        1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    int trusted = (int)AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
    return trusted;
}
*/
import "C"

import (
	"flag"
	"fmt"
	"os"
	"os/signal"
	"runtime"
	"syscall"

	"github.com/wattata/keyremapper/internal/config"
	"github.com/wattata/keyremapper/internal/device"
	"github.com/wattata/keyremapper/internal/keytap"
)

func init() {
	runtime.LockOSThread()
}

func resolveProfile(settings *config.Settings, devices []device.Info) *config.Profile {
	for _, dev := range devices {
		for i := range settings.Profiles {
			p := &settings.Profiles[i]
			if p.MatchesDevice(dev.VendorID, dev.ProductID) {
				return p
			}
		}
	}
	return settings.Common
}

func profileName(p *config.Profile) string {
	if p == nil {
		return "(なし)"
	}
	if p.Name != "" {
		return p.Name
	}
	return "common"
}

func main() {
	listDevicesFlag := flag.Bool("list-devices", false, "認識中のキーボードとプロファイルを表示")
	configPath := flag.String("config", "", "設定ファイルのパス（デフォルト: ~/.config/keyremapper/settings.json）")
	flag.Parse()

	if *listDevicesFlag {
		device.List()
		os.Exit(0)
	}

	settings, err := config.Load(*configPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "設定ファイルの読み込みに失敗しました: %v\n", err)
		os.Exit(1)
	}

	// 設定がない場合はハードウェアデフォルト（何もしない）
	if settings == nil {
		fmt.Fprintln(os.Stderr, "設定ファイルが見つかりません。ハードウェアデフォルトで動作します。")
		waitForSignal()
		return
	}

	if C.requestAccessibility() == 0 {
		fmt.Fprintln(os.Stderr, "アクセシビリティ権限がありません。システムダイアログで許可してから再度起動してください。")
		os.Exit(1)
	}

	devices := device.ListConnected()
	initialProfile := resolveProfile(settings, devices)

	var initKeymap []config.KeymapEntry
	initIME := false
	if initialProfile != nil {
		initKeymap = initialProfile.Keymap
		initIME = initialProfile.IMESwitching
	}
	fmt.Fprintf(os.Stderr, "初期プロファイル: %s\n", profileName(initialProfile))

	if !keytap.Start(initKeymap, initIME) {
		fmt.Fprintln(os.Stderr, "EventTap の作成に失敗しました。アクセシビリティ権限を確認してください。")
		os.Exit(1)
	}
	fmt.Fprintln(os.Stderr, "EventTap 起動成功")

	var lastDeviceKey uint64
	device.SetInputHandler(func(vendorID, productID uint32) {
		key := uint64(vendorID)<<32 | uint64(productID)
		if key == lastDeviceKey {
			return
		}
		lastDeviceKey = key
		p := resolveProfile(settings, []device.Info{{VendorID: vendorID, ProductID: productID}})
		var keymap []config.KeymapEntry
		ime := false
		if p != nil {
			keymap = p.Keymap
			ime = p.IMESwitching
		}
		keytap.Update(keymap, ime)
		fmt.Fprintf(os.Stderr, "プロファイル切り替え: %s\n", profileName(p))
	})
	device.StartMonitor()

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGTERM, syscall.SIGINT)
	go func() {
		<-sigCh
		C.CFRunLoopStop(C.CFRunLoopGetMain())
	}()

	C.CFRunLoopRun()

	keytap.Stop()
}

func waitForSignal() {
	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGTERM, syscall.SIGINT)
	<-sigCh
}
