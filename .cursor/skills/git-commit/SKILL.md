---
name: git-commit-message
description: Use this skill to standardize Git commit messages with Conventional Commits. Trigger whenever the user mentions “commit”，“提交”，“commit messages”，“提交信息”, “Conventional Commits”, or asks to make commit text more consistent. Even when only code changes are provided, first generate a structured and reviewable commit message with this skill.
---

# Git Commit Message 规范

## 目标

将提交信息统一为 Conventional Commits，减少含糊提交（如 `fix stuff`、`update`、`WIP`），提升变更可追踪性与可读性。

## 输出格式

始终优先输出以下结构：

```text
<type>[optional scope]: <subject>

[optional body]

[optional footer(s)]
```

约束：
- `type` 必须从下面的 “Type 选择规则” 中选择。
- `scope` 建议提供；当改动范围不明确或非常小，可省略为 `<type>: <subject>`。
- `subject` 使用祈使语气，不加句号，建议不超过 50 字符。可使用中文或英文。
- `body` 解释“为什么改”，而不是重复“改了什么”。如果内容较多，可以分成多行，或者用空行分隔不同段落。可使用中文或英文。
- `footer` 可用于 `BREAKING CHANGE`、`Closes #123` 等追踪信息。

## Type 选择规则

| Type | Use For | Example |
|------|---------|---------|
| `feat` | New feature | `feat(auth): add OAuth2 login` |
| `fix` | Bug fix | `fix(api): handle null response in user endpoint` |
| `docs` | Documentation | `docs(readme): update installation instructions` |
| `style` | Formatting, no code change | `style: fix indentation in login component` |
| `refactor` | Code refactoring | `refactor(db): extract connection pool to module` |
| `test` | Adding/updating tests | `test(auth): add unit tests for token validation` |
| `perf` | Performance improvement | `perf(query): add index to users table` |
| `build` | Build system | `build: update webpack config` |
| `ci` | CI/CD changes | `ci: add PostgreSQL service to test workflow` |
| `chore` | Maintenance tasks | `chore(deps): update dependencies` |
| `revert` | Revert previous commit | `revert: revert "feat(auth): add OAuth2 login"` |

## 生成提交信息时的工作流

1. 先判断变更类型，选择最贴近目的的 `type`（避免泛化成 `chore`）。
2. 提炼变更边界，给出简洁 `scope`（如 `api`、`ui`、`db`、`auth`）。
3. 写出可读的 `subject`（祈使语气 + 明确动作）。
4. 如有必要补充 `body`（背景、原因、权衡）。
5. 如有 issue 或破坏性变更，补 `footer`。

如果信息不足，应先向用户追问：变更目的、影响范围、是否关联 issue、是否包含 breaking change。

## Good vs Bad 示例

```bash
# BAD: Vague, no context
git commit -m "fixed stuff"
git commit -m "updates"
git commit -m "WIP"
```

```bash
# GOOD: Clear, specific, explains why
git commit -m "fix(api): retry requests on 503 Service Unavailable

The external API occasionally returns 503 errors during peak hours.
Added exponential backoff retry logic with max 3 attempts.

Closes #123"
```

## `.gitmessage` 模板

在仓库根目录创建 `.gitmessage`：

```text
# <type>[optional scope]: <subject>
# 
# [optional body]
# 
# [optional footer(s)]
#
# ============================================
# type：
#   feat:     新功能
#   fix:      修复问题
#   docs:     文档修改
#   style:    代码格式调整（不影响功能）
#   refactor: 代码重构
#   test:     测试相关
#   perf:     性能优化
#   build:    构建相关
#   ci:       CI/CD 相关
#   chore:    其他修改
#   revert:   回滚之前的提交
#
# subject：简短描述改动内容（不超过 50 字符）
# optional body：可选，说明改动原因、实现方式等
# optional footer：可选，关联 issue 或 breaking changes
# ============================================
```

启用模板：

```bash
git config commit.template .gitmessage
```

## 对用户的默认交付

当用户请求“帮我写 commit message”时，默认给出：

1. 一个推荐的单行提交信息（`<type>[optional scope]: <subject>`）
2. 一个扩展版（含 `body/footer`，若适用）
3. 一句简短理由，说明为什么选择该 `type` 和 `scope`
