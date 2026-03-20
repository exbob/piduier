# 技术实现方案（基于现有 piduier 工程）

本文档对应 [`PRODUCT_DESIGN_PROPOSAL.md`](./PRODUCT_DESIGN_PROPOSAL.md) 中的功能与非功能需求，说明**在现有代码库上如何落地**，供研发排期与客户了解实现路径。

---

## 1. 现有架构速览

| 层级 | 现状 | 路径/说明 |
|------|------|-----------|
| HTTP 服务 | Mongoose 单进程，`mg_http_listen`，默认 `0.0.0.0:8000` | `src/main.c` |
| 路由分发 | 按 URI 前缀匹配，JSON API + 静态文件 | `src/http/router.c`、`router_handle_request` |
| 静态前端 | `web/` → 安装到 `bin/web/` | `CMakeLists.txt` `install(DIRECTORY web/ ...)` |
| 系统/网络 | 已实现多组 `/api/system/*`、`/api/network/*` | `src/system/*`、`src/network/*` |
| GPIO | **libgpiod**，白名单 10 个 BCM 脚；REST 已接好 | `src/hardware/gpio.c`、`/api/gpio/*` |
| 构建 | CMake + `build.sh`，`ARCH=x86` / `ARCH=arm64` 交叉编译 | `build.sh`、`CMakeLists.txt` |
| 版本号 | **尚未**按 `git describe` 注入 | 需在 CMake 中增加（见 §6） |

**结论**：监控与网络能力可保留；产品方案中的 **PWM / I2C / SPI / UART** 与 **版本注入** 为新增工作；GPIO 与 40pin 展示需按 PRD 补齐体验与能力边界。**Web 控制台用户界面统一为英文**（§9）；**构建与部署分阶段执行**，制品在目标机运行于 **`~/Desktop/piduier`**（§7）。

---

## 2. 需求与现状对照（差距分析）

| PRD ID | 需求摘要 | 现状 | 实现动作 |
|--------|----------|------|----------|
| F-01 | 40pin 功能展示 | `web/index.html` + `app.js` 有 pinout 表与图例 | 与 Pi5 定义复核；为「不可用」状态预留 API 字段与 UI 提示 |
| F-02 | GPIO 方向/输入实时/输出 | 后端完整；前端轮询刷新 | 缩短轮询间隔或页面可见时加速轮询；x86 无 gpiod 时返回明确 JSON 错误 |
| F-03 | PWM | 无 | 新增 `src/hardware/pwm.*` + `/api/pwm/*` + 前端页 |
| F-04 | I2C/SPI 主、寄存器读写 | 无 | 新增 `i2c.*` / `spi.*` + REST；可选 `i2cdetect` 类扫描 |
| F-05 | UART 参数与收发 | 无 | 新增 `uart.*` + **推荐 WebSocket 或 SSE**（见 §4.5）+ 终端式 UI |
| 非功能 | `git describe` 版本 | 无 | CMake 生成宏或头文件， `/api/system/version` + 关于页 |
| 非功能 | x86 仅看 UI | 无 gpiod 时 GPIO API 失败 | 可选：`MOCK_HARDWARE` 编译开关返回假数据；或静态打开 `index.html` 仅看布局 |
| 非功能 | 全英文 UI | 当前部分文案为中文 | `web/` 全面改为英文（与 §9 一致） |
| 非功能 | 构建/部署分离；目标路径 | 文档曾混写 | 构建只用 `build.sh`/CMake；部署只同步到 `~/Desktop/piduier`（§7） |

---

## 3. 目标软件架构（增量）

在**不引入第二进程**的前提下，保持 Mongoose 事件循环为唯一网络入口：

```
main.c (mg_mgr_poll)
    └── router.c
            ├── 静态文件 web/
            ├── 已有 /api/system/*, /api/network/*
            └── 硬件调试 API（新增）
                    ├── /api/gpio/*     → gpio.c（已有）
                    ├── /api/pwm/*      → pwm.c（新）
                    ├── /api/i2c/*      → i2c.c（新）
                    ├── /api/spi/*      → spi.c（新）
                    └── /api/uart/*     → uart.c（新）+ 可能升级 ev_handler 处理 WS
```

**目录约定**（建议）：

- `src/hardware/gpio.c`（已有）
- `src/hardware/pwm.c`、`pwm.h`
- `src/hardware/i2c_master.c`、`i2c_master.h`（名称避免与 Linux 头冲突）
- `src/hardware/spi_master.c`、`spi_master.h`
- `src/hardware/uart_port.c`、`uart_port.h`
- `src/http/router.c`：仅做解析、鉴权（若以后有）、调用上述模块并回复 JSON

