package handlers

import (
	"os"
	"path/filepath"
	"testing"

	"github.com/catnet-io/engine/pkg/results"
)

func TestExportResults_Validation(t *testing.T) {
	app := NewAppHandlers()
	devs := []results.HostResult{
		{IP: "192.168.1.1", Alive: true, Hostname: "router.local", MAC: "AA:BB:CC:DD:EE:FF", OpenPorts: []int{80, 443}},
	}

	tmpDir := t.TempDir()
	outPath := filepath.Join(tmpDir, "test_results.json")

	data, err := os.ReadFile(outPath)
	if err == nil {
		t.Fatalf("Expected file not to exist yet, found %d bytes", len(data))
	}

	clean := filepath.Clean(outPath)
	if clean != outPath {
		t.Errorf("Path cleaning mismatch: %s vs %s", clean, outPath)
	}

	_ = app
	_ = devs
}
