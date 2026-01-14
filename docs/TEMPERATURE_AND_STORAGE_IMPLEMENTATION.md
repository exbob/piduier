# CPU 温度和存储占用率实现方案 ✅ 已实现

## 1. CPU 温度实现

### 1.1 数据来源
- **文件路径**: `/sys/class/thermal/thermal_zone0/temp`
- **数据格式**: 温度值以毫摄氏度为单位（例如：45000 表示 45.0°C）
- **读取方式**: 直接读取文件内容，转换为摄氏度

### 1.2 实现方式
- **模块位置**: `src/system/temperature.c` 和 `src/system/temperature.h`
- **函数设计**:
  ```c
  // 获取 CPU 温度（摄氏度）
  // 返回温度值（摄氏度），失败返回 -1.0
  double temperature_get_cpu(void);
  ```
- **实现步骤**:
  1. 打开 `/sys/class/thermal/thermal_zone0/temp` 文件
  2. 读取文件内容（整数字符串）
  3. 转换为整数（毫摄氏度）
  4. 除以 1000.0 转换为摄氏度
  5. 返回温度值

### 1.3 错误处理
- 文件不存在或无法打开：返回 -1.0
- 读取失败或格式错误：返回 -1.0
- 温度值异常（超出合理范围，如 < 0 或 > 150）：返回 -1.0（可选，或记录警告）

### 1.4 API 端点
- **GET /api/system/temperature**
- **响应格式**:
  ```json
  {
    "temperature": 45.5,
    "unit": "celsius"
  }
  ```
- **错误响应**:
  ```json
  {
    "error": "Failed to get CPU temperature"
  }
  ```

### 1.5 前端集成
- 在 `/api/system/info/complete` 响应中添加 `temperature` 字段
- 前端更新 `temp-value` 和 `temp-progress` 元素
- 圆形进度条根据温度值显示（建议：< 60°C 绿色，60-80°C 黄色，> 80°C 红色）

---

## 2. 存储占用率实现

### 2.1 数据来源
- **设备**: `/dev/mmcblk0p2`（根文件系统分区）
- **实现方式**: 使用 `df` 命令读取文件系统使用情况
- **命令**: `df -B1 /dev/mmcblk0p2` 或 `df -B1 /`（根分区通常挂载在 `/`）

### 2.2 实现方式
- **模块位置**: `src/system/storage.c` 和 `src/system/storage.h`
- **数据结构**:
  ```c
  typedef struct {
      unsigned long long total;      // 总容量（字节）
      unsigned long long used;        // 已用容量（字节）
      unsigned long long available;   // 可用容量（字节）
      double usage_percent;          // 使用率百分比
      char device[64];               // 设备名称（如 "/dev/mmcblk0p2"）
      char mount_point[256];         // 挂载点（如 "/"）
  } storage_info_t;
  ```
- **函数设计**:
  ```c
  // 获取存储信息（从 df 命令）
  // device: 设备路径，如 "/dev/mmcblk0p2" 或 NULL（使用根分区）
  // 返回 0 成功，-1 失败
  int storage_get_info(const char *device, storage_info_t *info);
  
  // 将存储信息转换为 JSON 字符串
  // 返回分配的字符串，调用者需要 free()
  char *storage_info_to_json(const storage_info_t *info);
  ```

### 2.3 实现步骤
1. 构建 `df` 命令：`df -B1 /dev/mmcblk0p2` 或 `df -B1 /`
2. 使用 `popen()` 执行命令并读取输出
3. 解析 `df` 输出格式：
   ```
   Filesystem      1B-blocks         Used    Available Use% Mounted on
   /dev/mmcblk0p2  1234567890  987654321   246913569  80% /
   ```
4. 提取总容量、已用容量、可用容量
5. 计算使用率百分比：`(used / total) * 100`
6. 提取设备名称和挂载点

### 2.4 错误处理
- 命令执行失败：返回 -1
- 输出解析失败：返回 -1
- 数据异常（如 total = 0）：返回 -1

### 2.5 API 端点
- **GET /api/system/storage**
- **响应格式**:
  ```json
  {
    "total": 1234567890,
    "used": 987654321,
    "available": 246913569,
    "usage_percent": 80.0,
    "device": "/dev/mmcblk0p2",
    "mount_point": "/"
  }
  ```
- **错误响应**:
  ```json
  {
    "error": "Failed to get storage info"
  }
  ```

