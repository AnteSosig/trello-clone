import React, { createContext, useContext, useState, useEffect } from 'react';
import { getCookie, removeCookies, isSessionValid } from '../utils/cookies';
import { jwtDecode } from 'jwt-decode';

const AuthContext = createContext();

export const useAuth = () => {
  const context = useContext(AuthContext);
  if (!context) {
    throw new Error('useAuth must be used within an AuthProvider');
  }
  return context;
};

export const AuthProvider = ({ children }) => {
  const [isAuthenticated, setIsAuthenticated] = useState(false);
  const [user, setUser] = useState(null);
  const [loading, setLoading] = useState(true);

  const checkAuth = () => {
    try {
      const token = getCookie('token');
      const role = getCookie('role');
      
      if (!token || !isSessionValid()) {
        logout();
        return false;
      }

      const decodedToken = jwtDecode(token);
      const userInfo = {
        id: decodedToken.sub,
        role: role || decodedToken.aud,
        token: token
      };

      setUser(userInfo);
      setIsAuthenticated(true);
      return true;
    } catch (error) {
      console.error('Error checking authentication:', error);
      logout();
      return false;
    }
  };

  const login = (token, role) => {
    try {
      const decodedToken = jwtDecode(token);
      const userInfo = {
        id: decodedToken.sub,
        role: role,
        token: token
      };
      
      setUser(userInfo);
      setIsAuthenticated(true);
      return true;
    } catch (error) {
      console.error('Error during login:', error);
      return false;
    }
  };

  const logout = () => {
    removeCookies();
    setUser(null);
    setIsAuthenticated(false);
  };

  const hasRole = (requiredRole) => {
    if (!user) return false;
    if (requiredRole === 'USER') return user.role === 'USER' || user.role === 'MANAGER';
    if (requiredRole === 'MANAGER') return user.role === 'MANAGER';
    return false;
  };

  const isManager = () => hasRole('MANAGER');
  const isUser = () => hasRole('USER');

  useEffect(() => {
    setLoading(true);
    checkAuth();
    setLoading(false);

    // Check authentication periodically
    const interval = setInterval(() => {
      if (!checkAuth() && isAuthenticated) {
        // Session expired
        logout();
        window.location.href = '/login';
      }
    }, 60000); // Check every minute

    return () => clearInterval(interval);
  }, []);

  const value = {
    isAuthenticated,
    user,
    loading,
    login,
    logout,
    hasRole,
    isManager,
    isUser,
    checkAuth
  };

  return (
    <AuthContext.Provider value={value}>
      {children}
    </AuthContext.Provider>
  );
};
