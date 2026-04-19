//go:build darwin

package config

import (
	"os"
	"path/filepath"
	"testing"
)

func TestLoad_FileNotExists(t *testing.T) {
	s, err := Load("/nonexistent/path/settings.json")
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if s != nil {
		t.Fatal("expected nil settings for missing file")
	}
}

func TestLoad_EmptyJSON(t *testing.T) {
	f := writeTempJSON(t, `{}`)
	s, err := Load(f)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if s.Common != nil || len(s.Profiles) != 0 {
		t.Error("expected empty settings")
	}
}

func TestLoad_CommonProfile(t *testing.T) {
	f := writeTempJSON(t, `{
		"common": {
			"ime_switching": true,
			"keymap": [{"from": 58, "to": 55}]
		}
	}`)
	s, err := Load(f)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if s.Common == nil {
		t.Fatal("expected common profile")
	}
	if !s.Common.IMESwitching {
		t.Error("expected ime_switching=true")
	}
	if len(s.Common.Keymap) != 1 || s.Common.Keymap[0].From != 58 {
		t.Errorf("unexpected keymap: %+v", s.Common.Keymap)
	}
}

func TestLoad_SpecificProfile(t *testing.T) {
	f := writeTempJSON(t, `{
		"profiles": [
			{
				"name": "Test KB",
				"vendor_id": "0x045E",
				"product_id": "0x07A5",
				"ime_switching": true,
				"keymap": [{"from": 58, "to": 55}]
			}
		]
	}`)
	s, err := Load(f)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(s.Profiles) != 1 {
		t.Fatalf("expected 1 profile, got %d", len(s.Profiles))
	}
	p := s.Profiles[0]
	if p.VendorID != "0x045E" || p.ProductID != "0x07A5" {
		t.Errorf("unexpected ids: %s %s", p.VendorID, p.ProductID)
	}
}

func TestMatchesDevice(t *testing.T) {
	p := Profile{VendorID: "0x045E", ProductID: "0x07A5"}
	if !p.MatchesDevice(0x045E, 0x07A5) {
		t.Error("expected match")
	}
	if p.MatchesDevice(0x045E, 0x0001) {
		t.Error("expected no match on different product id")
	}
}

func TestLoad_ModifiersCommand(t *testing.T) {
	f := writeTempJSON(t, `{
		"common": {
			"keymap": [{"from": 115, "to": 123, "modifiers": ["command"]}]
		}
	}`)
	s, err := Load(f)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if s.Common.Keymap[0].Modifiers != 0x00100000 {
		t.Errorf("expected 0x00100000, got 0x%08X", s.Common.Keymap[0].Modifiers)
	}
}

func TestLoad_MultipleModifiers(t *testing.T) {
	f := writeTempJSON(t, `{
		"common": {
			"keymap": [{"from": 1, "to": 2, "modifiers": ["command", "shift"]}]
		}
	}`)
	s, err := Load(f)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	want := uint32(0x00100000 | 0x00020000)
	if s.Common.Keymap[0].Modifiers != want {
		t.Errorf("expected 0x%08X, got 0x%08X", want, s.Common.Keymap[0].Modifiers)
	}
}

func TestLoad_InvalidJSON(t *testing.T) {
	f := writeTempJSON(t, `{invalid}`)
	_, err := Load(f)
	if err == nil {
		t.Fatal("expected error for invalid JSON")
	}
}

func writeTempJSON(t *testing.T, content string) string {
	t.Helper()
	dir := t.TempDir()
	path := filepath.Join(dir, "settings.json")
	if err := os.WriteFile(path, []byte(content), 0644); err != nil {
		t.Fatal(err)
	}
	return path
}
