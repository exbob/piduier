# linux c complex app demo

一个 Linux C11 示例工程，使用 CMake 构建，提供：

- JSON 主配置加载（间隔秒数 + zlog 配置路径）
- 独立 zlog `.ini` 配置（完全 zlog 原生格式）
- 定时日志输出与 `Ctrl+C` 退出
- `--help` / `--version` / `-f` 命令行参数

## 快速开始

```bash
# Debug 构建并安装到 ./deploy
ARCH=x86_64 ./build.sh debug

# 进入 deploy 目录
cd ./deploy

# 运行（默认读取 ../app.json）
./complex-app

# 显式指定配置文件
./complex-app -f ./app.json

# 查看帮助与版本
./complex-app --help
./complex-app --version
```

## 项目结构

```text
.
├── CMakeLists.txt
├── Config.h.in
├── README.md
├── build.sh
├── deploy.sh
├── git-version.sh
├── cmake/
├── configs/
│   ├── app.debug.json
│   ├── app.release.json
│   ├── zlog.debug.ini
│   └── zlog.release.ini
├── src/
│   ├── main.c
│   ├── app/
│   ├── config/
│   └── log/
├── thirdpart/
├── build/
└── deploy/
```

| 内容 | 路径 |
|------|------|
| 源码目录 | `./src/` |
| 构建输出 | `./build/` |
| 安装输出 | `./deploy/`（可执行文件：`./deploy/complex-app`） |
| 配置模板 | `./configs/` |
| 第三方源码（FetchContent） | `./thirdpart/` |

## 环境依赖

- CMake >= 3.15
- C11 编译器
- pthread
- Bash（用于 `build.sh` / `git-version.sh`）

第三方依赖由 CMake 自动拉取：

- cJSON `1.7.18`
- zlog `1.2.18`

首次构建需要联网。

## 构建与安装

`build.sh` 会根据 `ARCH` 选择工具链（默认 `x86`）：

| `ARCH`（示例） | 工具链 |
|----------------|--------|
| `x86`、`x86_64`、`X86` | `cmake/toolchain-x86_64.cmake` |
| `aarch64`、`arm64`、`ARM64` | `cmake/toolchain-aarch64.cmake` |

| 命令 | 行为 |
|------|------|
| `./build.sh` | Release 构建并安装到 `./deploy/` |
| `./build.sh debug` | Debug 构建并安装到 `./deploy/` |
| `./build.sh clean` | 清空并重建 `./build/`、`./deploy/` |
| `./build.sh --help` | 打印脚本帮助 |

说明：构建前会清空 `./build/` 与 `./deploy/`。

## 程序命令行

| 选项 | 行为 |
|------|------|
| `-f <path>` / `--config <path>` | 指定 JSON 主配置文件 |
| `-h` / `--help` | 打印帮助 |
| `-v` / `--version` | 打印版本与构建时间 |

默认配置文件路径是当前目录 `app.json`。

## 配置文件说明

### 1) 主配置（JSON）

主配置只包含两个选项：

```json
{
	"interval_seconds": 5,
	"zlog_config_path": "./zlog.ini"
}
```

- `interval_seconds`：循环间隔（秒），必须为正整数
- `zlog_config_path`：zlog ini 配置文件路径（非空字符串）

`config` 模块还包含 1MB 文件大小上限，超过上限会返回错误。

### 2) zlog 配置（INI）

`zlog.debug.ini`（Debug 默认）：

- 输出到 `stdout`
- 格式包含文件与行号

`zlog.release.ini`（Release 默认）：

- 输出到 `./complex-app.log`
- 按规则滚动
- 使用单条 `app.*` 规则，避免重复匹配写入

## 运行行为

- 程序启动后按 `interval_seconds` 定时打印日志
- 按 `Ctrl+C` 触发退出
- 退出前执行日志系统收尾

## 版本与构建信息

- 版本字符串来自 `git-version.sh`（`git describe --tags --always --long`）
- 失败时回退为 `unknown`
- CMake 在配置阶段生成 `Config.h`，注入 `VERSION` 与 `BUILD_TIME`
