# 实时数据流（WebSocket）实现方案

## 需求回顾

项目中的实时数据流需求主要来自：
- **UART 通信**: 实时显示串口收发数据（十六进制和ASCII）**（使用 WebSocket）**
- **系统监控**: 实时更新 CPU/内存使用率、网络状态等（使用 HTTP 轮询）

## 方案对比

### 方案 A：不使用 WebSocket，使用 HTTP 轮询（简单）

#### 实现方式
前端使用 `setInterval` 定期调用 HTTP API 获取数据。

```javascript
// 前端轮询示例
setInterval(async () => {
    const response = await fetch('/api/system/info');
    const data = await response.json();
    updateUI(data);
}, 1000);  // 每秒轮询一次
```

#### 优点
1. **实现简单**
   - 无需 WebSocket 支持
   - 代码量少，易于维护
   - 兼容性好（所有浏览器都支持）

2. **无状态**
   - 每个请求独立，无连接状态管理
   - 服务器端实现简单

3. **易于调试**
   - 可以使用浏览器开发者工具查看 HTTP 请求
   - 可以使用 `curl` 等工具测试

#### 缺点
1. **效率较低**
   - 每次请求都有 HTTP 开销（请求头、响应头等）
   - 即使数据没有变化，也会发送请求
   - 服务器需要处理大量重复请求

2. **实时性较差**
   - 轮询间隔限制了实时性（通常 1-2 秒）
   - 数据更新有延迟

3. **资源消耗**
   - 频繁的 HTTP 请求消耗网络带宽
   - 服务器需要处理更多请求

### 方案 B：使用 WebSocket（实时双向通信）

#### 实现方式
使用 Mongoose 的 WebSocket 支持，建立持久连接，服务器主动推送数据。

```c
// 后端 WebSocket 处理示例
void handle_websocket(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_WS_MSG) {
        // 处理客户端消息
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        // ...
    }
    
    // 主动推送数据
    mg_ws_send(c, json_data, strlen(json_data), WEBSOCKET_OP_TEXT);
}
```

```javascript
// 前端 WebSocket 示例
const ws = new WebSocket('ws://localhost:8000/ws');
ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    updateUI(data);
};
```

#### 优点
1. **实时性好**
   - 服务器可以立即推送数据
   - 无轮询延迟

2. **效率高**
   - 建立一次连接，持续使用
   - 减少 HTTP 开销
   - 服务器可以主动推送，无需客户端请求

3. **双向通信**
   - 客户端可以发送命令
   - 服务器可以推送数据
   - 适合 UART 等需要双向通信的场景

#### 缺点
1. **实现复杂**
   - 需要管理连接状态
   - 需要处理连接断开、重连等
   - 代码量相对较多

2. **资源占用**
   - 每个客户端保持一个连接
   - 服务器需要维护连接状态

3. **调试相对困难**
   - WebSocket 协议不如 HTTP 直观
   - 需要专门的工具调试

## 推荐方案

### 推荐：**HTTP 轮询（系统监控）+ WebSocket（仅用于 UART）**

#### 理由
1. **符合项目需求**
   - 系统监控数据更新频率不高（1-2 秒），HTTP 轮询足够
   - **UART 需要真正的实时双向通信，使用 WebSocket**

2. **实现简单**
   - 大部分功能使用 HTTP 轮询，实现简单
   - UART 专用 WebSocket，职责清晰

3. **职责分离**
   - 系统监控：HTTP 轮询
   - UART 通信：WebSocket 实时双向数据流

### 实现建议

#### 1. 系统监控：使用 HTTP 轮询

```javascript
// 前端：定期轮询系统信息
let monitoringInterval = null;

function startMonitoring() {
    monitoringInterval = setInterval(async () => {
        try {
            const response = await fetch('/api/system/info');
            const data = await response.json();
            updateSystemInfo(data);
        } catch (error) {
            console.error('监控数据获取失败:', error);
        }
    }, 2000);  // 每 2 秒轮询一次
}

function stopMonitoring() {
    if (monitoringInterval) {
        clearInterval(monitoringInterval);
        monitoringInterval = null;
    }
}
```

**优点**：
- 实现简单
- 2 秒间隔对监控数据足够
- 减少服务器负载

#### 2. UART 通信：使用 WebSocket（必需）

UART 需要实时双向通信，使用 WebSocket：

```c
// 后端：UART WebSocket 处理
void handle_uart_websocket(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        // 处理客户端发送的 UART 数据
        uart_send(wm->data.ptr, wm->data.len);
    }
    
    // 从 UART 读取数据并推送
    char buffer[256];
    ssize_t len = uart_read(buffer, sizeof(buffer));
    if (len > 0) {
        mg_ws_send(c, buffer, len, WEBSOCKET_OP_BINARY);
    }
}
```

```javascript
// 前端：UART WebSocket
const uartWs = new WebSocket('ws://localhost:8000/ws/uart');
uartWs.onmessage = (event) => {
    // 显示接收到的 UART 数据
    displayUartData(event.data);
};

function sendUartData(data) {
    uartWs.send(data);
}
```

**优点**：
- 实时双向通信
- 适合 UART 数据流场景

## 实现细节

### HTTP 轮询优化

1. **智能轮询**
   - 页面可见时轮询，不可见时停止
   - 减少不必要的请求

```javascript
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        stopMonitoring();
    } else {
        startMonitoring();
    }
});
```

2. **错误处理**
   - 请求失败时重试
   - 网络错误时提示用户

3. **数据缓存**
   - 如果数据没有变化，不更新 UI
   - 减少 DOM 操作

### WebSocket 实现（可选）

如果实现 WebSocket，需要注意：

1. **连接管理**
   - 处理连接断开和重连
   - 心跳检测保持连接

2. **数据格式**
   - 使用 JSON 格式传输结构化数据
   - 使用二进制格式传输原始数据（如 UART）

3. **错误处理**
   - 连接失败时提示用户
   - 自动重连机制

## API 设计

### HTTP 轮询 API（主要方式）

```
GET /api/system/info
返回: 系统信息（包括 CPU、内存、网络等）

GET /api/system/cpu
返回: CPU 占用率

GET /api/system/memory
返回: 内存使用情况
```

### WebSocket API（UART 专用）

```
WS /ws/uart/ttyAMA0
功能: UART 实时数据流（仅用于 UART）
- 客户端发送: UART 发送数据（二进制或文本）
- 服务器推送: UART 接收数据（二进制或文本）
- 双向通信：支持实时发送和接收
```

## 总结

**实现策略**：
- **系统监控**: 使用 HTTP 轮询（1-2 秒更新足够）
- **UART 通信**: 使用 WebSocket（实时双向数据流）

**理由**：
1. ✅ 职责清晰：系统监控用轮询，UART 用 WebSocket
2. ✅ 实现简单：大部分功能使用 HTTP 轮询，减少代码复杂度
3. ✅ 实时性好：UART 使用 WebSocket 提供真正的实时双向通信
4. ✅ 兼容性好：HTTP 轮询所有浏览器支持，WebSocket 现代浏览器支持

**WebSocket 仅用于 UART**：
- UART 是唯一使用 WebSocket 的模块
- 提供实时双向数据流，适合串口通信场景
- 其他功能使用 HTTP 轮询即可
