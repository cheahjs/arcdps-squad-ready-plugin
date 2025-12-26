INSTALL_PATH ?= addons/arcdps/arcdps_squad_ready.dll
FULL_INSTALL_PATH ?= ${GW2_PATH}/$(INSTALL_PATH)
TARGET ?= x86_64-pc-windows-msvc
TARGET_DIR = target/$(TARGET)

# Detect OS
ifeq ($(OS),Windows_NT)
    CARGO = cargo
else
    CARGO = cargo xwin
endif

# Common cargo command
CARGO_BUILD = $(CARGO) build --target $(TARGET)

.PHONY: build-debug build build-release copy-debug copy copy-release install-debug install install-release lint

build-debug:
	$(CARGO_BUILD)

build:
	$(CARGO_BUILD) --profile=release-with-debug

build-release:
	$(CARGO_BUILD) --release

copy-debug:
	cp -f $(TARGET_DIR)/debug/arcdps_squad_ready.dll "$(FULL_INSTALL_PATH)"

copy:
	cp -f $(TARGET_DIR)/release-with-debug/arcdps_squad_ready.dll "$(FULL_INSTALL_PATH)"

copy-release:
	cp -f $(TARGET_DIR)/release/arcdps_squad_ready.dll "$(FULL_INSTALL_PATH)"

install-debug: build-debug copy-debug

install: build copy

install-release: build-release copy-release

lint:
	cargo fmt
	$(CARGO) clippy --target $(TARGET)
