// 主应用 JavaScript

// API 基础 URL
const API_BASE = '';
const PRODUCT_TAGLINE = 'Raspberry Pi 5 40-pin interface debugging platform';

// 当前活动页面
let currentPage = 'dashboard';
let gpioPollInterval = null;
let lastGpioErrorAt = 0;
let gpioSocket = null;
let currentAttrGpio = null;
let gpioStatusCache = {};

// 初始化应用
document.addEventListener('DOMContentLoaded', () => {
    document.title = `PiDuier - ${PRODUCT_TAGLINE}`;
    initMenu();
    initMobileSidebar();
    initDashboard();
    loadVersionInfo();
    startAutoRefresh();
    
    // 刷新按钮
    const refreshBtn = document.getElementById('refreshBtn');
    if (refreshBtn) {
        refreshBtn.addEventListener('click', () => {
            if (currentPage === 'dashboard') {
                loadDashboard();
            }
        });
    }
    
    // 重启系统按钮
    const rebootBtn = document.getElementById('reboot-btn');
    if (rebootBtn) {
        rebootBtn.addEventListener('click', () => {
            if (confirm('Reboot the system now? This will immediately restart the Raspberry Pi.')) {
                rebootSystem();
            }
        });
    }
    
    // 关闭系统按钮
    const shutdownBtn = document.getElementById('shutdown-btn');
    if (shutdownBtn) {
        shutdownBtn.addEventListener('click', () => {
            if (confirm('Shutdown the system now? This will immediately power off the Raspberry Pi.')) {
                shutdownSystem();
            }
        });
    }
    
    // GPIO 配置页面初始化
    initGpioPage();
});

// 初始化菜单
function initMenu() {
    // 先处理 HAT-40pin 菜单切换（避免与其他事件冲突）
    const hatMenuToggle = document.getElementById('hat-menu-toggle');
    if (hatMenuToggle) {
        hatMenuToggle.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            const menuItem = hatMenuToggle.closest('.menu-item');
            if (menuItem) {
                menuItem.classList.toggle('expanded');
            }
        });
    }
    
    // 处理所有菜单链接
    const menuLinks = document.querySelectorAll('.menu-link');
    menuLinks.forEach(link => {
        // 跳过 hat-menu-toggle，已经单独处理
        if (link.id === 'hat-menu-toggle') {
            return;
        }
        
        link.addEventListener('click', (e) => {
            e.preventDefault();
            const page = link.dataset.page;
            const menuItem = link.closest('.menu-item');
            const isSubMenu = link.closest('.menu-sub') !== null;
            
            if (page) {
                // 如果是子菜单项，确保父菜单展开
                if (isSubMenu && menuItem) {
                    const parentMenuItem = menuItem.closest('.menu-item');
                    if (parentMenuItem) {
                        parentMenuItem.classList.add('expanded');
                    }
                }
                switchPage(page);
                
                // 移动端切换页面后关闭侧边栏
                if (window.innerWidth <= 768) {
                    document.querySelector('.sidebar').classList.remove('open');
                    setHidden(document.getElementById('sidebar-overlay'), true);
                }
            }
        });
    });
}

async function loadVersionInfo() {
    const versionEl = document.getElementById('about-version');
    if (!versionEl) return;

    try {
        const response = await fetch(`${API_BASE}/api/system/version`);
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }
        const data = await response.json();
        versionEl.textContent = data.version || 'unknown';
    } catch (error) {
        versionEl.textContent = 'unknown';
        console.warn('Failed to load version info:', error);
    }
}

function setHidden(el, hidden) {
    if (!el) return;
    el.classList.toggle('is-hidden', hidden);
}

function setGpioConnectionStatus(state, text) {
    const statusEl = document.getElementById('gpio-conn-status');
    const textEl = document.getElementById('gpio-conn-text');
    if (!statusEl || !textEl) return;
    statusEl.classList.remove('gpio-conn-status-live', 'gpio-conn-status-fallback', 'gpio-conn-status-idle');
    if (state === 'live') statusEl.classList.add('gpio-conn-status-live');
    else if (state === 'fallback') statusEl.classList.add('gpio-conn-status-fallback');
    else statusEl.classList.add('gpio-conn-status-idle');
    textEl.textContent = text;
}

async function parseErrorMessage(response, fallbackMessage) {
    try {
        const payload = await response.json();
        const parts = [];
        if (payload?.message) parts.push(payload.message);
        else if (payload?.error) parts.push(payload.error);
        if (payload?.hint) parts.push(payload.hint);
        if (parts.length > 0) return parts.join(' ');
    } catch (_) {
        // Keep fallback when response is not JSON
    }
    return fallbackMessage;
}

function initMobileSidebar() {
    const toggleBtn = document.getElementById('mobile-menu-toggle');
    const sidebar = document.querySelector('.sidebar');
    const overlay = document.getElementById('sidebar-overlay');
    if (!toggleBtn || !sidebar || !overlay) return;

    const closeSidebar = () => {
        sidebar.classList.remove('open');
        setHidden(overlay, true);
    };

    toggleBtn.addEventListener('click', () => {
        const opening = !sidebar.classList.contains('open');
        sidebar.classList.toggle('open', opening);
        setHidden(overlay, !opening);
    });

    overlay.addEventListener('click', closeSidebar);
    window.addEventListener('resize', () => {
        if (window.innerWidth > 768) {
            closeSidebar();
        }
    });
}

function pulseMetricCard(progressId) {
    const progressEl = document.getElementById(progressId);
    const metricCard = progressEl ? progressEl.closest('.card-metric') : null;
    if (!metricCard) return;
    metricCard.classList.remove('is-pulsing');
    // 强制回流，确保连续刷新时动画可重复触发
    void metricCard.offsetWidth;
    metricCard.classList.add('is-pulsing');
}

