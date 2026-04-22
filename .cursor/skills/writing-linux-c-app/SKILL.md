---
name: writing-linux-c-app
description: Use when planning or implementing Linux userspace C applications that should follow a CMake-first workflow with structured build/deploy conventions (for example CLI tools, daemons/services, cross-compilation-ready projects, and standardized logging/config layout). Trigger even if the user only says "Linux C program", "Linux C app", "写个 Linux C 程序", "Linux 上的 C 工具/服务", "交叉编译 C 项目", or asks for roadmap/design/task breakdown without naming CMake or this skill. Also trigger on broad wording like "write a program/software" or "写一个程序/软件" only when nearby context indicates Linux userspace C plus CMake-first intent (for example mentions Linux, C语言, CMake, 编译/交叉编译, CLI 工具, 守护进程/服务). Do not use for kernel/driver work or workflows that explicitly reject CMake-first conventions.
---

# Linux C 应用开发规范

## 概述与边界

在 Linux 上以 **C 为主**的用户态程序（命令行工具、常驻服务、交叉编译、结构化构建与安装等），只要最终**适合**采用 **CMake**、根目录 **`build.sh`**、安装到 **`deploy/`** 的范式，即应优先考虑本 Skill；用户未逐字提到 CMake 或本 Skill 名称，**不**构成不适用理由。**复杂**项目可附带少量 Web/Lua/JS/HTML/CSS（单独顶层目录），细节见 `references/complex-app.md`。

- **生效条件**：须先取得用户**明确同意**采用本 Skill；未同意不得按本规范改工程或假定工具链。
- **不适用**：内核/驱动；拒绝 CMake 的工程；多仓与发布编排远超模板能力时，类型选「其他」并单独约定。
- **实施总原则**：模板内很多文件可直接复用。默认流程是**先复制模板工程，再做最小必要改动**（先保证可编译可运行，再按项目增量调整），避免从零搭脚手架。

## 与 references 的分工

| 层级 | 职责 |
|------|------|
| **本文件（SKILL.md）** | 同意与边界；**强制**请用户在「小型 / 复杂 / 其他」三选一（或接受用户已等价表态，见下文）；提供面向用户的选项说明与对比；选定后**仅指向**对应 reference，**不写**具体 CMake 片段、`.gitignore` 规则、`ARCH`、工具链文件名等实施细节。 |
| **`references/small-app.md`** | 用户已选**小型**后的完整落地流程：以 `references/small-app/` 为模板，按“可直接用 / 必须改 / 按需改”执行；覆盖目录、`CMakeLists.txt`、`build.sh`、`log.h`、命令行、`.gitignore`、架构工具链、版本注入等。 |
| **`references/complex-app.md`** | 用户已选**复杂**后的完整落地流程：以 `references/complex-app/` 为模板，按“可直接用 / 必须改 / 按需改”执行；覆盖多模块、JSON+zlog、configs 与 `deploy/`、可选 `web/` 等。 |
| **其他** | 无默认模板；与用户逐项约定后实施，不套用上述两套目录与依赖默认值。 |

## 强制流程：先判定项目类型

在写计划、改代码或生成文件结构之前，须完成**类型判定**：用户在三类中**明确选一**，或**已用等价表述说清楚**（见下节）。不得默认替用户拍板。

### 用户已等价表态时（减轻重复提问）

若用户在同一段对话里**已经**清楚对应某一类（例如：明确说是命令行小工具、日志打屏即可、不要磁盘主配置 → 等价**小型**；明确要求 JSON 主配置 + 文件日志、多模块服务 → 等价**复杂**；明确不要 CMake/不要 JSON+zlog 默认栈 → 等价**其他**），则**不必**再展开整张对比表；用**一句话**复述你的理解并问「是否按这个类型走」即可，对方确认后继续。

若表述**模糊**或**可能跨类**（例如既要配置文件又不要 JSON），仍应按下列要求展示三选项。

### 向用户展示时的要求（强制）

1. **必须列出三个选项**：小型、复杂、其他（名称与下表一致）。
2. **必须说明区别**：使用下表「用户可见对比（精简）」即可；配一段简短总结，让用户能据此自主选择。
3. **必须得到明确答复**：用户确认选项（或确认你对「等价表态」的复述）后，再打开**对应** `references/*.md` 与模板目录；若用户犹豫，可根据「快速倾向」给建议，最终以用户选择为准。

### 三选项速览（给用户看的摘要）

| 选项 | 一句话 |
|------|--------|
| **小型** | 默认不设配置文件，主要靠命令行；允许为数据收发读写文件；简单日志打 stdout；结构最轻，适合工具类、单程序。 |
| **复杂** | 要 JSON 主配置 + zlog 文件日志；可多模块；可带少量 Web/Lua/JS 等单独目录；依赖与目录更重，适合长期运行的服务类程序。 |
| **其他** | 不用本 Skill 的默认栈（例如不用 JSON/zlog、或要别的构建方式）；由双方逐项约定后再做。 |

### 用户可见对比（精简）

| 维度 | 小型 | 复杂 | 其他 |
|------|------|------|------|
| **配置与参数** | 默认无主配置文件，主要靠命令行；允许按业务读写输入/输出文件。 | JSON 主配置 + 命令行（常见 `-f`/`--config`）。 | 按约定，不默认 JSON。 |
| **日志与规模** | stdout 简单日志；单程序/轻量工具。 | zlog 文件日志；多模块/服务化项目。 | 按约定。 |
| **默认模板** | `references/small-app.md` + `references/small-app/` | `references/complex-app.md` + `references/complex-app/` | 无默认模板。 |
| **何时选它** | 主要靠命令行控制行为。 | 主要靠配置文件控制行为。 | 非 JSON / 非 zlog / 非 CMake，或有特殊约束。 |

### 快速倾向（仅供建议；已等价表态时可直接用于复述确认）

- 修改程序行为主要靠**命令行**（可附带输入/输出文件）→ 小型；主要靠**配置文件**且用 **JSON** → 复杂；要配置文件但**不用 JSON** → 其他。
- 日志打屏即可还是落盘/滚动？前者 → 小型；后者 → 复杂或其他。
- 要多 C 子模块 + 可选 Web 目录？是 → 倾向复杂。
- 已指定 **非 JSON / 非 zlog / 非 CMake** → **其他**。

### 选定后的动作

| 用户选择 | 下一步 |
|----------|--------|
| **小型** | 打开并遵循 `references/small-app.md`：优先复用`references/small-app/`模板中的文件，再做最小必要修改。 |
| **复杂** | 打开并遵循 `references/complex-app.md`：优先复用`references/complex-app/`模板中的文件，再做最小必要修改。 |
| **其他** | 不得套用两套模板默认值；与用户确认日志、配置、目录、构建后再做。 |

## 实施前提问清单（仅入口级）

1. 是否采用本 Skill（不同意则不按本规范执行）。
2. 项目类型：若用户**尚未**等价表态，则展示**三选项及精简对比**；若**已**等价表态，则**一句话确认**即可；得到确认后再继续。
3. 选定类型后，**所有**后续细节（含是否新增 `.gitignore`、`ARCH` 与交叉编译、工具链文件、`build.sh` 行为、版本字符串与命令行选项等）均在所选类型的 **`references/*.md`** 中执行；**勿在本 SKILL.md 中展开或重复**。
