# 硬件通信权限管理实现方案

## 需求回顾

项目需要访问以下硬件接口：
- **GPIO**: 通过 `libgpiod` 访问 `/dev/gpiochip*`
- **PWM**: 通过 `/sys/class/pwm/` 访问
- **SPI**: 访问 `/dev/spidev0.0` 和 `/dev/spidev0.1`
- **I2C**: 访问 `/dev/i2c-1`
- **UART**: 访问 `/dev/ttyAMA0`

## 权限方案

### 推荐方案：使用 `piduier` 用户组（简化方案）

为了简化权限管理，统一使用 `piduier` 用户组访问所有硬件接口。通过 udev 规则配置，让 `piduier` 组拥有所有硬件设备的访问权限。

#### 优点
1. **简单统一**
   - 只需一个用户组，配置简单
   - 无需管理多个组

2. **安全性高**
   - 遵循最小权限原则
   - 只授予必要的硬件访问权限
   - 降低安全风险

3. **符合最佳实践**
   - 生产环境推荐方式
   - 符合 Linux 权限管理规范

#### 配置步骤

1. **创建 `piduier` 组（如果不存在）**
   ```bash
   sudo groupadd piduier
   ```

2. **将用户添加到 `piduier` 组**
   ```bash
   sudo usermod -aG piduier $USER
   ```

3. **配置 udev 规则**（统一使用 `piduier` 组）
   ```bash
   sudo tee /etc/udev/rules.d/99-piduier.rules > /dev/null <<EOF
   # GPIO
   SUBSYSTEM=="gpio", GROUP="piduier", MODE="0664"
   KERNEL=="gpiochip*", GROUP="piduier", MODE="0664"
   
   # PWM
   SUBSYSTEM=="pwm", GROUP="piduier", MODE="0664"
   KERNEL=="pwmchip*", GROUP="piduier", MODE="0664"
   
   # SPI
   KERNEL=="spidev*", GROUP="piduier", MODE="0664"
   
   # I2C
   KERNEL=="i2c-[0-9]*", GROUP="piduier", MODE="0664"
   
   # UART
   KERNEL=="ttyAMA*", GROUP="piduier", MODE="0664"
   EOF
   
   # 重新加载 udev 规则
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```

4. **重新登录**以使组权限生效

#### 验证权限

重新登录后，检查权限：
```bash
# 检查用户组
groups

# 检查设备权限
ls -l /dev/gpiochip0
ls -l /dev/spidev0.0
ls -l /dev/i2c-1
ls -l /dev/ttyAMA0
```

### 备选方案：使用 root 权限（仅用于开发测试）

开发阶段可以使用 root 权限快速测试：
```bash
sudo ./bin/server
```

**注意**：生产环境不推荐使用 root 权限运行。

## 各硬件接口的权限配置

### 1. GPIO (libgpiod)

#### 设备文件
- `/dev/gpiochip0`, `/dev/gpiochip1`, 等

#### 权限配置

**方式 1：使用 `gpio` 组（推荐）**

```bash
# 查看 gpio 组是否存在
getent group gpio

# 如果不存在，创建 gpio 组
sudo groupadd gpio

# 将用户添加到 gpio 组
sudo usermod -aG gpio $USER

# 配置 udev 规则，让 gpio 组有访问权限
sudo tee /etc/udev/rules.d/99-gpio.rules > /dev/null <<EOF
SUBSYSTEM=="gpio", GROUP="gpio", MODE="0664"
KERNEL=="gpiochip*", GROUP="gpio", MODE="0664"
EOF

# 重新加载 udev 规则
sudo udevadm control --reload-rules
sudo udevadm trigger
```

**方式 2：使用 `dialout` 组（某些系统）**

某些 Linux 发行版可能使用 `dialout` 组：
```bash
sudo usermod -aG dialout $USER
```

**验证权限**
```bash
# 重新登录后，检查用户组
groups

# 测试访问
ls -l /dev/gpiochip0
```

### 2. PWM (/sys/class/pwm/)

#### 设备路径
- `/sys/class/pwm/pwmchip0/`

#### 权限配置

**使用 udev 规则**

```bash
# 创建 udev 规则
sudo tee /etc/udev/rules.d/99-pwm.rules > /dev/null <<EOF
SUBSYSTEM=="pwm", GROUP="gpio", MODE="0664"
KERNEL=="pwmchip*", GROUP="gpio", MODE="0664"
EOF

# 重新加载 udev 规则
sudo udevadm control --reload-rules
sudo udevadm trigger
```

**注意**：PWM 通常与 GPIO 使用相同的组（`gpio`），因为它们都是 GPIO 相关的功能。

### 3. SPI (/dev/spidev*)

#### 设备文件
- `/dev/spidev0.0`
- `/dev/spidev0.1`

#### 权限配置

**方式 1：使用 `spi` 组（推荐）**

```bash
# 创建 spi 组（如果不存在）
sudo groupadd spi

# 将用户添加到 spi 组
sudo usermod -aG spi $USER

# 配置 udev 规则
sudo tee /etc/udev/rules.d/99-spi.rules > /dev/null <<EOF
KERNEL=="spidev*", GROUP="spi", MODE="0664"
EOF

# 重新加载 udev 规则
sudo udevadm control --reload-rules
sudo udevadm trigger
```

