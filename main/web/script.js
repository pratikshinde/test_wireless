// script.js - ESP32 LoRa Mesh Web Interface

const ICONS = {
    satellite_dish: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M21 3L3 21v-2l16-16h2zm-4 4l-4 4 1.41 1.41 4-4L17 7zm-4 4l-4 4 1.41 1.41 4-4L13 11z"/></svg>',
    tachometer_alt: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M12 4a9 9 0 0 1 9 9h-2a7 7 0 0 0-7-7 7 7 0 0 0-7 7H3a9 9 0 0 1 9-9zm0 5l2 5-2 1-2-1 2-5z"/></svg>',
    cog: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58a.49.49 0 0 0 .12-.61l-1.92-3.32a.488.488 0 0 0-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54a.484.484 0 0 0-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58a.49.49 0 0 0-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/></svg>',
    network_wired: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M20 16h-4v-2.07l-2.9-2.9C15.5 10.3 17 8.35 17 6c0-2.76-2.24-5-5-5S7 3.24 7 6c0 2.35 1.5 4.3 3.9 5.03l-2.9 2.9V16H4c-1.1 0-2 .9-2 2v4h4v-2h2v2h4v-2h2v2h4v-2h2v2h4v-4c0-1.1-.9-2-2-2zm-8-9c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3z"/></svg>',
    globe: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-.8 3.97-2.1 5.39z"/></svg>',
    satellite: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M5 18c0 .55.45 1 1 1h1V4H6c-.55 0-1 .45-1 1v13zM18 4h-1v15h1c.55 0 1-.45 1-1V5c0-.55-.45-1-1-1zM9 19h1V4H9v15zm5 0h1V4h-1v15z"/></svg>',
    signal: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M2 22h20V2z"/></svg>',
    route: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M19 15.18V7c0-2.21-1.79-4-4-4s-4 1.79-4 4v10c0 2.21-1.79 4-4 4s-4-1.79-4-4V5.82h2V17c0 1.1.9 2 2 2s2-.9 2-2V7c0-1.1.9-2 2-2s2 .9 2 2v8.18h2z"/></svg>',
    clock: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z"/></svg>',
    history: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M13 3c-4.97 0-9 4.03-9 9H1l3.89 3.89.07.14L9 12H6c0-3.87 3.13-7 7-7s7 3.13 7 7-3.13 7-7 7c-1.93 0-3.68-.79-4.94-2.06l-1.42 1.42C8.27 19.99 10.51 21 13 21c4.97 0 9-4.03 9-9s-4.03-9-9-9zm-1 5v5l4.28 2.54.72-1.21-3.5-2.08V8H12z"/></svg>',
    paper_plane: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z"/></svg>',
    broadcast_tower: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M12 12c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2zm0-2c-2.21 0-4 1.79-4 4s1.79 4 4 4 4-1.79 4-4-1.79-4-4-4zm-8 4c0 4.42 3.58 8 8 8s8-3.58 8-8c0-4.42-3.58-8-8-8s-8 3.58-8 8zm2 0c0-3.31 2.69-6 6-6s6 2.69 6 6-2.69 6-6 6-6-2.69-6-6z"/></svg>',
    bolt: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M11 21h-1l1-7H7.5c-.58 0-.57-.32-.38-.66.19-.34.05-.08.07-.12C8.48 10.94 10.42 7.54 13 3h1l-1 7h3.3c.49 0 .56.33.47.51l-.07.15C12.96 17.55 11 21 11 21z"/></svg>',
    bullhorn: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M22 6h-7c-1.1 0-2 .9-2 2v8c0 1.1.9 2 2 2h7c.55 0 1-.45 1-1V7c0-.55-.45-1-1-1zm-9 10h-2v4H9v-4H2V8h7V4h2v12z"/></svg>',
    search: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/></svg>',
    sync_alt: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M12 4V1L8 5l4 4V6c3.31 0 6 2.69 6 6 0 1.01-.25 1.97-.7 2.8l1.46 1.46C19.54 15.03 20 13.57 20 12c0-4.42-3.58-8-8-8zm0 14c-3.31 0-6-2.69-6-6 0-1.01.25-1.97.7-2.8L5.24 7.74C4.46 8.97 4 10.43 4 12c0 4.42 3.58 8 8 8v3l4-4-4-4v3z"/></svg>',
    sliders_h: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M3 13h2v-2H3v2zm0 4h2v-2H3v2zm0-8h2V7H3v2zm4 4h14v-2H7v2zm0 4h14v-2H7v2zM7 7v2h14V7H7z"/></svg>',
    heartbeat: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35zM11 7h2v5.5l2 1.2.9-1.8L12 7z"/></svg>',
    info_circle: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z"/></svg>',
    crown: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M5 16h14v2H5v-2zm0-5v3h14v-3l-2-2-5 5-5-5-2 2zM12 2L9 7l3 3 3-3-3-5z"/></svg>',
    microchip: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M18 10h4v4h-4zM6 10H2v4h4zM12 6V2h-2v4h2zm0 14v4h-2v-4h2zm8-6h-4v4h4v-4zm-14 0H2v4h4v-4zm10-5h-2V6h2v3zm-6 0H8V6h2v3zm2-3H8v2h4V6zm7 7h-3v2h3v-2zm-14 0H2v2h3v-2z"/></svg>',
    chart_line: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M3.5 18.49l6-6.01 4 4L22 6.92l-1.41-1.41-7.09 7.97-4-4L2 16.99z"/></svg>',
    envelope: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M20 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm0 4l-8 5-8-5V6l8 5 8-5v2z"/></svg>',
    trash: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg>',
};


