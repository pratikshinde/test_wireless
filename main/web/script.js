// script.js - ESP32 LoRa Mesh Web Interface
document.addEventListener('DOMContentLoaded', function () {
    console.log('ESP32 LoRa Mesh Interface loaded');

    // Check which page we're on and initialize accordingly
    const path = window.location.pathname;

    if (path === '/' || path === '/dashboard.html') {
        initDashboard();
    } else if (path === '/lora-config.html') {
        initConfigPage();
    } else if (path === '/devices.html') {
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
                document.getElementById('systemUptime')?.textContent =
                    formatUptime(data.data.uptime);
                document.getElementById('systemVersion')?.textContent =
                    data.data.version || '1.0.0';
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
    // Update stats counters
    document.getElementById('totalNodes')?.textContent = data.total || 0;
    document.getElementById('onlineNodes')?.textContent = data.online || 0;
    document.getElementById('offlineNodes')?.textContent =
        (data.total || 0) - (data.online || 0);

    // Update node list table
    const tableBody = document.getElementById('nodeTableBody');
    if (tableBody && data.nodes) {
        tableBody.innerHTML = '';

        data.nodes.forEach(node => {
            const row = document.createElement('tr');
            row.innerHTML = `
                <td>${node.id}</td>
                <td>${node.name || 'Unknown'}</td>
                <td><span class="status-badge ${node.online ? 'online' : 'offline'}">
                    ${node.online ? 'Online' : 'Offline'}
                </span></td>
                <td>${node.rssi || 0} dBm</td>
                <td>${node.hops || 0}</td>
                <td>${node.last_seen || 'Unknown'}</td>
            `;
            tableBody.appendChild(row);
        });
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
            <div class="node-icon">🏠</div>
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
                        <div class="node-icon">📡</div>
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
    initRangeDisplays();
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
                showToast('✅ Configuration loaded successfully', 'success');
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
    // Populate all form fields
    const fields = [
        'node_id', 'node_name', 'frequency', 'spread_factor', 'bandwidth',
        'coding_rate', 'tx_power', 'sync_word', 'ping_interval', 'beacon_interval',
        'route_timeout', 'max_hops', 'ack_timeout', 'healing_timeout',
        'wifi_ssid', 'web_port', 'preamble_length', 'symbol_timeout',
        'aes_key', 'iv'
    ];

    fields.forEach(field => {
        const element = document.getElementById(field);
        if (element && config[field] !== undefined) {
            if (element.type === 'checkbox') {
                element.checked = Boolean(config[field]);
            } else {
                element.value = config[field];
            }

            // Update any associated display elements
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
            if (element.type === 'checkbox') {
                formData[element.name] = element.checked;
                console.log(`${element.name}: ${element.checked ? 'true' : 'false'}`);
            } else if (element.value !== undefined) {
                formData[element.name] = element.value;
                console.log(`${element.name}: ${element.value}`);
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
            if (!response.ok) {
                console.log('POST /api/config failed, trying POST /config');
                // Try POST /config as fallback
                return fetch('/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                        'Accept': 'application/json'
                    },
                    body: JSON.stringify(formData)
                });
            }
            return response;
        })
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            return response.json();
        })
        .then(data => {
            console.log('Save response data:', data);
            if (data.success) {
                showToast('✅ Configuration saved successfully!', 'success');
                // Reload to verify
                setTimeout(loadCurrentConfig, 1000);
            } else {
                showToast(`❌ Save failed: ${data.message || 'Unknown error'}`, 'error');
            }
        })
        .catch(error => {
            console.error('Save error:', error);
            showToast(`❌ Save failed: ${error.message}`, 'error');

            // Try one more fallback - regular form submission
            console.log('Trying form fallback...');
            form.submit();
        })
        .finally(() => {
            if (submitBtn) {
                submitBtn.disabled = false;
                submitBtn.textContent = 'Save Configuration';
            }
        });
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

function initDevicesPage() {
    console.log('Initializing devices page...');

    // Load initial device list
    loadDevices();

    // Set up refresh interval
    setInterval(loadDevices, 10000);

    // Set up search functionality
    const searchInput = document.getElementById('deviceSearch');
    if (searchInput) {
        searchInput.addEventListener('input', filterDevices);
    }

    // Set up refresh button
    const refreshBtn = document.getElementById('refreshDevices');
    if (refreshBtn) {
        refreshBtn.addEventListener('click', loadDevices);
    }
}

function loadDevices() {
    const refreshBtn = document.getElementById('refreshDevices');
    if (refreshBtn) {
        refreshBtn.disabled = true;
        refreshBtn.textContent = 'Refreshing...';
    }

    fetch('/api/nodes')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                updateDeviceList(data.data);
                showToast(`Loaded ${data.data.total} devices`, 'success');
            }
        })
        .catch(error => {
            console.error('Error loading devices:', error);
            showToast('Failed to load devices', 'error');
        })
        .finally(() => {
            if (refreshBtn) {
                refreshBtn.disabled = false;
                refreshBtn.textContent = 'Refresh Devices';
            }
        });
}