// 切换页面
function switchPage(page) {
    // 隐藏所有页面
    document.querySelectorAll('.page').forEach(p => {
        p.classList.remove('active');
    });
    
    // 显示目标页面
    const targetPage = document.getElementById(`page-${page}`);
    if (targetPage) {
        targetPage.classList.add('active');
    }
    
    // 更新菜单活动状态
    document.querySelectorAll('.menu-link').forEach(link => {
        link.classList.remove('active');
        if (link.dataset.page === page) {
            link.classList.add('active');
        }
    });
    
    currentPage = page;
    updateGpioPollingState();
    
    // 根据页面加载数据
    if (page === 'dashboard') {
        loadDashboard();
    } else if (page === 'gpio') {
        loadGpioStatus();
    }
}

function updateGpioPollingState() {
    const shouldPoll = currentPage === 'gpio';
    if (shouldPoll) {
        ensureGpioSocket();
        if (!gpioPollInterval) {
            // Low-frequency fallback when websocket is unavailable
            gpioPollInterval = setInterval(() => {
                if (currentPage === 'gpio' && (!gpioSocket || gpioSocket.readyState !== WebSocket.OPEN)) {
                    setGpioConnectionStatus('fallback', 'Fallback polling');
                    loadGpioStatus();
                }
            }, 2000);
        }
    } else if (!shouldPoll) {
        if (gpioSocket) {
            gpioSocket.close();
            gpioSocket = null;
        }
        setGpioConnectionStatus('idle', 'Idle');
    }

    if (!shouldPoll && gpioPollInterval) {
        clearInterval(gpioPollInterval);
        gpioPollInterval = null;
    }
}

function ensureGpioSocket() {
    if (gpioSocket && (gpioSocket.readyState === WebSocket.OPEN || gpioSocket.readyState === WebSocket.CONNECTING)) {
        return;
    }

    setGpioConnectionStatus('fallback', 'Connecting...');
    const wsScheme = window.location.protocol === 'https:' ? 'wss' : 'ws';
    const wsUrl = `${wsScheme}://${window.location.host}/ws/gpio`;
    gpioSocket = new WebSocket(wsUrl);

    gpioSocket.onopen = () => {
        setGpioConnectionStatus('live', 'Realtime connected');
    };

    gpioSocket.onmessage = (event) => {
        try {
            const pins = JSON.parse(event.data);
            const gpioStatus = {};
            if (Array.isArray(pins)) {
                pins.forEach(pin => {
                    gpioStatus[pin.gpio_num] = pin;
                });
                gpioStatusCache = gpioStatus;
                renderGpioStatusTable(gpioStatus);
                setGpioConnectionStatus('live', 'Realtime connected');
            }
        } catch (error) {
            console.warn('Failed to parse GPIO websocket payload:', error);
        }
    };

    gpioSocket.onerror = () => {
        setGpioConnectionStatus('fallback', 'Fallback polling');
    };

    gpioSocket.onclose = () => {
        setGpioConnectionStatus('fallback', 'Fallback polling');
        gpioSocket = null;
    };
}

// 初始化监控面板
function initDashboard() {
    switchPage('dashboard');
}

// 加载监控面板数据
async function loadDashboard() {
    const refreshBtn = document.getElementById('refreshBtn');
    if (refreshBtn) {
        refreshBtn.classList.add('is-loading');
    }
    try {
        // 使用综合 API 获取所有数据
        const response = await fetch(`${API_BASE}/api/system/info/complete`);
        if (!response.ok) {
            throw new Error('Failed to fetch system info');
        }
        const data = await response.json();
        
        // 更新系统信息
        if (data.system) {
            updateSystemInfo(data.system);
        }
        
        // 更新日期时间信息
        if (data.datetime) {
            updateDateTimeInfo(data.datetime);
        }
        
        // 更新 CPU 占用率
        if (data.cpu_usage !== undefined) {
            updateCPUUsage(data.cpu_usage);
        }
        
        // 更新内存使用率
        if (data.memory) {
            updateMemory(data.memory);
        }
        
        // 更新系统运行时间
        if (data.uptime !== undefined) {
            updateUptime(data.uptime);
        }
        
        // 更新网络状态
        if (data.network && Array.isArray(data.network)) {
            updateNetworkStatus(data.network);
        }
        
        // 更新 CPU 温度
        if (data.temperature !== undefined) {
            updateTemperature(data.temperature);
        }
        
        // 更新存储占用率
        if (data.storage) {
            updateStorage(data.storage);
        }
    } catch (error) {
        console.error('Failed to load dashboard data:', error);
        showMessage('Failed to load dashboard data: ' + error.message, 'error');
    } finally {
        if (refreshBtn) {
            refreshBtn.classList.remove('is-loading');
        }
    }
}

// 获取系统信息（保留用于兼容）
async function fetchSystemInfo() {
    const response = await fetch(`${API_BASE}/api/system/info`);
    if (!response.ok) {
        throw new Error('Failed to fetch system info');
    }
    return await response.json();
}

// 更新系统信息显示
function updateSystemInfo(info) {
    if (!info) return;
    
    // 更新主机名
    const hostnameEl = document.getElementById('info-hostname');
    if (hostnameEl) {
        hostnameEl.textContent = info.static_hostname || 'N/A';
    }
    
    // 更新操作系统
    const osEl = document.getElementById('info-os');
    if (osEl) {
        osEl.textContent = info.operating_system || 'N/A';
    }
    
    // 更新CPU架构
    const archEl = document.getElementById('info-arch');
    if (archEl) {
        archEl.textContent = info.architecture || 'N/A';
    }
    
    // 更新硬件厂商
    const vendorEl = document.getElementById('info-vendor');
    if (vendorEl) {
        vendorEl.textContent = info.hardware_vendor || 'N/A';
    }
    
    // 更新硬件型号
    const modelEl = document.getElementById('info-model');
    if (modelEl) {
        modelEl.textContent = info.hardware_model || 'N/A';
    }
}

