---
name: writing-linux-c-complex-app
description: Use when planning, creating, or reviewing Linux userspace C complex apps (e.g., daemons/services, 多模块服务程序, 配置驱动系统, 交叉编译 C 项目) that should follow CMake-first workflow, JSON main config, zlog-based logging, modular source layout, and optional web/assets directories, including checks for whether an existing project or implementation plan meets these baseline conventions.
---

# 复杂 Linux C 应用模板规范

用于 Linux 用户态 C 复杂项目（长期运行服务、多模块工程、配置驱动应用）。该 skill 直接基于本目录模板落地，不再经过额外说明文档。

## 适用边界

- 采用 CMake-first 工作流，根目录使用 `build.sh` 构建，产物安装到 `deploy/`。
- 使用 JSON 主配置（支持 `-f/--config`）。
- 使用 zlog（含 `.ini` 规则）进行日志管理。
- 源码按模块组织（`src/main.c` + `src/<模块>/`）。
- 可选包含少量非 C 资源（如 `web/`）。

## 模板路径

- 直接复用：`complex-app/`

## 使用流程

0. 若用户在实现前要先写 plan 文档，先执行“计划文档模式”并完成合规检查。
1. 与用户确认采用该复杂模板。
2. 复制 `complex-app/` 到目标项目根目录。
3. 最小必要修改后完成首轮可编译可运行验证。

## 计划文档模式（实现前）

当用户请求“先写 plan/实施方案”时，先输出计划，再进入代码阶段。计划至少覆盖：

1. 项目边界：明确这是复杂规范（JSON 主配置、zlog、模块化目录）。
2. 文件改动清单：模板复用项、必须修改项、按需扩展项（如 `web/`）。
3. 构建与部署验证：`build.sh` 流程、`deploy/` 产物（可执行文件、`app.json`、`zlog.ini`）。
4. 合规检查清单：实现完成后如何逐条回验复杂规范。

## 现有项目检查模式

当用户不是“新建项目”，而是“检查当前项目是否符合复杂规范”时，按以下顺序检查：

1. 构建入口是否为 CMake + `build.sh`，并使用 `build/` 与 `deploy/`。
2. 是否存在 JSON 主配置，并可通过 `-f/--config` 指定。
3. 是否使用 zlog + `.ini` 规则文件，而非 stdout 轻量日志方案。
4. 目录是否模块化（`src/main.c` + `src/<模块>/`），并包含必要配置/日志模块。
5. 发现偏离项后，按“最小改动”给出修正建议。

## 迁移后必须修改

- `complex-app/CMakeLists.txt`（项目名、目标名、模块路径、安装规则）
- `complex-app/src/main.c`、`complex-app/src/app/*`（程序入口与业务逻辑）
- `complex-app/configs/*.json`、`complex-app/configs/*.ini`（默认配置与日志规则）
- `complex-app/README.md`（构建、运行、配置说明）

## 通常可直接复用

- `complex-app/build.sh`
- `complex-app/deploy.sh`
- `complex-app/git-version.sh`
- `complex-app/Config.h.in`
- `complex-app/cmake/toolchain-*.cmake`
- `complex-app/cmake/thirdparty.cmake`
- `complex-app/.gitignore`

## 构建与部署约定

- `./build.sh`：Release 构建
- `./build.sh debug`：Debug 构建
- `./build.sh clean`：清理 `build/` 与 `deploy/`
- 安装后 `deploy/` 至少包含：可执行文件、`app.json`、`zlog.ini`
- 若存在 `web/`，需一并安装到 `deploy/`

## 验收底线

- 至少一次完整构建成功。
- 参数入口可用（至少 `-h/--help`、`-v/--version`、`-f/--config`）。
- 配置加载与 zlog 初始化可用。
