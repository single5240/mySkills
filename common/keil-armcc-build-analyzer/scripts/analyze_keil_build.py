#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
analyze_keil_build.py
Parse Keil MDK (.build_log.htm / build log) and .map file to extract:
1. Compiler version, Error count, Warning count & detailed warning list
2. Program Size (Code, RO-data, RW-data, ZI-data) and Flash/RAM totals
3. Delta comparison against a saved baseline JSON
4. Top largest RAM (ZI/RW) and Flash (Code/RO) symbols from the .map file
"""

import argparse
import html
import json
import os
import re
import sys
from typing import Dict, List, Optional, Any

RE_COMPILER = re.compile(r"Toolchain:\s*(.+)|Compiler:\s*(.+)|ARMCC\s+V[\d.]+.*")
RE_PROG_SIZE = re.compile(
    r"Program Size:\s*Code=(\d+)\s+RO-data=(\d+)\s+RW-data=(\d+)\s+ZI-data=(\d+)"
)
RE_SUMMARY = re.compile(r'"[^"]+"\s*-\s*(\d+)\s*Error\(s\),\s*(\d+)\s*Warning\(s\)\.')
RE_DIAG_LINE = re.compile(
    r"^(.+?)\((\d+)\):\s*(warning|error):\s*(#[0-9A-Za-z-]+:.*)", re.IGNORECASE
)
RE_MAP_SYM = re.compile(
    r"^\s*(0x[0-9a-fA-F]{8})\s+(0x[0-9a-fA-F]{8})\s+(Code|Data|Zero)\s+(RO|RW)\s+\d+\s+(\S+)\s+(\S+)"
)


def strip_html(text: str) -> str:
    text = re.sub(r"<[^>]+>", "", text)
    return html.unescape(text)


def parse_build_log(log_path: str) -> Dict[str, Any]:
    with open(log_path, "r", encoding="utf-8", errors="replace") as f:
        raw = f.read()

    clean = strip_html(raw) if log_path.lower().endswith((".htm", ".html")) else raw
    lines = [line.strip() for line in clean.splitlines() if line.strip()]

    result: Dict[str, Any] = {
        "log_path": os.path.abspath(log_path),
        "compiler": "Unknown",
        "errors": None,
        "warnings": None,
        "size": None,
        "diagnostics": [],
    }

    for line in lines:
        if "V5.06" in line or "ARMClang" in line or "ArmCC" in line:
            result["compiler"] = line
        m_size = RE_PROG_SIZE.search(line)
        if m_size:
            code, ro, rw, zi = map(int, m_size.groups())
            result["size"] = {
                "code": code,
                "ro_data": ro,
                "rw_data": rw,
                "zi_data": zi,
                "total_flash": code + ro + rw,
                "total_ram": rw + zi,
            }
        m_sum = RE_SUMMARY.search(line)
        if m_sum:
            result["errors"] = int(m_sum.group(1))
            result["warnings"] = int(m_sum.group(2))

        m_diag = RE_DIAG_LINE.match(line)
        if m_diag:
            result["diagnostics"].append(
                {
                    "file": m_diag.group(1),
                    "line": int(m_diag.group(2)),
                    "severity": m_diag.group(3).lower(),
                    "message": m_diag.group(4),
                }
            )

    return result


def parse_top_map_symbols(map_path: str, top_n: int = 10) -> Dict[str, List[Dict[str, Any]]]:
    ram_syms: List[Dict[str, Any]] = []
    flash_syms: List[Dict[str, Any]] = []

    if not map_path or not os.path.isfile(map_path):
        return {"top_ram": [], "top_flash": []}

    with open(map_path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = RE_MAP_SYM.match(line)
            if not m:
                continue
            addr, size_hex, kind, attr, section, obj = m.groups()
            size_bytes = int(size_hex, 16)
            if size_bytes == 0:
                continue
            entry = {
                "addr": addr,
                "size": size_bytes,
                "type": f"{kind}/{attr}",
                "section": section,
                "object": obj,
            }
            if attr == "RW":
                ram_syms.append(entry)
            else:
                flash_syms.append(entry)

    ram_syms.sort(key=lambda x: x["size"], reverse=True)
    flash_syms.sort(key=lambda x: x["size"], reverse=True)
    return {"top_ram": ram_syms[:top_n], "top_flash": flash_syms[:top_n]}


def fmt_delta(curr: int, base: Optional[int]) -> str:
    if base is None:
        return f"{curr:,} B"
    diff = curr - base
    sign = "+" if diff >= 0 else ""
    return f"{curr:,} B ({sign}{diff:,} B)"


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze Keil MDK build log and map file.")
    parser.add_argument("--log", required=True, help="Path to .build_log.htm or build output log")
    parser.add_argument("--map", default="", help="Optional path to .map file")
    parser.add_argument("--save-baseline", default="", help="Save parsed metrics to JSON file")
    parser.add_argument("--compare-baseline", default="", help="Compare against saved baseline JSON")
    parser.add_argument("--top", type=int, default=8, help="Number of top symbols from .map")
    args = parser.parse_args()

    if not os.path.isfile(args.log):
        print(f"ERROR: Build log not found: {args.log}", file=sys.stderr)
        return 2

    data = parse_build_log(args.log)

    if args.save_baseline:
        with open(args.save_baseline, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        print(f"Saved baseline to {args.save_baseline}")

    base_size = None
    if args.compare_baseline and os.path.isfile(args.compare_baseline):
        with open(args.compare_baseline, "r", encoding="utf-8") as f:
            base_data = json.load(f)
            base_size = base_data.get("size")

    print("=== Keil MDK Build Summary ===")
    print(f"Compiler : {data['compiler']}")
    print(f"Result   : {data['errors']} Error(s), {data['warnings']} Warning(s)")

    sz = data.get("size")
    if sz:
        print("\n=== Memory Footprint (Program Size) ===")
        print(f"  Code        : {fmt_delta(sz['code'], base_size['code'] if base_size else None)}")
        print(f"  RO-data     : {fmt_delta(sz['ro_data'], base_size['ro_data'] if base_size else None)}")
        print(f"  RW-data     : {fmt_delta(sz['rw_data'], base_size['rw_data'] if base_size else None)}")
        print(f"  ZI-data     : {fmt_delta(sz['zi_data'], base_size['zi_data'] if base_size else None)}")
        print(f"  Total Flash : {fmt_delta(sz['total_flash'], base_size['total_flash'] if base_size else None)}")
        print(f"  Total RAM   : {fmt_delta(sz['total_ram'], base_size['total_ram'] if base_size else None)}")

    if data["diagnostics"]:
        print(f"\n=== Diagnostics ({len(data['diagnostics'])}) ===")
        for d in data["diagnostics"]:
            print(f"  [{d['severity'].upper()}] {d['file']}:{d['line']} -> {d['message']}")

    if args.map:
        top_syms = parse_top_map_symbols(args.map, args.top)
        if top_syms["top_ram"]:
            print(f"\n=== Top {args.top} RAM (RW/ZI) Sections in .map ===")
            for s in top_syms["top_ram"]:
                print(f"  {s['size']:>8,} B | {s['type']:<7} | {s['section']} ({s['object']})")

    return 0 if (data["errors"] == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