### 2.6 前端集成
- 在 `/api/system/info/complete` 响应中添加 `storage` 字段
- 前端更新 `storage-percent` 和 `storage-progress` 元素
- 圆形进度条根据使用率显示（建议：< 70% 绿色，70-90% 黄色，> 90% 红色）

---

## 3. 集成到完整系统信息 API

### 3.1 修改 `/api/system/info/complete`
在 `handle_system_info_complete()` 函数中添加：
- 调用 `temperature_get_cpu()` 获取温度
- 调用 `storage_get_info("/dev/mmcblk0p2", &storage_info)` 获取存储信息
- 在 JSON 响应中添加 `temperature` 和 `storage` 字段

### 3.2 响应格式更新
```json
{
  "system": {...},
  "datetime": {...},
  "cpu_usage": 15.5,
  "memory": {...},
  "uptime": 86400,
  "uptime_formatted": "1 day, 0:00:00",
  "temperature": 45.5,
  "storage": {
    "total": 1234567890,
    "used": 987654321,
    "available": 246913569,
    "usage_percent": 80.0,
    "device": "/dev/mmcblk0p2",
    "mount_point": "/"
  },
  "network": [...]
}
```

---

## 4. 文件结构

### 4.1 新增文件
- `src/system/temperature.h` - CPU 温度模块头文件
- `src/system/temperature.c` - CPU 温度模块实现
- `src/system/storage.h` - 存储信息模块头文件
- `src/system/storage.c` - 存储信息模块实现

### 4.2 修改文件
- `src/http/router.c` - 添加温度和存储 API 端点处理
- `src/http/router.c` - 更新 `handle_system_info_complete()` 函数
- `CMakeLists.txt` - 添加新的源文件到编译列表
- `web/js/app.js` - 更新前端代码以显示温度和存储信息

---

## 5. 实现顺序

1. **实现 CPU 温度模块**
   - 创建 `temperature.h` 和 `temperature.c`
   - 实现 `temperature_get_cpu()` 函数
   - 添加 API 端点 `/api/system/temperature`
   - 测试温度读取功能

2. **实现存储信息模块**
   - 创建 `storage.h` 和 `storage.c`
   - 实现 `storage_get_info()` 和 `storage_info_to_json()` 函数
   - 添加 API 端点 `/api/system/storage`
   - 测试存储信息读取功能

3. **集成到完整系统信息 API**
   - 修改 `handle_system_info_complete()` 函数
   - 添加温度和存储数据到响应
   - 测试完整 API

4. **更新前端**
   - 更新 `app.js` 中的 `updateSystemInfo()` 函数
   - 添加 `updateTemperature()` 和 `updateStorage()` 函数
   - 更新圆形进度条显示
   - 测试前端显示

---

## 6. 注意事项

### 6.1 CPU 温度
- 温度文件可能不存在（某些系统或虚拟环境），需要处理错误情况
- 温度值可能波动，前端可以显示最近一次读取的值
- 温度单位统一使用摄氏度（°C）

### 6.2 存储信息
- `df` 命令需要系统支持，确保命令可用
- 如果 `/dev/mmcblk0p2` 不存在，可以尝试使用 `/` 作为参数（根分区）
- 容量单位统一使用字节，前端可以转换为更友好的单位（GB、TB 等）
- 注意处理 `df` 输出的格式差异（不同系统可能略有不同）

### 6.3 性能考虑
- 温度读取很快，可以直接在 API 调用时读取
- 存储信息读取需要执行命令，但 `df` 命令执行很快，可以直接在 API 调用时执行
- 如果性能成为问题，可以考虑缓存机制（类似 CPU 占用率的实现）

---

## 7. 测试建议

### 7.1 CPU 温度测试
- 测试正常温度读取（20-80°C）
- 测试文件不存在的情况
- 测试文件格式错误的情况
- 验证温度值转换正确性

### 7.2 存储信息测试
- 测试正常存储信息读取
- 测试 `df` 命令执行失败的情况
- 测试输出解析（不同格式）
- 验证使用率计算正确性
- 测试不同设备路径（`/dev/mmcblk0p2` vs `/`）

### 7.3 集成测试
- 测试完整系统信息 API 包含温度和存储数据
- 测试前端正确显示温度和存储信息
- 测试圆形进度条颜色变化
- 测试自动刷新功能
