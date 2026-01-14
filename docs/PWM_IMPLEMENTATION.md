# PWM 控制实现方案对比

## 需求回顾

- **硬件限制**: 仅支持硬件 PWM
  - GPIO 12 (PWM0)
  - GPIO 13 (PWM1)
- **功能需求**: 
  - 设置 PWM 频率和占空比
  - 实时调整 PWM 参数

## 方案对比

### 方案 A：使用 `/sys/class/pwm/` 文件系统接口

#### 工作原理
Linux 内核 PWM 子系统通过 sysfs 接口暴露 PWM 控制功能。在树莓派上，PWM 控制器通常位于 `/sys/class/pwm/pwmchip0/`。

#### 实现方式

```c
// 1. 导出 PWM 通道（例如通道 0，对应 GPIO 12）
FILE *f = fopen("/sys/class/pwm/pwmchip0/export", "w");
fprintf(f, "0\n");
fclose(f);

// 2. 设置周期（单位：纳秒，例如 1ms = 1000000ns，对应 1kHz）
f = fopen("/sys/class/pwm/pwmchip0/pwm0/period", "w");
fprintf(f, "1000000\n");
fclose(f);

// 3. 设置占空比（单位：纳秒，例如 50% = 500000ns）
f = fopen("/sys/class/pwm/pwmchip0/pwm0/duty_cycle", "w");
fprintf(f, "500000\n");
fclose(f);

// 4. 启用 PWM 输出
f = fopen("/sys/class/pwm/pwmchip0/pwm0/enable", "w");
fprintf(f, "1\n");
fclose(f);

// 5. 禁用 PWM（可选）
f = fopen("/sys/class/pwm/pwmchip0/pwm0/enable", "w");
fprintf(f, "0\n");
fclose(f);

// 6. 取消导出（可选）
f = fopen("/sys/class/pwm/pwmchip0/unexport", "w");
fprintf(f, "0\n");
fclose(f);
```

#### 优点
1. **标准 Linux 接口**
   - 内核原生支持，无需额外依赖
   - 跨平台兼容（所有支持 PWM 子系统的 Linux 系统）
   - 不限于树莓派

2. **简单直接**
   - 通过文件读写操作，易于理解和实现
   - 无需编译额外的库
   - 调试方便（可以直接用 `echo` 命令测试）

3. **资源占用小**
   - 无额外守护进程
   - 无库依赖

4. **权限管理简单**
   - 可以通过 udev 规则设置权限
   - 或通过用户组权限控制

#### 缺点
1. **性能相对较低**
   - 每次操作需要多次文件 I/O
   - 不适合高频更新场景

2. **功能有限**
   - 仅支持基本的周期和占空比设置
   - 不支持高级功能（如相位调整、多通道同步等）

3. **错误处理**
   - 需要手动检查文件操作返回值
   - 错误信息需要通过 `errno` 获取

4. **树莓派特定限制**
   - 需要确保 PWM 在设备树中正确配置
   - GPIO 12/13 的复用功能需要正确设置

### 方案 B：使用 `pigpio` 库

#### 工作原理
`pigpio` 是专为树莓派设计的库，通过 `pigpiod` 守护进程提供硬件访问。它支持多种通信方式（本地 socket、远程 socket、管道）。

#### 实现方式

```c
// 注意：pigpio 主要是 Python 库，C 接口相对较少
// 需要通过 pigpio 的 C 接口或通过命令行工具调用

// 方式 1：通过命令行工具（pigs）
system("pigs p 12 1000 128");  // GPIO 12, 频率 1000Hz, 占空比 50% (128/255)

// 方式 2：使用 pigpio C 库（如果可用）
// 需要链接 libpigpio.so
```

#### 优点
1. **功能丰富**
   - 支持高精度 PWM（最高 125MHz）
   - 支持软件 PWM（任意 GPIO）
   - 支持伺服控制、波形生成等高级功能

2. **性能好**
   - 通过守护进程优化硬件访问
   - 支持批量操作

3. **易于使用（Python）**
   - Python 接口非常友好
   - 文档完善

#### 缺点
1. **树莓派专用**
   - 仅支持树莓派平台
   - 不适用于其他 Linux 系统

2. **需要守护进程**
   - 必须运行 `pigpiod` 守护进程
   - 增加系统资源占用
   - 守护进程故障会影响功能

