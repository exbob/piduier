# GitHub Action: Tag Triggered arm64 Release Design

## 1. Goal

为项目新增 GitHub Actions 流程，实现以下自动化能力：

- 当推送 `v*` 格式的 tag（例如 `v1.0.1`）时，自动触发工作流
- 使用项目现有构建脚本生成 `arm64` 产物
- 将 `deploy/` 目录打包为 `tar.gz`
- 自动发布到 GitHub Release

## 2. Confirmed Decisions

以下约束已与用户确认：

- 触发规则：仅 `v*` tag 触发
- 资产命名：`piduier-${tag}-arm64.tar.gz`
- Release 已存在策略：严格失败（不覆盖、不更新）
- 验证范围：仅验证触发条件
  - 推送 `v*` tag 时触发
  - 普通 push（无 tag）时不触发

## 3. Existing Project Context

当前仓库已具备以下基础条件：

- 使用根目录 `build.sh` 进行构建
- `ARCH=arm64 ./build.sh` 可生成面向 arm64 的 `deploy/` 目录产物
- `README.md` 已明确 `deploy/` 为发布产物目录，包含 `piduier`、配置、安装脚本和 `web/` 静态资源
- 仓库目前尚无 `.github/workflows/` 目录和现有 CI 发布流程

## 4. Proposed Workflow Design

建议新增工作流文件：

- `.github/workflows/release-arm64-on-tag.yml`

### 4.1 Trigger and Permissions

- `on.push.tags: ['v*']`
- `permissions.contents: write`（用于创建 Release 和上传资产）

### 4.2 Job Structure

单一 job：`build-and-release-arm64`（`runs-on: ubuntu-latest`）

执行顺序：

1. Checkout 代码
2. 安装构建依赖（包含交叉编译工具链）
3. 执行 `ARCH=arm64 ./build.sh`
4. 校验 `deploy/` 及关键文件存在
5. 打包产物为 `piduier-${GITHUB_REF_NAME}-arm64.tar.gz`
6. 检查同名 tag 的 Release 是否已存在（按 tag 判定，tag 为 `GITHUB_REF_NAME`）
   - 若存在：立即失败退出
   - 若不存在：继续
7. 创建 Release 并上传打包产物

关键文件最小集合（仅检查“存在”，不做内容校验）：

- `deploy/piduier`
- `deploy/piduier.conf`
- `deploy/install.sh`
- `deploy/uninstall.sh`
- `deploy/piduier.service`
- `deploy/web/`

Release 存在性检查实现约定：

- 判定维度：只按 tag 是否已有关联 Release
- 失败语义：
  - “Release 已存在”属于业务失败，直接 `exit 1`
  - API/网络/鉴权错误属于系统失败，也直接失败并输出明确错误原因
- 目标是保证“同一 tag 不可重复发布”

## 5. Error Handling and Idempotency

- 全流程使用失败即终止策略（例如 `set -euo pipefail`）
- 对构建失败、目录缺失、关键文件缺失统一 fail-fast
- Release 存在即失败，符合“严格失败”要求
- 资产名带 tag，具备明确可追溯性
- 压缩包结构约定：保留 `deploy/` 顶层目录（解压后应为 `deploy/...`）

## 6. Test and Acceptance (Simplified)

本次只验证工作流触发条件，不扩展到发布内容校验：

1. 使用一个全新 tag（非已存在 tag）执行推送  
   例如 `v1.0.1` 或 `v1.0.0-test1`
   - 预期：工作流被触发
2. 执行一次普通分支 push（不带 tag）
   - 预期：该工作流不触发

通过条件：

- `v*` tag push 触发成功
- 无 tag push 不触发
- 本轮不验证 Release 上传结果与产物内容，仅验证触发条件

## 7. Out of Scope

以下内容不在本轮范围内：

- 多架构并行发布（如 x86 + arm64）
- 自动覆盖已存在 Release
- 附加产物（sha256 文件、SBOM、签名等）
- 复杂发布矩阵与多环境发布策略

## 8. Implementation Notes

为降低改动风险，工作流应尽量复用现有 `build.sh` 与目录约定，不改动业务代码。CI 逻辑与项目运行时代码解耦，仅新增 `.github/workflows/` 配置文件。
