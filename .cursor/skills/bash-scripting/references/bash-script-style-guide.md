# Bash 脚本编写规范

**版本**: 1.0  
**最后更新**: 2026-04-12

---

## 📖 概述

本规范融合了 Google Shell Style Guide 和 Bash Strict Mode 的最佳实践，旨在帮助开发者编写**健壮、可靠、可维护**的 Bash 脚本。遵循本规范可以：

- 减少 90% 的隐藏 bug
- 提高代码可读性和可维护性
- 在生产环境中避免意外故障
- 降低调试时间

---

## ⚠️ 使用场景限制

### 适合使用 Bash 的场景

推荐使用：
- 小型工具脚本（< 200 行）
- 简单的包装脚本
- 主要调用其他命令行工具
- 系统管理任务
- CI/CD 流程脚本

### 不适合使用 Bash 的场景

应使用其他语言：
- 脚本超过 200 行
- 复杂的数据处理
- 性能敏感的应用
- 需要复杂数据结构
- 复杂的控制流逻辑

替代方案：Python、Go、Rust 等

---

## 1. 文件和权限

- 脚本的文件名应该用小写字母，如果有多个单词，用下划线`_`连接，**禁止**使用空格。
- 脚本文件的后缀必须是 `.sh`
- 脚本文件的权限默认为 `-rwxr-xr-x`
- 脚本内**禁止**使用 SUID 和 SGID ，如果需要更高权限，请使用 `sudo` 。

---

## 2. 文件结构

脚本文件的基本结构：

```bash
#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

#######################################
# 功能描述：（必需）
# 版权信息：（可选）
# 使用方法：
#   ./example.sh [options]
#
# 依赖：
#   - Oracle Client 19c+
#   - rsync 3.0+
#######################################

# 2. 常量定义
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly CONFIG_FILE="/etc/app/config.conf"
readonly MAX_RETRIES=3

# 3. 全局变量（如果需要）
declare -g GLOBAL_VAR=""

# 4. 工具函数
err() {
  echo "[ERROR]: $*" >&2
}

# 5. 业务函数
function process_data() {
  # 函数实现
}

# 6. main 函数（脚本有多个函数时必需）
function main() {
  # 主逻辑
}

# 7. 脚本入口
main "$@"
```

### 2.1 头部规范

所有脚本**必须**以如下内容开头：

```bash
#!/bin/bash
set -euo pipefail
IFS=$'\n\t'
```

解释：

| 设置 | 作用 | 为什么重要 |
|------|------|-----------|
| `set -e` | 任何命令返回非零状态立即退出 | 防止错误被掩盖，类似其他语言的异常处理 |
| `set -u` | 引用未定义变量时报错退出 | 防止拼写错误导致的静默 bug |
| `set -o pipefail` | 管道中任何命令失败都返回失败状态 | 确保管道中的错误不被最后一个命令掩盖 |
| `IFS=$'\n\t'` | 设置字段分隔符为换行和制表符 | 防止空格导致的字符串分割问题 |


### 2.2 注释要求

- 文件头必须有注释，描述脚本的功能和用法。
- 函数必须有注释。
- 关键的常量和全局变量需要注释。
- 其他部分原则上不做注释。

---

## 3. 变量规范

### 3.1 变量命名

```bash
# ✅ 局部变量和函数：小写 + 下划线
local user_name="john"
my_function() { ... }

# ✅ 常量和环境变量：大写 + 下划线
readonly MAX_RETRIES=3
export DATABASE_URL="postgresql://..."

# ✅ 循环变量
for file in "${files[@]}"; do
  echo "${file}"
done
```

### 3.2 变量引用

```bash
# ✅ 正确：始终使用双引号和大括号
echo "${variable}"
echo "${PATH}"
command --arg="${user_input}"

# ✅ 数组引用
echo "${array[@]}"        # 所有元素
echo "${#array[@]}"       # 数组长度
echo "${array[0]}"        # 第一个元素

# ✅ 特殊变量可省略大括号（但建议保留引号）
echo "Args: $1 $2 $3"
echo "Exit code: $?"
echo "PID: $$"

# ❌ 错误：不加引号（会导致词分割和通配符展开）
echo $variable
command --arg=$user_input
```

### 3.3 变量声明

```bash
# ✅ 使用 local 声明函数内变量
my_function() {
  local name="$1"
  local result
}

# ✅ 常量声明
readonly CONFIG_FILE="/etc/myapp/config.conf"
readonly -a ALLOWED_USERS=("alice" "bob" "charlie")

# ✅ 导出环境变量
export DATABASE_URL="postgresql://localhost/mydb"
declare -xr API_KEY="secret123"  # 导出并只读
```

