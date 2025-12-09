let ws;
let username = "";
let reconnectInterval = null;

function joinChat() {
    const input = document.getElementById('usernameInput');
    username = input.value.trim();
    
    if (username === "") {
        alert("Please enter your name!");
        return;
    }
    
    console.log('[Client] Joining chat as:', username);
    
    document.getElementById('loginScreen').style.display = 'none';
    document.getElementById('chatScreen').style.display = 'flex';
    document.getElementById('currentUser').textContent = username;
    
    connectWebSocket();
}

function connectWebSocket() {
    // WebSocket on same port as HTTP (80), path /ws
    const wsUrl = 'ws://' + window.location.hostname + '/ws';
    console.log('[Client] Connecting to:', wsUrl);
    
    ws = new WebSocket(wsUrl);
    
    ws.onopen = function() {
        console.log('[Client] WebSocket CONNECTED');
        addSystemMessage('Connected to server');
        
        const joinMsg = JSON.stringify({
            type: 'join',
            username: username
        });
        console.log('[Client] Sending join message:', joinMsg);
        ws.send(joinMsg);
        
        if (reconnectInterval) {
            clearInterval(reconnectInterval);
            reconnectInterval = null;
        }
    };
    
    ws.onmessage = function(event) {
        console.log('[Client] Received:', event.data);
        const data = JSON.parse(event.data);
        
        if (data.type === 'message') {
            addMessage(data.username, data.text, data.username === username);
        } else if (data.type === 'system') {
            addSystemMessage(data.text);
        } else if (data.type === 'userlist') {
            updateUserList(data.users);
        }
    };
    
    ws.onerror = function(error) {
        console.error('[Client] WebSocket ERROR:', error);
        addSystemMessage('Connection error');
    };
    
    ws.onclose = function() {
        console.log('[Client] WebSocket CLOSED');
        addSystemMessage('Disconnected. Reconnecting...');
        
        if (!reconnectInterval) {
            reconnectInterval = setInterval(function() {
                if (ws.readyState === WebSocket.CLOSED) {
                    console.log('[Client] Attempting reconnect...');
                    connectWebSocket();
                }
            }, 3000);
        }
    };
}

function sendMessage() {
    const input = document.getElementById('messageInput');
    const text = input.value.trim();
    
    if (text === "") {
        console.log('[Client] Empty message, not sending');
        return;
    }
    
    if (ws.readyState !== WebSocket.OPEN) {
        console.error('[Client] WebSocket not open! State:', ws.readyState);
        addSystemMessage('Not connected! Please wait...');
        return;
    }
    
    const msg = JSON.stringify({
        type: 'message',
        username: username,
        text: text
    });
    
    console.log('[Client] Sending message:', msg);
    ws.send(msg);
    
    input.value = '';
}

function addMessage(sender, text, isSelf) {
    const messageArea = document.getElementById('messageArea');
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message ' + (isSelf ? 'self' : 'other');
    
    const time = new Date().toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' });
    
    messageDiv.innerHTML = `
        <div class="message-sender">${sender}</div>
        <div class="message-text">${escapeHtml(text)}</div>
        <div class="message-time">${time}</div>
    `;
    
    messageArea.appendChild(messageDiv);
    messageArea.scrollTop = messageArea.scrollHeight;
}

function addSystemMessage(text) {
    const messageArea = document.getElementById('messageArea');
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message system';
    messageDiv.textContent = text;
    
    messageArea.appendChild(messageDiv);
    messageArea.scrollTop = messageArea.scrollHeight;
}

function updateUserList(users) {
    const userListSpan = document.getElementById('userList');
    userListSpan.textContent = users.length + ': ' + users.join(', ');
    console.log('[Client] User list updated:', users);
}

function leaveChat() {
    if (ws) {
        ws.close();
    }
    
    if (reconnectInterval) {
        clearInterval(reconnectInterval);
    }
    
    document.getElementById('chatScreen').style.display = 'none';
    document.getElementById('loginScreen').style.display = 'flex';
    document.getElementById('messageArea').innerHTML = '';
    document.getElementById('usernameInput').value = '';
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Enter key to send
document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('messageInput').addEventListener('keypress', function(e) {
        if (e.key === 'Enter') {
            sendMessage();
        }
    });
    
    document.getElementById('usernameInput').addEventListener('keypress', function(e) {
        if (e.key === 'Enter') {
            joinChat();
        }
    });
});
