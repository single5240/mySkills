"""Check report facts and failure handling using isolated copies of the demos."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from report_summary import render, write_report


def require(value, message):
    if not value:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", required=True)
    args = parser.parse_args()
    cc = shutil.which(args.cc)
    require(cc, "Compiler not found")
    work = Path(tempfile.mkdtemp(prefix="host-report-checks-"))
    print("Evidence:", work)
    records = []
    for mode in ("success", "build-failure", "test-failure"):
        fixture = work / mode
        shutil.copytree(ROOT, fixture, ignore=shutil.ignore_patterns("__pycache__"))
        module = fixture / "examples/state-machine/session.c"
        if mode == "build-failure":
            module.write_text(module.read_text(encoding="utf-8") + "\n#error deliberate_build_failure\n", encoding="utf-8")
        elif mode == "test-failure":
            text = module.read_text(encoding="utf-8")
            require(text.count(">= 12000U") == 1, "Fixture anchor changed")
            module.write_text(text.replace(">= 12000U", "> 12000U"), encoding="utf-8")
        command = [sys.executable, str(fixture / "scripts/run_examples.py"), "--cc", cc, "--negative-checks"]
        result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", timeout=30,
                                env={**os.environ, "PYTHONUTF8": "1"})
        (work / (mode + ".log")).write_text(result.stdout + result.stderr, encoding="utf-8")
        evidence = Path(next(line[len("Evidence: "):] for line in result.stdout.splitlines() if line.startswith("Evidence: ")))
        data = json.loads((evidence / "summary.json").read_text(encoding="utf-8"))
        statuses = [entry["status"] for entry in data["cases"]]
        expected = {"success": ["PASS", "PASS"], "build-failure": ["BLOCKED", "NOT RUN"],
                    "test-failure": ["FAIL", "NOT RUN"]}[mode]
        require(statuses == expected, mode + ": incorrect statuses " + str(statuses))
        require((result.returncode == 0) == (mode == "success"), mode + ": incorrect exit status")
        require(data["status"] == ("passed" if mode == "success" else "failed"), mode + ": incorrect summary")
        require(all("returncode" in step for entry in data["cases"] for step in entry.get("steps", [])), "Missing stage exit code")
        report = (evidence / "report.md").read_text(encoding="utf-8")
        require("待人工确认" in report and "原目标工具链" in report and "未执行" in report, "Report overstates acceptance")
        if mode == "success":
            require(all(entry["mutation"]["status"] == "PASS" for entry in data["cases"]), "Expected negative checks not recognized")
            output = work / "converted.md"
            write_report(evidence / "summary.json", output)
            before = output.read_bytes()
            try:
                write_report(evidence / "summary.json", output)
            except FileExistsError:
                pass
            else:
                raise RuntimeError("Existing report was overwritten")
            require(output.read_bytes() == before, "Existing report changed")
        records.append({"mode": mode, "command": command, "returncode": result.returncode,
                        "statuses": statuses, "evidence": str(evidence)})
        print("PASS:", mode)
    legacy = render({"cases": [{"name": "legacy", "result": "PASS old output"}]}, work, work)
    require("PASS=0" in legacy and "NOT RUN=1" in legacy, "Missing status inferred from PASS text")
    for bad in ({"cases": [{"status": "invalid"}]},
                {"status": "passed", "cases": [{"status": "FAIL"}]},
                {"status": "passed", "cases": []}):
        try:
            render(bad, work, work)
        except ValueError:
            pass
        else:
            raise RuntimeError("Invalid or inconsistent status silently accepted")
    (work / "checks.json").write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding="utf-8")
    print("PASS: incomplete records, invalid status, no overwrite, human acceptance boundary")


if __name__ == "__main__":
    main()
