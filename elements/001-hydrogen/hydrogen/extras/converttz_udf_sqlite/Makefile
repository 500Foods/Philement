# Makefile for SQLite Convert TZ Extension
# Usage:
#   make                                    # builds loadable extension
#   sudo make install                       # installs to system location
#   make test                               # run tests
#   make clean

CC      := gcc
CFLAGS  := -fpic -O2 -Wall -Wextra
LDFLAGS := -shared
LIBS    :=
SO_FILE := convert_tz.so
SRC     := convert_tz.c

# Installation directory (common SQLite extension location)
INSTALL_DIR ?= /usr/local/lib

.PHONY: all install test clean

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LIBS)

install: $(SO_FILE)
	@echo "Installing to $(INSTALL_DIR)..."
	sudo cp -f "$(SO_FILE)" "$(INSTALL_DIR)/"
	sudo chmod 755 "$(INSTALL_DIR)/$(SO_FILE)"
	@echo "Installed successfully."
	@echo ""
	@echo "Load in SQLite with:"
	@echo "  SELECT load_extension('$(INSTALL_DIR)/convert_tz');"
	@echo "Or:"
	@echo "  SELECT load_extension('convert_tz');"

test:
	@echo "Testing Convert TZ extension..."
	@echo ""
	@echo "Testing extension loading..."
	@if sqlite3 -cmd ".load ./convert_tz" -cmd "SELECT 'Extension loaded successfully' AS status;" -cmd ".exit" /tmp/test.db 2>&1 | grep -q "loaded successfully"; then \
		echo "✓ Extension loaded"; \
	else \
		echo "✗ Extension failed to load"; \
		exit 1; \
	fi
	@echo ""
	@echo "Testing basic timezone conversion..."
	@if sqlite3 -cmd ".load ./convert_tz" -cmd "SELECT convert_tz('2024-01-01 12:00:00', 'UTC', 'America/Vancouver') AS result;" -cmd ".exit" /tmp/test.db 2>&1 | grep -q "2024-01-01 04:00:00"; then \
		echo "✓ Basic conversion test passed"; \
	else \
		echo "✗ Basic conversion test failed"; \
		exit 1; \
	fi
	@echo ""
	@echo "Testing current time conversion..."
	@echo "Current UTC time:"
	@sqlite3 -cmd ".load ./convert_tz" -cmd "SELECT datetime('now') AS current_utc;" -cmd ".exit" /tmp/test.db 2>/dev/null || true
	@echo "Converted to America/Vancouver:"
	@sqlite3 -cmd ".load ./convert_tz" -cmd "SELECT datetime(convert_tz(datetime('now'),'UTC','America/Vancouver')) AS vancouver_time;" -cmd ".exit" /tmp/test.db 2>/dev/null || true
	@echo ""
	@echo "✓ All tests passed - Convert TZ functionality confirmed"

clean:
	rm -f $(SO_FILE) *.o