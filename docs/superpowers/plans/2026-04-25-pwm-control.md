# PWM Control Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在现有 PiDuier 项目中实现 GPIO12/GPIO13 双通道 PWM 控制（频率、占空比、开关）并交付可用 PWM 页面。

**Architecture:** 采用硬件层（`src/hardware/pwm.*`）+ 路由层（`src/http/router.c`）+ 前端层（`web/*`）的分层实现。PWM 功能依赖系统预配置（`pinctrl.gpio12/gpio13=a0` + overlay 已启用），后端统一通过 sysfs 读写并由 API 向前端提供状态与控制能力。

**Tech Stack:** C11 + Mongoose + cJSON + Linux sysfs PWM + Vanilla JS/CSS + CMake

---

## Cross-Compile Test Constraint

本项目以交叉编译产物部署到树莓派运行为主，运行时验证统一按以下路径执行：

1. 本地完成交叉编译，产物输出到 `deploy/`
2. 使用项目现有部署方式（`deploy.sh` 或手工复制）将 `deploy/` 下文件部署到树莓派
3. 在树莓派启动 `piduier`（使用部署目录中的 `piduier.conf`）
4. 运行 `scripts/tests/pwm-api-smoke.sh`：
   - 在树莓派本机执行：`bash scripts/tests/pwm-api-smoke.sh http://127.0.0.1:8000`
   - 或在开发机远程执行：`bash scripts/tests/pwm-api-smoke.sh http://<raspi-ip>:8000`

禁止把“本机直接运行交叉编译出的目标二进制”作为通过标准。

---

## File Structure（先锁定边界）

- Create: `src/hardware/pwm.h`（PWM 数据结构与对外接口）
- Create: `src/hardware/pwm.c`（sysfs PWM 读写与参数换算）
- Modify: `CMakeLists.txt`（把 `src/hardware/pwm.c` 编进主程序）
- Modify: `src/http/router.c`（新增 PWM API 路由与处理函数）
- Modify: `web/index.html`（PWM 页面从占位改为双通道控制 UI）
- Modify: `web/js/app.js`（PWM 页面初始化、轮询、提交参数、开关控制）
- Modify: `web/css/style.css`（PWM 卡片与表单样式）
- Modify: `config/piduier_debug.json`（`pinctrl.gpio12/gpio13` 调整为 `a0`）
- Modify: `config/piduier_release.json`（`pinctrl.gpio12/gpio13` 调整为 `a0`）
- Create: `scripts/tests/pwm-api-smoke.sh`（最小 API 冒烟脚本，作为验证入口）

---

### Task 1: 建立可失败的 PWM 验证基线（TDD 起点）

**Files:**
- Create: `scripts/tests/pwm-api-smoke.sh`

- [ ] **Step 1: 先写失败的冒烟脚本（此时接口应不存在）**

```bash
#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${1:-http://127.0.0.1:8000}"

echo "[pwm-smoke] GET /api/pwm/channels"
status="$(curl -s -o /tmp/pwm_channels.out -w "%{http_code}" "${BASE_URL}/api/pwm/channels")"
echo "[pwm-smoke] status=${status}"
cat /tmp/pwm_channels.out

if [[ "${status}" != "200" ]]; then
  echo "[pwm-smoke] FAIL: expected 200"
  exit 1
fi

echo "[pwm-smoke] PASS"
```

- [ ] **Step 2: 运行脚本验证当前必然失败**

Run: `bash scripts/tests/pwm-api-smoke.sh`  
Expected: `status` 非 `200`，脚本退出码为 `1`。

- [ ] **Step 3: 提交基线脚本**

```bash
git add scripts/tests/pwm-api-smoke.sh
git commit -m "test: add failing PWM API smoke baseline"
```

---

### Task 2: 实现硬件层 PWM 模块（最小可用）

**Files:**
- Create: `src/hardware/pwm.h`
- Create: `src/hardware/pwm.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 写硬件层接口与结构定义（先让编译失败，提醒实现缺失）**

```c
// src/hardware/pwm.h
#ifndef PWM_H
#define PWM_H

#include <stddef.h>

typedef struct {
    int channel;       // 0 or 1
    int gpio;          // 12 or 13
    int enabled;       // 0/1
    int exported;      // 0/1
    int frequency_hz;  // integer
    int duty_percent;  // 0..100
} pwm_channel_status_t;

int pwm_get_channels_status(pwm_channel_status_t *out, size_t count);
int pwm_apply_config(int channel, int frequency_hz, int duty_percent);
int pwm_set_enable(int channel, int enable);