**可选公共层**：`src/hardware/hw_error.h` 统一 `errno` / 驱动错误到 HTTP 状态码与 `{"error":"..."}` 文案，满足 PRD「错误可行动」；**API 错误文案与 Web UI 一律使用英文**（见 §9）。

---

## 4. 分模块实现要点

### 4.1 F-01：40pin 展示

- **数据源**：物理引脚定义继续以前端 `pinDefinitions`（`web/js/app.js`）为单一真源时，需与 Raspberry Pi 5 官方 pinout **书面核对**；或改为后端下发 JSON（利于客户验收「与系统一致」），但工作量更大。
- **「不可用」状态**：各子系统初始化失败时，在对应 API 返回 `hardware_available: false` 或顶层 `/api/hardware/capabilities`，前端在关于页与 pinout 上显示统一提示。

### 4.2 F-02：GPIO（强化）

- **后端**：`gpio.c` 已支持 `GET /api/gpio/status`、`GET/POST /api/gpio/{n}/mode|value`。若需扩大可配置引脚范围，修改 `GPIO_SUPPORTED_PINS` 并与安全策略（避免与串口/SPI 等冲突）一并评审。
- **输入实时性**：前端对 GPIO 页使用 `setInterval(200~500ms)` 调用 `GET /api/gpio/status`（或单脚 `GET`）；在 tab 不可见时降低频率。
- **x86 构建**：无 `HAVE_LIBGPIOD` 时，`gpio_*` 返回结构化错误；文档说明「实机调试需 ARM64 + libgpiod」。

### 4.3 F-03：PWM

- **Linux 接口**：通过 sysfs PWM（`/sys/class/pwm/...`）或 **libgpiod 不直接管 PWM**，通常操作 `pwmchip*` 导出、设 `period`/`duty_cycle`/`enable`。
- **建议 API**（示例）：
  - `GET /api/pwm/chips` — 列出可用 pwmchip 与 channel
  - `POST /api/pwm/{chip}/{channel}/config` — JSON：`frequency_hz` 或 `period_ns`、`duty_percent` 或 `duty_ns`、`enabled`
  - `POST /api/pwm/{chip}/{channel}/enable` — `true/false`
- **并发**：单进程内对同一 channel 保持单文件描述符或状态机，避免多次 export。
- **前端**：沿用 PRD 导航「PWM」页，表单 + 开关；错误展示驱动返回范围问题。

### 4.4 F-04：I2C / SPI（主设备）

**I2C**

- **实现**：打开 `/dev/i2c-N`，`ioctl` `I2C_SLAVE` + `I2C_RDWR` 或 `read`/`write` 实现「写寄存器地址 + 读/写数据」。
- **建议 API**：
  - `GET /api/i2c/buses` — 列出 `/dev/i2c-*`
  - `POST /api/i2c/{bus}/read` — body：`address`（7-bit）、`reg`（可选）、`length`
  - `POST /api/i2c/{bus}/write` — body：`address`、`reg`（可选）、`data`（hex 或 byte 数组）
  - 可选：`POST /api/i2c/{bus}/scan` — 返回响应地址列表（0x03–0x77）
- **权限**：依赖 README 中 `i2c` 设备组与 udev。

**SPI**

- **实现**：打开 `/dev/spidevB.C`，`ioctl` `SPI_IOC_MESSAGE(n)` 或 `write`+`read`。
- **建议 API**：
  - `GET /api/spi/devices` — 列出或固定常用节点
  - `POST /api/spi/{bus}.{cs}/transfer` — body：`mode`、`max_speed_hz`、`tx_hex` / `tx_bytes`，响应 `rx_hex`
- **寄存器语义**：在 `transfer` 之上提供可选便捷接口 `read_reg` / `write_reg`（与 I2C 对称），由应用层组帧（不同芯片协议不同，文档需说明「通用主设备读写」边界）。

### 4.5 F-05：UART（拍板建议）

| 方案 | 优点 | 缺点 |
|------|------|------|
| A. 短轮询 `GET /api/uart/rx` | 实现快，仍走现有 `MG_EV_HTTP_MSG` | 延迟高、效率低 |
| B. **WebSocket**（Mongoose 支持） | 双向实时，适合终端体验 | 需在 `main.c` / `router` 增加 WS 握手与 `uart` 读线程或 `select` |
| C. SSE 仅服务端推送 | 发送仍 HTTP | 实现复杂度介于 A/B |

**推荐**：**方案 B（WebSocket）** 作为 UART 正式方案，与 PRD「收发」一致；首版可先做 A 验证硬件，再换 B。

