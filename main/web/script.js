// Common JavaScript functions for all pages
// script.js - COMPLETE FIXED VERSION

// Utility functions
function showToast(message, type = 'info') {
    // Remove existing toast
    const existingToast = document.getElementById('global-toast');
    if (existingToast) {
        existingToast.remove();
    }
    
    // Create new toast
    const toast = document.createElement('div');
    toast.id = 'global-toast';
    toast.className = `toast toast-${type}`;
    toast.innerHTML = `
        <span>${message}</span>
        <button onclick="this.parentElement.remove()">&times;</button>
    `;
    
    document.body.appendChild(toast);
    
    // Show toast
    setTimeout(() => {
        toast.classList.add('show');
    }, 10);
    
    // Auto-remove after 5 seconds
    setTimeout(() => {
        toast.classList.remove('show');
        setTimeout(() => {
            if (toast.parentElement) {
                toast.remove();
            }
        }, 300);
    }, 5000);
}

// Format time difference from microseconds to human readable
function formatTimeDifference(microseconds) {
    const seconds = Math.floor(microseconds / 1000000);
    
    if (seconds < 60) {
        return `${seconds} seconds ago`;
    } else if (seconds < 3600) {
        const minutes = Math.floor(seconds / 60);
        return `${minutes} minute${minutes > 1 ? 's' : ''} ago`;
    } else if (seconds < 86400) {
        const hours = Math.floor(seconds / 3600);
        return `${hours} hour${hours > 1 ? 's' : ''} ago`;
    } else {
        const days = Math.floor(seconds / 86400);
        return `${days} day${days > 1 ? 's' : ''} ago`;
    }
}

