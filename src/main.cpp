#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>


// ==================== 配置 ====================
const char *ssid = "ESP-01S";
const char *password = "12345678";

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

bool waitingResponse = false;
String serialInBuffer = "";

// ==================== HTML 页面 ====================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN" data-theme="light">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Web 串口助手</title>
    <style>
        :root {
            --primary-color: #2563eb;
            --primary-hover: #1d4ed8;
            --bg-color: #f1f5f9;
            --surface-color: #ffffff;
            --text-main: #1e293b;
            --text-muted: #64748b;
            --border-color: #e2e8f0;
            --success-color: #10b981;
            --danger-color: #ef4444;
            --input-bg: #ffffff;
            --topbar-height: 64px;
        }

        [data-theme="dark"] {
            --bg-color: #0f172a;
            --surface-color: #1e293b;
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
            --border-color: #334155;
            --primary-color: #3b82f6;
            --primary-hover: #60a5fa;
            --input-bg: #0f172a;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        /* 滚动条美化 */
        ::-webkit-scrollbar {
            width: 8px;
            height: 8px;
        }
        ::-webkit-scrollbar-track {
            background: var(--bg-color);
            border-radius: 4px;
        }
        ::-webkit-scrollbar-thumb {
            background: var(--text-muted);
            border-radius: 4px;
            opacity: 0.5;
        }
        ::-webkit-scrollbar-thumb:hover {
            background: var(--text-main);
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-main);
            height: 100vh;
            overflow: hidden; /* 防止页面整体滚动，只允许浮动窗口内部滚动 */
            transition: background-color 0.3s, color 0.3s;
        }

        /* 顶部导航栏 */
        .top-bar {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            height: var(--topbar-height);
            background-color: var(--surface-color);
            border-bottom: 1px solid var(--border-color);
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0 24px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.05);
            z-index: 1000;
            transition: background-color 0.3s;
        }

        .top-bar-left {
            display: flex;
            align-items: center;
            gap: 20px;
        }

        .top-bar h1 {
            font-size: 1.25rem;
            color: var(--text-main);
        }

        .status-badge {
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 6px 12px;
            border-radius: 9999px;
            font-size: 0.875rem;
            font-weight: 500;
            background-color: #fee2e2;
            color: var(--danger-color);
        }

        .status-badge.connected {
            background-color: #d1fae5;
            color: var(--success-color);
        }

        [data-theme="dark"] .status-badge {
            background-color: rgba(239, 68, 68, 0.2);
        }
        [data-theme="dark"] .status-badge.connected {
            background-color: rgba(16, 185, 129, 0.2);
        }

        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background-color: currentColor;
        }

        .top-bar-right {
            display: flex;
            align-items: center;
            gap: 12px;
        }

        .settings-btn {
            background: none;
            border: none;
            color: var(--text-muted);
            cursor: pointer;
            padding: 8px;
            border-radius: 6px;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.2s;
        }

        .settings-btn:hover {
            background-color: var(--bg-color);
            color: var(--text-main);
        }

        /* 设置下拉面板 */
        .settings-panel {
            position: fixed;
            top: calc(var(--topbar-height) + 10px);
            right: 24px;
            display: none;
            background-color: var(--surface-color);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 20px;
            gap: 16px;
            flex-direction: column;
            width: 340px;
            box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);
            z-index: 1000;
        }

        .settings-panel.active {
            display: flex;
        }

        .setting-group {
            display: flex;
            align-items: flex-start;
            flex-direction: column;
            gap: 8px;
        }

        .setting-group label {
            font-size: 0.9rem;
            font-weight: 600;
        }

        .input-group {
            display: flex;
            width: 100%;
            gap: 8px;
        }

        input[type="text"], input[type="number"], select {
            width: 100%;
            padding: 8px 12px;
            border: 1px solid var(--border-color);
            border-radius: 6px;
            font-size: 0.95rem;
            outline: none;
            transition: border-color 0.2s;
            background-color: var(--input-bg);
            color: var(--text-main);
        }

        input[type="text"]:focus, input[type="number"]:focus, select:focus {
            border-color: var(--primary-color);
        }
        
        input[type="checkbox"] {
            width: 18px;
            height: 18px;
            cursor: pointer;
            accent-color: var(--primary-color);
        }

        button {
            padding: 8px 16px;
            border: none;
            border-radius: 6px;
            font-size: 0.95rem;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
            background-color: var(--primary-color);
            color: white;
            white-space: nowrap;
        }

        button:hover {
            background-color: var(--primary-hover);
        }

        button:disabled {
            background-color: var(--text-muted);
            cursor: not-allowed;
            opacity: 0.5;
        }

        button.btn-danger { background-color: var(--danger-color); }
        button.btn-danger:hover { background-color: #dc2626; }
        button.btn-secondary {
            background-color: transparent;
            color: var(--text-main);
            border: 1px solid var(--border-color);
        }
        button.btn-secondary:hover { background-color: var(--bg-color); }

        /* 可移动/调节大小的终端窗口 */
        .workspace {
            position: relative;
            width: 100%;
            height: calc(100vh - var(--topbar-height));
            margin-top: var(--topbar-height);
        }

        .floating-window {
            position: absolute;
            top: 40px;
            left: 50px;
            width: 600px;
            height: 450px;
            min-width: 300px;
            min-height: 180px;
            max-width: 100vw;
            max-height: calc(100vh - var(--topbar-height));
            background-color: var(--surface-color);
            border-radius: 8px;
            border: 1px solid var(--border-color);
            box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);
            display: flex;
            flex-direction: column;
            resize: both;
            overflow: hidden;
            z-index: 10;
        }

        .window-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px 16px;
            background-color: var(--bg-color);
            border-bottom: 1px solid var(--border-color);
            cursor: grab;
            user-select: none;
        }

        .window-header:active {
            cursor: grabbing;
        }

        .window-title {
            font-size: 0.9rem;
            font-weight: 600;
            color: var(--text-muted);
            pointer-events: none;
        }

        .window-controls {
            display: flex;
            gap: 12px;
            align-items: center;
        }

        .checkbox-group {
            display: flex;
            align-items: center;
            gap: 4px;
            font-size: 0.85rem;
            color: var(--text-muted);
            cursor: pointer;
        }

        .terminal-content {
            flex: 1;
            background-color: #1e1e1e;
            color: #d4d4d4;
            font-family: "Consolas", "Courier New", monospace;
            padding: 16px;
            overflow-y: auto;
            white-space: pre-wrap;
            word-wrap: break-word;
            font-size: 14px;
            line-height: 1.5;
            box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
            overflow-anchor: none; /* 防止大量加载时跳动 */
        }
        
        .terminal-content div {
            min-height: 21px; /* 统一行高防止抖动 */
        }

        .send-panel {
            padding: 12px 16px;
            border-top: 1px solid var(--border-color);
            background-color: var(--surface-color);
            display: flex;
            gap: 12px;
        }

        /* 输入框清空按钮 */
        .clear-input-btn {
            position: absolute;
            right: 8px;
            top: 0;
            height: 100%;
            display: none; /* 由 JS 控制切换为 flex */
            align-items: center;
            justify-content: center;
            cursor: pointer;
            color: var(--text-muted);
            transition: color 0.2s, transform 0.2s;
        }
        .clear-input-btn:hover {
            color: var(--danger-color);
            transform: scale(1.15);
        }

        /* 右键菜单 */
        .context-menu {
            position: fixed;
            background-color: var(--surface-color);
            border: 1px solid var(--border-color);
            border-radius: 6px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            display: none;
            flex-direction: column;
            padding: 4px 0;
            z-index: 2000;
        }
        .menu-item {
            padding: 8px 16px;
            font-size: 0.9rem;
            cursor: pointer;
            color: var(--text-main);
            min-width: 120px;
        }
        .menu-item:hover {
            background-color: var(--bg-color);
            color: var(--primary-color);
        }
        .menu-separator {
            height: 1px;
            background-color: var(--border-color);
            margin: 4px 0;
        }

        /* 模态弹窗 */
        .modal-overlay {
            position: fixed;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: rgba(0, 0, 0, 0.3);
            backdrop-filter: blur(4px);
            -webkit-backdrop-filter: blur(4px);
            z-index: 3000;
            display: none;
            justify-content: center;
            align-items: center;
            opacity: 0;
            transition: opacity 0.2s;
        }
        .modal-overlay.active {
            display: flex;
            opacity: 1;
        }
        .modal {
            background-color: var(--surface-color);
            border-radius: 12px;
            width: 480px;
            max-width: 90vw;
            box-shadow: 0 15px 30px rgba(0,0,0,0.15);
            display: flex;
            flex-direction: column;
            overflow: hidden;
            border: 1px solid var(--border-color);
        }
        .modal-header {
            padding: 20px 24px;
            border-bottom: 1px solid var(--border-color);
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .modal-header h3 {
            font-size: 1.25rem;
            color: var(--text-main);
        }
        .modal-content {
            padding: 24px;
            display: flex;
            flex-direction: column;
            gap: 20px;
        }
        .modal-footer {
            padding: 20px 24px;
            border-top: 1px solid var(--border-color);
            background-color: var(--bg-color);
            display: flex;
            justify-content: flex-end;
        }

        /* 侧边栏与遮罩 */
        .sidebar-overlay {
            position: fixed;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: rgba(0, 0, 0, 0.4);
            z-index: 1050;
            opacity: 0;
            visibility: hidden;
            transition: opacity 0.3s, visibility 0.3s;
        }
        .sidebar-overlay.active {
            opacity: 1;
            visibility: visible;
        }

        .sidebar {
            position: fixed;
            top: 0; left: -320px;
            width: 320px;
            height: 100vh;
            background-color: var(--surface-color);
            z-index: 1100;
            box-shadow: 4px 0 15px rgba(0,0,0,0.1);
            transition: left 0.3s ease;
            display: flex;
            flex-direction: column;
        }
        .sidebar.active {
            left: 0;
        }
        
        .sidebar-header {
            height: var(--topbar-height);
            display: flex;
            align-items: center;
            padding: 0 20px;
            border-bottom: 1px solid var(--border-color);
            font-size: 1.1rem;
            font-weight: 600;
        }

        .sidebar-content {
            padding: 20px;
            overflow-y: auto;
            flex: 1;
            display: flex;
            flex-direction: column;
        }

        .component-category {
            font-size: 0.85rem;
            color: var(--text-muted);
            margin-bottom: 12px;
            margin-top: 16px;
            font-weight: 500;
        }
        .component-category:first-child {
            margin-top: 0;
        }

        .component-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 16px;
            background-color: var(--bg-color);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            cursor: pointer;
            transition: border-color 0.2s, background-color 0.2s;
            margin-bottom: 10px;
        }
        .component-item:hover {
            border-color: var(--primary-color);
        }
        .component-item.disabled {
            opacity: 0.6;
            cursor: not-allowed;
        }
        .component-item.disabled:hover {
            border-color: var(--border-color);
        }

        /* 参数调试组件样式 */
        .param-list {
            padding: 16px;
            overflow-y: auto;
            flex: 1;
            display: flex;
            flex-direction: column;
            gap: 16px;
        }

        .param-item {
            display: flex;
            flex-direction: column;
            gap: 10px;
            background-color: var(--surface-color);
            padding: 16px;
            border-radius: 8px;
            border: 1px solid var(--border-color);
            box-shadow: 0 1px 3px rgba(0,0,0,0.05);
        }

        .param-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 0.95rem;
            font-weight: 600;
        }
        
        .param-val {
            font-family: 'Consolas', monospace;
            color: var(--primary-color);
            background: var(--bg-color);
            padding: 4px 8px;
            border-radius: 4px;
            border: 1px solid var(--border-color);
            font-size: 0.9rem;
        }

        .param-controls {
            display: flex;
            align-items: center;
            gap: 12px;
        }

        .param-slider {
            flex: 1;
            accent-color: var(--primary-color);
            height: 6px;
            border-radius: 3px;
        }

        .param-btn {
            background-color: transparent;
            border: none;
            color: var(--text-main);
            width: 32px;
            height: 32px;
            display: flex;
            align-items: center;
            justify-content: center;
            border-radius: 6px;
            cursor: pointer;
            font-size: 1.2rem;
            line-height: 1;
            transition: all 0.2s;
        }

        .param-btn:hover {
            background-color: rgba(0, 0, 0, 0.05); /* Slight feedback */
            color: var(--primary-color);
        }
        
        #paramConfigs {
            max-height: 50vh;
            overflow-y: auto;
            padding-right: 8px;
        }
        
        .cfg-item {
            border: 1px solid var(--border-color);
            padding: 12px;
            border-radius: 8px;
            background-color: var(--bg-color);
            margin-bottom: 12px;
            position: relative;
        }

        .cfg-header {
            display: flex;
            justify-content: space-between;
            margin-bottom: 12px;
            align-items: center;
        }
        
        .cfg-header input.cfg-name {
            font-weight: 600;
            font-size: 1rem;
            background: transparent;
            border: 1px solid transparent;
            width: 200px;
            padding: 4px 8px;
        }
        .cfg-header input.cfg-name:focus {
            background: var(--surface-color);
            border: 1px solid var(--primary-color);
        }

        .cfg-delete-btn {
            background: none;
            border: none;
            color: var(--danger-color);
            cursor: pointer;
            padding: 4px;
            opacity: 0.7;
        }
        .cfg-delete-btn:hover {
            opacity: 1;
        }

        .cfg-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
        }

    </style>
