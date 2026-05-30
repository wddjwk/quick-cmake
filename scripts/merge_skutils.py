#!/usr/bin/env python3
"""Merge multiple skutils headers into a single header-only file.

Usage: merge_skutils.py <output_file> <input_file1> <input_file2> ...
  Input files must be in dependency order (dependencies first).
"""

import re
import sys
import os
from collections import OrderedDict


# Section display names (basename → name)
SECTION_NAMES = {
    "noncopyable.h": "NonCopyable",
    "spinlock.h": "SpinLock",
    "config.h": "Config",
    "string_utils.h": "String Utilities",
    "time_utils.h": "Time Utilities",
    "printer.h": "Printer",
    "logger.h": "Logger",
}


def parse_header(filepath):
    """Parse a header file, returning (std_includes, body_lines).

    - Strips include guards (#ifndef/#define at top, #endif at bottom)
    - Collects #include <...> (standard library) separately
    - Removes #include "..." (internal project includes)
    - Preserves everything else as body
    """
    with open(filepath) as f:
        lines = f.readlines()

    std_includes = []
    body_lines = []

    # Detect and strip include guard
    guard_macro = None
    start_idx = 0
    end_idx = len(lines)

    # Find include guard at top (first non-empty, non-comment lines)
    for i, line in enumerate(lines):
        stripped = line.strip()
        if not stripped or stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*"):
            continue
        m = re.match(r"#ifndef\s+(\w+)", stripped)
        if m:
            guard_macro = m.group(1)
            # Check next non-empty line for matching #define
            for j in range(i + 1, min(i + 3, len(lines))):
                if re.match(rf"#define\s+{re.escape(guard_macro)}\b", lines[j].strip()):
                    start_idx = j + 1
                    break
        break

    # Find matching #endif at bottom
    if guard_macro:
        for i in range(len(lines) - 1, max(0, len(lines) - 5), -1):
            stripped = lines[i].strip()
            if stripped.startswith("#endif") and (guard_macro in stripped or stripped == "#endif"):
                end_idx = i
                break

    # Process lines between guards
    for i in range(start_idx, end_idx):
        line = lines[i]
        stripped = line.strip()

        # Collect standard includes
        m = re.match(r"#include\s+<(.+?)>", stripped)
        if m:
            std_includes.append(m.group(1))
            continue

        # Skip internal includes
        if re.match(r'#include\s+"', stripped):
            continue

        body_lines.append(line)

    # Strip leading/trailing blank lines
    while body_lines and body_lines[0].strip() == "":
        body_lines.pop(0)
    while body_lines and body_lines[-1].strip() == "":
        body_lines.pop()

    return std_includes, body_lines


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <output_file> <input1.h> [input2.h ...]", file=sys.stderr)
        sys.exit(1)

    output_file = sys.argv[1]
    input_files = sys.argv[2:]

    all_std_includes = OrderedDict()
    sections = []

    for filepath in input_files:
        if not os.path.exists(filepath):
            print(f"Error: {filepath} not found", file=sys.stderr)
            sys.exit(1)

        basename = os.path.basename(filepath)
        std_includes, body = parse_header(filepath)

        for inc in std_includes:
            all_std_includes[inc] = True

        section_name = SECTION_NAMES.get(basename, basename)
        sections.append((section_name, body))

    sorted_includes = sorted(all_std_includes.keys())

    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_file) or ".", exist_ok=True)

    with open(output_file, "w") as f:
        # File header
        f.write("/// @file skutils.h\n")
        f.write("/// @brief Single header-only library merging printer, logger, and all dependencies.\n")
        f.write('///        Just `#include "skutils.h"` — no other headers needed.\n')
        f.write("///\n")
        f.write("/// AUTO-GENERATED — do not edit manually. Modify source headers and rebuild.\n")
        basenames = [os.path.basename(fp) for fp in input_files]
        f.write(f"/// Merged from: {', '.join(basenames)}\n")
        f.write("\n")
        f.write("#ifndef SK_HEADER_ONLY_SKUTILS_H\n")
        f.write("#define SK_HEADER_ONLY_SKUTILS_H\n")
        f.write("\n")

        # Standard includes
        sep = "=" * 76
        f.write(f"// {sep}\n")
        f.write("// Standard library includes\n")
        f.write(f"// {sep}\n")
        f.write("\n")
        for inc in sorted_includes:
            f.write(f"#include <{inc}>\n")
        f.write("\n")

        # Sections
        for name, body in sections:
            f.write(f"// {sep}\n")
            f.write(f"// {name}\n")
            f.write(f"// {sep}\n")
            f.write("\n")
            for line in body:
                f.write(line)
            f.write("\n\n")

        f.write("#endif  // SK_HEADER_ONLY_SKUTILS_H\n")

    size = os.path.getsize(output_file)
    print(f"Generated {output_file} ({size} bytes, {len(sections)} sections, {len(sorted_includes)} includes)")


if __name__ == "__main__":
    main()
