# 树莓派5 使用 pinctrl 配置 GPIO 引脚

## 1. pinctrl 简介

`pinctrl` 是树莓派5引入的新工具，用于替代旧的 `raspi-gpio` 命令，提供更强大的引脚配置功能。它直接与 RP1 I/O 控制器交互。

## 2. pinctrl 语法

```bash
pinctrl <command> [args...]
```

常用命令：

- `get` ：查看引脚状态
- `set` ：设置引脚功能
- `funcs` ：列出引脚可用功能
- `help` ：显示帮助

常见用法：

```bash
pinctrl get [GPIO|GPIO范围]
pinctrl funcs <GPIO>
pinctrl set <GPIO> <功能/特性...>
```

`[GPIO]` 可以选择单个引脚，也可以选择引脚范围，例如：

- `pinctrl get 1` 表示查看 GPIO1 引脚的状态。
- `pinctrl get 1-10` 表示查看 GPIO1~GPIO10 引脚的状态。
- `pinctrl get` 表示查看所有引脚的状态。

## 3. 列出引脚可用功能

```bash
~ $ pinctrl funcs 12
12, GPIO12, PWM0_CHAN0, DPI_D8, TXD4, SDA2, AAUD_LEFT, SYS_RIO012, PROC_RIO012, PIO12, SPI5_CE0
```

返回的结果用逗号分为11个字段：
- `12` 是GPIO编号
- `GPIO12` 是引脚的默认名称
- 后面9个字段是该引脚可以配置的9种功能名称，a0~a8

## 4. 查看引脚状态

查看 GPIO2 的引脚状态：

```bash
~ $ pinctrl get 2
 2: a3    pu | hi // GPIO2 = SDA1
```

说明：
- `2`表示GPIO的编号，这里表示GPIO2
- `a3    pu`表示该引脚配置的功能和特性，可能的值包括：
	- `ip` ：GPIO输入
	- `op` ：GPIO输出
	- `a0-a8` ：映射为其他功能（a0~a8），GPIO0-GPIO27引脚可以映射的所有功能在 `## 树莓派5 引脚功能映射（GPIO0-GPIO27）`查看。
	- `no` ：没有设置功能
	- `pu` ：使能内部上拉
	- `pd` ：使能内部下拉
	- `pn` ：禁用内部上下拉
	- `dh` ：设为高电平 (设为`op`时有效)
	- `dl` ：设为低电平 (设为`op`时有效)
- `hi`表示引脚的实际电平状态：
	- `hi`：高电平（1）
	- `lo`：低电平（0）
- `// GPIO2 = SDA1`是注释，描述了该引脚的功能。

## 5. 设置引脚功能

引脚可以设置的功能和特性分为三类：

- 功能
	- `ip` ：GPIO输入
	- `op` ：GPIO输出
	- `a0-a8` ：映射为其他功能（a0~a8），GPIO0-GPIO27引脚可以映射的所有功能在 `## 树莓派5 引脚功能映射（GPIO0-GPIO27）`查看。
	- `no` ：没有设置功能
- 特性
	- `pu` ：使能内部上拉
	- `pd` ：使能内部下拉
	- `pn` ：禁用内部上下拉
- 电平
	- `dh` ：设为高电平 (设为`op`时有效)
	- `dl` ：设为低电平 (设为`op`时有效)

> 注意：部分系统需要 `sudo` 才能执行 `pinctrl set` 修改引脚配置。

设置GPIO12为输出高电平：

```bash
~ $ pinctrl set 12 op dh
~ $ pinctrl get 12
12: op dh pd | hi // GPIO12 = output
```

设置GPIO12为输入，并使能上拉：

```bash
~ $ pinctrl set 12 ip pu
~ $ pinctrl get 12
12: ip    pu | hi // GPIO12 = input
```

设置GPIO12为PWM功能：
```bash
~ $ pinctrl set 12 a0
~ $ pinctrl get 12
12: a0    pu | lo // GPIO12 = PWM0_CHAN0
```

## 树莓派5 引脚功能映射（GPIO0-GPIO27）

> 说明：本节仅列出 40-pin 排针常用的 GPIO0-GPIO27 复用功能映射。