</head>
<body>

    <!-- 顶部导航栏 -->
    <header class="top-bar">
        <div class="top-bar-left">
            <h1>Web 串口助手</h1>
            <div id="statusBadge" class="status-badge">
                <div class="status-dot"></div>
                <span id="statusText">未连接</span>
            </div>
        </div>
        <div class="top-bar-right">
            <button id="btnAddToggle" class="settings-btn" title="添加组件">
                <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="12" y1="5" x2="12" y2="19"></line><line x1="5" y1="12" x2="19" y2="12"></line></svg>
            </button>
            <button id="btnSettingsToggle" class="settings-btn" title="设置">
                <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
            </button>
        </div>
    </header>

    <!-- 设置侧边栏 -->
    <div id="settingsSidebar" class="sidebar">
        <div class="sidebar-header">
            设置
        </div>
        <div class="sidebar-content">
            <div class="setting-group">
                <label for="wsUrl">WebSocket连接地址</label>
                <div class="input-group">
                    <input type="text" id="wsUrl" value="ws://192.168.4.1:81" placeholder="ws://192.168.4.1:81">
                </div>
                <div class="input-group" style="margin-top: 4px;">
                    <button id="btnConnect" style="flex: 1;">连接</button>
                    <button id="btnDisconnect" class="btn-danger" style="display: none; flex: 1;">断开</button>
                </div>
            </div>
            
            <div class="setting-group" style="margin-top: 16px;">
                <label for="themeSelect">主题颜色</label>
                <select id="themeSelect">
                    <option value="light">浅色 (白色)</option>
                    <option value="dark">深色 (黑色)</option>
                </select>
            </div>
        </div>
    </div>

    <!-- 侧边栏及遮罩 -->
    <div id="sidebarOverlay" class="sidebar-overlay"></div>
    <div id="sidebar" class="sidebar">
        <div class="sidebar-header">
            添加组件
        </div>
        <div class="sidebar-content">
            <div class="component-item disabled" id="addTerminalBtn">
                <span>串口终端</span>
                <span class="status" style="font-size:0.8rem; color:var(--success-color);">已添加</span>
            </div>
            <div class="component-item" id="addParamBtn">
                <span>参数调试</span>
                <span class="status" style="font-size:0.8rem; color:var(--primary-color);">+ 添加</span>
            </div>
            <div class="component-item" id="addButtonBtn">
                <span>自定义按钮</span>
                <span class="status" style="font-size:0.8rem; color:var(--primary-color);">+ 添加</span>
            </div>

            <div style="margin-top:auto; display:flex; gap:8px; flex-direction:column; padding-top:20px; border-top:1px solid var(--border-color);">
                <button id="btnExportLayout" class="btn-secondary" style="padding:8px;">导出布局</button>
                <label style="display:flex; gap:8px; align-items:center;">
                    <button id="btnImportLayout" class="btn-secondary" style="padding:8px; flex:1;">导入布局</button>
                    <input id="importLayoutFile" type="file" accept="application/json" style="display:none;">
                </label>
                <button id="btnClearLayout" class="btn-secondary" style="padding:8px; color:var(--danger-color); border-color:var(--danger-color);">清除布局</button>
            </div>
        </div>
    </div>

    <!-- 工作区 -->
    <div class="workspace">
        <!-- 可移动/调整大小的终端窗口 -->
        <div id="terminalWindow" class="floating-window">
            <div class="window-header" id="windowHeader">
                <span class="window-title">串口终端</span>
                <div class="window-controls">
                    <label class="checkbox-group">
                        <input type="checkbox" id="autoScroll" checked> 自动滚动
                    </label>
                    <button id="btnClear" class="btn-secondary" style="padding: 4px 8px; font-size: 0.8rem;">清空</button>
                </div>
            </div>
            <div id="terminal" class="terminal-content"></div>
            <div class="send-panel">
                <div class="input-group" style="position: relative;">
                    <input type="text" id="sendData" placeholder="输入要发送的数据..." disabled style="padding-right: 32px;">
                    <span id="btnClearInput" class="clear-input-btn" title="清空输入">
                        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <polyline points="3 6 5 6 21 6"></polyline>
                            <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
                            <line x1="10" y1="11" x2="10" y2="17"></line>
                            <line x1="14" y1="11" x2="14" y2="17"></line>
                        </svg>
                    </span>
                </div>
                <button id="btnSend" disabled>发送</button>
            </div>
        </div>

        <!-- <div id="paramWindow" ...> 被动态生成替代了 -->
    </div>

    <!-- 右键菜单 -->
    <div id="contextMenu" class="context-menu">
        <div class="menu-item" id="menuFillH">横向填充</div>
        <div class="menu-item" id="menuFillV">纵向填充</div>
        <div class="menu-item" id="menuFillAll">全屏</div>
        <div class="menu-separator" id="menuSep1"></div>
        <div class="menu-item" id="menuLockState">锁定组件</div>
        
        <div class="menu-item" id="menuMoveButton" style="display:none;">调整位置</div>
        <div class="menu-item" id="menuResizeButton" style="display:none;">调整大小</div>
        <div class="menu-item" id="menuConfirmResize" style="display:none; color: var(--primary-color);">确定调整</div>
        
        <div class="menu-item" id="menuSettings">组件设置</div>
        <div class="menu-separator" id="menuSep2"></div>
        <div class="menu-item" id="menuDelete" style="color: var(--danger-color);">删除组件</div>
    </div>

    <!-- 组件设置弹窗 -->
    <div id="componentSettingsModal" class="modal-overlay">
        <div class="modal">
            <div class="modal-header">
                <h3>串口终端 设置</h3>
                <button id="btnCloseModal" class="settings-btn" style="padding: 4px;">
                    <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>
                </button>
            </div>
            <div class="modal-content">
                <div class="setting-group" style="flex-direction: row; justify-content: space-between; align-items: center;">
                    <label for="modalAutoScroll">自动滚动</label>
                    <input type="checkbox" id="modalAutoScroll" checked>
                </div>
                <div class="setting-group">
                    <label for="maxLines">保留的串口信息条数</label>
                    <input type="number" id="maxLines" value="1000" min="100" max="10000">
                </div>
                <div class="setting-group">
                    <label for="displayMode">显示方式</label>
                    <select id="displayMode">
                        <option value="string">字符串</option>
                        <option value="hex">十六进制</option>
                    </select>
                </div>
                <div class="setting-group">
                    <label for="fontSize">终端字体大小 (px)</label>
                    <input type="number" id="fontSize" value="14" min="8" max="48">
                </div>
            </div>
            <div class="modal-footer">
                <button id="btnSaveComponentSettings" style="width: 100%;">保存</button>
            </div>
        </div>
    </div>

    <!-- 参数调试设置弹窗 -->
    <div id="paramSettingsModal" class="modal-overlay">
        <div class="modal" style="width: 600px; max-height: 90vh;">
            <div class="modal-header">
                <h3>参数调试 设置</h3>
                <button id="btnCloseParamModal" class="settings-btn" style="padding: 4px;">
                    <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>
                </button>
            </div>
            <div class="modal-content" style="flex: 1; overflow: hidden; padding-bottom: 0;">
                <div class="setting-group" style="margin-bottom: 12px;">
                    <label for="paramWindowTitle" style="font-weight: 600;">组件标题</label>
                    <input type="text" id="paramWindowTitle" value="参数调试" style="padding: 8px;">
                </div>
                <div class="setting-group checkbox-group" style="margin-bottom: 16px;">
                    <input type="checkbox" id="paramSendOnRelease" checked>
                    <label for="paramSendOnRelease">松开滑条后才发送指令 (关闭则滑动时实时发送)</label>
                </div>
                <div class="setting-group" style="margin-bottom: 16px;">
                    <label for="globalParamCmd" style="font-weight: 600;">总体调试指令模板</label>
                    <div style="font-size: 0.8rem; color: var(--text-muted); margin-bottom: 4px;">例如: <code>PID {V1} {V2} {V3}</code>，其中参数1的值会自动替换<code>{V1}</code></div>
                    <input type="text" id="globalParamCmd" value="CMD {V1}" style="padding: 8px;">
                </div>
                <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; border-top: 1px solid var(--border-color); padding-top: 16px;">
                    <div style="font-weight: 600;">参数列表</div>
                    <button id="btnAddParamConfig" class="btn-secondary" style="padding: 4px 12px; font-size: 0.85rem;">+ 添加参数</button>
                </div>
                <!-- 动态生成的参数配置区域 -->
                <div id="paramConfigs"></div>
            </div>
            <div class="modal-footer">
                <button id="btnSaveParamSettings" style="width: 100%;">保存</button>
            </div>
        </div>
    </div>

    <!-- 按钮组件设置弹窗 -->
    <div id="buttonSettingsModal" class="modal-overlay">
        <div class="modal" style="width: 480px;">
            <div class="modal-header">
                <h3>按钮组件 设置</h3>
                <button id="btnCloseButtonModal" class="settings-btn" style="padding: 4px;">
                    <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>
                </button>
            </div>
            <div class="modal-content">
                <div class="setting-group">
                    <label for="btnWindowTitle">组件标题</label>
                    <input type="text" id="btnWindowTitle" value="自定义按钮">
                </div>
                <div class="setting-group">
                    <label for="btnDisplayTxt">按钮显示内容</label>
                    <input type="text" id="btnDisplayTxt" value="点击发送">
                </div>
                <div class="setting-group">
                    <label for="btnCmdTxt">按下发送的指令</label>
                    <input type="text" id="btnCmdTxt" value="COMMAND">
                </div>
                
                <div class="cfg-grid" style="margin-top: 8px;">
                    <div class="setting-group">
                        <label for="btnWidth">按钮宽度 (px)</label>
                        <input type="number" id="btnWidth" placeholder="例：120">
                    </div>
                    <div class="setting-group">
                        <label for="btnHeight">按钮高度 (px)</label>
                        <input type="number" id="btnHeight" placeholder="例：44">
                    </div>
                    <div class="setting-group">
                        <label for="btnPosX">X 坐标 (px)</label>
                        <input type="number" id="btnPosX" placeholder="例：150">
                    </div>
                    <div class="setting-group">
                        <label for="btnPosY">Y 坐标 (px)</label>
                        <input type="number" id="btnPosY" placeholder="例：150">
                    </div>
                </div>
                <div class="setting-group">
                    <label for="btnFontSize">按钮字体大小 (rem 或 px)</label>
                    <input type="text" id="btnFontSize" placeholder="例如：1rem 或 16px">
                </div>

                <div class="setting-group checkbox-group" style="margin-top: 8px;">
                    <input type="checkbox" id="btnDisableOnClick">
                    <label for="btnDisableOnClick">按下后禁用按钮</label>
                </div>
                
                <div id="btnReenableContainer" style="display: none; background-color: var(--bg-color); padding: 12px; border-radius: 6px; border: 1px solid var(--border-color); flex-direction: column; gap: 12px;">
                    <div class="setting-group">
                        <label for="btnReenableCond">恢复可用条件</label>
                        <select id="btnReenableCond">
                            <option value="none">无(手动点击解锁/不自动恢复)</option>
                            <option value="delay">等待延时时间</option>
                            <option value="receive">串口收到指定命令</option>
                        </select>
                    </div>
                    <div class="setting-group" id="btnDelayContainer" style="display: none;">
                        <label for="btnDelayMs">延时时间 (毫秒)</label>
                        <input type="number" id="btnDelayMs" value="1000" min="0">
                    </div>
                    <div class="setting-group" id="btnRecvContainer" style="display: none;">
                        <label for="btnReceiveCmd">要收到的命令 (包含匹配)</label>
                        <input type="text" id="btnReceiveCmd" value="OK">
                    </div>
                </div>
            </div>
            <div class="modal-footer">
                <button id="btnSaveButtonSettings" style="width: 100%;">保存</button>
            </div>
        </div>
    </div>

    <script>
        const wsUrlInput = document.getElementById('wsUrl');
        const btnConnect = document.getElementById('btnConnect');
        const btnDisconnect = document.getElementById('btnDisconnect');
        const statusBadge = document.getElementById('statusBadge');
        const statusText = document.getElementById('statusText');
        const terminal = document.getElementById('terminal');
        const btnClear = document.getElementById('btnClear');
        const autoScrollCheckbox = document.getElementById('autoScroll');
        const sendDataInput = document.getElementById('sendData');
        const btnClearInput = document.getElementById('btnClearInput');
        const btnSend = document.getElementById('btnSend');
        const btnSettingsToggle = document.getElementById('btnSettingsToggle');
        const settingsSidebar = document.getElementById('settingsSidebar');
        const themeSelect = document.getElementById('themeSelect');

        let ws = null;

        // --- Settings / Theme Toggle ---
        let maxTerminalLines = 1000;
        let displayModeHex = false;
        let terminalFontSize = 14;

        function toggleSettingsSidebar() {
            settingsSidebar.classList.toggle('active');
            sidebarOverlay.classList.toggle('active');
        }

        btnSettingsToggle.addEventListener('click', toggleSettingsSidebar);

        themeSelect.addEventListener('change', (e) => {
            document.documentElement.setAttribute('data-theme', e.target.value);
        });

        // --- Sidebar Logic ---
        const btnAddToggle = document.getElementById('btnAddToggle');
        const sidebar = document.getElementById('sidebar');
        sidebarOverlay = document.getElementById('sidebarOverlay');
        const workspace = document.querySelector('.workspace');

        function toggleSidebar() {
            sidebar.classList.toggle('active');
            sidebarOverlay.classList.toggle('active');
        }

        btnAddToggle.addEventListener('click', toggleSidebar);
        sidebarOverlay.addEventListener('click', () => {
            if (sidebar.classList.contains('active')) toggleSidebar();
            if (settingsSidebar.classList.contains('active')) toggleSettingsSidebar();
        });

        // --- Component Status logic ---
        const addTerminalBtn = document.getElementById('addTerminalBtn');

        addTerminalBtn.addEventListener('click', () => {
            if (addTerminalBtn.classList.contains('disabled')) return;
            document.getElementById('terminalWindow').style.display = 'flex';
            addTerminalBtn.classList.add('disabled');
            addTerminalBtn.querySelector('.status').textContent = '已添加';
            addTerminalBtn.querySelector('.status').style.color = 'var(--success-color)';
            saveLayoutToLocal();
        });

        // 参数添加按钮的逻辑被后面覆盖，先注释掉原来的：
        /*
        const addParamBtn = document.getElementById('addParamBtn');
        const paramWindow = document.getElementById('paramWindow');
        addParamBtn.addEventListener('click', () => {
            if (addParamBtn.classList.contains('disabled')) return;
            paramWindow.style.display = 'flex';
            addParamBtn.classList.add('disabled');
            addParamBtn.querySelector('.status').textContent = '已添加';
            addParamBtn.querySelector('.status').style.color = 'var(--success-color)';
        });
        */
        
        // --- Window Dragging Logic ---
        const terminalWindow = document.getElementById('terminalWindow');
        const windowHeader = document.getElementById('windowHeader');
        // const paramWindowHeader = document.getElementById('paramWindowHeader');
        
        let isDragging = false, startX, startY, startLeft, startTop, activeWindow = null;

        function initDraggable(header, win) {
            header.addEventListener('mousedown', (e) => {
                if (e.target.tagName.toLowerCase() === 'button' || e.target.tagName.toLowerCase() === 'input' || e.target.tagName.toLowerCase() === 'label') {
                    return;
                }
                if (win.dataset.locked === 'true') {
                    return; // 锁定状态不可拖动
                }

                isDragging = true;
                activeWindow = win;
                
                // 将当前窗口置于顶层
                document.querySelectorAll('.floating-window').forEach(w => w.style.zIndex = '10');
                win.style.zIndex = '11';

                startX = e.clientX;
                startY = e.clientY;
                startLeft = win.offsetLeft;
                startTop = win.offsetTop;
                
                document.addEventListener('mousemove', onMouseMove);
                document.addEventListener('mouseup', onMouseUp);
            });

            const resizeObserver = new ResizeObserver(() => {
                const rect = win.getBoundingClientRect();
                const maxX = window.innerWidth - rect.width;
                const maxY = window.innerHeight - rect.height;
                
                let newLeft = win.offsetLeft;
                let newTop = win.offsetTop;

                let changed = false;
                if (newLeft > maxX && maxX > 0) { newLeft = maxX; changed = true; }
                if (newTop > maxY && maxY > 0) { newTop = maxY; changed = true; }

                if (changed) {
                    win.style.left = `${newLeft}px`;
                    win.style.top = `${newTop}px`;
                }
            });
            // observe once appended to dom or here (it's already in DOM for terminal window)
            resizeObserver.observe(win);
        }

        initDraggable(windowHeader, terminalWindow);
        // initDraggable(paramWindowHeader, paramWindow);

        function onMouseMove(e) {
            if (!isDragging || !activeWindow) return;
            const dx = e.clientX - startX;
            const dy = e.clientY - startY;
            
            let newLeft = startLeft + dx;
            let newTop = startTop + dy;

            // 边界限制
            const maxX = window.innerWidth - activeWindow.offsetWidth;
            const maxY = window.innerHeight - activeWindow.offsetHeight - 64; 

            if (newLeft < 0) newLeft = 0;
            if (newLeft > maxX) newLeft = maxX;
            if (newTop < 0) newTop = 0;
            if (newTop > maxY) newTop = maxY;

            activeWindow.style.left = `${newLeft}px`;
            activeWindow.style.top = `${newTop}px`;
        }

        function onMouseUp() {
            if (!isDragging) return;
            isDragging = false;
            activeWindow = null;
            document.removeEventListener('mousemove', onMouseMove);
            document.removeEventListener('mouseup', onMouseUp);
            saveLayoutToLocal();
        }

        // --- Context Menu Logic ---
        const contextMenu = document.getElementById('contextMenu');
        let currentContextMenuWindow = null;
        let isButtonMoving = false;
        let movingButtonWin = null;
        
        function initContextMenu(win) {
            win.addEventListener('contextmenu', (e) => {
                e.preventDefault();
                if (isButtonMoving) return;
                
                currentContextMenuWindow = win;
                const isBtn = win.id.startsWith('btn_win_');
                
                // 控制不适用的菜单项
                document.getElementById('menuFillH').style.display = isBtn ? 'none' : 'block';
                document.getElementById('menuFillV').style.display = isBtn ? 'none' : 'block';
                document.getElementById('menuFillAll').style.display = isBtn ? 'none' : 'block';
                document.getElementById('menuSep1').style.display = isBtn ? 'none' : 'block';
                document.getElementById('menuLockState').style.display = isBtn ? 'none' : 'block';
                
                if (isBtn) {
                    if (win.dataset.resizing === 'true') {
                        document.getElementById('menuMoveButton').style.display = 'none';
                        document.getElementById('menuResizeButton').style.display = 'none';
                        document.getElementById('menuSettings').style.display = 'none';
                        document.getElementById('menuDelete').style.display = 'none';
                        document.getElementById('menuSep2').style.display = 'none';
                        document.getElementById('menuConfirmResize').style.display = 'block';
                    } else {
                        document.getElementById('menuMoveButton').style.display = 'block';
                        document.getElementById('menuResizeButton').style.display = 'block';
                        document.getElementById('menuSettings').style.display = 'block';
                        document.getElementById('menuDelete').style.display = 'block';
                        document.getElementById('menuSep2').style.display = 'block';
                        document.getElementById('menuConfirmResize').style.display = 'none';
                    }
                } else {
                    document.getElementById('menuMoveButton').style.display = 'none';
                    document.getElementById('menuResizeButton').style.display = 'none';
                    document.getElementById('menuConfirmResize').style.display = 'none';
                    document.getElementById('menuSettings').style.display = 'block';
                    document.getElementById('menuDelete').style.display = 'block';
                    document.getElementById('menuSep2').style.display = 'block';
                    
                    const lockBtn = document.getElementById('menuLockState');
                    if (win.dataset.locked === 'true') {
                        lockBtn.textContent = '解锁组件';
                    } else {
                        lockBtn.textContent = '锁定组件';
                    }
                }

                contextMenu.style.display = 'flex';
                
                let menuX = e.clientX;
                let menuY = e.clientY;
                
                if (menuX + 150 > window.innerWidth) menuX = window.innerWidth - 150;
                if (menuY + 160 > window.innerHeight) menuY = window.innerHeight - 160;
                
                contextMenu.style.left = `${menuX}px`;
                contextMenu.style.top = `${menuY}px`;
            });
        }
        
        initContextMenu(terminalWindow);
        // initContextMenu(paramWindow);

        document.addEventListener('click', (e) => {
            if (!e.target.closest('#contextMenu')) {
                contextMenu.style.display = 'none';
            }
        });

        document.getElementById('menuFillH').addEventListener('click', () => {
            if(currentContextMenuWindow && currentContextMenuWindow.dataset.locked !== 'true') {
                currentContextMenuWindow.style.left = '0px';
                currentContextMenuWindow.style.width = '100vw';
            }
            contextMenu.style.display = 'none';
        });

        document.getElementById('menuFillV').addEventListener('click', () => {
            if(currentContextMenuWindow && currentContextMenuWindow.dataset.locked !== 'true') {
                currentContextMenuWindow.style.top = '0px';
                currentContextMenuWindow.style.height = `calc(100vh - 64px)`;
            }
            contextMenu.style.display = 'none';
        });

        document.getElementById('menuFillAll').addEventListener('click', () => {
            if(currentContextMenuWindow && currentContextMenuWindow.dataset.locked !== 'true') {
                currentContextMenuWindow.style.left = '0px';
                currentContextMenuWindow.style.top = '0px';
                currentContextMenuWindow.style.width = '100vw';
                currentContextMenuWindow.style.height = `calc(100vh - 64px)`;
            }
            contextMenu.style.display = 'none';
        });

        document.getElementById('menuLockState').addEventListener('click', () => {
            if (currentContextMenuWindow) {
                if (currentContextMenuWindow.dataset.locked === 'true') {
                    currentContextMenuWindow.dataset.locked = 'false';
                    currentContextMenuWindow.style.resize = 'both';
                    currentContextMenuWindow.querySelector('.window-header').style.cursor = 'grab';
                } else {
                    currentContextMenuWindow.dataset.locked = 'true';
                    currentContextMenuWindow.style.resize = 'none';
                    currentContextMenuWindow.querySelector('.window-header').style.cursor = 'default';
                }
                saveLayoutToLocal();
            }
            contextMenu.style.display = 'none';
        });

        function onButtonMove(e) {
            if (!isButtonMoving || !movingButtonWin) return;
            const rect = workspaceEl.getBoundingClientRect();
            const w = movingButtonWin.offsetWidth;
            const h = movingButtonWin.offsetHeight;
            movingButtonWin.style.left = (e.clientX - rect.left - w/2) + 'px';
            movingButtonWin.style.top = (e.clientY - rect.top - h/2) + 'px';
        }

        function onButtonMoveEnd(e) {
            if (e.button === 0 && isButtonMoving) {
                isButtonMoving = false;
                document.removeEventListener('mousemove', onButtonMove);
                document.removeEventListener('mousedown', onButtonMoveEnd);
                
                if (movingButtonWin) {
                    movingButtonWin.style.pointerEvents = 'auto';
                    const btn = movingButtonWin.querySelector('button');
                    if (btn) btn.style.pointerEvents = 'auto'; // allow clicks again
                    // check logical disabled State
                    let btnIdx = buttonManager.instances.findIndex(inst => inst.win === movingButtonWin);
                    if (btnIdx !== -1) {
                        let inst = buttonManager.instances[btnIdx];
                        if (inst.win.dataset.disabled !== "true") {
                            btn.disabled = false;
                        }
                    }
                }
                movingButtonWin = null;
                saveLayoutToLocal();
            }
        }

        document.getElementById('menuMoveButton').addEventListener('click', () => {
            contextMenu.style.display = 'none';
            if (!currentContextMenuWindow) return;
            isButtonMoving = true;
            movingButtonWin = currentContextMenuWindow;
            
            movingButtonWin.style.pointerEvents = 'none'; // prevent hover/click states while moving
            
            document.addEventListener('mousemove', onButtonMove);
            document.addEventListener('mousedown', onButtonMoveEnd);
        });

        document.getElementById('menuResizeButton').addEventListener('click', () => {
            contextMenu.style.display = 'none';
            if (!currentContextMenuWindow) return;
            currentContextMenuWindow.dataset.resizing = 'true';
            currentContextMenuWindow.style.resize = 'both';
            currentContextMenuWindow.style.overflow = 'hidden';
            currentContextMenuWindow.style.border = '2px dashed var(--primary-color)';
            
            const btn = currentContextMenuWindow.querySelector('button');
            if (btn) btn.style.pointerEvents = 'none';
        });

        document.getElementById('menuConfirmResize').addEventListener('click', () => {
            contextMenu.style.display = 'none';
            if (!currentContextMenuWindow) return;
            currentContextMenuWindow.dataset.resizing = 'false';
            currentContextMenuWindow.style.resize = 'none';
            currentContextMenuWindow.style.border = 'none';
            currentContextMenuWindow.style.overflow = 'visible';
            
            const btn = currentContextMenuWindow.querySelector('button');
            if (btn) btn.style.pointerEvents = 'auto';
            saveLayoutToLocal();
        });

        document.getElementById('menuDelete').addEventListener('click', () => {
            contextMenu.style.display = 'none';
            if(currentContextMenuWindow === terminalWindow) {
                terminalWindow.style.display = 'none';
                addTerminalBtn.classList.remove('disabled');
                addTerminalBtn.querySelector('.status').textContent = '未添加';
                addTerminalBtn.querySelector('.status').style.color = 'var(--text-muted)';
            } else {
                let idx = paramManager.instances.findIndex(inst => inst.win === currentContextMenuWindow);
                if (idx !== -1) {
                    workspaceEl.removeChild(currentContextMenuWindow);
                    paramManager.instances.splice(idx, 1);
                } else {
                    let btnIdx = buttonManager.instances.findIndex(inst => inst.win === currentContextMenuWindow);
                    if (btnIdx !== -1) {
                        workspaceEl.removeChild(currentContextMenuWindow);
                        if (buttonManager.instances[btnIdx]._timer) {
                            clearInterval(buttonManager.instances[btnIdx]._timer);
                        }
                        buttonManager.instances.splice(btnIdx, 1);
                    }
                }
            }
            saveLayoutToLocal();
        });

        // --- Component Settings Modal Logic ---
        const modalOverlay = document.getElementById('componentSettingsModal');
        const paramModalOverlay = document.getElementById('paramSettingsModal');
        const buttonSettingsModal = document.getElementById('buttonSettingsModal');
        const btnCloseModal = document.getElementById('btnCloseModal');
        const btnCloseParamModal = document.getElementById('btnCloseParamModal');
        const btnCloseButtonModal = document.getElementById('btnCloseButtonModal');
        const btnSaveComponentSettings = document.getElementById('btnSaveComponentSettings');
        const modalAutoScroll = document.getElementById('modalAutoScroll');
        const modalMaxLines = document.getElementById('maxLines');
        const modalDisplayMode = document.getElementById('displayMode');
        const modalFontSize = document.getElementById('fontSize');

        document.getElementById('menuSettings').addEventListener('click', () => {
            contextMenu.style.display = 'none';
            if (currentContextMenuWindow === terminalWindow) {
                // Sync values to modal
                modalAutoScroll.checked = autoScrollCheckbox.checked;
                modalMaxLines.value = maxTerminalLines;
                modalDisplayMode.value = displayModeHex ? 'hex' : 'string';
                modalFontSize.value = terminalFontSize;
                
                modalOverlay.classList.add('active');
            } else {
                let idx = paramManager.instances.findIndex(inst => inst.win === currentContextMenuWindow);
                if (idx !== -1) {
                    currentEditingInst = paramManager.instances[idx];
                    renderParamConfigList();
                    paramModalOverlay.classList.add('active');
                } else {
                    let btnIdx = buttonManager.instances.findIndex(inst => inst.win === currentContextMenuWindow);
                    if (btnIdx !== -1) {
                        currentEditingBtnInst = buttonManager.instances[btnIdx];
                        renderButtonSettingsModal();
                        buttonSettingsModal.classList.add('active');
                    }
                }
            }
        });

        const closeModal = () => modalOverlay.classList.remove('active');
        const closeParamModal = () => paramModalOverlay.classList.remove('active');
        const closeButtonModal = () => buttonSettingsModal.classList.remove('active');
        btnCloseModal.addEventListener('click', closeModal);
        btnCloseParamModal.addEventListener('click', closeParamModal);
        btnCloseButtonModal.addEventListener('click', closeButtonModal);
        
        modalOverlay.addEventListener('click', (e) => {
            if (e.target === modalOverlay) closeModal();
        });
        paramModalOverlay.addEventListener('click', (e) => {
            if (e.target === paramModalOverlay) closeParamModal();
        });
        buttonSettingsModal.addEventListener('click', (e) => {
            if (e.target === buttonSettingsModal) closeButtonModal();
        });

        btnSaveComponentSettings.addEventListener('click', () => {
            autoScrollCheckbox.checked = modalAutoScroll.checked;
            maxTerminalLines = parseInt(modalMaxLines.value, 10) || 1000;
            displayModeHex = modalDisplayMode.value === 'hex';
            
            terminalFontSize = parseInt(modalFontSize.value, 10) || 14;
            terminal.style.fontSize = `${terminalFontSize}px`;
            
            closeModal();
            
            // 立即裁剪当前内容以符合新行数
            while (terminal.children.length > maxTerminalLines) {
                terminal.removeChild(terminal.firstChild);
            }
            saveLayoutToLocal();
        });

        // Sync main window checkbox backward
        autoScrollCheckbox.addEventListener('change', (e) => {
            modalAutoScroll.checked = e.target.checked;
        });

        // --- Param Component Logic ---
        let paramManager = {
            instances: []
        };
        
        let currentEditingInst = null;

        // 我们需要能多次添加参数组件实例
        const workspaceEl = document.querySelector('.workspace');

        function createParamWindow() {
            const winId = 'param_win_' + Date.now();
            const win = document.createElement('div');
            win.className = 'floating-window';
            win.id = winId;
            win.style.left = '100px';
            win.style.top = '100px';
            win.style.width = '400px';
            win.style.height = '500px';
            win.style.zIndex = '10';
            
            win.innerHTML = `
                <div class="window-header">
                    <span class="window-title">参数调试</span>
                </div>
                <div class="param-list"></div>
            `;
            
            workspaceEl.appendChild(win);
            
            const header = win.querySelector('.window-header');
            initDraggable(header, win);
            initContextMenu(win);
            
            const paramListEl = win.querySelector('.param-list');
            
            const newInst = {
                win: win,
                listContainer: paramListEl,
                globalCmd: "PID {V1}",
                sendOnRelease: true,
                title: "参数调试",
                configs: [
                    { id: Date.now(), name: "参数 1", min: 0, max: 100, step: 1, val: 50 }
                ]
            };
            
            paramManager.instances.push(newInst);
            renderParamUI(newInst);
            saveLayoutToLocal();
        }

        const addParamBtnNew = document.getElementById('addParamBtn');
        addParamBtnNew.addEventListener('click', () => {
            createParamWindow();
        });

        // 第二个 menuDelete 监听被移至顶部统一定义了。

        // 取消旧的单个paramWindow的相关逻辑
        // 我们会用 currentContextMenuWindow 检测

        const paramConfigsContainer = document.getElementById('paramConfigs');
        const btnSaveParamSettings = document.getElementById('btnSaveParamSettings');
        const btnAddParamConfig = document.getElementById('btnAddParamConfig');

        // Render in Modal
        function renderParamConfigList() {
            if (!currentEditingInst) return;
            document.getElementById('paramWindowTitle').value = currentEditingInst.title !== undefined ? currentEditingInst.title : '参数调试';
            document.getElementById('paramSendOnRelease').checked = currentEditingInst.sendOnRelease !== undefined ? currentEditingInst.sendOnRelease : true;
            document.getElementById('globalParamCmd').value = currentEditingInst.globalCmd;
            paramConfigsContainer.innerHTML = '';
            currentEditingInst.configs.forEach((cfg, index) => {
                const div = document.createElement('div');
                div.className = 'cfg-item';
                div.id = `cfg_div_${cfg.id}`;
                div.innerHTML = `
                    <div class="cfg-header">
                        <div style="display:flex; align-items:center; gap:8px;">
                            <span style="font-weight:600; color:var(--primary-color);">V${index + 1}</span>
                            <input type="text" id="cfg_name_${cfg.id}" class="cfg-name" value="${cfg.name}" placeholder="参数名称" />
                        </div>
                        <button class="cfg-delete-btn" title="删除参数" onclick="deleteParamConfig(${cfg.id})">
                            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path></svg>
                        </button>
                    </div>
                    <div class="cfg-grid">
                        <div class="setting-group">
                            <label style="font-size:0.8rem; font-weight:normal;">最小值</label>
                            <input type="number" id="cfg_min_${cfg.id}" value="${cfg.min}" />
                        </div>
                        <div class="setting-group">
                            <label style="font-size:0.8rem; font-weight:normal;">最大值</label>
                            <input type="number" id="cfg_max_${cfg.id}" value="${cfg.max}" />
                        </div>
                        <div class="setting-group">
                            <label style="font-size:0.8rem; font-weight:normal;">步进值</label>
                            <input type="number" id="cfg_step_${cfg.id}" value="${cfg.step}" />
                        </div>
                    </div>
                `;
                paramConfigsContainer.appendChild(div);
            });
        }

        window.deleteParamConfig = function(id) {
            if (!currentEditingInst) return;
            currentEditingInst.configs = currentEditingInst.configs.filter(c => c.id !== id);
            renderParamConfigList();
        }

        btnAddParamConfig.addEventListener('click', () => {
            if (!currentEditingInst) return;
            const newIndex = currentEditingInst.configs.length + 1;
            currentEditingInst.configs.push({
                id: Date.now(), name: `新参数 ${newIndex}`, min: 0, max: 100, step: 1, val: 50
            });
            renderParamConfigList();
            
            // let it scroll to bottom
            setTimeout(() => {
                paramConfigsContainer.scrollTop = paramConfigsContainer.scrollHeight;
            }, 50);
        });

        btnSaveParamSettings.addEventListener('click', () => {
            if (!currentEditingInst) return;
            currentEditingInst.globalCmd = document.getElementById('globalParamCmd').value || "";
            currentEditingInst.title = document.getElementById('paramWindowTitle').value || "参数调试";
            currentEditingInst.sendOnRelease = document.getElementById('paramSendOnRelease').checked;
            
            const titleEl = currentEditingInst.win.querySelector('.window-title');
            if (titleEl) titleEl.textContent = currentEditingInst.title;
            
            // 保存配置
            currentEditingInst.configs.forEach(cfg => {
                const nameInput = document.getElementById(`cfg_name_${cfg.id}`);
                const minInput = document.getElementById(`cfg_min_${cfg.id}`);
                const maxInput = document.getElementById(`cfg_max_${cfg.id}`);
                const stepInput = document.getElementById(`cfg_step_${cfg.id}`);
                
                if (nameInput) cfg.name = nameInput.value;
                if (minInput) cfg.min = Number(minInput.value);
                if (maxInput) cfg.max = Number(maxInput.value);
                if (stepInput) cfg.step = Number(stepInput.value);
                
                // 修正当前值范围
                if (cfg.val < cfg.min) cfg.val = cfg.min;
                if (cfg.val > cfg.max) cfg.val = cfg.max;
            });
            closeParamModal();
            renderParamUI(currentEditingInst);
            saveLayoutToLocal();
        });

        function sendGlobalParamCommand(inst) {
            if (!ws || ws.readyState !== WebSocket.OPEN || !inst.globalCmd) return;
            
            let cmdStr = inst.globalCmd;
            inst.configs.forEach((cfg, index) => {
                const placeholder = `{V${index + 1}}`;
                // replace all occurrences of this placeholder
                cmdStr = cmdStr.split(placeholder).join(cfg.val.toString());
            });
            
            ws.send(cmdStr);
            appendLog(`${cmdStr}`, 'send');
        }

        function renderParamUI(inst) {
            inst.listContainer.innerHTML = '';
            inst.configs.forEach((cfg) => {
                const item = document.createElement('div');
                item.className = 'param-item';
                item.innerHTML = `
                    <div class="param-header">
                        <span>${cfg.name}</span>
                        <span class="param-val" id="pval_${inst.win.id}_${cfg.id}">${cfg.val}</span>
                    </div>
                    <div class="param-controls">
                        <button class="param-btn" id="pbtn_minus_${inst.win.id}_${cfg.id}">-</button>
                        <input type="range" class="param-slider" id="pslider_${inst.win.id}_${cfg.id}" min="${cfg.min}" max="${cfg.max}" step="${cfg.step}" value="${cfg.val}">
                        <button class="param-btn" id="pbtn_plus_${inst.win.id}_${cfg.id}">+</button>
                    </div>
                `;
                inst.listContainer.appendChild(item);

                const valView = item.querySelector(`#pval_${inst.win.id}_${cfg.id}`);
                const slider = item.querySelector(`#pslider_${inst.win.id}_${cfg.id}`);
                const btnMinus = item.querySelector(`#pbtn_minus_${inst.win.id}_${cfg.id}`);
                const btnPlus = item.querySelector(`#pbtn_plus_${inst.win.id}_${cfg.id}`);

                const updateAndSend = (newVal) => {
                    newVal = Number(newVal);
                    if (newVal < cfg.min) newVal = cfg.min;
                    if (newVal > cfg.max) newVal = cfg.max;
                    
                    cfg.val = newVal;
                    
                    if(slider) slider.value = newVal;
                    if(valView) valView.textContent = newVal;

                    sendGlobalParamCommand(inst);
                };

                const updateValOnly = (newVal) => {
                    newVal = Number(newVal);
                    if (newVal < cfg.min) newVal = cfg.min;
                    if (newVal > cfg.max) newVal = cfg.max;
                    cfg.val = newVal;
                    if(slider) slider.value = newVal;
                    if(valView) valView.textContent = newVal;
                };

                if (slider) {
                    slider.addEventListener('input', (e) => {
                        const sendOnRelease = inst.sendOnRelease !== undefined ? inst.sendOnRelease : true;
                        if (sendOnRelease) {
                            // 预显示和存储数据，但不发送
                            updateValOnly(e.target.value);
                        } else {
                            // 实时发送
                            updateAndSend(e.target.value);
                        }
                    });
                    
                    slider.addEventListener('change', (e) => {
                        const sendOnRelease = inst.sendOnRelease !== undefined ? inst.sendOnRelease : true;
                        if (sendOnRelease) {
                            updateAndSend(e.target.value);
                        }
                    });
                }

                if (btnMinus) {
                    btnMinus.addEventListener('click', () => {
                        updateAndSend(cfg.val - cfg.step);
                    });
                }

                if (btnPlus) {
                    btnPlus.addEventListener('click', () => {
                        updateAndSend(cfg.val + cfg.step);
                    });
                }
            });
        }
        
        // --- Button Component Logic ---
        let buttonManager = {
            instances: []
        };
        let currentEditingBtnInst = null;

        function createButtonWindow() {
            const winId = 'btn_win_' + Date.now();
            const win = document.createElement('div');
            win.className = 'floating-window';
            win.id = winId;
            win.style.left = '150px';
            win.style.top = '150px';
            win.style.width = '120px';
            win.style.height = '44px';
            win.style.minWidth = '40px';
            win.style.minHeight = '30px';
            win.style.zIndex = '100'; // Make default on top
            win.style.background = 'transparent';
            win.style.border = 'none';
            win.style.boxShadow = 'none';
            win.style.resize = 'none';
            win.style.overflow = 'visible';
            
            win.innerHTML = `
                <button class="custom-cmd-btn" style="width:100%; height:100%; font-size:1rem; box-shadow:0 4px 6px rgba(0,0,0,0.1); margin:0; padding:0; display:block; outline:none; border-radius:6px; cursor:pointer;" title="操作提示\n右键调整位置/大小/设置"></button>
            `;
            
            workspaceEl.appendChild(win);
            
            initContextMenu(win);
            
            const btnEl = win.querySelector('.custom-cmd-btn');
            
            const newInst = {
                win: win,
                btnEl: btnEl,
                title: "自定义按钮",
                btnText: "点击发送",
                cmd: "COMMAND",
                disableOnClick: false,
                reenableCondition: 'none',
                delayMs: 1000,
                receiveCmd: "OK",
                _timer: null // For delay condition
            };
            
            buttonManager.instances.push(newInst);
            renderButtonUI(newInst);
            saveLayoutToLocal();
        }

        const addButtonBtn = document.getElementById('addButtonBtn');
        addButtonBtn.addEventListener('click', () => {
            createButtonWindow();
        });

        function renderButtonUI(inst) {
            const titleEl = inst.win.querySelector('.window-title');
            if (titleEl) titleEl.textContent = inst.title;
            
            inst.btnEl.textContent = inst.btnText;
            
            // Clean up old listeners by recreation or standard way? Easier to just replace with clone.
            const newBtn = inst.btnEl.cloneNode(true);
            inst.btnEl.parentNode.replaceChild(newBtn, inst.btnEl);
            inst.btnEl = newBtn;
            
            // If it was still disabled from previous states, and we turned off disableOnClick, re-enable it
            if (!inst.disableOnClick) {
                inst.btnEl.disabled = false;
                inst.win.dataset.disabled = "false";
            }

            inst.btnEl.addEventListener('click', () => {
                if (inst.btnEl.disabled) return;
                
                // send command
                if (ws && ws.readyState === WebSocket.OPEN && inst.cmd) {
                    ws.send(inst.cmd);
                    appendLog(inst.cmd, 'send');
                }
                
                if (inst.disableOnClick) {
                    inst.btnEl.disabled = true;
                    inst.win.dataset.disabled = "true";
                    
                    if (inst.reenableCondition === 'delay') {
                        if (inst._timer) clearTimeout(inst._timer);
                        let remaining = Math.ceil(inst.delayMs / 1000);
                        inst.btnEl.textContent = remaining + "s 后恢复";
                        
                        inst._timer = setInterval(() => {
                            remaining--;
                            if (remaining > 0) {
                                inst.btnEl.textContent = remaining + "s 后恢复";
                            } else {
                                clearInterval(inst._timer);
                                inst._timer = null;
                                inst.btnEl.disabled = false;
                                inst.btnEl.textContent = inst.btnText;
                                inst.win.dataset.disabled = "false";
                            }
                        }, 1000);
                        // Also handle case where delayMs is less than 1000ms
                        if (inst.delayMs < 1000) {
                            clearInterval(inst._timer);
                            inst._timer = setTimeout(() => {
                                inst.btnEl.disabled = false;
                                inst.btnEl.textContent = inst.btnText;
                                inst.win.dataset.disabled = "false";
                            }, inst.delayMs);
                        }
                    } else if (inst.reenableCondition === 'receive') {
                        inst.btnEl.textContent = "等待回传...";
                    }
                }
            });
        }

        // --- Layout save/load/export/import ---
        function getLayoutData() {
            const layout = {
                terminal: {
                    left: terminalWindow.style.left || terminalWindow.offsetLeft + 'px',
                    top: terminalWindow.style.top || terminalWindow.offsetTop + 'px',
                    width: terminalWindow.style.width || terminalWindow.offsetWidth + 'px',
                    height: terminalWindow.style.height || terminalWindow.offsetHeight + 'px',
                    display: terminalWindow.style.display || '',
                    locked: terminalWindow.dataset.locked === 'true'
                },
                params: paramManager.instances.map(inst => ({
                    left: inst.win.style.left || inst.win.offsetLeft + 'px',
                    top: inst.win.style.top || inst.win.offsetTop + 'px',
                    width: inst.win.style.width || inst.win.offsetWidth + 'px',
                    height: inst.win.style.height || inst.win.offsetHeight + 'px',
                    locked: inst.win.dataset.locked === 'true',
                    title: inst.title,
                    globalCmd: inst.globalCmd,
                    sendOnRelease: inst.sendOnRelease,
                    configs: inst.configs.map(c => ({ id: c.id, name: c.name, min: c.min, max: c.max, step: c.step, val: c.val }))
                })),
                buttons: buttonManager.instances.map(inst => ({
                    left: inst.win.style.left || inst.win.offsetLeft + 'px',
                    top: inst.win.style.top || inst.win.offsetTop + 'px',
                    width: inst.win.style.width || inst.win.offsetWidth + 'px',
                    height: inst.win.style.height || inst.win.offsetHeight + 'px',
                    fontSize: inst.btnEl.style.fontSize,
                    locked: inst.win.dataset.locked === 'true',
                    title: inst.title,
                    btnText: inst.btnText,
                    cmd: inst.cmd,
                    disableOnClick: inst.disableOnClick,
                    reenableCondition: inst.reenableCondition,
                    delayMs: inst.delayMs,
                    receiveCmd: inst.receiveCmd
                }))
            };
            return layout;
        }

        function saveLayoutToLocal() {
            try {
                const data = getLayoutData();
                localStorage.setItem('wsa_layout_v1', JSON.stringify(data));
            } catch (e) {
                appendLog('保存布局失败: ' + e, 'error');
            }
        }

        function clearExistingDynamic() {
            // remove param windows
            paramManager.instances.forEach(inst => {
                if (inst.win && inst.win.parentNode) inst.win.parentNode.removeChild(inst.win);
            });
            paramManager.instances = [];
            // remove button windows
            buttonManager.instances.forEach(inst => {
                if (inst._timer) clearInterval(inst._timer);
                if (inst.win && inst.win.parentNode) inst.win.parentNode.removeChild(inst.win);
            });
            buttonManager.instances = [];
        }

        function resetTerminalToDefault() {
            terminalWindow.style.display = 'flex';
            terminalWindow.style.left = '50px';
            terminalWindow.style.top = '40px';
            terminalWindow.style.width = '600px';
            terminalWindow.style.height = '450px';
            terminalWindow.style.resize = 'both';
            terminalWindow.style.overflow = 'hidden';
            terminalWindow.style.border = '1px solid var(--border-color)';
            terminalWindow.dataset.locked = 'false';
            delete terminalWindow.dataset.resizing;
            const header = terminalWindow.querySelector('.window-header');
            if (header) header.style.cursor = 'grab';

            const addTerminalBtn = document.getElementById('addTerminalBtn');
            addTerminalBtn.classList.add('disabled');
            addTerminalBtn.querySelector('.status').textContent = '已添加';
            addTerminalBtn.querySelector('.status').style.color = 'var(--success-color)';
        }

        function loadLayoutFromObject(obj) {
            try {
                clearExistingDynamic();
                
                // restore terminal
                if (obj.terminal) {
                    if (obj.terminal.left) terminalWindow.style.left = obj.terminal.left;
                    if (obj.terminal.top) terminalWindow.style.top = obj.terminal.top;
                    if (obj.terminal.width) terminalWindow.style.width = obj.terminal.width;
                    if (obj.terminal.height) terminalWindow.style.height = obj.terminal.height;
                    terminalWindow.dataset.locked = obj.terminal.locked ? 'true' : 'false';
                    terminalWindow.style.resize = terminalWindow.dataset.locked === 'true' ? 'none' : 'both';
                    const terminalHeader = terminalWindow.querySelector('.window-header');
                    if (terminalHeader) terminalHeader.style.cursor = terminalWindow.dataset.locked === 'true' ? 'default' : 'grab';
                    
                    if (obj.terminal.display === 'none') {
                        terminalWindow.style.display = 'none';
                        const addTerminalBtn = document.getElementById('addTerminalBtn');
                        addTerminalBtn.classList.remove('disabled');
                        addTerminalBtn.querySelector('.status').textContent = '未添加';
                        addTerminalBtn.querySelector('.status').style.color = 'var(--text-muted)';
                    } else {
                        terminalWindow.style.display = 'flex';
                        const addTerminalBtn = document.getElementById('addTerminalBtn');
                        addTerminalBtn.classList.add('disabled');
                        addTerminalBtn.querySelector('.status').textContent = '已添加';
                        addTerminalBtn.querySelector('.status').style.color = 'var(--success-color)';
                    }
                }

                // restore params
                if (Array.isArray(obj.params)) {
                    obj.params.forEach(s => {
                        const winId = 'param_win_' + Date.now() + Math.floor(Math.random()*1000);
                        const win = document.createElement('div');
                        win.className = 'floating-window';
                        win.id = winId;
                        win.style.left = s.left || '100px';
                        win.style.top = s.top || '100px';
                        if (s.width) win.style.width = s.width;
                        if (s.height) win.style.height = s.height;
                        win.dataset.locked = s.locked ? 'true' : 'false';
                        win.style.resize = win.dataset.locked === 'true' ? 'none' : 'both';
                        const header = win.querySelector('.window-header');
                        if (header) header.style.cursor = win.dataset.locked === 'true' ? 'default' : 'grab';
                        win.innerHTML = `
                            <div class="window-header">
                                <span class="window-title">${s.title || '参数调试'}</span>
                            </div>
                            <div class="param-list"></div>
                        `;
                        workspaceEl.appendChild(win);
                        initDraggable(win.querySelector('.window-header'), win);
                        initContextMenu(win);
                        const paramListEl = win.querySelector('.param-list');
                        const newInst = {
                            win: win,
                            listContainer: paramListEl,
                            title: s.title || '参数调试',
                            globalCmd: s.globalCmd || 'PID {V1}',
                            sendOnRelease: s.sendOnRelease !== undefined ? s.sendOnRelease : true,
                            configs: Array.isArray(s.configs) ? s.configs.map(c => ({ id: c.id || Date.now()+Math.floor(Math.random()*1000), name: c.name || '参数', min: c.min || 0, max: c.max || 100, step: c.step || 1, val: c.val || 0 })) : []
                        };
                        paramManager.instances.push(newInst);
                        renderParamUI(newInst);
                    });
                }

                // restore buttons
                if (Array.isArray(obj.buttons)) {
                    obj.buttons.forEach(s => {
                        const winId = 'btn_win_' + Date.now() + Math.floor(Math.random()*1000);
                        const win = document.createElement('div');
                        win.className = 'floating-window';
                        win.id = winId;
                        win.style.left = s.left || '150px';
                        win.style.top = s.top || '150px';
                        if (s.width) win.style.width = s.width;
                        if (s.height) win.style.height = s.height;
                        win.style.minWidth = '40px';
                        win.style.minHeight = '30px';
                        win.dataset.locked = s.locked ? 'true' : 'false';
                        win.style.resize = win.dataset.locked === 'true' ? 'none' : 'both';
                        win.style.background = 'transparent';
                        win.style.border = 'none';
                        win.style.boxShadow = 'none';
                        win.style.overflow = 'visible';
                        win.innerHTML = `<button class="custom-cmd-btn" style="width:100%; height:100%; font-size:1rem; box-shadow:0 4px 6px rgba(0,0,0,0.1); margin:0; padding:0; display:block; outline:none; border-radius:6px; cursor:pointer;" title="操作提示\n右键调整位置/大小/设置"></button>`;
                        workspaceEl.appendChild(win);
                        initContextMenu(win);
                        const btnEl = win.querySelector('.custom-cmd-btn');
                        if (s.fontSize) {
                            btnEl.style.fontSize = s.fontSize;
                        }
                        const newInst = {
                            win: win,
                            btnEl: btnEl,
                            title: s.title || '自定义按钮',
                            btnText: s.btnText || '点击发送',
                            cmd: s.cmd || '',
                            disableOnClick: !!s.disableOnClick,
                            reenableCondition: s.reenableCondition || 'none',
                            delayMs: parseInt(s.delayMs,10) || 1000,
                            receiveCmd: s.receiveCmd || '',
                            _timer: null
                        };
                        buttonManager.instances.push(newInst);
                        renderButtonUI(newInst);
                    });
                }
            } catch (e) {
                appendLog('加载布局失败: ' + e, 'error');
            }
        }

        function loadLayoutFromLocal() {
            try {
                const raw = localStorage.getItem('wsa_layout_v1');
                if (!raw) return false;
                const obj = JSON.parse(raw);
                loadLayoutFromObject(obj);
                return true;
            } catch (e) {
                appendLog('从本地加载布局失败: ' + e, 'error');
                return false;
            }
        }

        function exportLayout() {
            const data = getLayoutData();
            const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'wsa_layout.json';
            document.body.appendChild(a);
            a.click();
            a.remove();
            URL.revokeObjectURL(url);
        }

        document.getElementById('btnExportLayout').addEventListener('click', () => {
            exportLayout();
        });

        document.getElementById('btnImportLayout').addEventListener('click', () => {
            document.getElementById('importLayoutFile').click();
        });

        document.getElementById('importLayoutFile').addEventListener('change', (e) => {
            const file = e.target.files && e.target.files[0];
            if (!file) return;
            const reader = new FileReader();
            reader.onload = (ev) => {
                try {
                    const obj = JSON.parse(ev.target.result);
                    loadLayoutFromObject(obj);
                    // save to local automatically
                    localStorage.setItem('wsa_layout_v1', JSON.stringify(obj));
                } catch (err) {
                    appendLog('导入布局文件失败: ' + err, 'error');
                }
            };
            reader.readAsText(file);
            e.target.value = '';
        });

        document.getElementById('btnClearLayout').addEventListener('click', () => {
            if (confirm('清除所有布局（会移除所有动态添加的参数窗口与按钮），确定吗？')) {
                resetTerminalToDefault();
                clearExistingDynamic();
                // 先清理，再保存“仅终端默认布局”，确保刷新后结果稳定
                localStorage.removeItem('wsa_layout_v1');
                saveLayoutToLocal();
                contextMenu.style.display = 'none';
            }
        });

        // Settings Modal Handlers for Button
        const btnDisableOnClickChk = document.getElementById('btnDisableOnClick');
        const btnReenableContainer = document.getElementById('btnReenableContainer');
        const btnReenableCond = document.getElementById('btnReenableCond');
        const btnDelayContainer = document.getElementById('btnDelayContainer');
        const btnRecvContainer = document.getElementById('btnRecvContainer');

        btnDisableOnClickChk.addEventListener('change', (e) => {
            btnReenableContainer.style.display = e.target.checked ? 'flex' : 'none';
        });

        btnReenableCond.addEventListener('change', (e) => {
            btnDelayContainer.style.display = e.target.value === 'delay' ? 'flex' : 'none';
            btnRecvContainer.style.display = e.target.value === 'receive' ? 'flex' : 'none';
        });

        function renderButtonSettingsModal() {
            if (!currentEditingBtnInst) return;
            document.getElementById('btnWindowTitle').value = currentEditingBtnInst.title;
            document.getElementById('btnDisplayTxt').value = currentEditingBtnInst.btnText;
            document.getElementById('btnCmdTxt').value = currentEditingBtnInst.cmd;
            
            document.getElementById('btnWidth').value = parseInt(currentEditingBtnInst.win.style.width) || currentEditingBtnInst.win.offsetWidth || 120;
            document.getElementById('btnHeight').value = parseInt(currentEditingBtnInst.win.style.height) || currentEditingBtnInst.win.offsetHeight || 44;
            document.getElementById('btnPosX').value = parseInt(currentEditingBtnInst.win.style.left) || currentEditingBtnInst.win.offsetLeft || 150;
            document.getElementById('btnPosY').value = parseInt(currentEditingBtnInst.win.style.top) || currentEditingBtnInst.win.offsetTop || 150;
            document.getElementById('btnFontSize').value = currentEditingBtnInst.btnEl.style.fontSize || '1rem';
            
            btnDisableOnClickChk.checked = currentEditingBtnInst.disableOnClick;
            btnReenableCond.value = currentEditingBtnInst.reenableCondition;
            document.getElementById('btnDelayMs').value = currentEditingBtnInst.delayMs;
            document.getElementById('btnReceiveCmd').value = currentEditingBtnInst.receiveCmd;

            // Trigger events to update UI
            btnDisableOnClickChk.dispatchEvent(new Event('change'));
            btnReenableCond.dispatchEvent(new Event('change'));
        }

        document.getElementById('btnSaveButtonSettings').addEventListener('click', () => {
            if (!currentEditingBtnInst) return;
            
            currentEditingBtnInst.title = document.getElementById('btnWindowTitle').value || "自定义按钮";
            currentEditingBtnInst.btnText = document.getElementById('btnDisplayTxt').value || "点击发送";
            currentEditingBtnInst.cmd = document.getElementById('btnCmdTxt').value || "";
            currentEditingBtnInst.disableOnClick = btnDisableOnClickChk.checked;
            currentEditingBtnInst.reenableCondition = btnReenableCond.value;
              currentEditingBtnInst.delayMs = parseInt(document.getElementById('btnDelayMs').value, 10) || 1000;
            currentEditingBtnInst.receiveCmd = document.getElementById('btnReceiveCmd').value || "";
            
            // apply positional / size changes
            const btnWidth = document.getElementById('btnWidth').value;
            const btnHeight = document.getElementById('btnHeight').value;
            const btnPosX = document.getElementById('btnPosX').value;
            const btnPosY = document.getElementById('btnPosY').value;
            const btnFontSize = document.getElementById('btnFontSize').value;
            
            if (btnWidth !== '') currentEditingBtnInst.win.style.width = btnWidth + 'px';
            if (btnHeight !== '') currentEditingBtnInst.win.style.height = btnHeight + 'px';
            if (btnPosX !== '') currentEditingBtnInst.win.style.left = btnPosX + 'px';
            if (btnPosY !== '') currentEditingBtnInst.win.style.top = btnPosY + 'px';
            if (btnFontSize) {
                // handle missing units
                currentEditingBtnInst.btnEl.style.fontSize = isNaN(btnFontSize) && isNaN(parseFloat(btnFontSize)) ? '1rem' : (btnFontSize.includes('px') || btnFontSize.includes('rem') || btnFontSize.includes('em') ? btnFontSize : btnFontSize + 'px');
            }
            
            closeButtonModal();
            renderButtonUI(currentEditingBtnInst);
            saveLayoutToLocal();
        });

        // --- WebSocket Logic ---
        function updateStatus(connected) {
            if (connected) {
                statusBadge.classList.add('connected');
                statusText.textContent = '已连接';
                btnConnect.style.display = 'none';
                btnDisconnect.style.display = 'inline-flex';
                wsUrlInput.disabled = true;
                
                sendDataInput.disabled = false;
                btnSend.disabled = false;
            } else {
                statusBadge.classList.remove('connected');
                statusText.textContent = '未连接';
                btnConnect.style.display = 'inline-flex';
                btnDisconnect.style.display = 'none';
                wsUrlInput.disabled = false;

                sendDataInput.disabled = true;
                btnSend.disabled = true;
            }
        }

        function buf2hex(buffer) {
            return Array.prototype.map.call(new Uint8Array(buffer), x => ('00' + x.toString(16)).slice(-2).toUpperCase()).join(' ');
        }
        
        function str2hex(str) {
            let hex = '';
            for(let i=0; i<str.length; i++) {
                hex += ('00' + str.charCodeAt(i).toString(16)).slice(-2).toUpperCase() + ' ';
            }
            return hex.trim();
        }

        function appendLog(message, type = 'info', isRawBuffer = false) {
            const time = new Date().toLocaleTimeString();
            const logEntry = document.createElement('div');
            
            let displayMsg = message;
            
            if (displayModeHex) {
                if (isRawBuffer) {
                    displayMsg = buf2hex(message);
                } else if (typeof message === 'string') {
                    displayMsg = str2hex(message);
                }
            } else {
                if (isRawBuffer) {
                    // 如果是非 hex 模式但传入的是 buffer，进行简单的文本解码
                    displayMsg = new TextDecoder().decode(message);
                }
            }

            let color = '#10b981'; // 默认接收数据使用绿色
            if(type === 'error') color = '#ef4444';
            else if(type === 'send') color = '#60a5fa'; // 发送的数据依然保持蓝色
            else if(type === 'system') color = '#fbbf24';

            logEntry.style.color = color;
            logEntry.innerText = `[${time}] ${displayMsg}`; // 使用 innerText 防止 XSS，也可避免错误解析 HTML
            
            // 为时间加上单独的颜色
            const timeSpan = document.createElement('span');
            timeSpan.style.color = '#6b7280';
            timeSpan.innerText = `[${time}] `;
            logEntry.innerText = displayMsg;
            logEntry.prepend(timeSpan);
            
            terminal.appendChild(logEntry);

            // 限制保留信息的条数
            while (terminal.children.length > maxTerminalLines) {
                terminal.removeChild(terminal.firstChild);
            }

            // 批量时使用 requestAnimationFrame 来避免同步回流抖动
            if (autoScrollCheckbox.checked) {
                if (!window.scrollPending) {
                    window.scrollPending = true;
                    requestAnimationFrame(() => {
                        terminal.scrollTop = terminal.scrollHeight;
                        window.scrollPending = false;
                    });
                }
            }
        }

        btnConnect.addEventListener('click', () => {
            const url = wsUrlInput.value.trim();
            if (!url) {
                alert('请输入WebSocket地址');
                return;
            }

            appendLog(`正在连接到 ${url}...`, 'system');
            
            try {
                ws = new WebSocket(url);
            } catch (error) {
                appendLog(`连接创建失败: ${error}`, 'error');
                return;
            }

            ws.onopen = () => {
                appendLog('连接成功！', 'system');
                updateStatus(true);
            };

            ws.onmessage = async (event) => {
                let data = event.data;
                let strData = "";

                if (data instanceof Blob) {
                    const arrayBuffer = await data.arrayBuffer();
                    appendLog(arrayBuffer, 'info', true);
                    strData = await data.text();
                } else {
                    appendLog(data);
                    strData = String(data);
                }

                // Check disabled buttons relying on receive condition
                buttonManager.instances.forEach(inst => {
                    if (inst.disableOnClick && inst.reenableCondition === 'receive' && inst.btnEl.disabled) {
                        if (inst.receiveCmd && strData.includes(inst.receiveCmd)) {
                            inst.btnEl.disabled = false;
                            inst.btnEl.textContent = inst.btnText;
                            inst.win.dataset.disabled = "false";
                        }
                    }
                });
            };

            ws.onclose = () => {
                appendLog('连接已断开', 'system');
                updateStatus(false);
                ws = null;
            };

            ws.onerror = (error) => {
                appendLog('WebSocket 发生错误，请检查设备。', 'error');
            };
        });

        btnDisconnect.addEventListener('click', () => {
            if (ws) {
                ws.close();
            }
        });

        btnClear.addEventListener('click', () => {
            terminal.innerHTML = '';
        });

        function sendMsg() {
            if (!ws || ws.readyState !== WebSocket.OPEN) return;
            const text = sendDataInput.value;
            if (text) {
                ws.send(text);
                appendLog(`${text}`, 'send');
                sendDataInput.value = '';
            }
        }

        btnSend.addEventListener('click', sendMsg);
        sendDataInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                sendMsg();
            }
        });
        
        sendDataInput.addEventListener('input', () => {
            btnClearInput.style.display = sendDataInput.value.length > 0 ? 'flex' : 'none';
        });

        btnClearInput.addEventListener('click', () => {
            sendDataInput.value = '';
            btnClearInput.style.display = 'none';
            sendDataInput.focus();
        });

        // 页面按载入完毕后自动触发连接
        window.addEventListener('DOMContentLoaded', () => {
            setTimeout(() => {
                const url = wsUrlInput.value.trim();
                // 只有填写了地址才自动连接
                if (url) {
                    btnConnect.click();
                }
            }, 500);
            // 尝试从本地恢复布局（优先于自动连接之后）
            try {
                loadLayoutFromLocal();
            } catch (e) {
                // ignore
            }
        });

    </script>
