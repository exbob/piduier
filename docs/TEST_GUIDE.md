# 测试指南：验证系统信息获取

## 测试步骤

### 1. 在树莓派上运行测试脚本

将 `test_commands.sh` 复制到树莓派，然后运行：

```bash
./test_commands.sh > test_output.txt 2>&1
```

或者直接运行查看输出：

```bash
./test_commands.sh
```

### 2. 手动测试各个命令

#### 测试 hostnamectl（硬件信息）
```bash
hostnamectl status
```
**关注点**：
- 查找 "Hardware Vendor:" 和 "Hardware Model:" 行
- 检查这些行的实际格式（空格数量、冒号位置等）

#### 测试 timedatectl（时区和时间）
```bash
timedatectl status
```
**关注点**：
- 查找 "Time zone:" 行的实际格式
- 查找 "Local time:" 行的实际格式
- 检查日期时间的格式（是否包含星期几等）

#### 测试网络信息（以太网速度）
```bash
nmcli device show eth0 | grep -i speed
nmcli device show eth0 | grep GENERAL
```
**关注点**：
- 查找速度相关的字段名（可能是 "GENERAL.SPEED" 或其他）
- 检查速度值的格式

#### 测试 Wi-Fi 信息（SSID 和信号强度）
```bash
nmcli device show wlan0 | grep -E "WIFI|SSID|SIGNAL"
nmcli device wifi list
```
**关注点**：
- 查找 SSID 和信号强度在 `nmcli device show` 中的字段名
- 检查 `nmcli device wifi list` 的输出格式

### 3. 测试 API 端点

在树莓派上运行程序后，测试各个 API：

```bash
# 测试系统信息
curl http://localhost:8000/api/system/info

# 测试日期时间信息
curl http://localhost:8000/api/system/datetime

# 测试完整系统信息
curl http://localhost:8000/api/system/info/complete

# 测试网络设备
curl http://localhost:8000/api/network/devices
```

### 4. 收集测试结果

请将以下信息反馈给我：

1. **hostnamectl status 的完整输出**（特别是 Hardware Vendor 和 Hardware Model 行）
2. **timedatectl status 的完整输出**（特别是 Time zone 和 Local time 行）
3. **nmcli device show eth0 的输出**（查找速度相关字段）
4. **nmcli device show wlan0 的输出**（查找 Wi-Fi 相关字段）
5. **nmcli device wifi list 的前几行输出**（查看格式）

## 常见问题

### 问题1：硬件厂商和型号显示为 N/A

**可能原因**：
- 树莓派上 `hostnamectl` 可能不显示这些字段
- 字段名格式不同（空格数量不同）

**解决方法**：
- 检查实际输出格式
- 可能需要从 `/proc/cpuinfo` 或其他文件读取

### 问题2：时区和日期时间显示为 N/A

**可能原因**：
- `timedatectl` 输出格式不同
- 字符串匹配的前导空格数量不匹配

**解决方法**：
- 使用更灵活的字符串匹配（如 `strstr` 而不是 `strncmp`）
- 或者使用正则表达式解析

### 问题3：以太网速度显示为 -

**可能原因**：
- 字段名不是 "GENERAL.SPEED"
- 需要从其他位置获取（如 `/sys/class/net/eth0/speed`）

**解决方法**：
- 检查 `nmcli device show` 的实际输出
- 或者直接从 sysfs 读取

### 问题4：Wi-Fi SSID 和信号强度显示为 -

**可能原因**：
- `nmcli device wifi list` 的输出格式不同
- SSID 和信号强度在 `nmcli device show` 中有不同的字段名

**解决方法**：
- 检查 `nmcli device show wlan0` 中是否有 WIFI.SSID 或类似字段
- 检查信号强度字段名

## 下一步

根据测试结果，我会修改代码以适配实际的输出格式。
