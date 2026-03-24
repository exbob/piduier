# GitHub Action arm64 Release Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增一个 GitHub Actions 工作流：当推送 `v*` tag 时自动构建 arm64、打包 `deploy/` 为 `piduier-${tag}-arm64.tar.gz` 并发布到 Release；若同 tag 的 Release 已存在则失败。

**Architecture:** 仅新增一个 workflow 文件，复用现有 `build.sh`（`ARCH=arm64`）生成产物。工作流在单个 job 中完成检出、依赖安装、构建、产物校验、打包、Release 存在性检查与发布上传。触发验证仅覆盖“tag push 触发、无 tag push 不触发”。

**Tech Stack:** GitHub Actions (`ubuntu-latest`), Bash, `gh` CLI（通过 `GH_TOKEN`）, `softprops/action-gh-release`, 现有 `build.sh`.

---

硬约束映射：
- `v*` tag 触发：Task 1
- 资产命名 `piduier-${tag}-arm64.tar.gz`：Task 3 Step 1
- Release 存在则失败：Task 3 Step 2
- 验证仅触发条件：Task 4

### Task 1: 新建工作流骨架与触发条件

**Files:**
- Create: `.github/workflows/release-arm64-on-tag.yml`
- Test: N/A（本任务为静态配置落盘）

- [ ] **Step 1: 写出工作流最小骨架（先不填实现细节）**

```yaml
name: release-arm64-on-tag

on:
  push:
    tags:
      - "v*"

permissions:
  contents: write

jobs:
  build-and-release-arm64:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4
```

- [ ] **Step 2: 检查 YAML 语法结构（本地阅读 + 缩进检查）**

Run: 人工检查 `.github/workflows/release-arm64-on-tag.yml` 缩进、键级别、引号是否正确。  
Expected: 无明显缩进错误，`on.push.tags` 为 `v*`。

- [ ] **Step 3: 提交该最小骨架**

```bash
git add .github/workflows/release-arm64-on-tag.yml
git commit -m "ci: add arm64 tag-triggered release workflow skeleton"
```

---

### Task 2: 补全构建与产物校验步骤（TDD 风格先定义失败条件）

**Files:**
- Modify: `.github/workflows/release-arm64-on-tag.yml`
- Test: GitHub Actions job logs（远端）

- [ ] **Step 1: 写“失败优先”的产物检查脚本**

在 workflow 中加入 Bash step，使用 `set -euo pipefail` 并校验以下路径存在：

- `deploy/piduier`
- `deploy/piduier.conf`
- `deploy/install.sh`
- `deploy/uninstall.sh`
- `deploy/piduier.service`
- `deploy/web/`

参考实现片段：

```bash
set -euo pipefail
required_paths=(
  "deploy/piduier"
  "deploy/piduier.conf"
  "deploy/install.sh"
  "deploy/uninstall.sh"
  "deploy/piduier.service"
  "deploy/web"
)
for p in "${required_paths[@]}"; do
  if [ ! -e "$p" ]; then
    echo "Missing required artifact path: $p" >&2
    exit 1
  fi
done
```

- [ ] **Step 2: 加入构建依赖安装和 arm64 构建步骤**

Run in workflow:

```bash
sudo apt-get update
sudo apt-get install -y cmake pkg-config gcc-aarch64-linux-gnu
ARCH=arm64 ./build.sh
```

Expected: `build.sh` 结束后 `deploy/` 存在。

- [ ] **Step 3: 本地快速验证构建命令可执行（可选）**

Run: `ARCH=arm64 ./build.sh`  
Expected: 本地环境允许时可构建；该步骤可完全跳过，不影响本轮验收（本轮验收仅看触发条件）。

- [ ] **Step 4: 提交构建与校验逻辑**

```bash
git add .github/workflows/release-arm64-on-tag.yml
git commit -m "ci: add arm64 build and artifact validation steps"
```

---

### Task 3: 实现打包、Release 存在性检查与发布

**Files:**
- Modify: `.github/workflows/release-arm64-on-tag.yml`
- Test: GitHub Actions job logs（远端）