#endif
```

- [ ] **Step 2: 先运行构建，确认缺实现导致失败**

Run: `cmake -S . -B build && cmake --build build -j`  
Expected: 链接/符号缺失（`pwm_*` 未定义）失败。

- [ ] **Step 3: 实现最小 sysfs 逻辑（export、读取、写入、参数换算）**

```c
// src/hardware/pwm.c (核心骨架)
static int channel_to_gpio(int channel) { return channel == 0 ? 12 : (channel == 1 ? 13 : -1); }
static int ns_to_hz(int period_ns) { return period_ns > 0 ? (1000000000 / period_ns) : 0; }
static int percent_to_ns(int period_ns, int duty_percent) { return (period_ns * duty_percent) / 100; }

int pwm_apply_config(int channel, int frequency_hz, int duty_percent) {
    // validate
    // ensure exported
    // optional disable -> write period -> write duty_cycle -> restore enable
    return 0;
}
```

- [ ] **Step 4: 把模块加入构建**

```cmake
# CMakeLists.txt -> SOURCES
set(SOURCES
    src/main.c
    # ...
    src/hardware/gpio.c
    src/hardware/pwm.c
    src/third_party/mongoose/mongoose.c
)
```

- [ ] **Step 5: 重新构建验证通过**

Run: `cmake -S . -B build && cmake --build build -j`  
Expected: `piduier` 成功生成。

- [ ] **Step 6: 提交硬件层**

```bash
git add src/hardware/pwm.h src/hardware/pwm.c CMakeLists.txt
git commit -m "feat: add PWM hardware sysfs module"
```

---

### Task 3: 增加 PWM API 路由与后端校验

**Files:**
- Modify: `src/http/router.c`

- [ ] **Step 1: 先写失败验证（接口未接入时 404/非200）**

Run: `curl -i http://127.0.0.1:8000/api/pwm/channels`  
Expected: 非 `200`。

- [ ] **Step 2: 增加 PWM handler 声明与 URI 解析辅助函数**

```c
static void handle_pwm_channels(struct mg_connection *c, struct mg_http_message *hm);
static void handle_pwm_config(struct mg_connection *c, struct mg_http_message *hm);
static void handle_pwm_enable(struct mg_connection *c, struct mg_http_message *hm);
static int parse_pwm_channel_from_uri(struct mg_str uri, const char *suffix);
```

- [ ] **Step 3: 实现 3 个 API**

```c
// GET /api/pwm/channels
// POST /api/pwm/{channel}/config  {frequency_hz, duty_percent}
// POST /api/pwm/{channel}/enable  {enable}
// 输入错误返回 400；运行时失败返回 503 + hint
```

- [ ] **Step 4: 把路由接到主分发**

```c
else if (mg_match(hm->uri, mg_str("/api/pwm/channels"), NULL)) {
    handle_pwm_channels(c, hm);
} else if (uri_has_prefix(hm->uri, "/api/pwm/") && uri_has_suffix(hm->uri, "/config")) {
    handle_pwm_config(c, hm);
} else if (uri_has_prefix(hm->uri, "/api/pwm/") && uri_has_suffix(hm->uri, "/enable")) {
    handle_pwm_enable(c, hm);
}
```

- [ ] **Step 5: 运行服务并验证接口可达**

Run:
1. 本地交叉编译并产出 `deploy/`  
2. 使用现有部署流程将 `deploy/` 复制到树莓派  
3. 在树莓派启动服务：`./piduier -f ./piduier.conf`  
4. 本机或树莓派执行：`curl -s -i http://<raspi-ip-or-localhost>:8000/api/pwm/channels`  
Expected: HTTP `200` + JSON 数组。

- [ ] **Step 6: 提交路由层**

```bash
git add src/http/router.c
git commit -m "feat: add PWM REST APIs"
```

---

### Task 4: 配置前置条件落地（debug/release）

**Files:**
- Modify: `config/piduier_debug.json`
- Modify: `config/piduier_release.json`

- [ ] **Step 1: 先写失败验证（当前 pinctrl 非 a0）**

Run:
`rg '"gpio12":|"gpio13":' config/piduier_debug.json config/piduier_release.json`  
Expected: 看到 `"gpio12": "no"` 与 `"gpio13": "no"`。

- [ ] **Step 2: 修改配置为 PWM 复用**

```json
"gpio12": "a0",
"gpio13": "a0"
```

- [ ] **Step 3: 验证配置已生效**

Run:
`rg '"gpio12":|"gpio13":' config/piduier_debug.json config/piduier_release.json`  
Expected: 两个文件均显示 `a0`。

- [ ] **Step 4: 提交配置修正**

```bash
git add config/piduier_debug.json config/piduier_release.json
git commit -m "chore: set gpio12 and gpio13 pinctrl to a0 for PWM"
```

---

### Task 5: 实现 PWM 页面 HTML/CSS（先出可见界面）

