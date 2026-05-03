---
name: git-workflow
description: Use when the user needs guidance on feature/fix/chore branch naming, stash-before-checkout, pull-then-merge discipline, semver tagging decisions, or symptoms like working on main, huge uncommitted diffs before switching branches, or confusion about MAJOR/MINOR/PATCH bumps.
---

# Git 工作流规范

## 目标

在临时分支上小步提交、可回溯；合并回主开发分支前与远程同步；需要切换分支时用 stash 保护未提交改动；对外发布或里程碑用 `git tag` 标记语义化版本号；**仅软件功能与缺陷类**的发布级变更才递增版本号。提交信息必须遵守 **git-commit** skill。

## 主开发分支与临时分支

- **主开发分支**：使用团队约定的 `main`、`master` 或 `develop` 之一（以仓库实际默认分支为准），不在其上直接堆长期开发。
- **临时分支命名**：`<type>/<description>`
  - `feat/`：新功能
  - `fix/`：缺陷修复
  - `chore/`：杂项、非功能/非修复类改动（与 Conventional Commits 中 `chore` 语义一致时可选用）
- `description`：简短、小写、用连字符分隔单词（例如 `feat/user-login-oauth`），避免空格与含糊名称。

## 提交节奏与 Commit Message

- 每完成一小段**可验证**的改动后尽快 `git commit`，便于 bisect 与回溯。
- **提交信息**：生成或审阅 message 时必须遵循 **git-commit** skill（Conventional Commits、type/scope/subject 等），禁止长期保留 `WIP`、`fix`、`update` 这类无信息提交（除非即将 squash 且团队明确允许）。

## 切换分支且暂不想提交：`git stash`

在临时分支开发到一半，需要切去其他分支且**不想**为当前半成品单独 commit 时：

```bash
git stash push -m "简短说明当前上下文"
# … 在其他分支完成工作 …
git checkout <原临时分支>
git stash pop   # 注意：是 git stash pop，不是 git pop
```

- 冲突时按 Git 提示解决后继续；`stash pop` 成功后会从栈中移除该条 stash。
- 仅查看列表：`git stash list`；应用但保留栈顶条目：`git stash apply`。

## 开发完成：合并回主开发分支

在临时分支上功能或修复**已验证完成**后，再合回主开发分支。若远程主分支有保护规则或团队要求代码评审，应通过 **PR/MR** 合并；下列本地 `git merge` 仅适用于允许直接合并到主线的流程。

推荐顺序（以主分支名为 `main` 为例，请替换为实际的 `master` / `develop`）：

```bash
git checkout main
git pull
git merge <你的临时分支名>
```

合并成功后，若不再需要该临时分支，可删除本地分支（并视情况删除远程同名分支）：

```bash
git branch -d <你的临时分支名>
# 若已推送过远程：git push origin --delete <你的临时分支名>
```

**要点**：合并前主分支上的 `git pull` 用于拉取远程最新提交，减少不必要的合并冲突与「合入已过时的 main」类问题。

## 版本标签：`git tag` 与 `v<MAJOR>.<MINOR>.<PATCH>`

**总原则：只有「软件功能」或「缺陷」相关的发布级变更，才递增版本号并打新标签。** 新能力、对用户或运行时可见的行为增强属**功能**；错误修正属**缺陷**。纯文档、`chore`、格式、CI、依赖整理等**不**仅因这类提交而升级版本号。

用 **带 `v` 前缀的语义化版本号** 标记已发布或已冻结的里程碑，格式严格为：

```text
v<MAJOR>.<MINOR>.<PATCH>
```

三段均为非负整数，含义：

| 段 | 含义 | 典型何时递增 |
|----|------|----------------|
| **MAJOR** | 重要版本升级 | 不兼容变更、大架构调整、产品代际级变化 |
| **MINOR** | 小功能迭代 | 向后兼容的新功能、较小范围的能力增强 |
| **PATCH** | 缺陷修复 | 修 bug、小补丁，不改变对外约定的行为边界 |

**示例**：

- `v1.0.0`：满足初始需求集后的**第一个正式版本**（首个对外或对内「基线」发布）。
- 之后在已发布 `v1.0.0` 的前提下，仅修复若干缺陷可打 **`v1.0.1`**；累积小功能可到 `v1.1.0`；重大升级则 `v2.0.0`。

下列情况**不**单独触发版本号升级（正常合并即可，无需另起 `git tag`），与总原则一致即可判断：

- **纯文档**：`docs`、README、设计稿、注释等。
- **维护与工程化**：`chore`、`style`、`ci`、`build` 等，且**不涉及**对外可交付软件的功能增减或缺陷修复时。

标签应打在**主开发分支上已合并、已验证**的提交（常见为打 tag 前已 `pull` 到最新）。本地示例：

```bash
git checkout main
git pull
git tag -a v1.0.1 -m "patch: 修复 xxx"
git push origin v1.0.1
```

说明：`-a` 为附注标签（推荐，可写 release 摘要）；若团队仅用轻量标签，可用 `git tag v1.0.1`。**禁止**与语义化版本混用的随意字符串（如 `release1`、`ver-2024`）作为同一套版本体系的标签名。

## 快速对照

| 场景 | 做法 |
|------|------|
| 开始新功能/修复 | 从主开发分支拉出 `feat/...` 或 `fix/...` |
| 小步可验证改动 | 尽快 commit，message 跟 git-commit skill |
| 半成品要换分支 | `git stash`，回来 `git stash pop` |
| 准备合回主线 | 检出主分支 → `git pull` → `git merge <临时分支>` → 可删临时分支 |
| 发布 / 里程碑版本 | 主线最新且已验证 → `git tag` 为 `vMAJOR.MINOR.PATCH` → `git push origin <tag>` |
| 非功能、非缺陷类（文档、`chore`、格式/CI 等） | 合并即可，**不**升版本号、不打新 tag |

## 常见错误

| 问题 | 纠正 |
|------|------|
| 使用 `git pop` | 正确命令为 `git stash pop` |
| 在 main 上长期堆改 | 用 `feat/`、`fix/`、`chore/` 临时分支 |
| 分支名无 type 前缀 | 统一为 `<type>/<description>` |
| 合并前不 pull | 先更新主开发分支再 merge |
| 巨大单 commit | 拆成多步小提交，每步可验证 |
| 标签写成 `1.0.0` 或无 `v` | 统一为 `v1.0.0` 形式 |
| 同一逻辑版本多次改 tag 指向 | 已推送的 tag 勿强改；用下一 PATCH 或团队约定的撤回流程 |
| 非功能/非缺陷类也升 PATCH | 仅**功能与缺陷类**发布才递增版本号 |

## 何时不必死板

- 仓库已有强制策略（例如必须用 PR/MR、禁止本地 merge、必须用 rebase）时，以团队规范为准，本 skill 描述的是通用本地工作流骨架。
- 仅改一字节的文档 typo 且团队允许，可直接在主分支提交；默认仍推荐走临时分支以降低风险。