// Frequency Bands Definition
const FREQ_BANDS = {
    'IN865': [
        { val: 865400000, label: 'Channel 1 (865.4 MHz)' },
        { val: 865600000, label: 'Channel 2 (865.6 MHz)' },
        { val: 865800000, label: 'Channel 3 (865.8 MHz)' },
        { val: 866000000, label: 'Channel 4 (866.0 MHz)' },
        { val: 866200000, label: 'Channel 5 (866.2 MHz)' },
        { val: 866400000, label: 'Channel 6 (866.4 MHz)' },
        { val: 866600000, label: 'Channel 7 (866.6 MHz)' },
        { val: 866800000, label: 'Channel 8 (866.8 MHz)' }
    ],
    'EU868': [
        { val: 868100000, label: '868.100 MHz (Ch 1)' },
        { val: 868300000, label: '868.300 MHz (Ch 2)' },
        { val: 868500000, label: '868.500 MHz (Ch 3)' },
        { val: 868850000, label: '868.850 MHz' },
        { val: 869050000, label: '869.050 MHz' },
        { val: 869525000, label: '869.525 MHz' }
    ],
    'US915': [
        { val: 902300000, label: '902.300 MHz (Ch 0)' },
        { val: 902500000, label: '902.500 MHz (Ch 1)' },
        { val: 902700000, label: '902.700 MHz (Ch 2)' },
        { val: 902900000, label: '902.900 MHz (Ch 3)' },
        { val: 903900000, label: '903.900 MHz' },
        { val: 904600000, label: '904.600 MHz' },
        { val: 905100000, label: '905.100 MHz' },
        { val: 914900000, label: '914.900 MHz' }
    ],
    'EU433': [
        { val: 433175000, label: '433.175 MHz (Ch 1)' },
        { val: 433375000, label: '433.375 MHz (Ch 2)' },
        { val: 433575000, label: '433.575 MHz (Ch 3)' }
    ],
    'CN470': [
        { val: 470300000, label: '470.300 MHz (Ch 1)' },
        { val: 470500000, label: '470.500 MHz (Ch 2)' },
        { val: 470700000, label: '470.700 MHz (Ch 3)' },
        { val: 489300000, label: '489.300 MHz (Last)' }
    ],
    'CN779': [
        { val: 779500000, label: '779.500 MHz (Ch 1)' },
        { val: 779700000, label: '779.700 MHz (Ch 2)' },
        { val: 779900000, label: '779.900 MHz (Ch 3)' }
    ],
    'AS923': [
        { val: 923200000, label: '923.200 MHz (Ch 1)' },
        { val: 923400000, label: '923.400 MHz (Ch 2)' }
    ],
    'AU915': [
        { val: 915200000, label: '915.200 MHz (Ch 0)' },
        { val: 915400000, label: '915.400 MHz (Ch 1)' },
        { val: 927800000, label: '927.800 MHz (Ch 63)' }
    ]
};

function getIcon(name, className) {
    if (!ICONS[name]) return '';
    if (!className) return ICONS[name];
    return ICONS[name].replace('<svg ', `<svg class="${className}" `);
}

document.addEventListener('DOMContentLoaded', function () {
    console.log('ESP32 LoRa Mesh Interface loaded');

    // Check which page we're on and initialize accordingly
    const path = window.location.pathname;
    console.log('Current path:', path);

    if (path === '/' || path.includes('dashboard') || document.getElementById('networkStatsCard')) {
        initDashboard();
    } else if (path.includes('lora-config') || document.getElementById('configForm')) {
        initConfigPage();
    } else if (path.includes('devices') || document.getElementById('deviceListCard')) {
        initDevicesPage();
    }

    // Common initialization
    initCommon();
});

// ==================== COMMON FUNCTIONS ====================

function initCommon() {
    // Update navigation active state
    updateActiveNav();

    // Initialize logout button if exists
    const logoutBtn = document.getElementById('logoutBtn');
    if (logoutBtn) {
        logoutBtn.addEventListener('click', function () {
            if (confirm('Are you sure you want to logout?')) {
                window.location.href = '/logout';
            }
        });
    }

    // Initialize status indicator
    updateConnectionStatus();

    // Periodically update status
    setInterval(updateConnectionStatus, 10000);
}

function updateActiveNav() {
    const path = window.location.pathname;
    const navLinks = document.querySelectorAll('.nav-link');

    navLinks.forEach(link => {
        link.classList.remove('active');
        const linkPath = link.getAttribute('href');

        if ((path === '/' && linkPath === '/dashboard.html') ||
            (path === linkPath) ||
            (path.includes('dashboard') && linkPath === '/dashboard.html')) {
            link.classList.add('active');
        } else if (path.includes('lora-config') && linkPath === '/lora-config.html') {
            link.classList.add('active');
        } else if (path.includes('devices') && linkPath === '/devices.html') {
            link.classList.add('active');
        }
    });
}

function updateConnectionStatus() {
    // Check if we're connected to ESP32 AP
    fetch('/api/test', {
        method: 'GET',
        headers: { 'Accept': 'application/json' }
    })
        .then(response => {
            if (response.ok) {
                setStatus('connected', 'Connected to ESP32');
                return response.json();
            } else {
                setStatus('disconnected', 'Not connected to ESP32');
                throw new Error('Network error');
            }
        })
        .then(data => {
            if (data && data.success) {
                const uptimeEl = document.getElementById('systemUptime');
                if (uptimeEl) uptimeEl.textContent = formatUptime(data.data.uptime);

                const versionEl = document.getElementById('systemVersion');
                if (versionEl) versionEl.textContent = data.data.version || '1.0.0';

                const nodeIdEl = document.getElementById('nodeId');
                if (nodeIdEl && data.data.node_id) {
                    nodeIdEl.textContent = data.data.node_id;
                }
            }
        })
        .catch(error => {
            console.log('Connection check failed:', error);
            setStatus('disconnected', 'Not connected to ESP32');
        });
}