function updateDeviceList(data) {
    const container = document.getElementById('deviceList');
    if (!container) return;

    if (!data.nodes || data.nodes.length === 0) {
        container.innerHTML = '<div class="empty-state">No devices found in the network</div>';
        return;
    }

    let html = '';

    data.nodes.forEach(device => {
        const lastSeen = device.last_seen || 'Unknown';
        const signalStrength = getSignalStrength(device.rssi);

        html += `
            <div class="device-card" data-id="${device.id}" data-name="${device.name || ''}">
                <div class="device-header">
                    <div class="device-icon">
                        ${device.id === 1 ? '🏠' : '📡'}
                    </div>
                    <div class="device-info">
                        <h3 class="device-name">${device.name || `Node ${device.id}`}</h3>
                        <div class="device-id">ID: ${device.id}</div>
                    </div>
                    <div class="device-status ${device.online ? 'online' : 'offline'}">
                        ${device.online ? 'Online' : 'Offline'}
                    </div>
                </div>
                
                <div class="device-details">
                    <div class="detail-row">
                        <span class="detail-label">Signal:</span>
                        <span class="detail-value">
                            <div class="signal-strength ${signalStrength}">
                                <div class="signal-bar"></div>
                                <div class="signal-bar"></div>
                                <div class="signal-bar"></div>
                                <div class="signal-bar"></div>
                                <div class="signal-bar"></div>
                            </div>
                            ${device.rssi || 0} dBm
                        </span>
                    </div>
                    <div class="detail-row">
                        <span class="detail-label">Hops:</span>
                        <span class="detail-value">${device.hops || 0}</span>
                    </div>
                    <div class="detail-row">
                        <span class="detail-label">Last Seen:</span>
                        <span class="detail-value">${lastSeen}</span>
                    </div>
                </div>
                
                <div class="device-actions">
                    <button class="btn btn-sm btn-info ping-device" data-id="${device.id}">
                        Ping
                    </button>
                    <button class="btn btn-sm btn-warning restart-device" data-id="${device.id}" ${device.id === 1 ? 'disabled' : ''}>
                        Restart
                    </button>
                    <button class="btn btn-sm btn-danger remove-device" data-id="${device.id}" ${device.id === 1 ? 'disabled' : ''}>
                        Remove
                    </button>
                </div>
            </div>
        `;
    });

    container.innerHTML = html;

    // Add event listeners to action buttons
    document.querySelectorAll('.ping-device').forEach(btn => {
        btn.addEventListener('click', function () {
            const deviceId = this.getAttribute('data-id');
            pingDevice(deviceId);
        });
    });

    document.querySelectorAll('.restart-device').forEach(btn => {
        btn.addEventListener('click', function () {
            const deviceId = this.getAttribute('data-id');
            restartDevice(deviceId);
        });
    });

    document.querySelectorAll('.remove-device').forEach(btn => {
        btn.addEventListener('click', function () {
            const deviceId = this.getAttribute('data-id');
            removeDevice(deviceId);
        });
    });
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
}