---

## 4. 错误处理

### 4.1 标准错误输出

```bash
# ✅ 定义错误输出函数
err() {
  echo "[ERROR]: $*" >&2
}

info() {
  echo "[INFO]: $*"
}

warn() {
  echo "[WARN]: $*" >&2
}

# 使用
if ! do_something; then
  err "Failed to do_something"
  exit 1
fi
```

### 4.2 返回值检查

```bash
# ✅ 方法1：直接在 if 中检查
if ! command arg1 arg2; then
  err "Command failed"
  exit 1
fi

# ✅ 方法2：使用 || 
command arg1 arg2 || {
  err "Command failed"
  exit 1
}
```

### 4.3 Strict Mode 下的特殊处理

```bash
# ✅ 允许命令失败（临时禁用 -e）
set +e
command_that_may_fail
exit_code=$?
set -e

if [[ ${exit_code} -ne 0 ]]; then
  warn "Command failed but continuing..."
fi

# ✅ 使用 || true 允许失败
rm -f /tmp/file || true

# ✅ 条件检查未定义变量
if [[ -z "${VAR:-}" ]]; then
  err "VAR is not set"
  exit 1
fi

# ✅ 使用默认值
output_dir="${OUTPUT_DIR:-/tmp/output}"
```

---

## 5. 函数规范

### 5.1 函数注释（必需）

```bash
#######################################
# 备份指定数据库到远程服务器
# Globals:
#   BACKUP_DIR - 备份文件存储目录
#   REMOTE_HOST - 远程服务器地址
# Arguments:
#   $1 - 数据库名称
#   $2 - (可选) 备份类型 (full|incremental)，默认 full
# Outputs:
#   备份进度信息到 STDOUT
#   错误信息到 STDERR
# Returns:
#   0 - 备份成功
#   1 - 数据库不存在
#   2 - 远程传输失败
#######################################
function backup_database() {
  local db_name="$1"
  local backup_type="${2:-full}"
  
  # 函数实现...
}
```

### 5.2 函数定义规范

```bash
# ✅ 推荐：使用 function 关键字（提高可读性）
function my_function() {
  local param1="$1"
  local param2="$2"
  
  # 实现...
}

# ✅ 也可接受：省略 function 关键字
my_function() {
  # 实现...
}

# ✅ 命名空间（库函数）
mylib::process_data() {
  # 实现...
}
```

### 5.3 参数处理

```bash
function process_file() {
  # ✅ 检查参数数量
  if [[ $# -lt 1 ]]; then
    err "Usage: process_file <filename> [options]"
    return 1
  fi
  
  # ✅ 使用有意义的变量名
  local input_file="$1"
  local output_file="${2:-${input_file}.out}"
  
  # ✅ 验证参数
  if [[ ! -f "${input_file}" ]]; then
    err "File not found: ${input_file}"
    return 1
  fi
  
  # 处理逻辑...
}
```

---

## 6. 条件测试

### 6.1 使用 [[ ]] 而非 [ ]

```bash
# ✅ 推荐：使用 [[ ]]
if [[ "${string}" == "value" ]]; then
  action
fi

# ✅ 字符串测试
[[ -z "${var}" ]]          # 空字符串
[[ -n "${var}" ]]          # 非空字符串
[[ "${a}" == "${b}" ]]     # 相等（使用 == 而非 =）
[[ "${a}" != "${b}" ]]     # 不相等

# ✅ 文件测试
[[ -f "${file}" ]]         # 是文件
[[ -d "${dir}" ]]          # 是目录
[[ -e "${path}" ]]         # 存在
[[ -r "${file}" ]]         # 可读
[[ -w "${file}" ]]         # 可写
[[ -x "${file}" ]]         # 可执行

# ✅ 正则匹配
if [[ "${email}" =~ ^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$ ]]; then
  echo "Valid email"
fi

# ❌ 避免：使用 [ ] 或 test
if [ "$string" = "value" ]; then  # 不推荐
  action
fi
```

### 6.2 数值比较

```bash
# ✅ 推荐：使用 (( )) 进行数值比较
if (( count > 10 )); then
  action
fi

if (( age >= 18 && age <= 65 )); then
  action
fi

# ✅ 也可使用：-eq, -ne, -lt, -le, -gt, -ge
if [[ "${count}" -gt 10 ]]; then
  action
fi

# ❌ 错误：在 [[ ]] 中使用 > < 会进行字典序比较
if [[ "${count}" > 10 ]]; then  # 错误！这是字典序比较
  action
fi
```

