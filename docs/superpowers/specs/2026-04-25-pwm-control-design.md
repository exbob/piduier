# PWM 控制功能设计（Raspberry Pi 5）

## 1. 背景与目标

在现有 PiDuier 项目上新增 PWM 控制能力，覆盖 GPIO12 与 GPIO13 两个通道，并提供可用的 PWM 子页面。  
本次目标是首版可用能力，优先交付频率、占空比、极性与输出开关控制，不扩展超出需求的能力。

## 2. 需求确认（已冻结）

1. 控制对象为 GPIO12 与 GPIO13，对应双通道 PWM。
2. PWM 页面支持分别配置两路的：
   - 频率（Hz）
   - 占空比（%）
   - 输出开关（enable）
3. PWM 页面支持极性（polarity）设置：`normal` / `inversed`。
4. 输入约束：
   - `frequency_hz`：`1..1000000` 整数
   - `duty_percent`：`1..99` 整数，不支持小数
   - `polarity`：`normal` 或 `inversed`
5. 交互策略：
   - 参数（频率+占空比+极性）通过 `Apply` 按钮一次性下发
   - 开关单独即时生效
6. GPIO 页面对 GPIO12/GPIO13 不做特殊处理（保持现状）。

## 3. 前置条件与边界

本方案不负责自动完成 pin 复用与 overlay 配置，依赖系统与配置前置满足：

1. 配置文件 `pinctrl` 段需要保证：
   - `gpio12 = "a0"`
   - `gpio13 = "a0"`
2. 系统已启用 PWM（例如通过 overlay 使能双通道）。
3. 运行环境可访问 sysfs PWM 节点（通常是 `/sys/class/pwm/pwmchip0`）且具备写权限。

说明：当前程序启动时会根据配置文件执行 pinctrl 校正，因此若 `gpio12/gpio13` 不是 `a0`，会导致 PWM 复用被覆盖并失效。

## 4. 总体架构

采用分层方案（推荐方案A）：

1. **硬件层**：新增 `src/hardware/pwm.h`、`src/hardware/pwm.c`
   - 负责 sysfs 文件读写、参数换算、通道状态读取
2. **HTTP 路由层**：在 `src/http/router.c` 新增 PWM 路由与处理函数
   - 负责 URI 解析、参数校验、错误码与 JSON 返回
3. **前端页面层**：更新 `web/index.html`、`web/js/app.js`、`web/css/style.css`
   - 将 PWM 页从占位态升级为双通道控制页

通道映射在首版固定为：
- GPIO12 -> `pwmchip0/pwm0`（channel 0）
- GPIO13 -> `pwmchip0/pwm1`（channel 1）

## 5. 后端设计

### 5.1 数据模型

- `pwm_channel_t`
  - `chip`
  - `channel`
  - `gpio`
  - `exported`
  - `enabled`
- `pwm_config_t`
  - `frequency_hz`
  - `duty_percent`
  - `period_ns`
  - `duty_ns`

换算规则：
- `period_ns = 1_000_000_000 / frequency_hz`
- `duty_ns = period_ns * duty_percent / 100`

### 5.2 核心能力

建议导出接口：

1. `pwm_get_channels_status(...)`
   - 读取 ch0/ch1 当前状态并返回可用于 API 的结构
2. `pwm_apply_config(channel, frequency_hz, duty_percent, polarity)`
   - 应用频率、占空比与极性
3. `pwm_set_enable(channel, enable)`
   - 切换输出状态

建议私有辅助函数：
- `ensure_exported`
- `read_int_file`
- `write_int_file`
- `write_str_file`

### 5.3 写入顺序与安全性

为避免内核在运行态校验失败（如 `duty_cycle > period`），配置写入采用：

1. 记录当前 enable 状态
2. 若已启用，先临时写 `enable=0`
3. 写 `period`
4. 写 `duty_cycle`
5. 按原 enable 状态恢复

## 6. API 设计

### 6.1 获取状态

- `GET /api/pwm/channels`

返回示例：
```json
[
  {
    "channel": 0,
    "gpio": 12,
    "frequency_hz": 1000,
    "duty_percent": 50,
    "polarity": "normal",
    "enabled": 1
  },
  {
    "channel": 1,
    "gpio": 13,
    "frequency_hz": 500,
    "duty_percent": 25,
    "polarity": "inversed",
    "enabled": 0
  }
]
```

