# 树莓派5 通过 sysfs 操作 PWM

## 1. 树莓派5 PWM 硬件说明

树莓派5 使用 **RP1 I/O 控制器**，PWM 通常位于 `pwmchip0`。

不同系统版本下 `pwmchip` 编号可能变化，实际操作前请先通过 `ls /sys/class/pwm/` 确认。

硬件 PWM 引脚映射:

| GPIO编号 | 物理引脚 | pwmchip | channel |
|----|---------|---------|---------|
| 12 | 32      | pwmchip0 | 0 |
| 13 | 33      | pwmchip0 | 1 |
| 18 | 12      | pwmchip0 | 2 |
| 19 | 35      | pwmchip0 | 3 |

## 2. 设备树和引脚使能

树莓派5的 `/boot/firmware/overlays/` 目录下有很多设备树补丁，在说明文档中可以查看完整的 overlays 说明。

查询 pwm 相关补丁：

```bash
cat /boot/firmware/overlays/README | grep -A 20 "Name:   pwm"
```

或者用 `dtoverlay` 命令查看详细特定补丁的帮助信息：

```bash
lsc@raspberrypi:~ $ dtoverlay -h
Usage:
  dtoverlay <overlay> [<param>=<val>...]
                           Add an overlay (with parameters)
  dtoverlay -D             Dry-run (prepare overlay, but don't apply -
                           save it as dry-run.dtbo)
  dtoverlay -r [<overlay>] Remove an overlay (by name, index or the last)
  dtoverlay -R [<overlay>] Remove from an overlay (by name, index or all)
  dtoverlay -l             List active overlays/params
  dtoverlay -a             List all overlays (marking the active)
  dtoverlay -h             Show this usage message
  dtoverlay -h <overlay>   Display help on an overlay
  dtoverlay -h <overlay> <param>..  Or its parameters
    where <overlay> is the name of an overlay or 'dtparam' for dtparams
Options applicable to most variants:
    -d <dir>        Specify an alternate location for the overlays
                    (defaults to /boot/overlays or /flash/overlays)
    -p <string>     Force a compatible string for the platform
    -v              Verbose operation

Adding or removing overlays and parameters requires root privileges.
```

### 2.1 使能单个 PWM 通道

设备树补丁`/boot/firmware/overlays/pwm.dtbo`可以使能单个PWM通道。

```bash
lsc@raspberrypi:~ $ dtoverlay -h pwm
Name:   pwm

Info:   Configures a single PWM channel
        Legal pin,function combinations for each channel:
          PWM0: 12,4(Alt0) 18,2(Alt5) 40,4(Alt0)            52,5(Alt1)
          PWM1: 13,4(Alt0) 19,2(Alt5) 41,4(Alt0) 45,4(Alt0) 53,5(Alt1)
        N.B.:
          1) Pin 18 is the only one available on all platforms, and
             it is the one used by the I2S audio interface.
             Pins 12 and 13 might be better choices on an A+, B+ or Pi2.
          2) The onboard analogue audio output uses both PWM channels.
          3) So be careful mixing audio and PWM.
          4) Currently the clock must have been enabled and configured
             by other means.

Usage:  dtoverlay=pwm,<param>=<val>

Params: pin                     Output pin (default 18) - see table
        func                    Pin function (default 2 = Alt5) - see above
        clock                   PWM clock frequency (informational)

```

支持以下参数：

| 参数    | 说明                 | 示例值             |
| ------- | -------------------- | ------------------ |
| `pin`   | GPIO 引脚号          | 12, 13, 18, 19     |
| `func`  | 引脚功能（ALT 模式） | 与 `pin` 对应的合法组合（如 GPIO12 对应 `func=4`） |
| `clock` | PWM 时钟频率         | 100000000 (100MHz) |

#### 运行时加载（临时测试）

用 `dtoverlay`动态加载设备树补丁。例如，设置GPIO12为PWM功能：

```bash
# 带参数加载
~ $ sudo dtoverlay pwm,pin=12,func=4

# 查看已加载的 overlay
~ $ dtoverlay -l
Overlays (in load order):
0:  pwm  pin=12 func=4

# 移除 overlay
sudo dtoverlay -r pwm
```

**注意**：运行时加载在重启后失效。

#### 在 `config.txt` 中启用（推荐）

编辑配置文件：

```bash
sudo vi /boot/firmware/config.txt
```

添加以下内容：

```ini
# 或带参数启用
dtoverlay=pwm,pin=12,func=4
```