// Format uptime from microseconds
function formatUptime(microseconds) {
    const seconds = Math.floor(microseconds / 1000000);
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    
    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes}m ${secs}s`;
    } else if (minutes > 0) {
        return `${minutes}m ${secs}s`;
    } else {
        return `${secs}s`;
    }
}

// Format bytes to human readable
function formatBytes(bytes, decimals = 2) {
    if (bytes === 0) return '0 Bytes';
    
    const k = 1024;
    const dm = decimals < 0 ? 0 : decimals;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    
    return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
}

// Calculate signal quality from RSSI
function calculateSignalQuality(rssi) {
    // RSSI values range from about -30 (excellent) to -100 (poor)
    const maxRssi = -30;
    const minRssi = -100;
    
    // Normalize to 0-100
    let quality = ((rssi - minRssi) / (maxRssi - minRssi)) * 100;
    
    // Clamp between 0 and 100
    quality = Math.max(0, Math.min(100, quality));
    
    return Math.round(quality);
}

// Get signal quality color
function getSignalColor(quality) {
    if (quality >= 80) return '#48bb78'; // Green
    if (quality >= 60) return '#ed8936'; // Orange
    return '#f56565'; // Red
}

// Debounce function
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

// Throttle function
function throttle(func, limit) {
    let inThrottle;
    return function() {
        const args = arguments;
        const context = this;
        if (!inThrottle) {
            func.apply(context, args);
            inThrottle = true;
            setTimeout(() => inThrottle = false, limit);
        }
    };
}

// Copy to clipboard
function copyToClipboard(text) {
    navigator.clipboard.writeText(text)
        .then(() => {
            showToast('Copied to clipboard!', 'success');
        })
        .catch(err => {
            console.error('Failed to copy: ', err);
            showToast('Failed to copy to clipboard', 'error');
        });
}

// Validate IP address
function isValidIP(ip) {
    const pattern = /^(\d{1,3}\.){3}\d{1,3}$/;
    if (!pattern.test(ip)) return false;
    
    return ip.split('.').every(segment => {
        const num = parseInt(segment, 10);
        return num >= 0 && num <= 255;
    });
}

// Validate MAC address
function isValidMAC(mac) {
    const pattern = /^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/;
    return pattern.test(mac);
}

// Generate random string
function generateRandomString(length) {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    let result = '';
    for (let i = 0; i < length; i++) {
        result += chars.charAt(Math.floor(Math.random() * chars.length));
    }
    return result;
}

// Get current time in HH:MM:SS format
function getCurrentTime() {
    const now = new Date();
    return now.toTimeString().split(' ')[0];
}

// Get current date in YYYY-MM-DD format
function getCurrentDate() {
    const now = new Date();
    return now.toISOString().split('T')[0];
}

// Parse query parameters
function getQueryParams() {
    const params = {};
    const queryString = window.location.search.substring(1);
    const pairs = queryString.split('&');
    
    pairs.forEach(pair => {
        const [key, value] = pair.split('=');
        if (key) {
            params[decodeURIComponent(key)] = decodeURIComponent(value || '');
        }
    });
    
    return params;
}

// Set query parameter
function setQueryParam(key, value) {
    const params = new URLSearchParams(window.location.search);
    params.set(key, value);
    window.history.replaceState({}, '', `${window.location.pathname}?${params}`);
}

// Remove query parameter
function removeQueryParam(key) {
    const params = new URLSearchParams(window.location.search);
    params.delete(key);
    window.history.replaceState({}, '', `${window.location.pathname}?${params}`);
}

// Check if element is in viewport
function isInViewport(element) {
    const rect = element.getBoundingClientRect();
    return (
        rect.top >= 0 &&
        rect.left >= 0 &&
        rect.bottom <= (window.innerHeight || document.documentElement.clientHeight) &&
        rect.right <= (window.innerWidth || document.documentElement.clientWidth)
    );
}

// Smooth scroll to element
function smoothScrollTo(elementId) {
    const element = document.getElementById(elementId);
    if (element) {
        element.scrollIntoView({
            behavior: 'smooth',
            block: 'start'
        });
    }
}

// Format number with commas
function numberWithCommas(x) {
    return x.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ',');
}

// Calculate percentage
function calculatePercentage(part, total) {
    if (total === 0) return 0;
    return Math.round((part / total) * 100);
}

// Deep clone object
function deepClone(obj) {
    return JSON.parse(JSON.stringify(obj));
}

// Merge objects
function mergeObjects(target, source) {
    for (const key in source) {
        if (source[key] && typeof source[key] === 'object' && !Array.isArray(source[key])) {
            if (!target[key] || typeof target[key] !== 'object') {
                target[key] = {};
            }
            mergeObjects(target[key], source[key]);
        } else {
            target[key] = source[key];
        }
    }
    return target;
}

// Add CSS to page
function addCSS(css) {
    const style = document.createElement('style');
    style.textContent = css;
    document.head.appendChild(style);
}

// Add toast CSS
addCSS(`
    .toast {
        position: fixed;
        top: 20px;
        right: 20px;
        padding: 15px 20px;
        border-radius: 5px;
        color: white;
        font-weight: 500;
        z-index: 10000;
        display: flex;
        justify-content: space-between;
        align-items: center;
        min-width: 300px;
        max-width: 500px;
        transform: translateX(100%);
        transition: transform 0.3s ease-in-out;
        box-shadow: 0 4px 15px rgba(0,0,0,0.2);
    }
    
    .toast.show {
        transform: translateX(0);
    }
    
    .toast-info {
        background: #2196F3;
    }
    
    .toast-success {
        background: #4CAF50;
    }
    
    .toast-warning {
        background: #FF9800;
    }
    
    .toast-error {
        background: #f44336;
    }
    
    .toast button {
        background: none;
        border: none;
        color: white;
        font-size: 20px;
        cursor: pointer;
        margin-left: 15px;
        padding: 0;
        opacity: 0.8;
    }
    
    .toast button:hover {
        opacity: 1;
    }