function setStatus(status, message) {
    const statusEl = document.getElementById('connectionStatus');
    const messageEl = document.getElementById('statusMessage');

    if (statusEl) {
        statusEl.className = 'status-indicator ' + status;
        statusEl.textContent = status === 'connected' ? '●' : '○';
    }

    if (messageEl) {
        messageEl.textContent = message;
    }
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;

    if (days > 0) return `${days}d ${hours}h`;
    if (hours > 0) return `${hours}h ${minutes}m`;
    if (minutes > 0) return `${minutes}m ${secs}s`;
    return `${secs}s`;
}

function showToast(message, type = 'info') {
    // Remove existing toasts
    const existingToasts = document.querySelectorAll('.toast');
    existingToasts.forEach(toast => toast.remove());

    // Create new toast
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.innerHTML = `
        <div class="toast-content">
            <span class="toast-message">${message}</span>
            <button class="toast-close">&times;</button>
        </div>
    `;

    document.body.appendChild(toast);

    // Add close functionality
    toast.querySelector('.toast-close').addEventListener('click', () => {
        toast.remove();
    });

    // Auto-remove after 5 seconds
    setTimeout(() => {
        if (toast.parentNode) {
            toast.remove();
        }
    }, 5000);
}

// ==================== DASHBOARD PAGE ====================

function initDashboard() {
    console.log('Initializing dashboard...');

    // Load initial data
    loadNetworkStats();
    loadNodeList();

    // Set up refresh intervals
    setInterval(loadNetworkStats, 10000); // Every 10 seconds
    setInterval(loadNodeList, 15000); // Every 15 seconds

    // Set up button event listeners
    const pingBtn = document.getElementById('pingBtn');
    if (pingBtn) {
        pingBtn.addEventListener('click', sendPing);
    }

    const discoverBtn = document.getElementById('discoverBtn');
    if (discoverBtn) {
        discoverBtn.addEventListener('click', discoverNodes);
    }

    const rebootBtn = document.getElementById('rebootBtn');
    if (rebootBtn) {
        rebootBtn.addEventListener('click', rebootSystem);
    }
}

function loadNetworkStats() {
    fetch('/api/nodes')
        .then(response => {
            if (!response.ok) throw new Error('Network response was not ok');
            return response.json();
        })
        .then(data => {
            if (data.success) {
                updateNetworkStats(data.data);
            }
        })
        .catch(error => {
            console.error('Error loading network stats:', error);
            showToast('Failed to load network stats', 'error');
        });
}

function updateNetworkStats(data) {
    // Update stats counters targeting CORRECT IDs in dashboard.html
    const onlineEl = document.getElementById('nodeCount');
    if (onlineEl) onlineEl.textContent = data.online || 0;

    // Calculate Avg RSSI
    let totalRssi = 0;
    let count = 0;
    if (data.nodes) {
        data.nodes.forEach(n => {
            if (n.rssi != 0) { totalRssi += n.rssi; count++; }
        });
    }
    const avg = count > 0 ? Math.round(totalRssi / count) : 0;
    const rssiEl = document.getElementById('avgRssi');
    if (rssiEl) rssiEl.textContent = avg === 0 ? "--" : avg;

    // Populate Activity List (Recent Activity)
    const activityList = document.getElementById('activityList');
    if (activityList && data.nodes) {
        activityList.innerHTML = '';

        if (data.nodes.length === 0) {
            activityList.innerHTML = '<div style="padding:10px; color:#888;">No recent activity</div>';
        } else {
            // Sort by online status first? Or just list them.
            data.nodes.forEach(node => {
                const div = document.createElement('div');
                div.className = 'activity-item';
                div.style.cssText = "display:flex; justify-content:space-between; align-items:center; padding: 12px; border-bottom: 1px solid rgba(255,255,255,0.05);";

                const icon = node.id === 1 ? getIcon('crown') : getIcon('signal');
                const statusColor = node.online ? 'var(--sys-color-success)' : 'var(--sys-color-error)';

                div.innerHTML = `
                    <div style="display:flex; align-items:center; gap:10px;">
                        <span style="color:${statusColor}">${icon}</span>
                        <span>
                            <div style="font-weight:500;">${node.name || 'Node ' + node.id} (ID: ${node.id})</div>
                            <div style="font-size:0.8em; opacity:0.7;">RSSI: ${node.rssi} dBm | Hops: ${node.hops}</div>
                        </span>
                    </div>
                    <span class="text-muted" style="font-size:0.9em;">${node.last_seen}</span>
                 `;
                activityList.appendChild(div);
            });
        }
    }
}

function loadNodeList() {
    // This could be a separate endpoint for detailed node info
    fetch('/api/nodes')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                // Update any additional node visualizations
                updateNodeVisualization(data.data.nodes);
            }
        })
        .catch(error => console.error('Error loading node list:', error));
}