</body>
</html>
)rawliteral";

// ==================== WebSocket 事件处理 ====================
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] 断开连接\n", num);
    break;
  case WStype_CONNECTED: {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] 新连接来自 %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2],
                  ip[3]);
    webSocket.sendTXT(num, "Connected to ESP8266");
    break;
  }
  case WStype_TEXT: {
    String cmd = String((char *)payload);
    Serial.println(cmd); // 通过串口转发给主控 MCU（带换行符）

    break;
  }
  default:
    break;
  }
}

// ==================== HTTP 请求处理 ====================
void handleRoot() { server.send(200, "text/html", index_html); }

void handleNotFound() { server.send(404, "text/plain", "404: Not Found"); }

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200); // 与主控 MCU 通信的波特率
  Serial.println();

  // 配置 Wi-Fi AP
  WiFi.softAP(ssid, password);

  // 启动 HTTP 服务
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  // 启动 WebSocket 服务
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // 可选：板载 LED 指示
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // 低电平点亮（根据模块调整）
}

// ==================== 主循环 ====================
void loop() {
  server.handleClient();
  webSocket.loop();
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (serialInBuffer.length() > 0) {
        webSocket.broadcastTXT(serialInBuffer);
      }
      serialInBuffer = "";
    } else if (c == '\r') {
      continue;
    } else {
      serialInBuffer += c;
    }
  }
}