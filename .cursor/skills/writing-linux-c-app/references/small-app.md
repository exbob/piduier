# 小型 Linux C 项目规范

> **前置条件**：用户已在上一级 `SKILL.md` 中明确选择 **「小型」**。三选项对比、面向用户的说明**仅**写在 `SKILL.md`，本文不重复。

本文档与 `references/small-app/` 配套：说明约定与流程；目录内为可编译**模板工程**。新建小型项目可复制该目录为仓库根，改工程名与业务逻辑；**关键文件（`CMakeLists.txt`、`build.sh`、`Config.h.in`、`git-version.sh`、`inc/log.h`、`cmake/toolchain-*.cmake` 等）尽量沿用模板文件的结构**，仅替换程序名与源码。

## 何时使用本文档（技术边界）

- 无磁盘上的应用主配置文件；运行期参数走**命令行**，但允许按业务读写输入/输出数据文件。
- 日志为 **`inc/log.h`** → stdout，不用 zlog 配置文件。
- 源码以 `src/`、`inc/` 为主，无强制的 `src/<模块>/` 划分。

## 模板根路径

相对本 Skill：`references/small-app/`。

## 目录结构

```text
.
├── CMakeLists.txt
├── Config.h.in
├── README.md
├── build.sh
├── deploy.sh
├── git-version.sh
├── cmake/
├── src/
├── inc/
├── build/
└── deploy/
```

说明：

| 路径 | 作用 |
|------|------|
| `CMakeLists.txt` | C11、Debug/Release、`APP_DEBUG`、`-Wall -Wextra`、生成 `Config.h` |
| `Config.h.in` | 注入 `GIT_VERSION`、`BUILD_TIME` 等到 `build/Config.h` |
| `git-version.sh` | 生成版本号，等价 `git describe --tags --always --long` |
| `build.sh` | 构建入口，解析 `ARCH`、调用 CMake、执行构建和清除工作 |
| `cmake/toolchain-*.cmake` | 存放 CMake 的配置文件，例如工具链配置 |
| `inc/` | 存放项目头文件 |
| `src/` | 存放项目源文件 |
| `deploy.sh` | 使用 rsync 将 `deploy/` 同步到远端目标机 |
| `build/` | CMake 构建过程生成的文件都放在这个路径下 |
| `deploy/` | 需要部署到运行环境的文件都安装到这个路径下 |
| `README.md` | 项目说明文件 |

如果有其他文件类型可以新建文件夹：

- `docs/`: 存放文档。
- `thirdpart/`: 存放第三方模块的源码或者库文件。
- `configs/`: 存放其他配置文件或者脚本等。

复制新项目时，尽量使用模板中的原文件，然后按照新项目的情况，做必要修改。

建议按以下分类执行，避免遗漏：

- **可直接复用（通常只改变量）**：`build.sh`、`deploy.sh`、`git-version.sh`、`Config.h.in`、`cmake/toolchain-*.cmake`、`.gitignore`。
- **必须修改（项目身份与业务）**：`CMakeLists.txt`（项目名、目标名、源码路径）、`README.md`（构建/运行/参数说明）、`src/main.c` 与业务源码（程序名、简介、业务逻辑）。
- **按需修改（有明确需求再改）**：`docs/`、`thirdpart/`、`configs/`（仅放辅助脚本或额外数据，不作为主配置文件）。

## 版本控制与 `.gitignore`

- 若项目根**尚无** `.gitignore`：先征得用户同意新建，再**复制**模板根目录下的 `.gitignore` 到项目根。
- 该文件覆盖：`build/`、`deploy/`、常见 CMake 生成物、编辑器临时文件、可选 `.worktrees/` 等。
- 若团队已有全局 `.gitignore` 策略，可合并规则，但须保证 **`build/`、`deploy/` 仍被忽略**。

## CMake 约定

直接使用模板中的 `CMakeLists.txt` 和 `cmake/toolchain-*.cmake` ，然后按照新项目的情况，做必要修改，例如重命名项目名称，可执行目标，源文件和头文件路径等。

**必须**支持构建两种版本，使用 `CMAKE_BUILD_TYPE` 区分：

- Release：`-O2`；
- Debug：`APP_DEBUG`、`-g -O0`；

## 目标架构与工具链选择

通过环境变量 **`ARCH`** 与 **`cmake/toolchain-*.cmake`** 选择工具链；由 `build.sh` 传入 `-DCMAKE_TOOLCHAIN_FILE=...`。模板已经支持两种目标架构：

| `ARCH`（示例值） | 工具链文件（模板内） |
|------------------|----------------------|
| `x86`、`x86_64`、`X86` | `cmake/toolchain-x86_64.cmake` |
| `aarch64`、`arm64` | `cmake/toolchain-aarch64.cmake` |

实施前与用户确认是否有自定义的目标架构与编译环境，**勿在未确认时擅自假定**。

如果要自定义架构时：新增或修改 `cmake/toolchain-<name>.cmake`，并在 `build.sh` 中增加 `ARCH` 分支；与用户确认编译器前缀、`sysroot`、额外 `CFLAGS/LDFLAGS`。

工具链文件与 `build.sh` 内**禁止**写死仅适用于你个人机器的绝对路径；应通过变量或文档说明由用户在环境中提供工具链。

## 根目录 `build.sh`（推荐入口）

直接使用模板中的 `build.sh` 作为构建工作的入口，定义并解析选项参数，调用 cmake 执行构建和清除工作。

| 调用 | 行为 |
|------|------|
| `./build.sh` | **Release** 构建；out-of-source-tree 构建，生成物都在 `./build`，将需部署的文件安装到 `./deploy/` |
| `./build.sh debug` | **Debug** 构建；out-of-source-tree 构建，生成物都在 `./build`，将需部署的文件安装到 `./deploy/` |
| `./build.sh clean` | 清除 `./build/` 与 `./deploy/`（或等价空状态），可重新完整构建 |

每次非 `clean` 构建前会清空 `build/` 与 `deploy/`，属完整重构建。

必须提供 `--help` 选项，打印帮助信息。

示例：

```bash
ARCH=x86_64 ./build.sh
ARCH=aarch64 ./build.sh debug
```

## 版本信息

直接使用模板中的 `git-version.sh` 和 `Config.h.in`，在配置阶段写入 `Config.h`。

## 日志（`log.h`）

直接使用模板中的 `inc/log.h` ；不要为小型项目引入 zlog 或文件日志配置。

## 远程部署（`deploy.sh`）

直接使用模板中的 `deploy.sh` 。需要询问用户，如下三个参数是否使用默认值：

- TARGET_IP
- USERNAME
- TARGET_PATH

## 命令行

- **必须**使用命令行选项传递参数，不使用主配置文件。
- 允许通过命令行传入文件路径，用于读写输入/输出数据（不视为主配置文件）。
- **至少**支持两个选项：`-h`/`--help`（程序名、简介、选项说明）、`-v`/`--version`（程序名、简介、版本、构建时间等）。
- 业务参数用 `getopt_long` 等；非法选项与多余位置参数须明确报错与退出码。

## 复制模板后的最小检查清单

1. 已基于 `references/small-app/` 完成复制，并能成功执行一次构建。
2. 已修改 `CMakeLists.txt`、`README.md`、`src/main.c`（及业务源码）中的项目身份信息。
3. 已确认 `.gitignore`、`ARCH` 与工具链设置符合目标环境。
4. 仍满足小型边界：无主配置文件、参数走命令行、日志为 `inc/log.h`。
5. 若后续需要 JSON 主配置或 zlog，回到 `SKILL.md` 改选「复杂」再继续。