function updateNodeVisualization(nodes) {
    const visualization = document.getElementById('nodeVisualization');
    if (!visualization) return;

    // Simple visualization - could be enhanced with SVG/D3
    let html = '<div class="node-visualization">';

    // Master node (ESP32 itself)
    html += `
        <div class="node-node master">
            <div class="node-icon">${getIcon('crown')}</div>
            <div class="node-label">Master</div>
            <div class="node-status online">Online</div>
        </div>
    `;

    // Child nodes
    if (nodes && nodes.length > 0) {
        nodes.forEach((node, index) => {
            if (node.id !== 1) { // Skip master if included
                const angle = (index * 360 / (nodes.length - 1)) * Math.PI / 180;
                const distance = 100 + (node.hops || 0) * 50;
                const x = 150 + Math.cos(angle) * distance;
                const y = 150 + Math.sin(angle) * distance;

                html += `
                    <div class="node-node child" style="left: ${x}px; top: ${y}px;">
                        <div class="node-icon">${getIcon('satellite')}</div>
                        <div class="node-label">${node.name || `Node ${node.id}`}</div>
                        <div class="node-status ${node.online ? 'online' : 'offline'}">
                            ${node.online ? 'Online' : 'Offline'}
                        </div>
                        <div class="node-info">
                            RSSI: ${node.rssi || 0}dBm<br>
                            Hops: ${node.hops || 0}
                        </div>
                    </div>
                    <svg class="node-connection" width="300" height="300">
                        <line x1="150" y1="150" x2="${x + 25}" y2="${y + 25}" 
                              stroke="${node.online ? '#4CAF50' : '#f44336'}" 
                              stroke-width="2" stroke-dasharray="${node.hops > 1 ? '5,5' : '0'}"/>
                    </svg>
                `;
            }
        });
    }

    html += '</div>';
    visualization.innerHTML = html;
}

function sendPing() {
    const pingBtn = document.getElementById('pingBtn');
    if (pingBtn) {
        pingBtn.disabled = true;
        pingBtn.textContent = 'Pinging...';
    }

    fetch('/api/ping', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                showToast('Ping sent to network', 'success');
                // Refresh node list after ping
                setTimeout(loadNetworkStats, 2000);
            } else {
                showToast(data.message || 'Ping failed', 'error');
            }
        })
        .catch(error => {
            console.error('Ping error:', error);
            showToast('Network error', 'error');
        })
        .finally(() => {
            if (pingBtn) {
                pingBtn.disabled = false;
                pingBtn.textContent = 'Ping Network';
            }
        });
}

function discoverNodes() {
    const discoverBtn = document.getElementById('discoverBtn');
    if (discoverBtn) {
        discoverBtn.disabled = true;
        discoverBtn.textContent = 'Discovering...';
    }

    fetch('/api/discover', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                showToast('Network discovery started', 'success');
                // Refresh after discovery
                setTimeout(loadNetworkStats, 5000);
            } else {
                showToast(data.message || 'Discovery failed', 'error');
            }
        })
        .catch(error => {
            console.error('Discovery error:', error);
            showToast('Network error', 'error');
        })
        .finally(() => {
            if (discoverBtn) {
                discoverBtn.disabled = false;
                discoverBtn.textContent = 'Discover Nodes';
            }
        });
}

function rebootSystem() {
    if (confirm('Are you sure you want to reboot the system?')) {
        fetch('/api/reboot', { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showToast('System will reboot in 5 seconds...', 'warning');
                    setTimeout(() => {
                        window.location.reload();
                    }, 5000);
                }
            })
            .catch(error => {
                console.error('Reboot error:', error);
                showToast('Reboot failed', 'error');
            });
    }
}

// ==================== CONFIGURATION PAGE ====================

function initConfigPage() {
    console.log('Initializing config page...');

    // Load current configuration
    loadCurrentConfig();

    // Set up form submission
    const configForm = document.getElementById('configForm');
    if (configForm) {
        configForm.addEventListener('submit', saveConfiguration);
    }

    // Set up reset button
    const resetBtn = document.getElementById('resetBtn');
    if (resetBtn) {
        resetBtn.addEventListener('click', resetToDefaults);
    }

    // Initialize range value displays
    // Initialize range value displays
    initRangeDisplays();

    // Initialize frequency options
    const bandSelect = document.getElementById('freq_band');
    if (bandSelect) {
        bandSelect.addEventListener('change', updateFrequencyOptions);
        updateFrequencyOptions();
    }

    // Initialize Airtime Calculator
    initAirtimeCalculator();
}

function initAirtimeCalculator() {
    const inputs = ['spreading_factor', 'bandwidth', 'coding_rate', 'preamble_length', 'enable_crc', 'enable_ldro'];
    inputs.forEach(id => {
        const el = document.getElementById(id);
        if (el) {
            el.addEventListener('change', calculateAirtime);
            el.addEventListener('input', calculateAirtime);
        }
    });
    // Delay initial calc to allow form population
    setTimeout(calculateAirtime, 1000);
}

