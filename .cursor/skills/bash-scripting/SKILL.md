---
name: bash-scripting
description: MUST use when writing, editing, reviewing, or refactoring Bash/shell scripts—`.sh` files, `#!/bin/bash` / `#!/usr/bin/env bash`, CI/CD shell steps, Docker ENTRYPOINT/CMD scripts, `build.sh`, install/deploy shells, one-off ops scripts, or turning ad-hoc commands into scripts. Use even if the user only says things like "write a script", "add shell", "turn these lines into a script", "review this sh", or "strict mode" without naming this skill. If the task clearly belongs in Python/Go/etc. (see the guide's when-not-Bash section), say so first and follow the user's choice. Excludes POSIX-only `#!/bin/sh` work when the user forbids Bash features (follow their constraints; this repo guide targets Bash); pure conceptual chat with no script file may skip reading the full reference.
---

# Bash 脚本规范（项目内）

## 与本 Skill 的关系

本 Skill 将**项目作者**在 `references/bash-script-style-guide.md` 中定义的 Bash 写法固化为可执行指令。编写或修改任何 Bash 脚本时，以该文件为**唯一权威**；本文件只提供**何时读、如何落地、如何自检**，不重复规范全文。

**关于篇幅**：规范放在 `references/` 是刻意为之——通常**按需分段阅读**（见下表与 §13 清单），不必每次整份载入。若某次任务只涉及变量与引用，读第 3 节即可；只有大改或心里没底时才通读。若将来觉得单文件仍偏长，可再拆成 `references/` 下多文件并在本 SKILL 里列「何时读哪份」，属可选优化。

## 何时必读全文规范

在下列任一情况，**先打开并遵循** `references/bash-script-style-guide.md`（可按目录跳读对应章节）：

- 新建 `.sh` 或向仓库添加可执行脚本
- 大范围改动：函数拆分、错误处理、参数与返回值约定、循环/管道结构
- 用户明确要求「符合规范」「按项目 bash 规范」「strict mode」等

若仅是极小的单行修补（例如改一个常量或一行日志），仍须遵守规范中的硬性约定（shebang、`set -euo pipefail`、`IFS`、引用规则等），可只对照本文件「硬性摘要」与规范第 13 节清单，但心里要有数：不确定时回到全文。

## 规范文档结构（便于按需跳转）

| 章节 | 内容 |
|------|------|
| 概述与使用场景限制 | 何时适合/不适合 Bash（如行数与复杂度） |
| 1–2 | 文件名、权限、文件结构与头部三件套 |
| 3 | 变量命名、引用、`local`/`readonly` |
| 4 | 错误处理、`err`/`warn`、`set +e` 等 |
| 5 | 函数注释模板与参数校验 |
| 6–7 | `[[ ]]与 (( ))`、循环与避免管道子 shell 陷阱 |
| 8–9 | 数组、字符串与默认值 |
| 10 | 缩进 2 空格、行长 80 |
| 11–12 | 命令替换 `$()`、算术、`mktemp`/`trap`、安全禁忌 |
| 13 | **提交前快速检查清单** |
| 14 | 外部参考链接 |

## 硬性摘要（修改代码时默认遵守）

以下与规范正文一致；若摘要与全文冲突，**以 `references/bash-script-style-guide.md` 为准**。

1. **文件**：小写+下划线；扩展名 `.sh`；Shebang 使用 `#!/bin/bash`（与规范一致）。
2. **头部三行**：`set -euo pipefail`、`IFS=$'\n\t'`，顺序与规范示例一致。
3. **引用**：非平凡变量使用 `"${var}"`；数组用 `"${array[@]}"` 等形式。
4. **条件**：字符串/文件测试优先 `[[ ]]`；数值比较用 `(( ))` 或 `[[ n -eq m ]]`，避免在 `[[ ]]` 里用 `>`/`<` 做数值比较。
5. **函数**：每个函数有注释（格式见规范 5.1）；函数内局部变量用 `local`；多函数脚本须有 `main` 且入口为 `main "$@"`。
6. **格式**：2 空格缩进，行长不超过 80。
7. **安全与资源**：慎用 `eval`；通配符场景用显式路径；临时文件用 `mktemp`，配合 `trap` 清理。

## 推荐工作流

1. **判断**：若脚本会长期膨胀或涉及复杂数据结构，提醒规范中的「不适合 Bash」情形，由用户决定是否换语言。
2. **编写/改写**：按规范第 2 节结构组织文件；新增函数补全注释与参数检查。
3. **自检**：过一遍规范 **§13 快速检查清单**。
4. **交付**：用简短中文说明相对规范做了哪些对齐（例如：补全头部、统一引用、补函数注释），便于审查。

## 与「仅 POSIX sh」的冲突

本规范以 **Bash** 为目标。若用户明确要求 `#!/bin/sh` 且禁止 Bash 扩展，不得在未经用户同意的情况下强行套用本文中 Bash 专属写法；此时应说明差异并让用户选择遵循本规范（Bash）或单独约定 `sh` 子集。