// 更新日期时间信息显示
function updateDateTimeInfo(datetime) {
    if (!datetime) return;
    
    // 更新时区
    const timezoneEl = document.getElementById('info-timezone');
    if (timezoneEl) {
        timezoneEl.textContent = datetime.timezone || 'N/A';
    }
    
    // 更新日期时间
    const datetimeEl = document.getElementById('info-datetime');
    if (datetimeEl) {
        // 使用本地时间，格式：YYYY-MM-DD HH:MM:SS
        datetimeEl.textContent = datetime.local || 'N/A';
    }
}

// 更新 CPU 占用率显示
function updateCPUUsage(usage) {
    const valueEl = document.getElementById('cpu-percent');
    const progressEl = document.getElementById('cpu-progress');
    
    if (valueEl && progressEl) {
        const percent = Math.round(usage || 0);
        valueEl.textContent = percent;
        
        // 更新圆形进度条（377 是圆的周长，2 * π * 60）
        const offset = 377 - (377 * percent / 100);
        progressEl.style.strokeDashoffset = offset;
        
        // 根据使用率改变颜色
        if (percent > 80) {
            progressEl.style.stroke = 'var(--color-error)';
        } else if (percent > 60) {
            progressEl.style.stroke = 'var(--color-warning)';
        } else {
            progressEl.style.stroke = 'var(--color-success)';
        }
        pulseMetricCard('cpu-progress');
    }
}

// 更新内存使用率显示
function updateMemory(memory) {
    const valueEl = document.getElementById('memory-percent');
    const progressEl = document.getElementById('memory-progress');
    
    if (valueEl && progressEl && memory) {
        const percent = Math.round(memory.usage_percent || 0);
        
        valueEl.textContent = percent;
        
        // 更新圆形进度条（377 是圆的周长，2 * π * 60）
        const offset = 377 - (377 * percent / 100);
        progressEl.style.strokeDashoffset = offset;
        
        // 根据使用率改变颜色
        if (percent > 80) {
            progressEl.style.stroke = 'var(--color-error)';
        } else if (percent > 60) {
            progressEl.style.stroke = 'var(--color-warning)';
        } else {
            progressEl.style.stroke = 'var(--color-warning)';
        }
        pulseMetricCard('memory-progress');
    }
}

// 更新系统运行时间显示
function updateUptime(uptime) {
    // 更新系统信息卡片中的运行时间
    const uptimeEl = document.getElementById('info-uptime');
    if (uptimeEl && uptime !== undefined) {
        uptimeEl.textContent = formatUptime(uptime);
    }
}

// 更新 CPU 温度显示
function updateTemperature(temperature) {
    const valueEl = document.getElementById('temp-value');
    const progressEl = document.getElementById('temp-progress');
    
    if (valueEl && progressEl && temperature !== undefined && temperature >= 0) {
        const temp = Math.round(temperature);
        valueEl.textContent = temp;
        
        // 更新圆形进度条（377 是圆的周长，2 * π * 60）
        // 温度范围假设为 0-100°C，超过100°C显示为100%
        const percent = Math.min(100, Math.max(0, (temperature / 100.0) * 100));
        const offset = 377 - (377 * percent / 100);
        progressEl.style.strokeDashoffset = offset;
        
        // 根据温度改变颜色
        if (temperature > 80) {
            progressEl.style.stroke = 'var(--color-error)';
        } else if (temperature > 60) {
            progressEl.style.stroke = 'var(--color-warning)';
        } else {
            progressEl.style.stroke = 'var(--color-success)';
        }
        pulseMetricCard('temp-progress');
    }
}

// 更新存储占用率显示
function updateStorage(storage) {
    const valueEl = document.getElementById('storage-percent');
    const progressEl = document.getElementById('storage-progress');
    
    if (valueEl && progressEl && storage) {
        const percent = Math.round(storage.usage_percent || 0);
        
        valueEl.textContent = percent;
        
        // 更新圆形进度条（377 是圆的周长，2 * π * 60）
        const offset = 377 - (377 * percent / 100);
        progressEl.style.strokeDashoffset = offset;
        
        // 根据使用率改变颜色
        if (percent > 90) {
            progressEl.style.stroke = 'var(--color-error)';
        } else if (percent > 70) {
            progressEl.style.stroke = 'var(--color-warning)';
        } else {
            progressEl.style.stroke = 'var(--color-success)';
        }
        pulseMetricCard('storage-progress');
    }
}

