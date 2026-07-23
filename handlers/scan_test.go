package handlers

import (
	"testing"
)

func TestAppHandlers_StopScan(t *testing.T) {
	app := NewAppHandlers()
	if app == nil {
		t.Fatal("Expected NewAppHandlers to return non-nil")
	}

	// Test StopScan when engine is present
	app.StopScan()
}