**方式 2：使用 `dialout` 组**

某些系统可能使用 `dialout` 组：
```bash
sudo usermod -aG dialout $USER
```

**验证权限**
```bash
# 重新登录后
ls -l /dev/spidev0.0
# 应该显示类似：crw-rw-r-- 1 root spi 153, 0 ...
```

### 4. I2C (/dev/i2c-*)

#### 设备文件
- `/dev/i2c-1`

#### 权限配置

**使用 `i2c` 组（标准方式）**

```bash
# 创建 i2c 组（如果不存在）
sudo groupadd i2c

# 将用户添加到 i2c 组
sudo usermod -aG i2c $USER

# 配置 udev 规则
sudo tee /etc/udev/rules.d/99-i2c.rules > /dev/null <<EOF
KERNEL=="i2c-[0-9]*", GROUP="i2c", MODE="0664"
EOF

# 重新加载 udev 规则
sudo udevadm control --reload-rules
sudo udevadm trigger
```

**注意**：树莓派 OS 通常已经配置了 I2C 权限，可能只需要将用户添加到 `i2c` 组即可。

**验证权限**
```bash
# 重新登录后
ls -l /dev/i2c-1
# 应该显示类似：crw-rw-r-- 1 root i2c 89, 1 ...
```

### 5. UART (/dev/ttyAMA0)

#### 设备文件
- `/dev/ttyAMA0`

#### 权限配置

**使用 `dialout` 组（标准方式）**

```bash
# 将用户添加到 dialout 组
sudo usermod -aG dialout $USER

# 配置 udev 规则（如果需要）
sudo tee /etc/udev/rules.d/99-uart.rules > /dev/null <<EOF
KERNEL=="ttyAMA*", GROUP="dialout", MODE="0664"
EOF

# 重新加载 udev 规则
sudo udevadm control --reload-rules
sudo udevadm trigger
```

**验证权限**
```bash
# 重新登录后
ls -l /dev/ttyAMA0
# 应该显示类似：crw-rw-r-- 1 root dialout 204, 64 ...
```

## 权限配置总结

统一使用 `piduier` 用户组访问所有硬件接口：

| 硬件接口 | 设备文件 | 用户组 | udev 规则 |
|---------|---------|--------|----------|
| GPIO | `/dev/gpiochip*` | `piduier` | `KERNEL=="gpiochip*", GROUP="piduier"` |
| PWM | `/sys/class/pwm/pwmchip*` | `piduier` | `KERNEL=="pwmchip*", GROUP="piduier"` |
| SPI | `/dev/spidev*` | `piduier` | `KERNEL=="spidev*", GROUP="piduier"` |
| I2C | `/dev/i2c-*` | `piduier` | `KERNEL=="i2c-[0-9]*", GROUP="piduier"` |
| UART | `/dev/ttyAMA*` | `piduier` | `KERNEL=="ttyAMA*", GROUP="piduier"` |

## 注意事项

1. **重新登录**
   - 添加用户到组后，需要重新登录才能生效
   - 可以使用 `newgrp gpio` 命令临时切换组（不推荐）

2. **树莓派 OS 特殊配置**
   - 某些树莓派 OS 版本可能已经配置了部分权限
   - 建议先检查现有权限，再决定是否需要配置

3. **权限调试**
   - 如果遇到权限问题，检查：
     - 用户是否在 `gpio` 组中：`groups`
     - 设备文件权限：`ls -l /dev/...`
     - udev 规则是否生效：`udevadm info /dev/...`

4. **网络配置权限**
   - NetworkManager 操作需要 root 权限或通过 `nmcli` 的权限机制
   - 可能需要额外的权限配置（如 PolicyKit）

## 注意事项

1. **重新登录**
   - 添加用户到组后，需要重新登录才能生效
   - 可以使用 `newgrp` 命令临时切换组（不推荐）

2. **树莓派 OS 特殊配置**
   - 某些树莓派 OS 版本可能已经配置了部分权限
   - 建议先检查现有权限，再决定是否需要配置

3. **权限调试**
   - 如果遇到权限问题，检查：
     - 用户是否在 `piduier` 组中：`groups`
     - 设备文件权限：`ls -l /dev/...`
     - udev 规则是否生效：`udevadm info /dev/...`

4. **网络配置权限**
   - NetworkManager 操作需要 root 权限或通过 `nmcli` 的权限机制
   - 可能需要额外的权限配置（如 PolicyKit）

## 总结

**推荐使用用户组权限方案**，因为：
1. ✅ 安全性高，遵循最小权限原则
2. ✅ 符合 Linux 最佳实践
3. ✅ 便于权限管理和审计
4. ✅ 生产环境推荐方式

**实现要点**：
- 统一使用 `piduier` 用户组
- 程序启动时检查权限并给出提示
- 开发阶段可以使用 root 模式，生产环境必须使用组权限
- 在 README 中详细说明权限配置步骤