---

## 7. 循环和迭代

### 7.1 遍历数组

```bash
# ✅ 正确：引用数组
declare -a files=("file1.txt" "file 2.txt" "file3.txt")

for file in "${files[@]}"; do
  echo "Processing: ${file}"
done

# ✅ 带索引的遍历
for i in "${!files[@]}"; do
  echo "File $((i+1)): ${files[i]}"
done
```

### 7.2 读取文件

```bash
# ✅ 推荐：使用进程替换
while IFS= read -r line; do
  echo "Line: ${line}"
done < <(command)

# ✅ 读取文件
while IFS= read -r line; do
  process_line "${line}"
done < "${input_file}"

# ✅ 使用 readarray（Bash 4+）
readarray -t lines < <(command)
for line in "${lines[@]}"; do
  echo "${line}"
done

# ❌ 避免：管道到 while（变量在子 shell 中）
count=0
cat file | while read -r line; do
  ((count++))  # count 在子 shell 中，外部不可见
done
echo "${count}"  # 输出 0！
```

### 7.3 数值循环

```bash
# ✅ C 风格循环
for ((i=0; i<10; i++)); do
  echo "Iteration: ${i}"
done

# ✅ 范围循环
for i in {1..10}; do
  echo "Number: ${i}"
done

# ✅ 步长循环
for i in {0..100..10}; do
  echo "Value: ${i}"
done
```

---

## 8. 数组使用

### 8.1 数组声明和操作

```bash
# ✅ 声明数组
declare -a my_array
my_array=("item1" "item2" "item 3")

# ✅ 追加元素
my_array+=("item4" "item5")

# ✅ 访问元素
echo "${my_array[0]}"      # 第一个元素
echo "${my_array[@]}"      # 所有元素
echo "${#my_array[@]}"     # 数组长度

# ✅ 遍历
for item in "${my_array[@]}"; do
  echo "${item}"
done

# ✅ 切片
echo "${my_array[@]:1:3}"  # 从索引1开始，取3个元素
```

### 8.2 构建命令参数

```bash
# ✅ 使用数组构建命令参数
declare -a docker_flags
docker_flags=(
  "--rm"
  "--interactive"
  "--tty"
  "--volume=${PWD}:/workspace"
  "--env=DEBUG=true"
)

docker run "${docker_flags[@]}" ubuntu:latest bash

# ❌ 避免：使用字符串（引号嵌套问题）
flags="--rm --volume=${PWD}:/workspace --env=\"DEBUG=true\""
docker run ${flags} ubuntu:latest  # 错误！
```

---

## 9. 字符串处理

### 9.1 字符串操作

```bash
# ✅ 字符串长度
string="hello"
echo "${#string}"  # 输出 5

# ✅ 子字符串
echo "${string:0:2}"   # "he"
echo "${string:2}"     # "llo"

# ✅ 替换
filename="document.txt"
echo "${filename%.txt}.pdf"        # "document.pdf"
echo "${filename/txt/pdf}"         # "document.pdf"
echo "${filename//o/0}"            # "d0cument.txt"

# ✅ 大小写转换
echo "${string^^}"     # "HELLO"
echo "${string,,}"     # "hello"
echo "${string^}"      # "Hello"

# ✅ 删除前缀/后缀
path="/usr/local/bin/script.sh"
echo "${path##*/}"     # "script.sh"
echo "${path%/*}"      # "/usr/local/bin"
```

### 9.2 默认值和替换

```bash
# ✅ 使用默认值
output_dir="${OUTPUT_DIR:-/tmp/output}"

# ✅ 如果未设置则赋值
: "${CONFIG_FILE:=/etc/app/config.conf}"

# ✅ 如果未设置则报错
required_var="${REQUIRED_VAR:?Error: REQUIRED_VAR is not set}"

# ✅ 如果已设置则使用替代值
debug_flag="${DEBUG:+--verbose}"
```

---

## 10. 格式规范

### 10.1 缩进和空格

```bash
# ✅ 使用 2 个空格缩进（禁止 Tab）
if [[ condition ]]; then
  action1
  if [[ condition2 ]]; then
    action2
  fi
fi

# ✅ 控制流格式
for item in "${items[@]}"; do
  process "${item}"
done

while [[ condition ]]; do
  action
done

# ✅ case 语句
case "${var}" in
  pattern1)
    action1
    ;;
  pattern2)
    action2
    ;;
  *)
    default_action
    ;;
esac
```

### 10.2 行长度