// 更新网络状态显示
function updateNetworkStatus(devices) {
    if (!devices || !Array.isArray(devices)) {
        return;
    }
    
    // 查找以太网设备
    const ethDevice = devices.find(d => d.type === 'ethernet');
    const ethStatusBadge = document.getElementById('eth-status-badge');
    const ethStatusEl = document.getElementById('eth-status');
    const ethStatusTextEl = document.getElementById('eth-status-text');
    const ethIpEl = document.getElementById('eth-ip');
    const ethMacEl = document.getElementById('eth-mac');
    const ethSpeedEl = document.getElementById('eth-speed');
    
    if (ethDevice && ethDevice.state === 'connected') {
        if (ethStatusBadge) {
            ethStatusBadge.className = 'status-badge connected';
        }
        if (ethStatusEl) ethStatusEl.className = 'status-indicator success';
        if (ethStatusTextEl) ethStatusTextEl.textContent = 'Connected';
        if (ethIpEl) ethIpEl.textContent = ethDevice.ip || '-';
        if (ethMacEl) ethMacEl.textContent = ethDevice.mac || '-';
        if (ethSpeedEl) ethSpeedEl.textContent = ethDevice.speed ? ethDevice.speed + ' Mbps' : '-';
    } else {
        if (ethStatusBadge) {
            ethStatusBadge.className = 'status-badge disconnected';
        }
        if (ethStatusEl) ethStatusEl.className = 'status-indicator error';
        if (ethStatusTextEl) ethStatusTextEl.textContent = 'Disconnected';
        if (ethIpEl) ethIpEl.textContent = '-';
        if (ethMacEl) ethMacEl.textContent = '-';
        if (ethSpeedEl) ethSpeedEl.textContent = '-';
    }
    
    // 查找 Wi-Fi 设备
    const wifiDevice = devices.find(d => d.type === 'wifi');
    const wifiStatusBadge = document.getElementById('wifi-status-badge');
    const wifiStatusEl = document.getElementById('wifi-status');
    const wifiStatusTextEl = document.getElementById('wifi-status-text');
    const wifiInfoEl = document.getElementById('wifi-info');
    const wifiEmptyEl = document.getElementById('wifi-empty');
    const wifiSsidEl = document.getElementById('wifi-ssid');
    const wifiIpEl = document.getElementById('wifi-ip');
    const wifiSignalEl = document.getElementById('wifi-signal');
    
    if (wifiDevice && wifiDevice.state === 'connected') {
        if (wifiStatusBadge) {
            wifiStatusBadge.className = 'status-badge connected';
        }
        if (wifiStatusEl) wifiStatusEl.className = 'status-indicator success';
        if (wifiStatusTextEl) wifiStatusTextEl.textContent = 'Connected';
        setHidden(wifiInfoEl, false);
        setHidden(wifiEmptyEl, true);
        if (wifiSsidEl) wifiSsidEl.textContent = wifiDevice.ssid || '-';
        if (wifiIpEl) wifiIpEl.textContent = wifiDevice.ip || '-';
        if (wifiSignalEl) {
            wifiSignalEl.textContent = wifiDevice.signal ? wifiDevice.signal + ' dBm' : '-';
        }
    } else {
        if (wifiStatusBadge) {
            wifiStatusBadge.className = 'status-badge disconnected';
        }
        if (wifiStatusEl) wifiStatusEl.className = 'status-indicator error';
        if (wifiStatusTextEl) wifiStatusTextEl.textContent = 'Disconnected';
        setHidden(wifiInfoEl, true);
        setHidden(wifiEmptyEl, false);
    }
}

// 自动刷新
let refreshInterval = null;

function startAutoRefresh() {
    if (refreshInterval) {
        clearInterval(refreshInterval);
    }
    
    refreshInterval = setInterval(() => {
        if (currentPage === 'dashboard') {
            loadDashboard();
        }
    }, 2000); // 每 2 秒刷新一次
}

// 显示消息
function showMessage(text, type = 'success') {
    const messageEl = document.getElementById('message');
    if (messageEl) {
        messageEl.textContent = text;
        messageEl.className = `message ${type} show`;
        setTimeout(() => {
            messageEl.classList.remove('show');
        }, 5000);
    }
}

// 格式化字节
function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
}