### 6.2 应用参数

- `POST /api/pwm/{channel}/config`

请求体：
```json
{
  "frequency_hz": 1000,
  "duty_percent": 50,
  "polarity": "normal"
}
```

行为：
- 仅应用频率/占空比，不改变开关状态。

### 6.3 开关输出

- `POST /api/pwm/{channel}/enable`

请求体：
```json
{
  "enable": 1
}
```

行为：
- 即时切换指定通道输出状态。

### 6.4 参数校验

1. `channel` 仅允许 `0` 或 `1`
2. `frequency_hz` 必须为 `1..1000000` 整数
3. `duty_percent` 必须为 `1..99` 整数
4. `polarity` 仅允许 `normal` 或 `inversed`
5. `enable` 仅允许 `0` 或 `1`

### 6.5 错误返回约定

- 入参错误：HTTP `400`
- 运行时失败（权限、节点缺失、写入失败等）：HTTP `503`
- 返回风格沿用现有接口，包含 `error/message/hint`

hint 示例方向：
- 检查 `/sys/class/pwm/pwmchip0` 是否存在
- 检查 GPIO12/GPIO13 pinctrl 是否为 `a0`
- 检查 PWM overlay 是否启用
- 检查进程权限

## 7. 前端设计

## 7.1 页面结构

PWM 页面展示两张通道卡片：

1. `PWM Channel 0 (GPIO12)`
2. `PWM Channel 1 (GPIO13)`

每卡片包含：
- `Frequency (Hz)` 输入框（整数 1..1000000）
- `Duty Cycle (%)` 输入框（整数 1..99）
- `Polarity` 选择框（`normal` / `inversed`）
- `Apply` 按钮
- `Output` 开关按钮（On/Off）
- 状态标签（Enabled/Disabled，当前值）

## 7.2 交互流程

1. 进入 `pwm` 页面：
   - 调用 `GET /api/pwm/channels` 初始化 UI
2. 点击 `Apply`：
   - 前端校验输入
   - 调用 `POST /api/pwm/{ch}/config`
   - 成功后刷新该通道状态并提示
3. 切换 `Output` 开关：
   - 调用 `POST /api/pwm/{ch}/enable`
   - 成功后即时更新状态标签

## 7.3 刷新策略

- 仅在当前页为 `pwm` 时启动轮询（建议 2 秒）
- 离开 `pwm` 页面立即停止轮询
- 与已有页面策略保持一致，避免全局无意义请求

## 7.4 失败反馈

- 前端输入校验失败：本地提示，不发请求
- 后端失败：展示后端 message/hint，便于现场定位

## 8. 与现有 GPIO 页面关系

按需求保持现状：

1. GPIO 页面不对 GPIO12/GPIO13 做禁用、隐藏或占用标识
2. 若用户在 GPIO 页变更这两个引脚导致冲突，PWM 页以实际失败结果提示
3. 冲突处理不在首版内做联动治理

## 9. 配置文件调整建议

需要同步至少以下配置文件的 `pinctrl` 段：

1. `config/piduier_debug.json`
2. `config/piduier_release.json`（若存在并用于发布）

调整项：
- `gpio12: "a0"`
- `gpio13: "a0"`

## 10. 测试与验收

### 10.1 手工测试清单

1. 打开 PWM 页，能读取两通道状态
2. 设置 `1000Hz + 50%` 并应用，读回一致
3. 分别对 ch0/ch1 开关输出，状态即时更新且可读回
4. 边界值验证：
   - `frequency=1`、`frequency=1000000`
   - `duty=1`、`duty=99`
   - `polarity=normal/inversed` 切换后读回一致
   - 高频场景（保证 `period_ns>=1`）
5. 异常场景：
   - 非法输入被前端拦截
   - 缺权限/缺节点时后端返回可诊断错误

### 10.2 验收标准（DoD）

1. PWM 页可独立控制 GPIO12/GPIO13 的频率、占空比、极性、开关
2. API 稳定返回，错误可定位
3. 不影响 Dashboard/GPIO 现有能力
4. 文档清晰标注前置条件与配置要求

## 11. 非目标（本次不做）

1. 自动配置 overlay / pinctrl
2. GPIO 与 PWM 的冲突联动治理
3. 超出 GPIO12/13 的更多 PWM 通道抽象