function calculateAirtime() {
    const display = document.getElementById('airtimeValue');
    if (!display) return;

    // Get parameters
    const sfEl = document.getElementById('spreading_factor');
    const bwEl = document.getElementById('bandwidth');
    const crEl = document.getElementById('coding_rate');
    const plEl = document.getElementById('preamble_length');
    const crcEl = document.getElementById('enable_crc');
    const ldroEl = document.getElementById('enable_ldro');

    if (!sfEl || !bwEl || !crEl) return;

    const SF = parseInt(sfEl.value) || 7;
    const BW = parseInt(bwEl.value) || 125000;
    const CR = (parseInt(crEl.value) || 5) - 4; // 5->1 (4/5), 8->4 (4/8)
    const PL = 32; // Fixed Payload Length (bytes)
    const Preamble = parseInt(plEl ? plEl.value : 8) || 8;
    const CRC = (crcEl && crcEl.checked) ? 1 : 0; // Usually CRC on
    const IH = 0; // Explicit Header (0)
    const DE = (ldroEl && ldroEl.checked) ? 1 : 0; // Low Data Rate Optimize

    // Symbol time
    const Tsym = Math.pow(2, SF) / BW; // seconds

    // Preamble time
    const Tpreamble = (Preamble + 4.25) * Tsym;

    // Payload symbol count
    // Formula: 8 + max(ceil((8*PL - 4*SF + 28 + 16*CRC - 20*IH) / (4*(SF - 2*DE))) * (CR+4), 0)
    const numerator = 8 * PL - 4 * SF + 28 + 16 * CRC - 20 * IH;
    const denominator = 4 * (SF - 2 * DE);
    const innerTerm = Math.ceil(numerator / denominator) * (CR + 4);
    const PayloadSymbNb = 8 + Math.max(innerTerm, 0);

    // Payload time
    const Tpayload = PayloadSymbNb * Tsym;

    // Total time
    const Tpacket = (Tpreamble + Tpayload) * 1000; // ms

    display.textContent = Tpacket.toFixed(2) + ' ms';

    // Warn if airtime is very high (>2s)
    if (Tpacket > 2000) display.style.color = 'var(--sys-color-error)';
    else display.style.color = '';
}

// Frequency Bands Definition


function updateFrequencyOptions() {
    console.log('updateFrequencyOptions called');
    const bandSelect = document.getElementById('freq_band');
    const freqSelect = document.getElementById('frequency_select');
    const freqInput = document.getElementById('frequency');

    if (!bandSelect) {
        console.error('freq_band element not found');
        return;
    }

    const selectedBand = bandSelect.value;
    console.log('Selected band:', selectedBand);

    if (!freqSelect || !freqInput) {
        console.error('freqSelect or freqInput not found');
        return;
    }

    // Clear existing options
    freqSelect.innerHTML = '';

    if (selectedBand === 'CUSTOM') {
        freqSelect.classList.add('d-none');
        freqInput.classList.remove('d-none');
    } else {
        freqSelect.classList.remove('d-none');
        freqInput.classList.add('d-none');

        const bands = FREQ_BANDS[selectedBand] || [];
        console.log(`Updating options for band: ${selectedBand}, found ${bands.length} channels`);

        if (bands.length === 0) {
            const option = document.createElement('option');
            option.textContent = "No channels defined for " + selectedBand;
            freqSelect.appendChild(option);
        } else {
            bands.forEach(band => {
                const option = document.createElement('option');
                option.value = band.val;
                option.textContent = band.label;
                freqSelect.appendChild(option);
            });
        }

        // Sync select to input immediately if possible
        if (bands.length > 0) {
            freqInput.value = bands[0].val;
            freqSelect.value = bands[0].val;
        }

        // Add change listener to sync input hidden value
        freqSelect.onchange = function () {
            freqInput.value = this.value;
        };
    }
}
// Expose to global scope for HTML event handlers
window.updateFrequencyOptions = updateFrequencyOptions;

// Ensure it's available immediately even if loaded async
if (typeof window !== 'undefined') {
    window.updateFrequencyOptions = updateFrequencyOptions;
}