| GPIO | a0         | a1        | a2          | a3         | a4         | a5         | a6          | a7    | a8        |
| ---- | ---------- | --------- | ----------- | ---------- | ---------- | ---------- | ----------- | ----- | --------- |
| 0    | SPI0_SIO3  | DPI_PCLK  | TXD1        | SDA0       | -          | SYS_RIO00  | PROC_RIO00  | PIO0  | SPI2_CE0  |
| 1    | SPI0_SIO2  | DPI_DE    | RXD1        | SCL0       | -          | SYS_RIO01  | PROC_RIO01  | PIO1  | SPI2_SIO1 |
| 2    | SPI0_CE3   | DPI_VSYNC | CTS1        | SDA1       | IR_RX0     | SYS_RIO02  | PROC_RIO02  | PIO2  | SPI2_SIO0 |
| 3    | SPI0_CE2   | DPI_HSYNC | RTS1        | SCL1       | IR_TX0     | SYS_RIO03  | PROC_RIO03  | PIO3  | SPI2_SCLK |
| 4    | GPCLK0     | DPI_D0    | TXD2        | SDA2       | RI0        | SYS_RIO04  | PROC_RIO04  | PIO4  | SPI3_CE0  |
| 5    | GPCLK1     | DPI_D1    | RXD2        | SCL2       | DTR0       | SYS_RIO05  | PROC_RIO05  | PIO5  | SPI3_SIO1 |
| 6    | GPCLK2     | DPI_D2    | CTS2        | SDA3       | DCD0       | SYS_RIO06  | PROC_RIO06  | PIO6  | SPI3_SIO0 |
| 7    | SPI0_CE1   | DPI_D3    | RTS2        | SCL3       | DSR0       | SYS_RIO07  | PROC_RIO07  | PIO7  | SPI3_SCLK |
| 8    | SPI0_CE0   | DPI_D4    | TXD3        | SDA0       | -          | SYS_RIO08  | PROC_RIO08  | PIO8  | SPI4_CE0  |
| 9    | SPI0_MISO  | DPI_D5    | RXD3        | SCL0       | -          | SYS_RIO09  | PROC_RIO09  | PIO9  | SPI4_SIO0 |
| 10   | SPI0_MOSI  | DPI_D6    | CTS3        | SDA1       | -          | SYS_RIO010 | PROC_RIO010 | PIO10 | SPI4_SIO1 |
| 11   | SPI0_SCLK  | DPI_D7    | RTS3        | SCL1       | -          | SYS_RIO011 | PROC_RIO011 | PIO11 | SPI4_SCLK |
| 12   | PWM0_CHAN0 | DPI_D8    | TXD4        | SDA2       | AAUD_LEFT  | SYS_RIO012 | PROC_RIO012 | PIO12 | SPI5_CE0  |
| 13   | PWM0_CHAN1 | DPI_D9    | RXD4        | SCL2       | AAUD_RIGHT | SYS_RIO013 | PROC_RIO013 | PIO13 | SPI5_SIO1 |
| 14   | PWM0_CHAN2 | DPI_D10   | CTS4        | SDA3       | TXD0       | SYS_RIO014 | PROC_RIO014 | PIO14 | SPI5_SIO0 |
| 15   | PWM0_CHAN3 | DPI_D11   | RTS4        | SCL3       | RXD0       | SYS_RIO015 | PROC_RIO015 | PIO15 | SPI5_SCLK |
| 16   | SPI1_CE2   | DPI_D12   | DSI0_TE_EXT | -          | CTS0       | SYS_RIO016 | PROC_RIO016 | PIO16 | -         |
| 17   | SPI1_CE1   | DPI_D13   | DSI1_TE_EXT | -          | RTS0       | SYS_RIO017 | PROC_RIO017 | PIO17 | -         |
| 18   | SPI1_CE0   | DPI_D14   | I2S0_SCLK   | PWM0_CHAN2 | I2S1_SCLK  | SYS_RIO018 | PROC_RIO018 | PIO18 | GPCLK1    |
| 19   | SPI1_MISO  | DPI_D15   | I2S0_WS     | PWM0_CHAN3 | I2S1_WS    | SYS_RIO019 | PROC_RIO019 | PIO19 | -         |
| 20   | SPI1_MOSI  | DPI_D16   | I2S0_SDI0   | GPCLK0     | I2S1_SDI0  | SYS_RIO020 | PROC_RIO020 | PIO20 | -         |
| 21   | SPI1_SCLK  | DPI_D17   | I2S0_SDO0   | GPCLK1     | I2S1_SDO0  | SYS_RIO021 | PROC_RIO021 | PIO21 | -         |
| 22   | SD0_CLK    | DPI_D18   | I2S0_SDI1   | SDA3       | I2S1_SDI1  | SYS_RIO022 | PROC_RIO022 | PIO22 | -         |
| 23   | SD0_CMD    | DPI_D19   | I2S0_SDO1   | SCL3       | I2S1_SDO1  | SYS_RIO023 | PROC_RIO023 | PIO23 | -         |
| 24   | SD0_DAT0   | DPI_D20   | I2S0_SDI2   | -          | I2S1_SDI2  | SYS_RIO024 | PROC_RIO024 | PIO24 | SPI2_CE1  |
| 25   | SD0_DAT1   | DPI_D21   | I2S0_SDO2   | MIC_CLK    | I2S1_SDO2  | SYS_RIO025 | PROC_RIO025 | PIO25 | SPI3_CE1  |
| 26   | SD0_DAT2   | DPI_D22   | I2S0_SDI3   | MIC_DAT0   | I2S1_SDI3  | SYS_RIO026 | PROC_RIO026 | PIO26 | SPI5_CE1  |
| 27   | SD0_DAT3   | DPI_D23   | I2S0_SDO3   | MIC_DAT1   | I2S1_SDO3  | SYS_RIO027 | PROC_RIO027 | PIO27 | SPI1_CE1  |
