import React, { useState } from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import Login from './component/Login';
import Register from './component/Register';
import Home from './component/home';
import './App.css';

function App() {
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [username, setUsername] = useState('');

  return (
    <Router>
      <Routes>
        <Route path="/" element={
          isLoggedIn ? <Navigate to="/home" /> : <Navigate to="/login" />
        } />
        <Route path="/login" element={
          isLoggedIn ? <Navigate to="/home" /> : 
          <Login setIsLoggedIn={setIsLoggedIn} setUsername={setUsername} />
        } />
        <Route path="/register" element={
          isLoggedIn ? <Navigate to="/home" /> : <Register />
        } />
        <Route path="/home" element={
          isLoggedIn ? <Home username={username} setIsLoggedIn={setIsLoggedIn} /> : <Navigate to="/login" />
        } />
      </Routes>
    </Router>
  );
}

export default App;