function loadCurrentConfig() {
    console.log('Loading configuration...');

    // Use the CORRECT endpoint that exists: /config (not /api/config)
    fetch('/api/config', {
        method: 'GET',
        headers: {
            'Accept': 'application/json',
            'Cache-Control': 'no-cache'
        }
    })
        .then(response => {
            console.log('Response status:', response.status, response.statusText);
            if (response.ok) {
                return response.json();
            } else {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
        })
        .then(data => {
            console.log('Received config data:', data);
            if (data.success && data.data) {
                populateConfigForm(data.data);
                if (document.getElementById('nodeId')) {
                    document.getElementById('nodeId').textContent = data.data.node_id || 1;
                }
                showToast('✅ Configuration loaded successfully', 'success');
                // Update airtime after load
                calculateAirtime();
            } else {
                console.error('Invalid response format:', data);
                showToast('Failed to load configuration: Invalid response', 'error');
                loadSampleConfig();
            }
        })
        .catch(error => {
            console.error('Error loading config:', error);
            showToast('Failed to load configuration. Check console.', 'error');
            loadSampleConfig();
        });
}

function populateConfigForm(config) {
    // Handle device identification (and nav header update)
    const nodeId = config.node_id || 1;
    if (document.getElementById('node_id')) document.getElementById('node_id').value = nodeId;
    if (document.getElementById('node_name')) document.getElementById('node_name').value = config.node_name || 'LoRa-Node';

    // Update header if present
    const navNodeId = document.getElementById('nodeId');
    if (navNodeId) navNodeId.textContent = nodeId;

    // Handle Frequency & Band
    const freq = config.frequency || 865400000;
    const freqInput = document.getElementById('frequency');
    const bandSelect = document.getElementById('freq_band');

    if (freqInput && bandSelect) {
        freqInput.value = freq;

        // Auto-detect band
        let detectedBand = 'CUSTOM';
        for (const [bandName, channels] of Object.entries(FREQ_BANDS)) {
            if (channels.some(ch => ch.val === freq)) {
                detectedBand = bandName;
                break;
            }
        }
        // Fallback for ranges if exact match not found but within range
        if (detectedBand === 'CUSTOM') {
            if (freq >= 865400000 && freq <= 866800000) detectedBand = 'IN865';
            else if (freq >= 868000000 && freq <= 869000000) detectedBand = 'EU868';
            else if (freq >= 902000000 && freq <= 928000000) detectedBand = 'US915';
            else if (freq >= 433000000 && freq <= 434000000) detectedBand = 'EU433';
            else if (freq >= 470000000 && freq <= 510000000) detectedBand = 'CN470';
            else if (freq >= 779000000 && freq <= 780000000) detectedBand = 'CN779';
        }

        bandSelect.value = detectedBand;

        // IMPORTANT: Update options first based on the detected band
        updateFrequencyOptions();



        // Now set the selected frequency
        const freqSelect = document.getElementById('frequency_select');
        if (detectedBand !== 'CUSTOM' && freqSelect) {
            // Check if freq is in the options
            let match = false;
            for (let i = 0; i < freqSelect.options.length; i++) {
                if (parseInt(freqSelect.options[i].value) === parseInt(freq)) {
                    freqSelect.value = freq;
                    match = true;
                    break;
                }
            }

            if (!match) {
                // If the frequency isn't in the standard list, switch to CUSTOM
                console.log('Frequency ' + freq + ' not in band ' + detectedBand + ', switching to CUSTOM');
                bandSelect.value = 'CUSTOM';
                updateFrequencyOptions();
                freqInput.value = freq;
            }
        }
    }

    // Populate other standard fields
    const fields = [
        'spread_factor', 'bandwidth',
        'coding_rate', 'tx_power', 'sync_word', 'ping_interval', 'beacon_interval',
        'route_timeout', 'max_hops', 'ack_timeout', 'healing_timeout',
        'wifi_ssid', 'web_port', 'preamble_length', 'symbol_timeout',
        'aes_key', 'iv'
    ];

    fields.forEach(field => {
        const element = document.getElementById(field);
        if (element && config[field] !== undefined) {
            if (element.type !== 'checkbox') {
                if (field === 'sync_word') {
                    // Convert to hex for display
                    element.value = '0x' + Number(config[field]).toString(16).toUpperCase().padStart(2, '0');
                } else if (field === 'aes_key' || field === 'iv') {
                    // Convert byte array or string to hex
                    if (Array.isArray(config[field])) {
                        element.value = config[field].map(b => b.toString(16).toUpperCase().padStart(2, '0')).join('');
                    } else {
                        element.value = config[field];
                    }
                } else {
                    element.value = config[field];
                }
            }
            // Update displays
            const displayElement = document.getElementById(field + 'Display');
            if (displayElement) {
                displayElement.textContent = config[field];
            }
        }
    });

    // Handle boolean fields separately
    const boolFields = ['enable_ack', 'enable_self_healing', 'enable_web_server',
        'enable_encryption', 'enable_crc', 'enable_ldro'];

    boolFields.forEach(field => {
        const element = document.getElementById(field);
        if (element && config[field] !== undefined) {
            element.checked = Boolean(config[field]);
        }
    });

    console.log('Configuration form populated');

    // Initialize security fields visibility
    toggleSecurityFields();
}

function toggleSecurityFields() {
    const isEnabled = document.getElementById('enable_encryption').checked;
    const fieldsDiv = document.getElementById('securityFields');
    if (fieldsDiv) {
        fieldsDiv.style.display = isEnabled ? 'block' : 'none';
    }
}

function loadSampleConfig() {
    // Sample configuration for testing
    const sampleConfig = {
        node_id: 1,
        node_name: 'LoRa-Master',
        frequency: 868.0,
        spread_factor: 7,
        bandwidth: 125,
        coding_rate: 5,
        tx_power: 17,
        sync_word: 0x12,
        ping_interval: 60,
        beacon_interval: 30,
        route_timeout: 300,
        max_hops: 5,
        enable_ack: true,
        ack_timeout: 1000,
        enable_self_healing: true,
        healing_timeout: 60,
        wifi_ssid: 'ESP32-LoRa-Mesh',
        enable_web_server: true,
        web_port: 80,
        enable_encryption: false,
        enable_crc: true,
        enable_ldro: false,
        preamble_length: 8,
        symbol_timeout: 10
    };

    populateConfigForm(sampleConfig);
    showToast('Loaded sample configuration (real config unavailable)', 'warning');
}

function initRangeDisplays() {
    // Update displays when range inputs change
    const rangeInputs = document.querySelectorAll('input[type="range"]');
    rangeInputs.forEach(input => {
        input.addEventListener('input', function () {
            const displayId = this.id + 'Display';
            const displayElement = document.getElementById(displayId);
            if (displayElement) {
                displayElement.textContent = this.value;
            }
        });
    });
}

function saveConfiguration(event) {
    event.preventDefault();
    console.log('Saving configuration...');

    const form = event.target;
    const submitBtn = form.querySelector('button[type="submit"]');

    if (submitBtn) {
        submitBtn.disabled = true;
        submitBtn.textContent = 'Saving...';
    }

    // Gather form data
    const formData = {};
    const formElements = form.elements;

    for (let element of formElements) {
        if (element.name && element.type !== 'button' && element.type !== 'submit') {
            const name = element.name;
            const value = element.value;

            if (element.type === 'checkbox') {
                formData[name] = element.checked;
                console.log(`${name}: ${element.checked}`);
            } else if (value !== undefined) {
                // Handle numeric fields
                if (element.type === 'number' || element.type === 'range') {
                    formData[name] = value === '' ? 0 : Number(value);
                }
                // Handle numeric selects and special fields
                else if (['spread_factor', 'bandwidth', 'coding_rate', 'tx_power', 'max_hops', 'sync_word'].includes(name)) {
                    formData[name] = Number(value);
                }
                else {
                    formData[name] = value;
                }
                console.log(`${name}: ${formData[name]} (${typeof formData[name]})`);
            }
        }
    }

    console.log('Sending to server:', JSON.stringify(formData, null, 2));

    // Check if POST /api/config exists, otherwise try POST /config
    fetch('/api/config', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Accept': 'application/json'
        },
        body: JSON.stringify(formData)
    })
        .then(response => {
            console.log('Save response status:', response.status, response.statusText);

            if (response.ok) {
                showToast('Configuration saved successfully', 'success');
                // Reload to reflect changes after short delay
                setTimeout(() => window.location.reload(), 1500);
            } else {
                showToast('Failed to save configuration', 'error');
            }
        })
        .catch(error => {
            console.error('Save error:', error);
            showToast('Error saving configuration', 'error');
        })
        .finally(() => {
            if (submitBtn) {
                submitBtn.disabled = false;
                submitBtn.textContent = 'Save Settings'; // Reset button text
                // Check which button was clicked effectively? 
                // Actually the textContent logic above is a bit generic.
                // Better to restore original text or just "Save".
            }
        });
}

