// 主应用 JavaScript

// API 基础 URL
const API_BASE = '';

// 当前活动页面
let currentPage = 'dashboard';

// 初始化应用
document.addEventListener('DOMContentLoaded', () => {
    initMenu();
    initDashboard();
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
            if (confirm('确定要重启系统吗？此操作将立即重启树莓派。')) {
                rebootSystem();
            }
        });
    }
    
    // 关闭系统按钮
    const shutdownBtn = document.getElementById('shutdown-btn');
    if (shutdownBtn) {
        shutdownBtn.addEventListener('click', () => {
            if (confirm('确定要关闭系统吗？此操作将立即关闭树莓派。')) {
                shutdownSystem();
            }
        });
    }
});

// 初始化菜单
function initMenu() {
    const menuLinks = document.querySelectorAll('.menu-link');
    menuLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();
            const page = link.dataset.page;
            const menuItem = link.closest('.menu-item');
            
            if (page) {
                // 如果有子菜单，展开/收起
                if (menuItem && menuItem.querySelector('.menu-sub')) {
                    menuItem.classList.toggle('expanded');
                }
                switchPage(page);
                
                // 移动端切换页面后关闭侧边栏
                if (window.innerWidth <= 768) {
                    document.querySelector('.sidebar').classList.remove('open');
                }
            } else if (menuItem && menuItem.querySelector('.menu-sub')) {
                // 切换子菜单展开/收起
                menuItem.classList.toggle('expanded');
            }
        });
    });
    
    // HAT-40pin 菜单切换
    const hatMenuToggle = document.getElementById('hat-menu-toggle');
    if (hatMenuToggle) {
        hatMenuToggle.addEventListener('click', (e) => {
            e.preventDefault();
            const menuItem = hatMenuToggle.closest('.menu-item');
            if (menuItem) {
                menuItem.classList.toggle('expanded');
            }
        });
    }
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
    
    // 根据页面加载数据
    if (page === 'dashboard') {
        loadDashboard();
    }
}

// 初始化监控面板
function initDashboard() {
    switchPage('dashboard');
}

// 加载监控面板数据
async function loadDashboard() {
    try {
        // 使用综合 API 获取所有数据
        const response = await fetch(`${API_BASE}/api/system/info/complete`);
        if (!response.ok) {
            throw new Error('获取系统信息失败');
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
        console.error('加载数据失败:', error);
        showMessage('加载数据失败: ' + error.message, 'error');
    }
}

// 获取系统信息（保留用于兼容）
async function fetchSystemInfo() {
    const response = await fetch(`${API_BASE}/api/system/info`);
    if (!response.ok) {
        throw new Error('获取系统信息失败');
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
        if (ethStatusTextEl) ethStatusTextEl.textContent = '已连接';
        if (ethIpEl) ethIpEl.textContent = ethDevice.ip || '-';
        if (ethMacEl) ethMacEl.textContent = ethDevice.mac || '-';
        if (ethSpeedEl) ethSpeedEl.textContent = ethDevice.speed ? ethDevice.speed + ' Mbps' : '-';
    } else {
        if (ethStatusBadge) {
            ethStatusBadge.className = 'status-badge disconnected';
        }
        if (ethStatusEl) ethStatusEl.className = 'status-indicator error';
        if (ethStatusTextEl) ethStatusTextEl.textContent = '未连接';
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
        if (wifiStatusTextEl) wifiStatusTextEl.textContent = '已连接';
        if (wifiInfoEl) wifiInfoEl.style.display = 'block';
        if (wifiEmptyEl) wifiEmptyEl.style.display = 'none';
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
        if (wifiStatusTextEl) wifiStatusTextEl.textContent = '未连接';
        if (wifiInfoEl) wifiInfoEl.style.display = 'none';
        if (wifiEmptyEl) wifiEmptyEl.style.display = 'block';
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
        return `${days} 天 ${hours} 小时 ${minutes} 分钟`;
    } else if (hours > 0) {
        return `${hours} 小时 ${minutes} 分钟`;
    } else {
        return `${minutes} 分钟`;
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
            showMessage('系统正在重启...', 'success');
            // 系统即将重启，等待几秒后显示提示
            setTimeout(() => {
                showMessage('系统正在重启，请稍候...', 'success');
            }, 1000);
        } else {
            showMessage('重启失败: ' + (data.message || '未知错误'), 'error');
        }
    } catch (error) {
        console.error('重启系统失败:', error);
        showMessage('重启失败: ' + error.message, 'error');
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
            showMessage('系统正在关闭...', 'success');
            // 系统即将关闭，等待几秒后显示提示
            setTimeout(() => {
                showMessage('系统正在关闭，请稍候...', 'success');
            }, 1000);
        } else {
            showMessage('关闭失败: ' + (data.message || '未知错误'), 'error');
        }
    } catch (error) {
        console.error('关闭系统失败:', error);
        showMessage('关闭失败: ' + error.message, 'error');
    }
}