// 格式化时间
function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    
    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes}m`;
    } else {
        return `${minutes}m`;
    }
}

// 重启系统
async function rebootSystem() {
    try {
        const response = await fetch(`${API_BASE}/api/system/reboot`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        });
        
        const data = await response.json();
        if (response.ok) {
            showMessage('System reboot initiated...', 'success');
            // 系统即将重启，等待几秒后显示提示
            setTimeout(() => {
                showMessage('System is rebooting, please wait...', 'success');
            }, 1000);
        } else {
            showMessage('Reboot failed: ' + (data.message || 'Unknown error'), 'error');
        }
    } catch (error) {
        console.error('Failed to reboot system:', error);
        showMessage('Reboot failed: ' + error.message, 'error');
    }
}

// 关闭系统
async function shutdownSystem() {
    try {
        const response = await fetch(`${API_BASE}/api/system/shutdown`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        });
        
        const data = await response.json();
        if (response.ok) {
            showMessage('System shutdown initiated...', 'success');
            // 系统即将关闭，等待几秒后显示提示
            setTimeout(() => {
                showMessage('System is shutting down, please wait...', 'success');
            }, 1000);
        } else {
            showMessage('Shutdown failed: ' + (data.message || 'Unknown error'), 'error');
        }
    } catch (error) {
        console.error('Failed to shutdown system:', error);
        showMessage('Shutdown failed: ' + error.message, 'error');
    }
}

// ==================== GPIO 控制功能 ====================

// 初始化 GPIO 配置页面
function initGpioPage() {
    const refreshBtn = document.getElementById('gpio-refresh-btn');
    if (refreshBtn) {
        refreshBtn.addEventListener('click', loadGpioStatus);
    }

    const attrsSaveBtn = document.getElementById('gpio-attrs-save-btn');
    if (attrsSaveBtn) {
        attrsSaveBtn.addEventListener('click', saveGpioAttrs);
    }

    const attrsCancelBtn = document.getElementById('gpio-attrs-cancel-btn');
    if (attrsCancelBtn) {
        attrsCancelBtn.addEventListener('click', () => {
            setHidden(document.getElementById('gpio-attrs-modal'), true);
            currentAttrGpio = null;
        });
    }

    const attrsModal = document.getElementById('gpio-attrs-modal');
    if (attrsModal) {
        attrsModal.addEventListener('click', (event) => {
            if (event.target === attrsModal) {
                setHidden(attrsModal, true);
                currentAttrGpio = null;
            }
        });
    }
}

// 40pin 引脚定义（按表格顺序：每行两个引脚）
const pinDefinitions = [
    // 行1: 引脚1, 2
    { pin: 1, type: 'power', description: '3.3V', gpio: null, configurable: false },
    { pin: 2, type: 'power', description: '5V', gpio: null, configurable: false },
    // 行2: 引脚3, 4
    { pin: 3, type: 'i2c', description: '(I2C1 SDA) GPIO 2', gpio: 2, configurable: false },
    { pin: 4, type: 'power', description: '5V', gpio: null, configurable: false },
    // 行3: 引脚5, 6
    { pin: 5, type: 'i2c', description: '(I2C1 SCL) GPIO 3', gpio: 3, configurable: false },
    { pin: 6, type: 'ground', description: 'GND', gpio: null, configurable: false },
    // 行4: 引脚7, 8
    { pin: 7, type: 'gpio', description: 'GPIO 4', gpio: 4, configurable: true },
    { pin: 8, type: 'uart', description: 'GPIO 14 (UART0 TX)', gpio: 14, configurable: false },
    // 行5: 引脚9, 10
    { pin: 9, type: 'ground', description: 'GND', gpio: null, configurable: false },
    { pin: 10, type: 'uart', description: 'GPIO 15 (UART0 RX)', gpio: 15, configurable: false },
    // 行6: 引脚11, 12
    { pin: 11, type: 'gpio', description: 'GPIO 17', gpio: 17, configurable: true },
    { pin: 12, type: 'pcm', description: 'GPIO 18 (PCM CLK)', gpio: 18, configurable: false },
    // 行7: 引脚13, 14
    { pin: 13, type: 'gpio', description: 'GPIO 27', gpio: 27, configurable: true },
    { pin: 14, type: 'ground', description: 'GND', gpio: null, configurable: false },
    // 行8: 引脚15, 16
    { pin: 15, type: 'gpio', description: 'GPIO 22', gpio: 22, configurable: true },
    { pin: 16, type: 'gpio', description: 'GPIO 23', gpio: 23, configurable: true },
    // 行9: 引脚17, 18
    { pin: 17, type: 'power', description: '3.3V', gpio: null, configurable: false },
    { pin: 18, type: 'gpio', description: 'GPIO 24', gpio: 24, configurable: true },
    // 行10: 引脚19, 20
    { pin: 19, type: 'spi', description: '(SPI0 MOSI) GPIO 10', gpio: 10, configurable: false },
    { pin: 20, type: 'ground', description: 'GND', gpio: null, configurable: false },
    // 行11: 引脚21, 22
    { pin: 21, type: 'spi', description: '(SPI0 MISO) GPIO 9', gpio: 9, configurable: false },
    { pin: 22, type: 'gpio', description: 'GPIO 25', gpio: 25, configurable: true },
    // 行12: 引脚23, 24
    { pin: 23, type: 'spi', description: '(SPI0 SCLK) GPIO 11', gpio: 11, configurable: false },
    { pin: 24, type: 'spi', description: 'GPIO 8 (SPI0 CE0)', gpio: 8, configurable: false },
    // 行13: 引脚25, 26
    { pin: 25, type: 'ground', description: 'GND', gpio: null, configurable: false },
    { pin: 26, type: 'spi', description: 'GPIO 7 (SPI0 CE1)', gpio: 7, configurable: false },
    // 行14: 引脚27, 28
    { pin: 27, type: 'eeprom', description: '(EEPROM SDA) GPIO 0', gpio: 0, configurable: false },
    { pin: 28, type: 'eeprom', description: 'GPIO 1 (EEPROM SCL)', gpio: 1, configurable: false },
    // 行15: 引脚29, 30
    { pin: 29, type: 'gpio', description: 'GPIO 5', gpio: 5, configurable: true },
    { pin: 30, type: 'ground', description: 'GND', gpio: null, configurable: false },
    // 行16: 引脚31, 32
    { pin: 31, type: 'gpio', description: 'GPIO 6', gpio: 6, configurable: true },
    { pin: 32, type: 'pwm', description: 'GPIO 12 (PWM0)', gpio: 12, configurable: false },
    // 行17: 引脚33, 34
    { pin: 33, type: 'pwm', description: '(PWM1) GPIO 13', gpio: 13, configurable: false },
    { pin: 34, type: 'ground', description: 'GND', gpio: null, configurable: false },
    // 行18: 引脚35, 36
    { pin: 35, type: 'pcm', description: '(PCM FS) GPIO 19', gpio: 19, configurable: false },
    { pin: 36, type: 'gpio', description: 'GPIO 16', gpio: 16, configurable: true },
    // 行19: 引脚37, 38
    { pin: 37, type: 'gpio', description: 'GPIO 26', gpio: 26, configurable: true },
    { pin: 38, type: 'pcm', description: 'GPIO 20 (PCM DIN)', gpio: 20, configurable: false },
    // 行20: 引脚39, 40
    { pin: 39, type: 'ground', description: 'GND', gpio: null, configurable: false },
    { pin: 40, type: 'pcm', description: 'GPIO 21 (PCM DOUT)', gpio: 21, configurable: false }
];

// 获取引脚颜色类名
function getPinColorClass(type) {
    const colorMap = {
        'power': 'pin-color-power',
        'ground': 'pin-color-ground',
        'gpio': 'pin-color-gpio',
        'i2c': 'pin-color-i2c',
        'uart': 'pin-color-uart',
        'spi': 'pin-color-spi',
        'pwm': 'pin-color-pwm',
        'eeprom': 'pin-color-eeprom',
        'pcm': 'pin-color-pcm'
    };
    return colorMap[type] || 'pin-color-gpio';
}

function getPinTextColorClass(type) {
    const textColorMap = {
        'power': 'pin-text-power',
        'ground': 'pin-text-ground',
        'gpio': 'pin-text-gpio',
        'i2c': 'pin-text-i2c',
        'uart': 'pin-text-uart',
        'spi': 'pin-text-spi',
        'pwm': 'pin-text-pwm',
        'eeprom': 'pin-text-eeprom',
        'pcm': 'pin-text-pcm'
    };
    return textColorMap[type] || 'pin-text-gpio';
}

// 加载 GPIO 状态
async function loadGpioStatus() {
    try {
        // 获取 GPIO 状态
        let gpioStatus = {};
        try {
            const response = await fetch(`${API_BASE}/api/gpio/status`);
            if (response.ok) {
                const pins = await response.json();
                pins.forEach(pin => {
                    gpioStatus[pin.gpio_num] = pin;
                });
                gpioStatusCache = gpioStatus;
                renderGpioStatusTable(gpioStatus);
            } else {
                let details = 'GPIO backend is unavailable on this target.';
                try {
                    const errorData = await response.json();
                    if (errorData?.hint) details = errorData.hint;
                } catch (_) {
                    // ignore parse errors and use default hint
                }
                if (Date.now() - lastGpioErrorAt > 5000) {
                    showMessage(`Failed to load GPIO status. ${details}`, 'error');
                    lastGpioErrorAt = Date.now();
                }
            }
        } catch (e) {
            console.warn('Failed to load GPIO status:', e);
            if (Date.now() - lastGpioErrorAt > 5000) {
                showMessage('Failed to load GPIO status. Check service connectivity.', 'error');
                lastGpioErrorAt = Date.now();
            }
        }
    } catch (error) {
        showMessage('Failed to load GPIO status: ' + error.message, 'error');
    }
}

function getPullText(pull) {
    if (pull === 'up') return 'UP';
    if (pull === 'down') return 'DOWN';
    if (pull === 'off') return 'OFF';
    return '--';
}

function getDriveText(drive) {
    if (drive === 'dh') return 'DH';
    if (drive === 'dl') return 'DL';
    return '--';
}

function canConfigureAttrs(gpioData) {
    if (!gpioData) return false;
    return gpioData.mode === 'input' || gpioData.mode === 'output';
}

function buildAdvancedButton(gpioNum, enabled) {
    const disabledAttr = enabled ? '' : ' disabled';
    const title = enabled ? 'Advanced settings' : 'Available only when mode is In/Out';
    return `<button class="pin-attr-btn" onclick="showGpioAttrsForm(${gpioNum})" title="${title}" aria-label="Advanced settings"${disabledAttr}>
        <svg viewBox="0 0 24 24" class="pin-attr-icon" aria-hidden="true">
            <path d="M19.14 12.94c.04-.31.06-.63.06-.94s-.02-.63-.06-.94l2.03-1.58a.5.5 0 0 0 .12-.64l-1.92-3.32a.5.5 0 0 0-.6-.22l-2.39.96a7.1 7.1 0 0 0-1.63-.94l-.36-2.54a.5.5 0 0 0-.5-.42h-3.84a.5.5 0 0 0-.5.42l-.36 2.54c-.58.22-1.12.54-1.63.94l-2.39-.96a.5.5 0 0 0-.6.22L2.7 8.84a.5.5 0 0 0 .12.64l2.03 1.58c-.04.31-.06.63-.06.94s.02.63.06.94L2.82 14.52a.5.5 0 0 0-.12.64l1.92 3.32c.13.22.39.31.6.22l2.39-.96c.5.4 1.05.72 1.63.94l.36 2.54c.04.24.25.42.5.42h3.84c.25 0 .46-.18.5-.42l.36-2.54c.58-.22 1.13-.54 1.63-.94l2.39.96c.22.09.47 0 .6-.22l1.92-3.32a.5.5 0 0 0-.12-.64l-2.03-1.58ZM12 15.5A3.5 3.5 0 1 1 12 8.5a3.5 3.5 0 0 1 0 7Z"/>
        </svg>
    </button>`;
}

async function loadGpioAttrs(gpioNum) {
    const response = await fetch(`${API_BASE}/api/gpio/${gpioNum}/attrs`);
    if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
    }
    const data = await response.json();
    return data;
}

async function showGpioAttrsForm(gpioNum) {
    const gpioData = gpioStatusCache[gpioNum];
    if (!canConfigureAttrs(gpioData)) {
        showMessage(`GPIO ${gpioNum} advanced settings are available only in In/Out mode`, 'error');
        return;
    }

    const form = document.getElementById('gpio-attrs-modal');
    const gpioField = document.getElementById('gpio-attrs-gpio');
    const pullField = document.getElementById('gpio-attrs-pull');
    const driveField = document.getElementById('gpio-attrs-drive');
    if (!form || !gpioField || !pullField || !driveField) return;

    currentAttrGpio = gpioNum;
    gpioField.value = `GPIO ${gpioNum}`;

    try {
        const attrs = await loadGpioAttrs(gpioNum);
        pullField.value = ['off', 'up', 'down'].includes(attrs.pull) ? attrs.pull : 'off';
        driveField.value = ['dh', 'dl'].includes(attrs.drive) ? attrs.drive : 'dl';
    } catch (error) {
        pullField.value = 'off';
        driveField.value = 'dl';
        showMessage(`Failed to load GPIO ${gpioNum} attrs`, 'error');
    }

    setHidden(form, false);
}

async function saveGpioAttrs() {
    if (currentAttrGpio === null) return;
    const gpioData = gpioStatusCache[currentAttrGpio];
    if (!canConfigureAttrs(gpioData)) {
        showMessage(`GPIO ${currentAttrGpio} is not in In/Out mode`, 'error');
        return;
    }

    const pullField = document.getElementById('gpio-attrs-pull');
    const driveField = document.getElementById('gpio-attrs-drive');
    const form = document.getElementById('gpio-attrs-modal');
    if (!pullField || !driveField || !form) return;

    const payload = { pull: pullField.value };
    if (gpioData.mode === 'output') {
        payload.drive = driveField.value;
    }

    try {
        const response = await fetch(`${API_BASE}/api/gpio/${currentAttrGpio}/attrs`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        if (!response.ok) {
            const message = await parseErrorMessage(response, 'Failed to save GPIO attributes');
            throw new Error(message);
        }

        await loadGpioStatus();
        setHidden(form, true);
        showMessage(`GPIO ${currentAttrGpio} attributes saved`, 'success');
        currentAttrGpio = null;
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

function renderGpioStatusTable(gpioStatus) {
    const tableBody = document.getElementById('gpio-pins-table-body');
    if (!tableBody) return;

    tableBody.innerHTML = '';

    // 每行两个引脚（奇数和偶数）
    for (let i = 0; i < pinDefinitions.length; i += 2) {
        const leftPin = pinDefinitions[i];
        const rightPin = pinDefinitions[i + 1];
        const leftGpioData = leftPin.configurable ? gpioStatus[leftPin.gpio] : null;
        const rightGpioData = rightPin.configurable ? gpioStatus[rightPin.gpio] : null;

        const row = document.createElement('tr');

        // 列1: 左侧引脚状态
        const leftStatusCell = document.createElement('td');
        leftStatusCell.className = 'text-right';
        if (leftPin.configurable) {
            if (leftGpioData) {
                const isHigh = Number(leftGpioData.value) === 1;
                const value = isHigh ? '1' : '0';
                const statusClass = isHigh ? 'high' : 'low';
                leftStatusCell.innerHTML = `<span class="pin-status-value ${statusClass}">${value}</span>`;
            } else {
                leftStatusCell.innerHTML = '-';
            }
        } else {
            leftStatusCell.innerHTML = '';
        }

        // 列2: 左侧引脚配置按钮
        const leftConfigCell = document.createElement('td');
        leftConfigCell.className = 'text-right';
        if (leftPin.configurable) {
            const mode = leftGpioData ? leftGpioData.mode : 'unknown';
            const value = leftGpioData ? Number(leftGpioData.value) : -1;
            const attrEnabled = canConfigureAttrs(leftGpioData);
            let configText = 'N/A';
            if (mode === 'input') {
                configText = 'In';
            } else if (mode === 'output') {
                configText = value === 1 ? 'Out High' : (value === 0 ? 'Out Low' : 'Out');
            }
            leftConfigCell.innerHTML = `<div class="pin-config-actions"><button class="pin-config-btn" onclick="showGpioConfigMenu(${leftPin.gpio}, ${leftPin.pin}, 'left')">${configText}</button>${buildAdvancedButton(leftPin.gpio, attrEnabled)}</div>`;
        } else {
            leftConfigCell.innerHTML = '';
        }

        // 列3: 左侧引脚描述
        const leftDescCell = document.createElement('td');
        leftDescCell.className = 'text-right';
        const leftPinClass = getPinColorClass(leftPin.type);
        const leftPinTextClass = getPinTextColorClass(leftPin.type);
        if (leftPin.configurable) {
            const attrs = leftGpioData || {};
            leftDescCell.innerHTML = `<span class="pin-attrs"><span class="pin-attr-chip pin-attr-chip-drive">DRV:${getDriveText(attrs.drive)}</span><span class="pin-attr-chip pin-attr-chip-pull">PULL:${getPullText(attrs.pull)}</span></span><span class="pin-description ${leftPinTextClass}">${leftPin.description}</span>`;
        } else {
            leftDescCell.innerHTML = `<span class="pin-description ${leftPinTextClass}">${leftPin.description}</span>`;
        }

        // 列4: 左侧引脚圆形
        const leftPinCell = document.createElement('td');
        leftPinCell.className = 'text-center';
        leftPinCell.innerHTML = `<div class="pin-circle ${leftPinClass}">${leftPin.pin}</div>`;

        // 列5: 右侧引脚圆形
        const rightPinCell = document.createElement('td');
        rightPinCell.className = 'text-center';
        const rightPinClass = getPinColorClass(rightPin.type);
        rightPinCell.innerHTML = `<div class="pin-circle ${rightPinClass}">${rightPin.pin}</div>`;

        // 列6: 右侧引脚描述
        const rightDescCell = document.createElement('td');
        rightDescCell.className = 'text-left';
        const rightPinTextClass = getPinTextColorClass(rightPin.type);
        if (rightPin.configurable) {
            const attrs = rightGpioData || {};
            rightDescCell.innerHTML = `<span class="pin-description ${rightPinTextClass}">${rightPin.description}</span><span class="pin-attrs"><span class="pin-attr-chip pin-attr-chip-pull">PULL:${getPullText(attrs.pull)}</span><span class="pin-attr-chip pin-attr-chip-drive">DRV:${getDriveText(attrs.drive)}</span></span>`;
        } else {
            rightDescCell.innerHTML = `<span class="pin-description ${rightPinTextClass}">${rightPin.description}</span>`;
        }

        // 列7: 右侧引脚配置按钮
        const rightConfigCell = document.createElement('td');
        rightConfigCell.className = 'text-left';
        if (rightPin.configurable) {
            const mode = rightGpioData ? rightGpioData.mode : 'unknown';
            const value = rightGpioData ? Number(rightGpioData.value) : -1;
            const attrEnabled = canConfigureAttrs(rightGpioData);
            let configText = 'N/A';
            if (mode === 'input') {
                configText = 'In';
            } else if (mode === 'output') {
                configText = value === 1 ? 'Out High' : (value === 0 ? 'Out Low' : 'Out');
            }
            rightConfigCell.innerHTML = `<div class="pin-config-actions"><button class="pin-config-btn" onclick="showGpioConfigMenu(${rightPin.gpio}, ${rightPin.pin}, 'right')">${configText}</button>${buildAdvancedButton(rightPin.gpio, attrEnabled)}</div>`;
        } else {
            rightConfigCell.innerHTML = '';
        }

        // 列8: 右侧引脚状态
        const rightStatusCell = document.createElement('td');
        rightStatusCell.className = 'text-left';
        if (rightPin.configurable) {
            if (rightGpioData) {
                const isHigh = Number(rightGpioData.value) === 1;
                const value = isHigh ? '1' : '0';
                const statusClass = isHigh ? 'high' : 'low';
                rightStatusCell.innerHTML = `<span class="pin-status-value ${statusClass}">${value}</span>`;
            } else {
                rightStatusCell.innerHTML = '-';
            }
        } else {
            rightStatusCell.innerHTML = '';
        }

        row.appendChild(leftStatusCell);
        row.appendChild(leftConfigCell);
        row.appendChild(leftDescCell);
        row.appendChild(leftPinCell);
        row.appendChild(rightPinCell);
        row.appendChild(rightDescCell);
        row.appendChild(rightConfigCell);
        row.appendChild(rightStatusCell);

        tableBody.appendChild(row);
    }
}

// 显示 GPIO 配置菜单
function showGpioConfigMenu(gpioNum, pinNum, side) {
    // 移除已存在的菜单
    const existingMenu = document.querySelector('.gpio-config-menu');
    if (existingMenu) {
        document.body.removeChild(existingMenu);
    }
    
    // 创建下拉菜单
    const menu = document.createElement('div');
    menu.className = 'gpio-config-menu';
    
    const options = [
        { text: 'In', mode: 'input', value: 0 },
        { text: 'Out High', mode: 'output', value: 1 },
        { text: 'Out Low', mode: 'output', value: 0 }
    ];
    
    options.forEach(opt => {
        const item = document.createElement('div');
        item.className = 'gpio-config-menu-item';
        item.textContent = opt.text;
        item.onclick = async (e) => {
            e.stopPropagation();
            const applied = await applyGpioConfig(gpioNum, opt.mode, opt.value);
            if (document.body.contains(menu)) {
                document.body.removeChild(menu);
            }
            if (applied) {
                loadGpioStatus();
            }
        };
        menu.appendChild(item);
    });
    
    // 获取按钮位置
    const event = window.event || {};
    const btn = event.target;
    if (btn) {
        const rect = btn.getBoundingClientRect();
        menu.style.left = (rect.left + rect.width / 2 - 60) + 'px';
        menu.style.top = (rect.bottom + 4) + 'px';
    } else {
        menu.style.left = (window.innerWidth / 2 - 60) + 'px';
        menu.style.top = (window.innerHeight / 2) + 'px';
    }
    
    document.body.appendChild(menu);
    
    // 点击外部关闭菜单
    const closeMenu = (e) => {
        if (menu && !menu.contains(e.target) && e.target !== btn) {
            if (document.body.contains(menu)) {
                document.body.removeChild(menu);
            }
            document.removeEventListener('click', closeMenu);
        }
    };
    setTimeout(() => document.addEventListener('click', closeMenu), 100);
}

// 应用 GPIO 配置
async function applyGpioConfig(gpioNum, mode, value) {
    try {
        // 先设置模式
        const modeResponse = await fetch(`${API_BASE}/api/gpio/${gpioNum}/mode`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                mode: mode,
                initial_value: value
            })
        });
        
        if (!modeResponse.ok) {
            const message = await parseErrorMessage(modeResponse, 'Failed to set GPIO mode');
            throw new Error(message);
        }
        
        // 如果是输出模式，设置值
        if (mode === 'output') {
            const valueResponse = await fetch(`${API_BASE}/api/gpio/${gpioNum}/value`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    value: value
                })
            });
            
            if (!valueResponse.ok) {
                const message = await parseErrorMessage(valueResponse, 'Failed to set GPIO value');
                throw new Error(message);
            }
        }
        
        showMessage('GPIO configuration saved', 'success');
        return true;
    } catch (error) {
        showMessage('Failed to save GPIO configuration: ' + error.message, 'error');
        return false;
    }
}

// 显示 GPIO 配置对话框
function showGpioConfig(gpioNum) {
    currentGpioPin = gpioNum;
    const dialog = document.getElementById('gpio-config-dialog');
    const title = document.getElementById('gpio-config-title');
    const modeSelect = document.getElementById('gpio-config-mode');
    const valueGroup = document.getElementById('gpio-config-value-group');
    
    if (dialog && title && modeSelect) {
        title.textContent = `配置 GPIO ${gpioNum}`;
        setHidden(dialog, false);
        
        // 加载当前状态
        fetch(`${API_BASE}/api/gpio/${gpioNum}`)
            .then(response => response.json())
            .then(data => {
                if (data.mode) {
                    modeSelect.value = data.mode;
                    if (valueGroup) {
                        setHidden(valueGroup, data.mode !== 'output');
                    }
                }
                if (data.value !== undefined && data.value !== '-1') {
                    const valueSelect = document.getElementById('gpio-config-value');
                    if (valueSelect) {
                        valueSelect.value = data.value;
                    }
                }
            })
            .catch(error => {
                console.error('Failed to load GPIO pin status:', error);
            });
    }
}

// 保存 GPIO 配置
async function saveGpioConfig() {
    if (currentGpioPin < 0) {
        return;
    }
    
    const modeSelect = document.getElementById('gpio-config-mode');
    const valueSelect = document.getElementById('gpio-config-value');
    
    if (!modeSelect) {
        return;
    }
    
    const mode = modeSelect.value;
    const initialValue = mode === 'output' && valueSelect ? parseInt(valueSelect.value) : 0;
    
    try {
        // 先设置模式
        const modeResponse = await fetch(`${API_BASE}/api/gpio/${currentGpioPin}/mode`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                mode: mode,
                initial_value: initialValue
            })
        });
        
        if (!modeResponse.ok) {
            throw new Error('Failed to set GPIO mode');
        }
        
        // 如果是输出模式，设置值
        if (mode === 'output' && valueSelect) {
            const value = parseInt(valueSelect.value);
            const valueResponse = await fetch(`${API_BASE}/api/gpio/${currentGpioPin}/value`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    value: value
                })
            });
            
            if (!valueResponse.ok) {
                throw new Error('Failed to set GPIO value');
            }
        }
        
        showMessage('GPIO configuration saved', 'success');
        setHidden(document.getElementById('gpio-config-dialog'), true);
        setTimeout(loadGpioStatus, 500);
    } catch (error) {
        showMessage('Failed to save GPIO configuration: ' + error.message, 'error');
    }
}
