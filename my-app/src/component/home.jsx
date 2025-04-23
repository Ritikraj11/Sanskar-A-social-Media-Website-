import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { FaBell, FaComments, FaSignOutAlt, FaUser } from 'react-icons/fa';
import './Home.css';

const Home = ({ username, setIsLoggedIn }) => {
  const navigate = useNavigate();
  const [postText, setPostText] = useState('');
  const [posts, setPosts] = useState([]);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState('');

  const fetchPosts = async () => {
    try {
      const res = await fetch('http://localhost:7000/api/posts');
      const data = await res.json();
      setPosts(data.reverse()); // newest first
    } catch (err) {
      console.error('Error fetching posts:', err);
    }
  };

  useEffect(() => {
    fetchPosts();
  }, []);

  const handleLogout = () => {
    setIsLoggedIn(false);
    navigate('/login');
  };

  const handlePost = async () => {
    if (!postText.trim()) {
      setError('Post cannot be empty');
      return;
    }

    setIsLoading(true);
    setError('');

    try {
      const res = await fetch('http://localhost:7000/api/posts', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ author: username, content: postText })
      });

      if (res.ok) {
        const newPost = await res.json();
        setPosts((prev) => [newPost, ...prev]);
        setPostText('');
      } else {
        setError('Failed to post. Try again.');
      }
    } catch (err) {
      setError('Server error');
    }

    setIsLoading(false);
  };

  const handleKeyPress = (e) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handlePost();
    }
  };

  return (
    <div className="home-container">
      <header className="home-header">
        <div className="app-title">
          <span className="sanskrit">संस्कार</span>
          <span className="english">Sanskruti</span>
        </div>
        <div className="header-controls">
          <div className="user-info">
            <FaUser className="user-icon" />
            <span className="username">{username}</span>
          </div>
          <div className="header-icons">
            <FaComments className="icon" title="Chat" />
            <FaBell className="icon" title="Notifications" />
            <FaSignOutAlt
              className="icon logout"
              onClick={handleLogout}
              title="Logout"
            />
          </div>
        </div>
      </header>

      <main className="feed-container">
        <div className="post-creator">
          <textarea
            className="post-input"
            value={postText}
            onChange={(e) => setPostText(e.target.value)}
            onKeyDown={handleKeyPress}
            placeholder={`What's on your mind, ${username}?`}
            rows={3}
            maxLength={500}
          />
          <div className="post-controls">
            <span className="char-count">{postText.length}/500</span>
            <button
              className="post-btn"
              onClick={handlePost}
              disabled={isLoading || !postText.trim()}
            >
              {isLoading ? 'Posting...' : 'Post'}
            </button>
          </div>
          {error && <p className="error-message">{error}</p>}
        </div>

        <div className="posts-feed">
          {posts.length === 0 ? (
            <div className="empty-feed">
              <p>No posts yet. Be the first to share something!</p>
            </div>
          ) : (
            posts.map((post) => (
              <div className="post-card" key={post.id}>
                <div className="post-header">
                  <FaUser className="post-author-icon" />
                  <div className="post-meta">
                    <span className="post-author">{post.author}</span>
                    <span className="post-time">{post.title}</span>
                  </div>
                </div>
                <div className="post-content">
                  <p>{post.content}</p>
                </div>
              </div>
            ))
          )}
        </div>
      </main>
    </div>
  );
};

export default Home;
