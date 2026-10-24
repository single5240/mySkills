"""Compile separate demo modules and tests in a fresh temporary directory."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parent))
from report_summary import write_report

ROOT = Path(__file__).resolve().parents[1]
CASES = (
    ("state-machine", "session.c", "test_session.c", ">= 12000U", "> 12000U"),
    ("paired-protocol", "link.c", "test_link.c", "sum != frame[4]", "sum == frame[4]"),
)


def execute(command, work, log, record):
    step = {"command": command, "log": log}
    record.setdefault("steps", []).append(step)
    try:
        result = subprocess.run(command, cwd=work, capture_output=True, timeout=30)
    except subprocess.TimeoutExpired as error:
        step.update(returncode=None, timeout=True)
        (work / log).write_bytes((error.stdout or b"") + (error.stderr or b""))
        raise
    except OSError as error:
        step.update(returncode=None, error=str(error))
        (work / log).write_text(str(error), encoding="utf-8")
        raise
    step["returncode"] = result.returncode
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    (work / log).write_text(output, encoding="utf-8")
    return result.returncode, output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", required=True, help="GCC, Clang or TinyCC executable")
    parser.add_argument("--negative-checks", action="store_true")
    args = parser.parse_args()
    compiler = shutil.which(args.cc)
    if not compiler:
        parser.error("Host compiler not found: " + args.cc)
    work = Path(tempfile.mkdtemp(prefix="embedded-host-regression-"))
    print("Evidence:", work, flush=True)
    manifest = {"compiler": str(Path(compiler).resolve()),
                "invocation": [sys.executable, *sys.argv],
                "started_at": datetime.now(timezone.utc).isoformat(),
                "platform": platform.platform(),
                "cases": [{"name": case[0], "directory": case[0], "status": "NOT RUN"} for case in CASES],
                "scripts_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                   for p in (Path(__file__), Path(__file__).with_name("report_summary.py"))}}
    try:
        try:
            commit = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], stderr=subprocess.DEVNULL, timeout=5).decode().strip()
            dirty = subprocess.check_output(["git", "-C", str(ROOT), "status", "--porcelain"], stderr=subprocess.DEVNULL, timeout=5).decode("utf-8", errors="replace")
            manifest["repository"] = {"commit": commit, "dirty": bool(dirty), "path": str(ROOT)}
        except (OSError, subprocess.SubprocessError):
            manifest["repository"] = "未知；以源码哈希为准"
        rc, version = execute([compiler, "--version"], work, "compiler.log", manifest)
        if rc:  # TinyCC reports its version with -v.
            rc, version = execute([compiler, "-v"], work, "compiler-fallback.log", manifest)
        if rc:
            raise RuntimeError("Cannot identify compiler: " + version)
        manifest["compiler_version"] = version.strip()
        for entry, (name, module, test, original, mutation) in zip(manifest["cases"], CASES):
            entry["status"] = "BLOCKED"
            case = work / name
            case.mkdir()
            hashes = {}
            for source in sorted((ROOT / "examples" / name).iterdir()):
                if source.suffix in (".c", ".h"):
                    shutil.copy2(source, case / source.name)
                    hashes[source.name] = hashlib.sha256(source.read_bytes()).hexdigest()
            exe = case / ("test.exe" if os.name == "nt" else "test")
            command = [compiler, "-I.", module, test, "-o", str(exe)]
            entry.update(sources_sha256=hashes, command=command)
            rc, output = execute(command, case, "build.log", entry)
            if rc:
                raise RuntimeError("Build failed: " + name + "\n" + output)
            rc, output = execute([str(exe)], case, "test.log", entry)
            entry["status"] = "FAIL" if rc else "PASS"
            if rc:
                raise RuntimeError("Test failed: " + name + "\n" + output)
            print(output, end="")
            entry["result"] = output.strip()
            if args.negative_checks:
                entry["mutation"] = {"from": original, "to": mutation, "status": "BLOCKED"}
                target = case / module
                text = target.read_text(encoding="utf-8")
                if text.count(original) != 1:
                    raise RuntimeError("Mutation anchor changed: " + name)
                try:
                    target.write_text(text.replace(original, mutation), encoding="utf-8")
                    rc, output = execute(command, case, "negative-build.log", entry)
                    if rc:
                        raise RuntimeError("Negative check must compile: " + output)
                    rc, output = execute([str(exe)], case, "negative-test.log", entry)
                    entry["mutation"]["status"] = "PASS" if rc == 1 and "FAIL line" in output else "FAIL"
                    if rc != 1 or "FAIL line" not in output:
                        raise RuntimeError("Mutation not detected as assertion failure: " + name)
                    entry["mutation"]["detected"] = True
                    print("PASS: negative check detected:", name)
                finally:
                    target.write_text(text, encoding="utf-8")
        manifest["status"] = "passed"
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        manifest["status"] = "failed"
        manifest["error"] = str(error)
        raise
    finally:
        manifest["finished_at"] = datetime.now(timezone.utc).isoformat()
        summary = work / "summary.json"
        summary.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
        write_report(summary, work / "report.md")


if __name__ == "__main__":
    main()