- [ ] **Step 1: 增加打包步骤并固定文件名规则**

在 workflow 中定义环境变量并打包：

```bash
set -euo pipefail
TAG_NAME="${GITHUB_REF_NAME}"
ASSET_NAME="piduier-${TAG_NAME}-arm64.tar.gz"
tar -czf "${ASSET_NAME}" deploy
echo "asset_name=${ASSET_NAME}" >> "$GITHUB_OUTPUT"
```

Expected: 生成 `piduier-${tag}-arm64.tar.gz`，且压缩包内包含顶层 `deploy/`。

- [ ] **Step 2: 增加“Release 已存在即失败”步骤**

在 workflow 中使用 `gh api` 检查，并显式区分三种情况：

- HTTP 200：Release 已存在 -> 业务失败（`exit 1`）
- HTTP 404：Release 不存在 -> 正常继续
- 其他错误（网络/鉴权/API 异常）：系统失败（`exit 1`，打印错误）

参考实现：

```bash
set -euo pipefail
api_path="repos/${GITHUB_REPOSITORY}/releases/tags/${GITHUB_REF_NAME}"

set +e
api_output="$(gh api "${api_path}" 2>&1)"
api_status=$?
set -e

if [ "${api_status}" -eq 0 ]; then
  echo "Release for tag ${GITHUB_REF_NAME} already exists, failing as required." >&2
  exit 1
fi

if printf '%s' "${api_output}" | grep -q "HTTP 404"; then
  echo "No existing release for tag ${GITHUB_REF_NAME}, continue."
else
  echo "Release lookup failed with non-404 error:" >&2
  printf '%s\n' "${api_output}" >&2
  exit 1
fi
```

并设置环境变量：

```yaml
env:
  GH_TOKEN: ${{ github.token }}
```

Expected:  
- Release 已存在 -> 该 step 失败并终止  
- Release 不存在（404）-> 继续后续步骤  
- API/网络/鉴权异常 -> 失败并输出错误原因

- [ ] **Step 3: 创建 Release 并上传资产**

使用 action：

```yaml
- name: Publish release asset
  uses: softprops/action-gh-release@v2
  with:
    tag_name: ${{ github.ref_name }}
    files: ${{ steps.package.outputs.asset_name }}
```

Expected: 实现层面应可创建 Release 并上传资产；但本轮验收不以该结果为通过条件（验收仅看触发条件）。

- [ ] **Step 4: 提交发布逻辑**

```bash
git add .github/workflows/release-arm64-on-tag.yml
git commit -m "ci: publish arm64 deploy tarball on v* tags"
```

---

### Task 4: 触发条件验收（按已确认的简化范围）

**Files:**
- Modify: 无（仅操作 git 与远端）
- Test: GitHub Actions workflow runs 页面

- [ ] **Step 1: 验证普通 push 不触发**

Run:

```bash
git push origin <working-branch>
```

Expected: `release-arm64-on-tag` 不触发。

- [ ] **Step 2: 验证新 `v*` tag push 触发**

Run（示例）:

```bash
git tag v1.0.1
git push origin v1.0.1
```

Expected: `release-arm64-on-tag` 被触发。

- [ ] **Step 3: 记录验收结论**

在 PR 描述或评论中记录：
- 无 tag push：未触发  
- `v*` tag push：已触发

- [ ] **Step 4: 提交验收记录（如有本地文档变更）**

```bash
git add <any-updated-docs>
git commit -m "docs: record workflow trigger verification results"
```

---

### Task 5: 收尾与交付

**Files:**
- Modify: `README.md`（可选，若需补充 CI 发布说明）
- Test: N/A

- [ ] **Step 1: 可选更新 README 的发布说明（仅在团队需要时）**

补充一句：推送 `v*` tag 会自动构建并发布 arm64 包。

- [ ] **Step 2: 汇总风险与后续可选增强**

记录非本轮范围项（例如 `sha256`、多架构矩阵、签名发布）。

- [ ] **Step 3: 最终检查**

Run:

```bash
git status
```

Expected: 工作区干净，提交历史清晰（按任务分段提交）。