function generateAESKey() {
    // Generate 32 random hex characters (16 bytes)
    const charset = '0123456789ABCDEF';
    let key = '';
    for (let i = 0; i < 32; i++) {
        const randomIndex = Math.floor(Math.random() * charset.length);
        key += charset[randomIndex];
    }
    document.getElementById('aes_key').value = key;
    showToast('New AES Key generated', 'success');
}

function copyToClipboard(elementId) {
    const el = document.getElementById(elementId);
    if (!el) return;

    // Select the text field
    el.select();
    el.setSelectionRange(0, 99999); // For mobile devices

    // Copy the text
    try {
        navigator.clipboard.writeText(el.value).then(() => {
            showToast('Copied to clipboard', 'success');
        });
    } catch (err) {
        // Fallback
        document.execCommand('copy');
        showToast('Copied to clipboard', 'success');
    }
}


function resetToDefaults() {
    if (confirm('Reset all settings to default values?')) {
        fetch('/api/config/reset', { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showToast('Configuration reset to defaults', 'success');
                    // Reload the default config
                    setTimeout(loadCurrentConfig, 1000);
                }
            })
            .catch(error => {
                console.error('Reset error:', error);
                showToast('Reset failed', 'error');
            });
    }
}

// ==================== DEVICES PAGE ====================

let refreshTimer = null;
const REFRESH_INTERVAL = 10000;

function startAutoRefresh() {
    if (refreshTimer) clearInterval(refreshTimer);
    const startTime = Date.now();

    refreshTimer = setInterval(() => {
        const elapsed = Date.now() - startTime;
        const progress = Math.min((elapsed / REFRESH_INTERVAL) * 100, 100);

        const spinner = document.getElementById('refreshSpinner');
        if (spinner) {
            spinner.setAttribute('stroke-dasharray', `${progress}, 100`);
        }

        if (elapsed >= REFRESH_INTERVAL) {
            clearInterval(refreshTimer);
            loadDevices();
        }
    }, 100);
}

function initDevicesPage() {
    console.log('Initializing devices page...');
    loadDevices();

    // Set up search functionality
    const searchInput = document.getElementById('deviceSearch');
    if (searchInput) {
        searchInput.addEventListener('input', filterDevices);
    }
}

function loadDevices() {
    // Visual feedback: Change spinner color during load
    const spinner = document.querySelector('.circle');
    if (spinner) spinner.style.stroke = 'var(--sys-color-secondary)';

    fetch('/api/nodes')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                updateDeviceList(data.data);
                if (data.data.routes) updateRoutingTable(data.data.routes);
            }
        })
        .catch(error => {
            console.error('Error loading devices:', error);
            // Only toast on error
            showToast('Failed to connect to device', 'error');
        })
        .finally(() => {
            if (spinner) spinner.style.stroke = ''; // Reset color
            startAutoRefresh();
        });
}

function updateDeviceList(data) {
    const container = document.getElementById('devicesList');
    if (!container) return;

    if (!data.nodes || data.nodes.length === 0) {
        container.innerHTML = '<div class="empty-state">No devices found in the network</div>';
        return;
    }

    // Table View Construction
    let html = `
    <div class="card">
        <div style="overflow-x: auto;">
            <table class="routing-table" style="width: 100%; border-collapse: collapse; min-width: 600px;">
                <thead>
                    <tr style="text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); background: rgba(0,0,0,0.2);">
                        <th style="padding:12px;">ID</th>
                        <th style="padding:12px;">Name</th>
                        <th style="padding:12px;">Status</th>
                        <th style="padding:12px;">Signal</th>
                        <th style="padding:12px;">Hops</th>
                        <th style="padding:12px;">Last Seen</th>
                        <th style="padding:12px; text-align:right;">Actions</th>
                    </tr>
                </thead>
                <tbody>
    `;

    data.nodes.forEach(device => {
        const lastSeen = device.last_seen || 'Unknown';
        const rowBg = device.online ? 'rgba(76, 175, 80, 0.05)' : 'rgba(255, 255, 255, 0.02)';

        html += `
            <tr style="border-bottom: 1px solid rgba(255,255,255,0.05); background: ${rowBg};">
                <td style="padding:12px;"><strong>${device.id}</strong></td>
                <td style="padding:12px;">${device.name || '-'}</td>
                <td style="padding:12px;">
                    <span class="status-badge ${device.online ? 'online' : 'offline'}">
                        ${device.online ? 'Online' : 'Offline'}
                    </span>
                </td>
                <td style="padding:12px;">
                    <span style="display:flex; align-items:center; gap:6px;">
                        ${getIcon('signal')} ${device.rssi} dBm
                    </span>
                </td>
                <td style="padding:12px;">${device.hops}</td>
                <td style="padding:12px;">${lastSeen}</td>
                <td style="padding:12px; text-align:right;">
                    <button class="btn btn-sm btn-info ping-device" data-id="${device.id}" style="padding:4px 12px; margin-left:4px;">
                        Ping
                    </button>
                    ${device.id !== 1 ? `
                    <button class="btn btn-sm btn-danger remove-device" data-id="${device.id}" style="padding:4px 12px; margin-left:4px;">
                        Remove
                    </button>` : ''}
                </td>
            </tr>
        `;
    });

    html += `</tbody></table></div></div>`;

    container.innerHTML = html;

    // Add event listeners to action buttons
    document.querySelectorAll('.ping-device').forEach(btn => {
        btn.addEventListener('click', function () {
            const deviceId = this.getAttribute('data-id');
            pingDevice(deviceId);
        });
    });

    document.querySelectorAll('.remove-device').forEach(btn => {
        btn.addEventListener('click', function () {
            const deviceId = this.getAttribute('data-id');
            removeDevice(deviceId);
        });
    });
}

