import React, { useState, useEffect, useRef } from 'react';
import './Chat.css';

const Chat = () => {
  const [currentUser, setCurrentUser] = useState('');
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [message, setMessage] = useState('');
  const [messages, setMessages] = useState([]);
  const [users, setUsers] = useState([]);
  const [selectedUser, setSelectedUser] = useState('');
  const messagesEndRef = useRef(null);

  const fetchUsers = async () => {
    console.log('Fetching users...');
    try {
      const response = await fetch('http://localhost:8000/getusers');
      const text = await response.text();
      console.log('Raw users response:', text);
      const userList = text.trim().split('\n').filter(u => u && u !== currentUser);
      console.log('Parsed users:', userList);
      setUsers(userList);
    } catch (err) {
      console.error('Failed to fetch users:', err);
    }
  };

  const fetchMessages = async () => {
  if (!selectedUser || !currentUser) return;
  
  try {
    console.log("Fetching messages between:", currentUser, "and", selectedUser);
    const response = await fetch(
      `http://localhost:8000/getmessages?user1=${currentUser}&user2=${selectedUser}`
    );
    const text = await response.text();
    console.log("Raw server response:", text);
    

    // More robust parsing
    const parsed = text.trim() === '' ? [] : text.trim()
      .split('\n')
      .filter(line => line.trim() !== '')
      .map(line => {
        const parts = line.split('|');
        // Ensure we have exactly 3 parts (sender|content|timestamp)
        if (parts.length === 3) {
          return {
            sender: parts[0].trim(),
            content: parts[1].trim(),
            timestamp: parts[2].trim()
          };
        }
        console.warn("Invalid message format:", line);
        return null;
      })
      .filter(msg => msg !== null); // Filter out invalid messages

    console.log("Parsed messages:", parsed);
    setMessages(parsed);
  } catch (err) {
    console.error('Fetch error:', err);
  }
};

  const sendMessage = async () => {
    if (!message.trim() || !selectedUser) {
      console.log('No message or selected user - not sending');
      return;
    }
    
    console.log(`Sending message to ${selectedUser}: ${message}`);
    try {
      const encodedMessage = encodeURIComponent(message);
      const url = `http://localhost:8000/send?from=${currentUser}&to=${selectedUser}&content=${encodedMessage}`;
      console.log('Sending to URL:', url);
      
      await fetch(url);
      setMessage('');
      await fetchMessages();
    } catch (err) {
      console.error('Failed to send message:', err);
    }
  };

  const handleLogin = (e) => {
    e.preventDefault();
    console.log('Logging in with username:', username);
    setCurrentUser(username);
  };

  useEffect(() => {
    if (currentUser) {
      console.log('Current user changed to:', currentUser);
      fetchUsers();
    }
  }, [currentUser]);

  useEffect(() => {
    if (currentUser && selectedUser) {
      console.log('Selected user changed to:', selectedUser);
      fetchMessages();
      const interval = setInterval(fetchMessages, 2000);
      return () => clearInterval(interval);
    }
  }, [currentUser, selectedUser]);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    console.log('Messages updated:', messages);
  }, [messages]);

  if (!currentUser) {
    return (
      <div className="login-container">
        <form onSubmit={handleLogin}>
          <h2>Login</h2>
          <input
            type="text"
            placeholder="Username"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            required
          />
          <input
            type="password"
            placeholder="Password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
          />
          <button type="submit">Login</button>
        </form>
      </div>
    );
  }

  return (
    <div className="chat-app">
      <div className="sidebar">
        <div className="header">
          <h3>Logged in as: {currentUser}</h3>
          <button onClick={() => setCurrentUser('')}>Logout</button>
        </div>
        <div className="user-list">
          <h4>Users</h4>
          {users.map((user) => (
            <div
              key={user}
              className={`user ${selectedUser === user ? 'active' : ''}`}
              onClick={() => setSelectedUser(user)}
            >
              {user}
            </div>
          ))}
        </div>
      </div>

      <div className="chat-area">
        {selectedUser ? (
          <>
            <div className="messages">
  {messages.length > 0 ? (
    messages.map((msg, i) => (
      <div 
        key={i} 
        className={`message ${msg.sender === currentUser ? 'sent' : 'received'}`}
      >
        <div className="message-content">
          {msg.content}
        </div>
        <div className="message-meta">
          <span className="sender">{msg.sender}</span>
          <span className="timestamp">{msg.timestamp}</span>
        </div>
      </div>
    ))
  ) : (
    <div className="no-messages">
      No messages yet. Start the conversation!
    </div>
  )}
  <div ref={messagesEndRef} />
</div>

            <div className="message-input">
              <input
                type="text"
                value={message}
                onChange={(e) => setMessage(e.target.value)}
                onKeyPress={(e) => e.key === 'Enter' && sendMessage()}
                placeholder="Type a message..."
              />
              <button onClick={sendMessage}>Send</button>
            </div>
          </>
        ) : (
          <div className="select-user">
            <p>Select a user to start chatting</p>
          </div>
        )}
      </div>
    </div>
  );
};

export default Chat;