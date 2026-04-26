---
name: writing-linux-c-small-app
description: Use when planning, creating, or reviewing Linux userspace C small apps (e.g., CLI tools, 单程序小工具, 命令行工具) that should follow CMake-first workflow, command-line driven behavior, no main config file, and stdout-style logging (non-JSON, non-zlog), including checks for whether an existing project or implementation plan meets these baseline conventions.
---

# 小型 Linux C 应用模板规范

用于 Linux 用户态 C 小型项目（工具类、单程序、轻量服务原型）。该 skill 直接基于本目录模板落地，不再经过额外说明文档。

## 适用边界

- 采用 CMake-first 工作流，根目录使用 `build.sh` 构建，产物安装到 `deploy/`。
- 运行时行为主要通过命令行参数控制，不要求主配置文件。
- 日志为 stdout 轻量方案（`inc/log.h`），不使用 zlog。
- 不适用于内核/驱动项目，或明确拒绝 CMake 的项目。

## 模板路径

- 直接复用：`small-app/`

## 使用流程

0. 若用户在实现前要先写 plan 文档，先执行“计划文档模式”并完成合规检查。
1. 与用户确认采用该小型模板。
2. 复制 `small-app/` 到目标项目根目录。
3. 最小必要修改后完成首轮可编译可运行验证。

## 计划文档模式（实现前）

当用户请求“先写 plan/实施方案”时，先输出计划，再进入代码阶段。计划至少覆盖：

1. 项目边界：明确这是小型规范（命令行主导、无主配置、stdout 日志）。
2. 文件改动清单：哪些文件直接复用模板，哪些文件必须改。
3. 构建与验证步骤：`./build.sh`、`./build.sh debug`、关键参数验证。
4. 合规检查清单：实现完成后如何逐条回验小型规范。

## 现有项目检查模式

当用户不是“新建项目”，而是“检查当前项目是否符合小型规范”时，按以下顺序检查：

1. 构建入口是否为 CMake + `build.sh`，并使用 `build/` 与 `deploy/`。
2. 运行参数是否以命令行为主（至少 `-h/--help`、`-v/--version`）。
3. 是否无主配置文件（允许业务输入/输出文件，但不应有 JSON 主配置）。
4. 日志是否为 stdout 轻量方案（不依赖 zlog）。
5. 发现偏离项后，按“最小改动”给出修正建议。

## 迁移后必须修改

- `small-app/CMakeLists.txt`（项目名、目标名、源码路径）
- `small-app/src/main.c`（程序名、参数、业务逻辑）
- `small-app/README.md`（构建与运行说明）

## 通常可直接复用

- `small-app/build.sh`
- `small-app/deploy.sh`
- `small-app/git-version.sh`
- `small-app/Config.h.in`
- `small-app/cmake/toolchain-*.cmake`
- `small-app/.gitignore`

## 构建约定

- `./build.sh`：Release 构建
- `./build.sh debug`：Debug 构建
- `./build.sh clean`：清理 `build/` 与 `deploy/`
- 交叉编译通过 `ARCH` + `cmake/toolchain-*.cmake` 选择

## 验收底线

- 至少一次完整构建成功。
- 参数入口可用（至少 `-h/--help` 与 `-v/--version`）。
- 仍满足小型边界：无主配置文件、命令行主导、stdout 日志。