**Files:**
- Modify: `web/index.html`
- Modify: `web/css/style.css`

- [ ] **Step 1: 先写失败验证（PWM 仍是占位文案）**

Run: `rg "PWM control features are under development" web/index.html`  
Expected: 有匹配。

- [ ] **Step 2: 替换 `page-pwm` 为双通道卡片结构**

```html
<div id="page-pwm" class="page">
  <div class="page-header">
    <h1 class="page-title">PWM Control</h1>
  </div>
  <div class="grid grid-2">
    <div class="card pwm-card" id="pwm-card-0">...</div>
    <div class="card pwm-card" id="pwm-card-1">...</div>
  </div>
</div>
```

- [ ] **Step 3: 添加 PWM 表单与状态样式**

```css
.pwm-card .form-field { margin-bottom: 12px; }
.pwm-status-chip { border: 1px solid var(--color-border); border-radius: 999px; padding: 4px 10px; }
.pwm-actions { display: flex; gap: 8px; }
```

- [ ] **Step 4: 验证占位文案已移除**

Run: `rg "PWM control features are under development" web/index.html`  
Expected: 无匹配。

- [ ] **Step 5: 提交页面骨架**

```bash
git add web/index.html web/css/style.css
git commit -m "feat: add PWM page layout and styles"
```

---

### Task 6: 实现 PWM 前端交互逻辑并完成冒烟测试

**Files:**
- Modify: `web/js/app.js`
- Modify: `scripts/tests/pwm-api-smoke.sh`

- [ ] **Step 1: 先写失败验证（脚本仍失败）**

Run: `bash scripts/tests/pwm-api-smoke.sh`  
Expected: FAIL（直到前端与接口联通后再改脚本完整断言）。

- [ ] **Step 2: 增加 PWM 页面状态与初始化逻辑**

```javascript
let pwmPollInterval = null;

function initPwmPage() { /* bind apply + enable handlers */ }
function loadPwmChannels() { /* GET /api/pwm/channels */ }
function updatePwmPollingState() { /* only when currentPage==='pwm' */ }
```

- [ ] **Step 3: 增加参数提交与开关控制**

```javascript
async function applyPwmConfig(channel) {
  // validate integer frequency_hz and duty_percent
  // POST /api/pwm/{channel}/config
}

async function togglePwmEnable(channel, enable) {
  // POST /api/pwm/{channel}/enable
}
```

- [ ] **Step 4: 把 `switchPage` 与自动刷新逻辑接入 PWM**

```javascript
if (page === 'pwm') {
  loadPwmChannels();
}
// in page-state updater: start/stop pwm poll interval
```

- [ ] **Step 5: 更新冒烟脚本为“完整通过断言”**

```bash
# 追加:
# 1) POST /api/pwm/0/config -> expect 200
# 2) POST /api/pwm/0/enable -> expect 200
# 3) GET /api/pwm/channels  -> verify channel 0 fields exist
```

- [ ] **Step 6: 运行最终验证**

Run:
1. 本地交叉编译并生成 `deploy/`
2. 使用 `deploy.sh` 或手工复制到树莓派
3. 在树莓派启动服务：`./piduier -f ./piduier.conf`
4. 运行冒烟：
   - 树莓派本机：`bash scripts/tests/pwm-api-smoke.sh http://127.0.0.1:8000`
   - 或开发机远程：`bash scripts/tests/pwm-api-smoke.sh http://<raspi-ip>:8000`

Expected:
- 交叉编译与部署成功
- 冒烟脚本输出 `PASS`
- 打开 `http://<raspi-ip>:8000`，PWM 页可控制两通道

- [ ] **Step 7: 提交前端交互与测试**

```bash
git add web/js/app.js scripts/tests/pwm-api-smoke.sh
git commit -m "feat: implement PWM page interactions and smoke test"
```

---

## Plan Self-Review

### 1) Spec coverage

- 双通道 GPIO12/GPIO13 控制：Task 2/3/5/6 覆盖
- 参数模型（整数 Hz 与整数 %）：Task 3/6 覆盖
- Apply 与开关分离：Task 6 覆盖
- 不做极性：未引入 polarity 字段，覆盖
- 不做 GPIO 页特殊处理：计划未修改 GPIO 逻辑，覆盖
- 前置条件 pinctrl 与配置要求：Task 4 覆盖

### 2) Placeholder scan

- 已检查，无 `TODO/TBD/implement later` 等占位语句
- 每个代码步骤都给出明确代码片段
- 每个验证步骤都有命令与预期

### 3) Type consistency

- 后端统一使用 `channel=0/1`、`frequency_hz`、`duty_percent`、`enable`
- 前端与冒烟脚本接口字段一致
- 与 spec 命名一致，无冲突