保存后重启：

```bash
sudo reboot
```

### 2.2 使能两个 PWM 通道

设备树补丁`/boot/firmware/overlays/pwm-2chan.dtbo`可以同时使能两个PWM通道。

```bash
~ $ cat /boot/firmware/overlays/README | grep -A 20 "Name:   pwm-2chan"
Name:   pwm-2chan
Info:   Configures both PWM channels
        Legal pin,function combinations for each channel:
          PWM0: 12,4(Alt0) 18,2(Alt5) 40,4(Alt0)            52,5(Alt1)
          PWM1: 13,4(Alt0) 19,2(Alt5) 41,4(Alt0) 45,4(Alt0) 53,5(Alt1)
        N.B.:
          1) Pin 18 is the only one available on all platforms, and
             it is the one used by the I2S audio interface.
             Pins 12 and 13 might be better choices on an A+, B+ or Pi2.
          2) The onboard analogue audio output uses both PWM channels.
          3) So be careful mixing audio and PWM.
          4) Currently the clock must have been enabled and configured
             by other means.
Load:   dtoverlay=pwm-2chan,<param>=<val>
Params: pin                     Output pin (default 18) - see table
        pin2                    Output pin for other channel (default 19)
        func                    Pin function (default 2 = Alt5) - see above
        func2                   Function for pin2 (default 2 = Alt5)
        clock                   PWM clock frequency (informational)
```

#### 运行时加载（临时测试）

用 `dtoverlay`动态加载设备树补丁。例如，设置GPIO12和GPIO13为PWM功能：

```bash
# 带参数加载
~ $ sudo dtoverlay pwm-2chan,pin=12,func=4,pin2=13,func2=4

# 查看已加载的 overlay
~ $ dtoverlay -l
Overlays (in load order):
0:  pwm-2chan  pin=12 func=4 pin2=13 func2=4

# 移除 overlay
sudo dtoverlay -r pwm-2chan
```

**注意**：运行时加载在重启后失效。

#### 在 `config.txt` 中启用（推荐）

编辑配置文件：

```bash
sudo vi /boot/firmware/config.txt
```

添加以下内容：

```ini
# 或带参数启用
dtoverlay=pwm-2chan,pin=12,func=4,pin2=13,func2=4
```

保存后重启：

```bash
sudo reboot
```

### 3. 检查

使能PWM通道后，可以检测设备树节点的状态：

```bash
# 设备树节点状态
~ $ cat /proc/device-tree/axi/pcie@1000120000/rp1/pwm@98000/status
okay

# 查看 PWM 节点
~ $ ls /sys/class/pwm/pwmchip0/
device  export  npwm  power  subsystem  uevent  unexport
```

确保GPIO12引脚是PWM功能：

``` 
~ $ pinctrl get 12
12: a0    pd | lo // GPIO12 = PWM0_CHAN0
~ $ pinctrl funcs 12
12, GPIO12, PWM0_CHAN0, DPI_D8, TXD4, SDA2, AAUD_LEFT, SYS_RIO012, PROC_RIO012, PIO12, SPI5_CE0

# 双通道场景可继续检查 GPIO13
~ $ pinctrl get 13
13: a0    pd | lo // GPIO13 = PWM0_CHAN1
```

## 4. 通过 sysfs 接口控制 PWM

以 GPIO12 引脚为例。

```bash
# 导出 PWM0_CHAN0
~ $ echo 0 | sudo tee /sys/class/pwm/pwmchip0/export
0
~ $ ls /sys/class/pwm/pwmchip0/
device  export  npwm  power  pwm0  subsystem  uevent  unexport
~ $ cat /sys/class/pwm/pwmchip0/pwm0/enable
0

# 设置周期（纳秒，100ms = 10Hz）
~ $ echo 100000000  | sudo tee /sys/class/pwm/pwmchip0/pwm0/period
100000000

# 设置占空比（纳秒，50ms = 50%）
# 注意：duty_cycle 必须小于等于 period，否则会报 Invalid argument
~ $ echo 50000000 | sudo tee /sys/class/pwm/pwmchip0/pwm0/duty_cycle
50000000

# 启用 PWM
~ $ echo 1 | sudo tee /sys/class/pwm/pwmchip0/pwm0/enable
1

# 禁用 PWM
~ $ echo 0 | sudo tee /sys/class/pwm/pwmchip0/pwm0/enable

# 回收 PWM 通道
~ $ echo 0 | sudo tee /sys/class/pwm/pwmchip0/unexport
```