```bash
# ✅ 最大行长 80 字符

# ✅ 长字符串使用 here-document
cat <<EOF
This is a very long string
that spans multiple lines
and is easier to read.
EOF

# ✅ 长命令使用反斜杠续行
docker run \
  --rm \
  --interactive \
  --tty \
  --volume="${PWD}:/workspace" \
  ubuntu:latest \
  bash

# ✅ 管道分行
command1 \
  | command2 \
  | command3 \
  | command4
```

---

## 11. 命令执行

### 11.1 命令替换

```bash
# ✅ 使用 $() 而非反引号
result="$(command arg1 arg2)"
nested="$(outer "$(inner)")"

# ❌ 避免：反引号（难以嵌套）
result=`command arg1 arg2`
```

### 11.2 算术运算

```bash
# ✅ 使用 (( )) 或 $(( ))
((count++))
((sum = a + b))
result=$((10 * 5 + 3))

# ✅ 在 (( )) 中可省略 $
count=5
((count += 10))
echo "$((count * 2))"  # 输出 30

# ❌ 避免：expr, let, $[ ]
result=$(expr 5 + 3)   # 慢，外部命令
let result=5+3         # 不推荐
result=$[5+3]          # 已废弃
```

---

## 12. 最佳实践

### 12.1 避免的陷阱

```bash
# ❌ 避免：eval（安全风险）
eval "$(get_commands)"  # 危险！

# ❌ 避免：通配符扩展问题
rm *  # 如果有 -rf 文件会删除所有内容

# ✅ 使用显式路径
rm ./*
rm -- *  # 使用 -- 分隔选项和参数

# ❌ 避免：cd 不检查
cd /some/dir
rm -rf *  # 如果 cd 失败会删除错误目录

# ✅ 检查 cd 结果
cd /some/dir || exit 1
rm -rf ./*
```

### 12.2 临时文件处理

```bash
# ✅ 使用 mktemp 创建临时文件
temp_file="$(mktemp)"
trap 'rm -f "${temp_file}"' EXIT

# 使用临时文件
echo "data" > "${temp_file}"
process_file "${temp_file}"

# ✅ 临时目录
temp_dir="$(mktemp -d)"
trap 'rm -rf "${temp_dir}"' EXIT
```

### 12.3 信号处理

```bash
# temp_file / temp_dir 应在脚本中由 mktemp / mktemp -d 赋值后再使用
# ✅ 清理函数：仅当变量非空时再删除，避免 rm 作用于空路径
cleanup() {
  local exit_code=$?
  [[ -n "${temp_file:-}" ]] && rm -f -- "${temp_file}"
  [[ -n "${temp_dir:-}" ]] && rm -rf -- "${temp_dir}"
  info "Cleanup completed"
  exit "${exit_code}"
}

trap cleanup EXIT
trap 'err "Script interrupted"; exit 130' INT TERM
```

---

## 13. 快速检查清单

在提交脚本前，请确认：

**基础设置**
- [ ] 包含 `#!/bin/bash`
- [ ] 包含 `set -euo pipefail`
- [ ] 包含 `IFS=$'\n\t'`
- [ ] 包含文件头注释

**变量和引用**
- [ ] 所有变量使用 `"${var}"` 引用
- [ ] 数组使用 `"${array[@]}"`
- [ ] 函数内使用 `local` 声明变量
- [ ] 常量使用 `readonly`

**函数**
- [ ] 所有函数都有注释
- [ ] 函数名使用小写+下划线
- [ ] 检查参数数量和有效性

**错误处理**
- [ ] 检查命令返回值
- [ ] 错误信息输出到 STDERR
- [ ] 使用有意义的退出码

**测试和条件**
- [ ] 使用 `[[ ]]` 而非 `[ ]`
- [ ] 数值比较使用 `(( ))`
- [ ] 字符串比较使用 `==`

**格式**

- [ ] 2 空格缩进
- [ ] 行长 ≤ 80 字符

**安全**
- [ ] 避免使用 `eval`
- [ ] 通配符使用 `./*` 而非 `*`
- [ ] 临时文件使用 `mktemp`
- [ ] 设置 `trap` 清理资源

---

## 14. 参考资源

- [Google Shell Style Guide](https://google.github.io/styleguide/shellguide.html)
- [Unofficial Bash Strict Mode](http://redsymbol.net/articles/unofficial-bash-strict-mode/)
- [Bash Reference Manual](https://www.gnu.org/software/bash/manual/)
- [Advanced Bash-Scripting Guide](https://tldp.org/LDP/abs/html/)

---

**遵循本规范，让您的 Bash 脚本更加健壮可靠！** 