3. **C 接口支持有限**
   - 主要是 Python 库
   - C 接口文档和示例较少
   - 需要通过命令行工具或 socket 通信

4. **额外依赖**
   - 需要安装 `pigpio` 包
   - 需要启动 `pigpiod` 服务
   - 增加系统复杂度

5. **权限要求**
   - 守护进程需要 root 权限运行
   - 或配置适当的权限

## 推荐方案

### 推荐：**方案 A（`/sys/class/pwm/`）**

#### 理由
1. **符合项目需求**
   - 我们的需求是基本的 PWM 频率和占空比设置
   - 不需要 `pigpio` 的高级功能（软件 PWM、波形生成等）

2. **统一性**
   - 与 GPIO 控制（libgpiod）保持一致的现代标准接口
   - 都是内核原生接口，不依赖第三方守护进程

3. **简单可靠**
   - 实现简单，代码量少
   - 无额外依赖，减少故障点
   - 易于调试和维护

4. **跨平台兼容**
   - 虽然项目主要针对树莓派，但使用标准接口更通用
   - 便于未来扩展或移植

5. **性能足够**
   - 对于监控软件的应用场景，文件 I/O 的性能开销可以接受
   - PWM 参数调整不是高频操作

### 备选：方案 B（`pigpio`）

如果未来需要以下功能，可以考虑切换到 `pigpio`：
- 软件 PWM（任意 GPIO）
- 高精度 PWM（> 1MHz）
- 波形生成
- 伺服控制

## 实现细节

### GPIO 到 PWM 通道映射

在树莓派上：
- **GPIO 12** → PWM0 → `/sys/class/pwm/pwmchip0/pwm0/`
- **GPIO 13** → PWM1 → `/sys/class/pwm/pwmchip0/pwm1/`

### 频率和占空比计算

```c
// 频率（Hz）转换为周期（纳秒）
// period_ns = 1000000000 / frequency_hz

// 占空比（百分比）转换为占空比时间（纳秒）
// duty_cycle_ns = period_ns * duty_percent / 100

// 示例：1kHz，50% 占空比
// period_ns = 1000000000 / 1000 = 1000000ns
// duty_cycle_ns = 1000000 * 50 / 100 = 500000ns
```

### 错误处理

```c
int pwm_set_period(const char *pwm_path, unsigned long period_ns) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/period", pwm_path);
    
    FILE *f = fopen(filepath, "w");
    if (!f) {
        return -1;  // 文件打开失败
    }
    
    if (fprintf(f, "%lu\n", period_ns) < 0) {
        fclose(f);
        return -2;  // 写入失败
    }
    
    fclose(f);
    return 0;  // 成功
}
```

### 权限设置

可以通过 udev 规则设置权限，避免需要 root：

```bash
# /etc/udev/rules.d/99-pwm.rules
SUBSYSTEM=="pwm", MODE="0666"
```

或者将用户添加到 `gpio` 组（如果系统配置了相应权限）。

## API 设计

### PWM 控制 API

```
POST /api/pwm/config
请求体: {
  "gpio": 12,           // GPIO 12 或 13
  "frequency": 1000,    // 频率（Hz），范围：1 - 1000000
  "duty_cycle": 50,     // 占空比（%），范围：0 - 100
  "enable": true        // 是否启用
}

GET /api/pwm/status
返回: {
  "pwm0": {             // GPIO 12
    "gpio": 12,
    "frequency": 1000,
    "duty_cycle": 50,
    "enabled": true,
    "period_ns": 1000000,
    "duty_cycle_ns": 500000
  },
  "pwm1": {             // GPIO 13
    "gpio": 13,
    "frequency": 0,
    "duty_cycle": 0,
    "enabled": false,
    "period_ns": 0,
    "duty_cycle_ns": 0
  }
}

POST /api/pwm/enable
请求体: {
  "gpio": 12,
  "enable": true
}
```

## 总结

**推荐使用 `/sys/class/pwm/` 文件系统接口**，因为：
1. ✅ 满足基本需求（频率和占空比设置）
2. ✅ 标准 Linux 接口，无额外依赖
3. ✅ 实现简单，易于维护
4. ✅ 与项目其他模块（libgpiod）保持一致的设计理念
5. ✅ 性能足够，适合监控软件场景

如果未来需要更高级的 PWM 功能，可以考虑添加 `pigpio` 支持作为可选功能。
