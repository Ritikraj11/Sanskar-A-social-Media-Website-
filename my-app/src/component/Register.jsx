import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import './Register.css';

const Register = () => {
  const navigate = useNavigate();
  const [formData, setFormData] = useState({ 
    username: '', 
    password: '',
    confirmPassword: '' 
  });
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [isFocused, setIsFocused] = useState({
    username: false,
    password: false,
    confirmPassword: false
  });

  const handleChange = (e) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: value }));
  };

  const handleFocus = (field) => {
    setIsFocused(prev => ({ ...prev, [field]: true }));
  };

  const handleBlur = (field) => {
    setIsFocused(prev => ({ ...prev, [field]: false }));
  };

  const handleRegister = async (e) => {
    e.preventDefault();
    setError('');

    // Client-side validation
    if (!formData.username.trim() || !formData.password.trim()) {
      setError('Both fields are required');
      return;
    }

    if (formData.password.length < 6) {
      setError('Password must be at least 6 characters');
      return;
    }

    if (formData.password !== formData.confirmPassword) {
      setError('Passwords do not match');
      return;
    }

    setIsLoading(true);

    try {
      // Format data to match server expectations
      const requestData = {
        username: formData.username.trim(),
        password: formData.password.trim()
      };

      const response = await fetch('http://localhost:8080/api/register', {
        method: 'POST',
        headers: { 
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(requestData)
      });

      const responseText = await response.text();
      let data;
      
      try {
        data = JSON.parse(responseText);
      } catch {
        throw new Error('Invalid server response');
      }

      if (!response.ok) {
        throw new Error(data.message || 'Registration failed');
      }

      // Success - redirect to login with success state
      navigate('/login', { 
        state: { 
          registrationSuccess: true,
          username: formData.username 
        } 
      });
    } catch (err) {
      setError(err.message || 'Failed to connect to server');
      console.error('Registration error:', err);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="register-page">
      <div className="register-box">
        <h1 className="title">🌟 संस्कार</h1>
        <p className="subtitle">Create an account</p>
        
        {error && (
          <div className="error-message">
            <span className="error-icon">⚠️</span>
            {error}
          </div>
        )}

        <form onSubmit={handleRegister} className="register-form">
          <div className={`form-group ${isFocused.username ? 'focused' : ''}`}>
            <label>Username</label>
            <input
              type="text"
              name="username"
              value={formData.username}
              onChange={handleChange}
              onFocus={() => handleFocus('username')}
              onBlur={() => handleBlur('username')}
              placeholder="Enter username"
              required
              disabled={isLoading}
            />
          </div>
          
          <div className={`form-group ${isFocused.password ? 'focused' : ''}`}>
            <label>Password</label>
            <input
              type="password"
              name="password"
              value={formData.password}
              onChange={handleChange}
              onFocus={() => handleFocus('password')}
              onBlur={() => handleBlur('password')}
              placeholder="Enter password (min 6 characters)"
              minLength="6"
              required
              disabled={isLoading}
            />
          </div>

          <div className={`form-group ${isFocused.confirmPassword ? 'focused' : ''}`}>
            <label>Confirm Password</label>
            <input
              type="password"
              name="confirmPassword"
              value={formData.confirmPassword}
              onChange={handleChange}
              onFocus={() => handleFocus('confirmPassword')}
              onBlur={() => handleBlur('confirmPassword')}
              placeholder="Confirm your password"
              minLength="6"
              required
              disabled={isLoading}
            />
          </div>
          
          <button 
            className={`register-btn ${isLoading ? 'loading' : ''}`}
            type="submit"
            disabled={isLoading}
          >
            {isLoading ? (
              <>
                <span className="spinner"></span>
                Registering...
              </>
            ) : (
              'Register'
            )}
          </button>
        </form>

        <div className="login-section">
          <p>Already have an account?</p>
          <button 
            onClick={() => navigate('/login')} 
            className="login-btn"
            disabled={isLoading}
          >
            Login
          </button>
        </div>
      </div>
    </div>
  );
};

export default Register;