function updateRoutingTable(routes) {
    const tbody = document.getElementById('routingTableBody');
    if (!tbody) return;

    tbody.innerHTML = '';

    if (!routes || routes.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" style="text-align:center; padding:20px; color:#888;">No active routes found</td></tr>';
        return;
    }

    let html = '';
    routes.forEach(route => {
        html += `
            <tr style="border-bottom: 1px solid rgba(255,255,255,0.05);">
                <td style="padding:12px;">Node ${route.dest}</td>
                <td style="padding:12px;">Node ${route.next}</td>
                <td style="padding:12px;">${route.hops}</td>
                <td style="padding:12px;">${route.rssi} dBm</td>
                <td style="padding:12px;">
                    <span class="status-badge online" style="font-size:0.85em;">${route.status}</span>
                </td>
            </tr>
        `;
    });

    tbody.innerHTML = html;
}

function getSignalStrength(rssi) {
    if (rssi >= -50) return 'excellent';
    if (rssi >= -60) return 'good';
    if (rssi >= -70) return 'fair';
    if (rssi >= -80) return 'weak';
    return 'poor';
}

function filterDevices() {
    const searchInput = document.getElementById('deviceSearch');
    const searchTerm = searchInput.value.toLowerCase();
    const deviceCards = document.querySelectorAll('.device-card');

    deviceCards.forEach(card => {
        const deviceName = card.getAttribute('data-name').toLowerCase();
        const deviceId = card.getAttribute('data-id');

        if (deviceName.includes(searchTerm) ||
            deviceId.includes(searchTerm) ||
            searchTerm === '') {
            card.style.display = 'block';
        } else {
            card.style.display = 'none';
        }
    });
}

function pingDevice(deviceId) {
    fetch('/api/ping', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ target: parseInt(deviceId) })
    })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                showToast(`Ping sent to device ${deviceId}`, 'success');
            } else {
                showToast(`Failed to ping device ${deviceId}`, 'error');
            }
        })
        .catch(error => {
            console.error('Ping device error:', error);
            showToast('Network error', 'error');
        });
}

function restartDevice(deviceId) {
    if (confirm(`Restart device ${deviceId}?`)) {
        fetch('/api/device/restart', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ device_id: parseInt(deviceId) })
        })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showToast(`Device ${deviceId} restart initiated`, 'success');
                } else {
                    showToast(`Failed to restart device ${deviceId}`, 'error');
                }
            })
            .catch(error => {
                console.error('Restart device error:', error);
                showToast('Network error', 'error');
            });
    }
}

function removeDevice(deviceId) {
    if (confirm(`Remove device ${deviceId} from network?`)) {
        fetch('/api/device/remove', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ device_id: parseInt(deviceId) })
        })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showToast(`Device ${deviceId} removed`, 'success');
                    // Refresh device list
                    setTimeout(loadDevices, 1000);
                } else {
                    showToast(`Failed to remove device ${deviceId}`, 'error');
                }
            })
            .catch(error => {
                console.error('Remove device error:', error);
                showToast('Network error', 'error');
            });
    }
}

// ==================== UTILITY FUNCTIONS ====================

// Format bytes to human readable size
function formatBytes(bytes, decimals = 2) {
    if (bytes === 0) return '0 Bytes';

    const k = 1024;
    const dm = decimals < 0 ? 0 : decimals;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];

    const i = Math.floor(Math.log(bytes) / Math.log(k));

    return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
}

// Debounce function for limiting API calls
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

// Export functions for debugging (optional)
if (typeof window !== 'undefined') {
    window.ESP32LoRa = {
        loadConfig: loadCurrentConfig,
        saveConfig: saveConfiguration,
        loadDevices: loadDevices,
        sendPing: sendPing,
        discoverNodes: discoverNodes
    };
    // Expose functions required by HTML inline handlers
    window.saveConfig = function (section) {
        console.log('Saving config section:', section);
        const form = document.getElementById('configForm');
        if (form) {
            // Create a synthetic event to reuse existing logic
            const event = {
                preventDefault: () => { },
                target: form
            };
            saveConfiguration(event);
        }
    };
    window.factoryReset = resetToDefaults;
    window.refreshDevices = loadDevices; // For backward compatibility / ease of use
}