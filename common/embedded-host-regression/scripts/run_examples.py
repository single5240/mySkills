"""Compile separate demo modules and tests in a fresh temporary directory."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CASES = (
    ("state-machine", "session.c", "test_session.c", ">= 12000U", "> 12000U"),
    ("paired-protocol", "link.c", "test_link.c", "sum != frame[4]", "sum == frame[4]"),
)


def execute(command, work, log):
    result = subprocess.run(command, cwd=work, capture_output=True, timeout=30)
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
    manifest = {"compiler": str(Path(compiler).resolve()), "cases": []}
    try:
        rc, version = execute([compiler, "--version"], work, "compiler.log")
        if rc:  # TinyCC reports its version with -v.
            rc, version = execute([compiler, "-v"], work, "compiler.log")
        if rc:
            raise RuntimeError("Cannot identify compiler: " + version)
        manifest["compiler_version"] = version.strip()
        for name, module, test, original, mutation in CASES:
            case = work / name
            case.mkdir()
            hashes = {}
            for source in sorted((ROOT / "examples" / name).iterdir()):
                if source.suffix in (".c", ".h"):
                    shutil.copy2(source, case / source.name)
                    hashes[source.name] = hashlib.sha256(source.read_bytes()).hexdigest()
            exe = case / ("test.exe" if __import__("os").name == "nt" else "test")
            command = [compiler, "-I.", module, test, "-o", str(exe)]
            entry = {"name": name, "sources_sha256": hashes, "command": command}
            manifest["cases"].append(entry)
            rc, output = execute(command, case, "build.log")
            if rc:
                raise RuntimeError("Build failed: " + name + "\n" + output)
            rc, output = execute([str(exe)], case, "test.log")
            if rc:
                raise RuntimeError("Test failed: " + name + "\n" + output)
            print(output, end="")
            entry["result"] = output.strip()
            if args.negative_checks:
                target = case / module
                text = target.read_text(encoding="utf-8")
                if text.count(original) != 1:
                    raise RuntimeError("Mutation anchor changed: " + name)
                try:
                    target.write_text(text.replace(original, mutation), encoding="utf-8")
                    rc, output = execute(command, case, "negative-build.log")
                    if rc:
                        raise RuntimeError("Negative check must compile: " + output)
                    rc, output = execute([str(exe)], case, "negative-test.log")
                    if rc != 1 or "FAIL line" not in output:
                        raise RuntimeError("Mutation not detected as assertion failure: " + name)
                    entry["mutation"] = {"from": original, "to": mutation, "detected": True}
                    print("PASS: negative check detected:", name)
                finally:
                    target.write_text(text, encoding="utf-8")
        manifest["status"] = "passed"
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        manifest["status"] = "failed"
        manifest["error"] = str(error)
        raise
    finally:
        (work / "summary.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