- **串口打开**：`open(/dev/ttyAMA0, O_RDWR | O_NOCTTY)` + `termios` 设置波特率、8N1 等。
- **线程模型**：独立 `pthread` 阻塞读 UART，将数据写入环形缓冲，WS 发送线程或主循环取出（注意锁与 `mg_wakeup` 若需跨线程唤醒 Mongoose，可查 Mongoose 文档）。
- **安全**：同时仅允许一个客户端连接 UART，或最后连接者抢占（需在文档写明）。

### 4.6 系统监控与网络（可选保留）

当前 `dashboard`、网络配置与 PRD「40pin 调试」并存无冲突；发布给客户时可作为增值功能或在构建选项中裁剪（非本阶段必须）。

---

## 5. HTTP API 扩展清单（研发落地表）

在保持现有风格（JSON + `mg_json_get_str` 解析）前提下，建议新增：

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/system/version` | 返回 `version`（git describe）、可选 `build_arch` |
| GET | `/api/hardware/capabilities` | 可选：gpiod/pwm/i2c/spi/uart 可用性 |
| * | `/api/pwm/...` | 见 §4.3 |
| * | `/api/i2c/...` | 见 §4.4 |
| * | `/api/spi/...` | 见 §4.4 |
| GET/POST | `/api/uart/...` | 若首版轮询：open/close/config + rx buffer pull |
| WS | `/api/uart/stream` 或 `/ws/uart` | 终版：二进制或文本帧 |

**错误格式统一**：建议统一为 `{"error":"human_readable","code":"E_I2C_NACK"}`；**前端直接展示英文**，不另做中文映射（与全英文 UI 一致）。

---

## 6. 版本号：`git describe` 注入

在 `CMakeLists.txt` 中：

```cmake
execute_process(
  COMMAND git describe --tags --always --long
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE PIDUIER_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-DPIDUIER_VERSION="${PIDUIER_VERSION}")
```

- C 侧：`router.c` 中 `handle_version` 返回 JSON。
- 前端：`/api/system/version` 拉取后显示在 **About** 页（替换硬编码 `1.0.0`）。

**注意**：无 `.git` 的发布包需 fallback（如 `"unknown"`）。

---

## 7. 构建与部署（两阶段分离，禁止合并为单条命令）

开发与调试流程明确拆成 **阶段 A：构建** 与 **阶段 B：部署**，不得在仓库中提供或使用「一条命令同时完成构建 + 上传 + 启动」的脚本；便于本地反复编译与独立重发制品。

### 7.1 阶段 A — 构建（仅在本机执行）

- **x86（UI/联调）**：`./build.sh` 或 `ARCH=x86 ./build.sh` → 产物在仓库内 `./bin/piduier` 与 `./bin/web/`。无 libgpiod 时 GPIO 等 API 返回错误，仍可用于英文 UI 布局与路由联调。
- **树莓派 5 目标架构（交叉编译）**：在同一台开发机上执行 `ARCH=arm64 ./build.sh`，**仅生成** ARM64 可执行文件与静态资源，**不**在本步骤访问 SSH 或目标机。
- 依赖与编译选项见 `README.md`、`build.sh`、`CMakeLists.txt`。

### 7.2 阶段 B — 部署（仅传输与目标机运行）

- **目标路径（开发调试约定）**：制品部署到目标机用户主目录下 **`~/Desktop/piduier`**（即 `$HOME/Desktop/piduier`）。目录内至少包含：
  - `piduier`（可执行文件）
  - `web/`（与构建产物 `bin/web/` 内容一致）
- **推荐布局示例**：
  - `~/Desktop/piduier/piduier`
  - `~/Desktop/piduier/web/...`
- **上传方式**：提供一个脚本完成部署工作，**仅**将 **阶段 A** 已生成的 `./bin/` 下内容同步到上述路径。示例（在开发机执行，**不要**与 `build.sh` 写在同一条命令里）：

```bash
# 示例：仅部署（构建须在此命令之前已完成）
rsync -avz ./bin/ user@target-host:~/Desktop/piduier/
# 或 scp -r ./bin/piduier ./bin/web user@target-host:~/Desktop/piduier/
```

- **SSH 密码**：可使用密码认证；执行 `scp`/`rsync`/`ssh` 时由工具 **交互提示输入密码**（不把密码写入脚本、仓库或文档）。若已配置 SSH 密钥，则无需每次输入密码。
- **启动**（在目标机 SSH 登录后执行，与构建、上传分离）：

```bash
cd ~/Desktop/piduier
./piduier   # 或按权限需要的前缀；监听端口见 main.c（默认 8000）
```

- **目标机环境**：与 `README.md` 一致——`libgpiod`、启用 I2C/SPI/Serial、`piduier` 组与 udev 等。
- **systemd（可选）**：若需开机自启，单元文件中 `ExecStart` 应指向 **`/home/<user>/Desktop/piduier/piduier`**（或实际绝对路径），与上述调试路径一致；**不与**构建脚本合并。

### 7.3 脚本约定（建议）

- 若增加辅助脚本：命名与职责清晰区分，例如 **`scripts/build.sh`**（或沿用根目录 `build.sh` 仅做构建）与 **`scripts/deploy.sh`**（仅 `rsync`/`scp`，内部**不**调用 `cmake` 或 `build.sh`）。
- `deploy.sh` 可接受 `TARGET=user@host`，其余由操作者交互输入密码。

---

## 8. CMake 与链接库增量

| 模块 | 典型依赖 |
|------|----------|
| GPIO | 已有 `pkg-config libgpiod` |
| I2C/SPI/UART | 标准 libc + Linux 设备节点，**无需**额外第三方库 |
| PWM | sysfs 文件 I/O 或少量 ioctl |

在 `CMakeLists.txt` 的 `SOURCES` 中追加新 `.c`，`target_include_directories` 已含 `src/hardware`。

---

## 9. 前端工作拆分（`web/`）与 **全英文 UI**

**语言规范（强制）**

- 所有**用户可见**文案使用 **English**：`index.html` 标题与标签、`app.js` 中菜单名、按钮、占位符、`confirm`/`alert`、空状态提示、表格列头等。
- **本技术方案文档**可继续使用中文撰写；**产品界面与面向用户的 API 错误字符串**与文档语言脱钩，统一英文。
- Pin 描述等与硬件相关的字符串（如 `3.3V`、`GND`、`GPIO 17`）保持业界通用英文/符号即可。

| 页面/区域 | 工作 |
|-----------|------|
| GPIO / 40pin | 已有结构英文化；接 capabilities API 显示 unavailable 原因（英文） |
| PWM | 新页：chip/channel、frequency、duty、enable switch |
| I2C | Bus、address、register、length、read/write、optional scan list |
| SPI | Device、mode、speed、TX hex、RX display |
| UART | Device、serial params、send/receive log；后期 WebSocket |
| About | `GET /api/system/version`，英文展示 |

保持现有「单页 + `data-page` 切换」结构，减少路由改造成本。

---

## 10. 里程碑与任务拆解（与 PRD M1–M6 对应）

1. **M1**：版本 API + 关于页；GPIO 轮询优化 + 错误提示；40pin 文案与 Pi5 核对。  
2. **M2**：PWM sysfs 模块 + REST + 前端页。  
3. **M3**：I2C 模块 + 读写 + 可选 scan + 前端。  
4. **M4**：SPI `spidev` transfer + 前端。  
5. **M5**：UART termios + 轮询或直连 WebSocket + 终端 UI。  
6. **M6**：更新 `docs/BUILD_AND_DEPLOY.md`：**构建与部署分节、分命令**；可选提供**仅部署**的 `deploy.sh`（交互密码）；验收清单走查。

---

## 11. 测试与验收

- **单元级**：各 hardware 模块在错误路径上返回可预测码（打开不存在的 `/dev/i2c-9` 等）。
- **集成级**：实板用万用表/逻辑分析仪/已知 I2C EEPROM 或传感器 demo 对照 PRD §7.3 检查清单。
- **回归**：每次合并保证 x86 编译通过；ARM64 交叉编译在 CI 或发布前必跑。

---

## 12. 风险与缓解（实现视角）

| 风险 | 缓解 |
|------|------|
| Pi5 引脚复用与内核版本差异 | 文档列出测试内核版本；capabilities 接口暴露实际节点 |
| UART 被 console 占用 | 文档说明关闭串口登录或换用次要 UART |
| Mongoose 单线程与阻塞读 | UART/SPI 短操作可同步；长读用独立线程 + 缓冲 |
| WebSocket 与现有仅 HTTP  handler | 在 `ev_handler` 中分支 `MG_EV_WS` / `MG_EV_HTTP_MSG` |

---

## 13. 文档关系

- **产品范围与验收**：[`PRODUCT_DESIGN_PROPOSAL.md`](./PRODUCT_DESIGN_PROPOSAL.md)  
- **本文件**：研发任务拆分与技术选型  
- **环境与权限**：仓库根目录 [`README.md`](../README.md)  

---

| 版本 | 说明 |
|------|------|
| v0.1 | 初稿：对齐当前 router/gpio/build 现状与 PRD 全量功能 |
| v0.2 | 全英文 UI 约定；构建与部署两阶段分离；目标机调试路径 `~/Desktop/piduier`；部署允许 SSH 交互输入密码 |
