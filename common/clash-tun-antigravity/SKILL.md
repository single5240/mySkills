---
name: clash-tun-antigravity
description: >-
  [Scope: project] Keep Antigravity, Cursor, Codex, and GitHub Copilot on the
  Clash Verge AI-Code group, and fix Antigravity not responding with TUN plus
  rule mode. Use when the user mentions 反重力, Antigravity, AI-Code, Cursor,
  Codex, Copilot, Clash TUN, 系统代理, 国内网站打不开, or GitHub failing after TUN.
---

# Clash TUN and AI-Code

Two separate changes. Do not mix them up.

- AI-Code is a Clash Verge enhancement prepended in front of the subscription. It sends Antigravity, Cursor, Codex, and Copilot to their own node. Updating the subscription must not delete these files.
- TUN plus rule mode is what makes Antigravity's direct Google dials hit those rules. System proxy alone does not. Global mode plus TUN breaks domestic sites and GitHub.

Do not print proxy passwords, UUIDs, or subscription URLs from `clash-verge.yaml` or `profiles.yaml`.

## This machine

| Item | Value |
|------|-------|
| Clash Verge | `C:\Program Files\Clash Verge\clash-verge.exe` |
| Config dir | `%APPDATA%\io.github.clash-verge-rev.clash-verge-rev` |
| Core pipe | `\\.\pipe\verge-mihomo` (HTTP `127.0.0.1:9097` is often not listening) |
| Mixed port | `127.0.0.1:7897` |
| Antigravity log | `%APPDATA%\Antigravity\logs\language_server.log` |
| Active profile | `iKuuu_V2.yaml` (`profiles.yaml` → `current`) |
| Spare profile | `🐮牛逼` |

On another PC, keep this workflow and re-read that machine's `profiles.yaml`. Node names below belong to these two subscriptions only.

## AI-Code enhancement

Edit the enhancement files, not the downloaded subscription. `prepend` survives "更新订阅".

| Profile | Groups file | Rules file |
|---------|-------------|------------|
| iKuuu_V2（当前） | `profiles/gw3vYhuWDVBw.yaml` | `profiles/rJ9cutWb3gxH.yaml` |
| 🐮牛逼（备用） | `profiles/gHKYWV0vBigD.yaml` | `profiles/rBno0t4JGdBl.yaml` |

`profiles.yaml` wires them with `option.groups` and `option.rules`. There is no `selected` entry named `🤖 AI-Code` yet. The live default is the first proxy in the group file. Add a `selected` item only after the user picks a node in the UI.

iKuuu group order: `🇺🇲 美国Y01 | IEPL | x1.5` (default), `🇺🇲 美国Y02`, `🇯🇵 日本Y01`, `🇯🇵 日本Y02`, `🇸🇬 新加坡Y01`, `🇨🇳 台湾Y01`, `🔰 选择节点`, `DIRECT`.

🐮牛逼 group order: `🇺🇸亚特兰大01|流媒体解锁|AI|移动优化`, `🇺🇸凤凰城03|流媒体解锁|AI|移动优化`, `🇸🇬新加坡01|流媒体解锁|AI|移动优化`, `🇯🇵日本01|流媒体解锁|AI|移动优化`, `🐮牛逼`, `自动选择`, `DIRECT`.

Both rules files prepend the same matchers, highest priority, all to `🤖 AI-Code`:

| Service | Matchers |
|---------|----------|
| Antigravity & Gemini | `DOMAIN-SUFFIX`: `antigravity.google`, `generativelanguage.googleapis.com`, `aiplatform.googleapis.com`, `alkalimakersuite-pa.clients6.google.com`, `gemini.google.com`, `proactivebackend-pa.googleapis.com`. `DOMAIN-KEYWORD,antigravity`. `PROCESS-NAME`: `Antigravity.exe`, `agy.exe` |
| Cursor | `DOMAIN-SUFFIX`: `cursor.sh`, `cursor.com`, `cursorapi.com`, `anysphere.co`. `DOMAIN-KEYWORD,cursor`. `PROCESS-NAME,Cursor.exe` |
| OpenAI / Codex | `DOMAIN-SUFFIX`: `openai.com`, `chatgpt.com`, `oaistatic.com`, `oaiusercontent.com`, `auth0.openai.com` |
| GitHub Copilot | `DOMAIN-SUFFIX`: `githubcopilot.com`, `api.githubcopilot.com`, `copilot-proxy.githubusercontent.com`, `copilot-telemetry.githubusercontent.com`, `default.exp-tas.com` |

