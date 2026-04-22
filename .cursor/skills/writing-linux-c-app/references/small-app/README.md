# linux c small app demo

一个 Linux C11 示例工程，使用 CMake 构建，提供：

- 简单日志宏（ERR/WARN/INFO/DEBUG）
- Git 版本与构建时间注入
- 基于 `getopt_long` 的命令行（`--help` / `--version`）

## 快速开始

```bash
ARCH=x86_64 ./build.sh
./deploy/small-app --help
./deploy/small-app --version
```

## 项目结构

```text
.
├── CMakeLists.txt
├── Config.h.in          # 生成 Config.h 的模板（版本、构建时间）
├── README.md
├── build.sh             # 推荐构建入口
├── deploy.sh            # 远程同步 deploy/ 到目标机（rsync over SSH）
├── git-version.sh       # 输出 git describe 版本字符串
├── cmake/               # 工具链文件（x86_64 / aarch64）
├── build/               # CMake 生成物（勿提交）
├── deploy/              # 安装输出目录（勿提交）
├── src/                 # C 源文件
├── inc/                 # 头文件（如 log.h）
├── thirdpart/           # 预留：第三方源码或二进制
└── configs/             # 预留：配置文件
```

| 内容 | 路径 |
|------|------|
| 头文件 | `./inc/` |
| 源文件 | `./src/` |
| 生成头文件 | `./build/Config.h` |
| CMake 输出目录 | `./build/` |
| 安装/部署目录 | `./deploy/` |
| 远程部署脚本 | `./deploy.sh` |
| 第三方预留目录 | `./thirdpart/` |
| 配置预留目录 | `./configs/` |

## 环境依赖

- CMake >= 3.15
- Bash（用于 `build.sh`、`git-version.sh`）
- 本机 x86_64 构建：系统需有 `gcc` 等工具链
- aarch64 交叉编译：需有交叉编译器（如 `aarch64-linux-gnu-gcc`）

远程部署额外依赖（`deploy.sh`）：

- `rsync`
- `sshpass`
- 远端机器可 SSH 登录

## 构建与安装（`build.sh`）

`build.sh` 会根据环境变量 `ARCH` 选择工具链（默认 `x86`，等同 `x86_64`）：

| `ARCH`（示例） | 工具链文件 |
|----------------|------------|
| `x86`、`x86_64`、`X86` | `cmake/toolchain-x86_64.cmake` |
| `aarch64`、`arm64` | `cmake/toolchain-aarch64.cmake` |

| 调用方式 | 行为 |
|----------|------|
| `./build.sh` | Release 构建并安装到 `./deploy/` |
| `./build.sh debug` | Debug 构建并安装到 `./deploy/` |
| `./build.sh clean` | 清空并重建 `./build/` 与 `./deploy/` |
| `./build.sh --help`（或 `help`） | 打印脚本帮助 |

说明：每次执行构建（非 `clean`）前，会清空 `./build/` 与 `./deploy/`，属于完整重构建。

常用示例：

```bash
# Debug 构建
ARCH=x86_64 ./build.sh debug
./deploy/small-app

# aarch64 交叉编译
ARCH=aarch64 ./build.sh
```

## 远程部署（`deploy.sh`）

`deploy.sh` 会将本地 `./deploy/` 同步到远端目录（rsync over SSH）。

默认远端参数（脚本内置，不交互询问）：

- 目标 IP：`172.16.1.101`
- 用户名：`lsc`
- 目标路径：`~/deploy`

运行方式（仅询问密码，隐藏输入）：

```bash
./deploy.sh
```

同步行为：

- 等价参数：`rsync -az --delete --progress`
- 源目录：`./deploy/`
- 目标目录：`lsc@172.16.1.101:~/deploy/`
- 启用 `--delete`，会删除远端中本地已不存在的文件

## 程序命令行

程序使用 `getopt_long` 解析参数，无配置文件。

| 选项 | 行为 |
|------|------|
| `-h` / `--help` | 打印用法与选项说明 |
| `-v` / `--version` | 打印程序名、版本号、构建时间（UTC） |

- 非法选项：打印帮助并以非零退出
- 多余位置参数：报错并打印帮助
- 无参数：正常运行并打印若干 INFO 日志

## 版本与构建信息

- 版本字符串来自 `git-version.sh`，等价于 `git describe --tags --always --long`
- 获取失败时版本为 `unknown`
- CMake 在配置阶段把 `GIT_VERSION` 与 `BUILD_TIME` 写入 `build/Config.h`

## 日志说明（`inc/log.h`）

- 日志级别：ERR、WARN、INFO、DEBUG（输出到 stdout）
- Debug 构建（定义 `APP_DEBUG`）下，日志包含 `__FILE__`/`__LINE__`
- Release 下日志为简短格式，`LOG_DEBUG` 为空操作

## CMake 配置要点

- 最低版本：`cmake_minimum_required(VERSION 3.15)`
- 语言标准：C11（`CMAKE_C_EXTENSIONS OFF`）
- 工具链通过 `-DCMAKE_TOOLCHAIN_FILE=...` 指定
- 构建类型仅支持 Debug/Release（单配置生成器默认 Release）
- 编译选项：`-Wall`、`-Wextra`；Debug：`-g -O0`；Release：`-O2`
