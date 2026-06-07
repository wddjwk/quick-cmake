# =============================================================================
# Quick-CMake Convenience Build Script
# =============================================================================
# Cross-platform: uses bash on Linux/macOS, pwsh on Windows
# =============================================================================

Generator := Ninja

BUILD_DIR := $(CURDIR)/build

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
  # On Windows, use PowerShell as the shell
  SHELL       := pwsh.exe
  .SHELLFLAGS := -NoProfile -Command
  WITH_ERROR  := 2>&1 | Out-Null
  RM_FLAGS    := -Recurse -Force -ErrorAction SilentlyContinue
else
  SHELL       := /usr/bin/bash
  WITH_ERROR  := > /dev/null 2>&1
endif

ANSI_CLEAR       := \033[0m
ANSI_INFO_COLOR  := \033[36;3;108m
ANSI_CHANGE_LINE := \033[A\033[2K

# ---------------------------------------------------------------------------
# Default target
# ---------------------------------------------------------------------------
.PHONY: all config build clean remove rebuild clangd ctest cpack coverage install format line help

all: build

# ---------------------------------------------------------------------------
# config — run CMake configure only
# ---------------------------------------------------------------------------
config:
ifeq ($(OS),Windows_NT)
	cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "$(Generator)"
else
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Configuring ...$(ANSI_CLEAR)"
	@cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "$(Generator)"
endif

# ---------------------------------------------------------------------------
# build — configure and build all targets
# ---------------------------------------------------------------------------
build:
ifeq ($(OS),Windows_NT)
	cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "$(Generator)"
	cmake --build $(BUILD_DIR)
else
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] BEGIN BUILD ALL TARGET ...$(ANSI_CLEAR)"
	@cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "$(Generator)"
	@cmake --build $(BUILD_DIR) -j10
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] BUILD DONE !$(ANSI_CLEAR)"
endif

# ---------------------------------------------------------------------------
# clean — remove build outputs but preserve build/_deps
# ---------------------------------------------------------------------------
clean:
ifeq ($(OS),Windows_NT)
	pwsh -NoProfile -Command 'if (Test-Path "$(BUILD_DIR)") { Get-ChildItem -Path "$(BUILD_DIR)" -Force -Exclude "_deps" | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue }'
else
	@cd $(BUILD_DIR) 2>/dev/null && find . -mindepth 1 -maxdepth 1 ! -name '_deps' -exec rm -rf {} + || true
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Cleaned (kept _deps)$(ANSI_CLEAR)"
endif

# ---------------------------------------------------------------------------
# remove — remove entire build directory
# ---------------------------------------------------------------------------
remove:
ifeq ($(OS),Windows_NT)
	pwsh -NoProfile -Command 'Remove-Item -Path "$(BUILD_DIR)" -Recurse -Force -ErrorAction SilentlyContinue'
else
	@rm -rf $(BUILD_DIR)
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Removed $(BUILD_DIR)$(ANSI_CLEAR)"
endif

# ---------------------------------------------------------------------------
# rebuild — clean + build
# ---------------------------------------------------------------------------
rebuild: clean build

# ---------------------------------------------------------------------------
# clangd — generate .clangd for clangd LSP with MSVC toolchain (Windows only)
# ---------------------------------------------------------------------------
clangd:
ifeq ($(OS),Windows_NT)
	pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/gen_clangd.ps1
else
	@echo "clangd target is only supported on Windows with MSVC"
endif

# ---------------------------------------------------------------------------
# ctest — rebuild tests and run CTest
# ---------------------------------------------------------------------------
ctest:
ifeq ($(OS),Windows_NT)
	pwsh -NoProfile -Command 'Get-ChildItem -Path "$(BUILD_DIR)" -Force -Exclude "_deps" | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue'
	cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "$(Generator)"
	cmake --build $(BUILD_DIR)
	cd $(BUILD_DIR) && ctest
else
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] ReBuilding Tests ...$(ANSI_CLEAR)"
	@cd $(BUILD_DIR) 2>/dev/null && find . -mindepth 1 -maxdepth 1 ! -name '_deps' -exec rm -rf {} + || true
	@cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "$(Generator)" $(WITH_ERROR)
	@cmake --build $(BUILD_DIR) -j10 $(WITH_ERROR)
	@echo -en "$(ANSI_CHANGE_LINE)"
	@cd $(BUILD_DIR) && ctest
endif

# ---------------------------------------------------------------------------
# cpack — configure release build and pack
# ---------------------------------------------------------------------------
cpack:
ifeq ($(OS),Windows_NT)
	cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -G "$(Generator)"
	cd $(BUILD_DIR) && cpack --config CPackConfig.cmake
	cmake --install $(BUILD_DIR) --prefix $(BUILD_DIR)/installed
else
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Preparing To Pack And Install ...$(ANSI_CLEAR)"
	@cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -G "$(Generator)" $(WITH_ERROR)
	@cd $(BUILD_DIR) && cpack --config CPackConfig.cmake
	@cmake --install $(BUILD_DIR) --prefix $(BUILD_DIR)/installed
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Packed Successfully!$(ANSI_CLEAR)"
endif

# ---------------------------------------------------------------------------
# coverage — build with coverage (Linux only, requires gcov/lcov)
# ---------------------------------------------------------------------------
coverage:
ifeq ($(OS),Windows_NT)
	@echo "Coverage is only supported on Linux"
else
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] ReBuilding Projects ...$(ANSI_CLEAR)"
	@rm -rf $(BUILD_DIR)/*
	@cmake -S $(CURDIR) -B $(BUILD_DIR) -DBUILD_WITH_COVERAGE=ON -G "$(Generator)" $(WITH_ERROR)
	@cmake --build $(BUILD_DIR) -j10 $(WITH_ERROR)
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Calculating Coverage ...$(ANSI_CLEAR)"
	@cmake --build $(BUILD_DIR) --target=coverage | tail -n 3
	@echo -e "$(ANSI_INFO_COLOR)Done!$(ANSI_CLEAR)"
	@echo -e "$(ANSI_INFO_COLOR)[COVERAGE_FILE]:$(ANSI_CLEAR) $(BUILD_DIR)/coverage_report/index.html"
endif

# ---------------------------------------------------------------------------
# install — build release and install
# ---------------------------------------------------------------------------
install:
ifeq ($(OS),Windows_NT)
	cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -G "$(Generator)"
	cmake --build $(BUILD_DIR) --target=install
else
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Preparing To Install ...$(ANSI_CLEAR)"
	@cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -G "$(Generator)" $(WITH_ERROR)
	@sudo cmake --build $(BUILD_DIR) --target=install
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Install Successfully!$(ANSI_CLEAR)"
endif

# ---------------------------------------------------------------------------
# format — run clang-format on all source files
# ---------------------------------------------------------------------------
format:
ifeq ($(OS),Windows_NT)
	pwsh -NoProfile -Command "Get-ChildItem -Recurse -Include *.cpp,*.h,*.hpp | ForEach-Object { clang-format -i $$_ }"
else
	@fdfind -e cpp -e h -e hpp -x clang-format -i
	@echo -e "$(ANSI_INFO_COLOR)Done!$(ANSI_CLEAR)"
endif

# ---------------------------------------------------------------------------
# line — count total lines of source code
# ---------------------------------------------------------------------------
line:
ifeq ($(OS),Windows_NT)
	pwsh -NoProfile -Command "(Get-ChildItem -Recurse -Include *.h,*.cpp,*.txt,*.cmake | Get-Content | Measure-Object -Line).Lines | ForEach-Object { Write-Host \"Total Lines: $$_\" }"
else
	@fdfind -e h -e cpp -e txt -e cmake -x wc -l | awk '{ line += $$1 } END { print "Total Lines: " line }'
endif

# ---------------------------------------------------------------------------
# Build specific target (catch-all)
# ---------------------------------------------------------------------------
%:
ifeq ($(OS),Windows_NT)
	cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "$(Generator)"
	cmake --build $(BUILD_DIR) --target=$@
	pwsh -NoProfile -Command '& "$(BUILD_DIR)/bin/$@.exe"'
else
	@echo -e "$(ANSI_INFO_COLOR)[MAKE] Building Target $@ ...$(ANSI_CLEAR)"
	@cmake -S $(CURDIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "$(Generator)" $(WITH_ERROR)
	@cmake --build $(BUILD_DIR) --target=$@ -j2 $(WITH_ERROR)
	@echo -en "$(ANSI_CHANGE_LINE)"
	@cd $(BUILD_DIR)/bin && ./$@
endif

# ---------------------------------------------------------------------------
# help — show available targets
# ---------------------------------------------------------------------------
help:
	@echo "Targets:"
	@echo "  all (default)  - configure + build"
	@echo "  config         - CMake configure only"
	@echo "  build          - configure + build"
	@echo "  clean          - remove build outputs (keep build/_deps)"
	@echo "  remove         - remove entire build directory"
	@echo "  rebuild        - clean + build"
	@echo "  clangd         - generate .clangd for MSVC clangd support (Windows only)"
	@echo "  ctest          - rebuild tests and run CTest"
	@echo "  cpack          - build release and pack"
	@echo "  coverage       - build with coverage (Linux only)"
	@echo "  install        - build release and install"
	@echo "  format         - run clang-format on all source files"
	@echo "  line           - count total lines of source code"
	@echo "  <target>       - build and run a specific target"
	@echo "  help           - show this help"