`PROCESS-NAME` matches only after the packet enters Mihomo. That requires TUN. Leave subscription rules alone: `GEOIP,CN` → `🇨🇳 国内网站` (first choice `DIRECT`), `github.com` → `🔰 选择节点`, final `MATCH` → `🐟 漏网之鱼`.

After editing an enhancement, reload the core. `curl --unix-socket` does not work for this pipe. Use `NamedPipeClientStream` to `verge-mihomo` and `PUT /configs?force=true` with `{"path":"<config dir>\\clash-verge.yaml"}` only if Verge has already regenerated `clash-verge.yaml`. Otherwise toggle the profile from Clash Verge so it merges enhancements, then confirm `GET /configs` and `GET /proxies` show group `🤖 AI-Code`. Do not hand-edit the generated `clash-verge.yaml`; the next subscription update overwrites it.

## TUN plus rule mode

Antigravity's language server dials Google directly and ignores the Windows system proxy. Timeouts in `language_server.log` to `googleapis.com`, `play.googleapis.com`, or `daily-cloudcode-pa.googleapis.com` mean those dials never entered Clash.

Already applied on this PC:

- `verge.yaml`: `enable_tun_mode: true`. `enable_system_proxy` left `true`.
- `config.yaml`: `tun.enable: true` (`gvisor`, `auto-route: true`, `dns-hijack: any:53`).
- Live core: `PATCH /configs?force=true` enabled TUN. A `Meta` / `Meta Tunnel` adapter comes up.
- Runtime mode stayed `rule`. On-disk `clash-verge.yaml` can still say `mode: global`; trust `GET /configs`.

If TUN is off, PATCH:

```powershell
$pipe = New-Object System.IO.Pipes.NamedPipeClientStream('.', 'verge-mihomo', [System.IO.Pipes.PipeDirection]::InOut)
$pipe.Connect(5000)
$sw = New-Object System.IO.StreamWriter($pipe)
$body = '{"tun":{"enable":true,"stack":"gvisor","auto-route":true,"auto-detect-interface":true,"dns-hijack":["any:53"]}}'
$sw.Write("PATCH /configs?force=true HTTP/1.1`r`nHost: 127.0.0.1`r`nContent-Type: application/json`r`nContent-Length: $($body.Length)`r`nConnection: close`r`n`r`n$body")
$sw.Flush()
Start-Sleep -Milliseconds 800
(New-Object System.IO.StreamReader($pipe)).ReadToEnd()
$pipe.Dispose()
```

Expect `204`, then `"mode":"rule"` and `"tun":{"enable":true`. If mode is `global`, PATCH `{"mode":"rule"}`. Persist `enable_tun_mode` and `tun.enable` as above. Do not turn system proxy off unless asked.

## Verify

Bypass the system proxy. Plain `Invoke-WebRequest` follows `ProxyEnable` and hides a broken TUN path.

```powershell
curl.exe -4 -sS -o NUL -w "%{http_code} time=%{time_total}`n" --noproxy "*" --max-time 15 https://www.baidu.com
curl.exe -4 -sS -o NUL -w "%{http_code} time=%{time_total}`n" --noproxy "*" --max-time 15 https://github.com
curl.exe -4 -sS -o NUL -w "%{http_code} time=%{time_total}`n" --noproxy "*" --max-time 15 https://www.google.com
```

Pass: all three return an HTTP status. Baidu stays fast via `DIRECT`. Then the tail of `language_server.log` should show `daily-cloudcode-pa.googleapis.com` without a new `i/o timeout`.

If domestic sites or GitHub die after TUN: mode is `global`, or `🇨🇳 国内网站` is not `DIRECT`. Only GitHub failing means the selected node is the problem. Keep the tray on 规则.
