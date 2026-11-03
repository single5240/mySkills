"""Render this skill's execution summary; never infer product acceptance."""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
from urllib.parse import quote

STATES = {"PASS", "FAIL", "BLOCKED", "NOT RUN", "N/A"}


def cell(value):
    return str(value).replace("|", "\\|").replace("\r", "").replace("\n", " / ")


def render(data, evidence, report_dir):
    cases = data.get("cases", [])
    def status(entry):
        value = entry.get("status", "NOT RUN")
        if value not in STATES:
            raise ValueError("Unknown test status: " + str(value))
        return value
    counts = Counter(status(entry) for entry in cases)
    if data.get("status") == "passed" and (
        not cases or any(status(entry) not in ("PASS", "N/A") or
                         status(entry.get("mutation", {})) in ("FAIL", "BLOCKED") for entry in cases)
    ):
        raise ValueError("Passed summary conflicts with test-group records")
    lines = ["# 主机回归自动执行摘要", "",
             "这是教学示例的执行事实，不是产品验收报告。需求覆盖、目标构建、硬件与发布结论待人工确认。", "",
             "## 身份与环境", "",
             "- 开始时间（UTC）：" + cell(data.get("started_at", "未知")),
             "- 结束时间（UTC）：" + cell(data.get("finished_at", "未知")),
             "- 执行状态：" + cell(data.get("status", "未知")),
             "- 主机：" + cell(data.get("platform", "未知")),
             "- 编译器：" + cell(data.get("compiler", "未知")),
             "- 编译器版本：" + cell(data.get("compiler_version", "未知")),
             "- 仓库身份：" + cell(data.get("repository", "未知")),
             "- 运行器命令：" + cell(json.dumps(data.get("invocation", []), ensure_ascii=False)),
             "- 执行错误：" + cell(data.get("error", "未记录；不代表无缺陷")), "",
             "脚本 SHA256：", "```json",
             json.dumps(data.get("scripts_sha256", {}), ensure_ascii=False, indent=2), "```", "",
             "## 测试组结果", "",
             "统计单位为测试组，不是独立用例、断言或设备数。", "",
             "| 测试组 | 状态 | 可选负向检查 |", "| --- | --- | --- |"]
    for entry in cases:
        lines.append("| " + cell(entry.get("name", "未命名")) + " | " + status(entry) +
                     " | " + status(entry.get("mutation", {})) + " |")
    lines += ["", "; ".join(f"{state}={counts[state]}" for state in
                             ("PASS", "FAIL", "BLOCKED", "NOT RUN", "N/A")), "",
              "## 命令、返回码与证据", ""]
    for entry in [data, *cases]:
        lines += ["### " + cell(entry.get("name", "编译器识别")), ""]
        for step in entry.get("steps", []):
            log = (evidence / entry.get("directory", "") / step["log"]).resolve()
            try:
                link = quote(os.path.relpath(log, report_dir).replace(os.sep, "/"), safe="/.")
            except ValueError:  # Different Windows drives.
                link = log.as_uri()
            lines += ["- 阶段：" + cell(step.get("log")) + "; 返回码：" +
                      cell(step.get("returncode", "未知")) + "; 超时：" +
                      cell(step.get("timeout", False)) + f"; [日志]({link})",
                      "```text", json.dumps(step.get("command", []), ensure_ascii=False), "```", ""]
        if entry.get("sources_sha256"):
            lines += ["源码 SHA256：", "```json",
                      json.dumps(entry["sources_sha256"], ensure_ascii=False, indent=2), "```", ""]
    lines += ["## 待人工补充", "",
              "- 需求／协议版本、用例预期与实际观察、覆盖范围：待确认。",
              "- 原目标工具链、资源与链接边界：未执行。",
              "- IDE 最终输出处理、实板与真实故障注入：未执行。",
              "- 遗留问题、模型限制、下一阶段及发布建议：待确认。",
              "- 长期归档：临时目录不是长期证据，请同时归档 JSON、日志和必要输入。", ""]
    return "\n".join(lines)


def write_report(summary, output):
    summary, output = Path(summary).resolve(), Path(output).resolve()
    data = json.loads(summary.read_text(encoding="utf-8"))
    content = render(data, summary.parent, output.parent)
    with output.open("x", encoding="utf-8") as stream:
        stream.write(content)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("summary", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    write_report(args.summary, args.output)