`);

// Dashboard specific functions
async function updateDashboard() {
    try {
        // Update node count
        const nodesRes = await fetch('/api/nodes');
        const nodes = await nodesRes.json();
        document.getElementById('nodeCount').textContent = nodes.length;
        
        // Calculate average RSSI
        let totalRssi = 0;
        let count = 0;
        nodes.forEach(node => {
            if (node.online && node.rssi !== 0) {
                totalRssi += node.rssi;
                count++;
            }
        });
        document.getElementById('avgRssi').textContent = count > 0 ? Math.round(totalRssi / count) : 0;
        
        // Update route count
        const routesRes = await fetch('/api/routes');
        const routes = await routesRes.json();
        let activeRoutes = routes.filter(route => route.active).length;
        document.getElementById('routeCount').textContent = activeRoutes;
        
        // Update uptime (get from first node which should be self)
        if (nodes.length > 0) {
            const uptimeFormatted = formatUptime(nodes[0].uptime * 1000000); // Convert back to microseconds
            document.getElementById('uptime').textContent = uptimeFormatted;
        }
        
    } catch (error) {
        console.error('Error updating dashboard:', error);
    }
}

// Devices page functions
async function loadDevices() {
    try {
        // Load nodes
        const nodesRes = await fetch('/api/nodes');
        const nodes = await nodesRes.json();
        
        const devicesList = document.getElementById('devicesList');
        devicesList.innerHTML = '';
        
        nodes.forEach(node => {
            const deviceCard = document.createElement('div');
            deviceCard.className = `device-card ${node.id === 1 ? 'master' : ''} ${!node.online ? 'offline' : ''}`;
            
            // Convert seconds to human readable
            const timeAgo = formatTimeDifference(node.last_seen * 1000000);
            const uptimeFormatted = formatUptime(node.uptime * 1000000);
            
            deviceCard.innerHTML = `
                <div class="device-icon">
                    <i class="fas ${node.id === 1 ? 'fa-crown' : 'fa-microchip'}"></i>
                </div>
                <div class="device-info">
                    <h3>${node.name}</h3>
                    <p>ID: ${node.id} ${node.id === 1 ? '(Master)' : '(Slave)'}</p>
                    <div class="device-stats">
                        <span class="stat"><i class="fas fa-signal"></i> ${node.rssi} dBm</span>
                        <span class="stat"><i class="fas fa-route"></i> ${node.hop_count} hops</span>
                        <span class="stat"><i class="fas fa-clock"></i> ${timeAgo}</span>
                    </div>
                </div>
                <div class="device-status">
                    <span class="status-indicator ${node.online ? 'online' : 'offline'}"></span>
                    <span>${node.online ? 'Online' : 'Offline'}</span>
                </div>
            `;
            
            deviceCard.onclick = () => showNodeDetails(node);
            devicesList.appendChild(deviceCard);
        });
        
        // Load routes
        const routesRes = await fetch('/api/routes');
        const routes = await routesRes.json();
        
        const tableBody = document.getElementById('routingTableBody');
        tableBody.innerHTML = '';
        
        routes.forEach(route => {
            const row = tableBody.insertRow();
            const timeAgo = formatTimeDifference(route.last_update * 1000000);
            
            row.innerHTML = `
                <td>${route.destination}</td>
                <td>${route.next_hop}</td>
                <td>${route.hop_count}</td>
                <td>
                    <div class="quality-bar">
                        <div class="quality-fill" style="width: ${Math.min(100, (route.link_quality + 130) * 2)}%"></div>
                    </div>
                    ${route.link_quality} dBm
                </td>
                <td>${timeAgo}</td>
                <td>
                    <span class="status-badge ${route.active ? 'active' : 'inactive'}">
                        ${route.active ? 'Active' : 'Inactive'}
                    </span>
                </td>
            `;
        });
        
    } catch (error) {
        console.error('Error loading devices:', error);
        showToast('Failed to load devices: ' + error.message, 'error');
    }
}

// Show node details modal
function showNodeDetails(node) {
    const modal = document.getElementById('nodeModal');
    const details = document.getElementById('nodeDetails');
    
    const timeAgo = formatTimeDifference(node.last_seen * 1000000);
    const uptimeFormatted = formatUptime(node.uptime * 1000000);
    const signalQuality = calculateSignalQuality(node.rssi);
    const signalColor = getSignalColor(signalQuality);
    
    details.innerHTML = `
        <div class="node-detail-section">
            <h3><i class="fas fa-info-circle"></i> Basic Information</h3>
            <p><strong>Node ID:</strong> ${node.id}</p>
            <p><strong>Name:</strong> ${node.name}</p>
            <p><strong>Role:</strong> ${node.id === 1 ? 'Master' : 'Slave'}</p>
            <p><strong>Status:</strong> <span class="status-badge ${node.online ? 'active' : 'inactive'}">
                ${node.online ? 'Online' : 'Offline'}
            </span></p>
            <p><strong>Last Seen:</strong> ${timeAgo}</p>
            <p><strong>Uptime:</strong> ${uptimeFormatted}</p>
        </div>
        
        <div class="node-detail-section">
            <h3><i class="fas fa-chart-line"></i> Performance Metrics</h3>
            <p><strong>RSSI:</strong> ${node.rssi} dBm</p>
            <p><strong>Signal Quality:</strong> 
                <span style="color: ${signalColor}; font-weight: bold;">${signalQuality}%</span>
            </p>
            <p><strong>Hop Count:</strong> ${node.hop_count}</p>
        </div>
        
        <div class="node-detail-section">
            <h3><i class="fas fa-cogs"></i> Actions</h3>
            <div class="action-buttons">
                <button class="btn btn-small" onclick="pingNode(${node.id})">
                    <i class="fas fa-bullhorn"></i> Ping Node
                </button>
                <button class="btn btn-small" onclick="sendMessage(${node.id})">
                    <i class="fas fa-envelope"></i> Send Message
                </button>
                ${node.id !== 1 ? `
                    <button class="btn btn-small btn-danger" onclick="removeNode(${node.id})">
                        <i class="fas fa-trash"></i> Remove Node
                    </button>
                ` : ''}
            </div>
        </div>
    `;
    
    modal.style.display = 'block';
}

// Dashboard functions
function sendPing() {
    fetch('/api/ping', { method: 'POST' })
        .then(response => {
            showToast('Ping sent to all nodes', 'success');
            addActivity('Sent ping to all nodes', 'tx');
        })
        .catch(error => {
            console.error('Error sending ping:', error);
            showToast('Failed to send ping: ' + error.message, 'error');
        });
}

function discoverNodes() {
    fetch('/api/discover', { method: 'POST' })
        .then(response => {
            showToast('Node discovery initiated', 'success');
            addActivity('Initiating node discovery', 'tx');
        })
        .catch(error => {
            console.error('Error discovering nodes:', error);
            showToast('Failed to discover nodes: ' + error.message, 'error');
        });
}

function refreshNetwork() {
    if (window.location.pathname === '/') {
        updateDashboard();
    } else if (window.location.pathname === '/devices') {
        loadDevices();
    }
    showToast('Network data refreshed', 'info');
}

function addActivity(message, type) {
    const activityList = document.getElementById('activityList');
    if (!activityList) return;
    
    const activityItem = document.createElement('div');
    activityItem.className = 'activity-item';
    
    const iconClass = type === 'tx' ? 'fas fa-paper-plane' : 
                     type === 'rx' ? 'fas fa-broadcast-tower' : 
                     'fas fa-info-circle';
    const iconColor = type === 'tx' ? 'tx' : 
                     type === 'rx' ? 'rx' : 'info';
    
    activityItem.innerHTML = `
        <div class="activity-icon ${iconColor}">
            <i class="${iconClass}"></i>
        </div>
        <div class="activity-details">
            <p>${message}</p>
            <span class="activity-time">${getCurrentTime()}</span>
        </div>
    `;
    
    activityList.prepend(activityItem);
    
    // Keep only last 10 activities
    while (activityList.children.length > 10) {
        activityList.removeChild(activityList.lastChild);
    }
}

// Export functions for use in other scripts
window.CommonUtils = {
    showToast,
    formatBytes,
    formatTimeDifference,
    formatUptime,
    calculateSignalQuality,
    getSignalColor,
    debounce,
    throttle,
    copyToClipboard,
    isValidIP,
    isValidMAC,
    generateRandomString,
    getCurrentTime,
    getCurrentDate,
    getQueryParams,
    setQueryParam,
    removeQueryParam,
    isInViewport,
    smoothScrollTo,
    numberWithCommas,
    calculatePercentage,
    deepClone,
    mergeObjects
};

// Initialize based on current page
document.addEventListener('DOMContentLoaded', function() {
    const path = window.location.pathname;
    
    if (path === '/' || path === '/index.html') {
        // Dashboard page
        updateDashboard();
        setInterval(updateDashboard, 10000); // Update every 10 seconds
    } else if (path === '/devices') {
        // Devices page
        loadDevices();
        setInterval(loadDevices, 10000); // Update every 10 seconds
    } else if (path === '/config') {
        // Config page - loadConfig is called from lora_config.html
        console.log('Config page loaded');
    }
    
    // Update current node ID if available
    fetch('/api/config')
        .then(response => response.json())
        .then(config => {
            const nodeIdElements = document.querySelectorAll('#nodeId, #nodeIdValue');
            nodeIdElements.forEach(el => {
                if (el.id === 'nodeIdValue') {
                    el.textContent = config.node_id;
                } else {
                    el.textContent = config.node_id;
                }
            });
        })
        .catch(error => {
            console.error('Error loading node ID:', error);
        });
});