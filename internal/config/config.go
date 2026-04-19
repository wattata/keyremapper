//go:build darwin

package config

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

type KeymapEntry struct {
	From      int
	To        int
	Modifiers uint32
}

type Profile struct {
	Name         string        `json:"name,omitempty"`
	VendorID     string        `json:"vendor_id,omitempty"`
	ProductID    string        `json:"product_id,omitempty"`
	IMESwitching bool          `json:"ime_switching"`
	Keymap       []KeymapEntry `json:"keymap"`
}

type Settings struct {
	Common   *Profile  `json:"common,omitempty"`
	Profiles []Profile `json:"profiles,omitempty"`
}

var modifierMap = map[string]uint32{
	"command": 0x00100000,
	"shift":   0x00020000,
	"option":  0x00080000,
	"control": 0x00040000,
}

func (p *Profile) MatchesDevice(vendorID, productID uint32) bool {
	return parseHexID(p.VendorID) == vendorID && parseHexID(p.ProductID) == productID
}

func parseHexID(s string) uint32 {
	s = strings.TrimPrefix(strings.ToLower(s), "0x")
	v, _ := strconv.ParseUint(s, 16, 32)
	return uint32(v)
}

func validateHexID(field, value string) error {
	if value == "" {
		return nil
	}
	s := strings.TrimPrefix(strings.ToLower(value), "0x")
	if _, err := strconv.ParseUint(s, 16, 32); err != nil {
		return fmt.Errorf("%s に無効な16進数 ID: %q", field, value)
	}
	return nil
}

type settingsJSON struct {
	Common   *profileJSON  `json:"common,omitempty"`
	Profiles []profileJSON `json:"profiles,omitempty"`
}

type profileJSON struct {
	Name         string `json:"name,omitempty"`
	VendorID     string `json:"vendor_id,omitempty"`
	ProductID    string `json:"product_id,omitempty"`
	IMESwitching bool   `json:"ime_switching"`
	Keymap       []struct {
		From      int      `json:"from"`
		To        int      `json:"to"`
		Modifiers []string `json:"modifiers,omitempty"`
	} `json:"keymap"`
}

func Load(path string) (*Settings, error) {
	if path == "" {
		home, err := os.UserHomeDir()
		if err != nil {
			return nil, fmt.Errorf("ホームディレクトリの取得に失敗: %w", err)
		}
		path = filepath.Join(home, ".config", "keyremapper", "settings.json")
	}

	data, err := os.ReadFile(path)
	if os.IsNotExist(err) {
		return nil, nil
	}
	if err != nil {
		return nil, err
	}

	var s settingsJSON
	if err := json.Unmarshal(data, &s); err != nil {
		return nil, err
	}

	settings := &Settings{}
	if s.Common != nil {
		p, err := convertProfile(s.Common)
		if err != nil {
			return nil, err
		}
		settings.Common = &p
	}
	for _, pj := range s.Profiles {
		p, err := convertProfile(&pj)
		if err != nil {
			return nil, err
		}
		settings.Profiles = append(settings.Profiles, p)
	}
	return settings, nil
}

func convertProfile(pj *profileJSON) (Profile, error) {
	if err := validateHexID("vendor_id", pj.VendorID); err != nil {
		return Profile{}, err
	}
	if err := validateHexID("product_id", pj.ProductID); err != nil {
		return Profile{}, err
	}
	p := Profile{
		Name:         pj.Name,
		VendorID:     pj.VendorID,
		ProductID:    pj.ProductID,
		IMESwitching: pj.IMESwitching,
	}
	for _, e := range pj.Keymap {
		var mods uint32
		for _, m := range e.Modifiers {
			if v, ok := modifierMap[m]; ok {
				mods |= v
			}
		}
		p.Keymap = append(p.Keymap, KeymapEntry{From: e.From, To: e.To, Modifiers: mods})
	}
	return p, nil
}
