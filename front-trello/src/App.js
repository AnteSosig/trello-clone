import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import { AuthProvider } from './contexts/AuthContext';
import { ProtectedRoute, PublicRoute, RoleProtectedRoute } from './components/ProtectedRoute';
import Navbar from './components/Navbar';
import Home from './pages/Home';
import LoginForm from './components/Login';
import RegisterUser from './pages/RegisterUser';
import SearchFilter from './components/SearchFilter';
import NotFound from './pages/NotFound';
import EditProject from './pages/EditProject';
import AccountConfirm from './pages/AccountConfirm';
import AddTasks from './pages/AddTasks';

function App() {
  return (
    <AuthProvider>
      <Router>
        <Navbar />
        <Routes>
          {/* Protected routes - require authentication */}
          <Route path="/" element={
            <ProtectedRoute>
              <Home />
            </ProtectedRoute>
          } />
          
          <Route path="/pretraga" element={
            <ProtectedRoute>
              <SearchFilter />
            </ProtectedRoute>
          } />
          
          <Route path="/add-tasks/:projectId" element={
            <ProtectedRoute>
              <AddTasks />
            </ProtectedRoute>
          } />
          
          {/* Manager-only routes */}
          <Route path="/edit-project/:id" element={
            <ProtectedRoute>
              <RoleProtectedRoute requiredRole="MANAGER">
                <EditProject />
              </RoleProtectedRoute>
            </ProtectedRoute>
          } />

          {/* Public routes - redirect to home if authenticated */}
          <Route path="/login" element={
            <PublicRoute>
              <LoginForm />
            </PublicRoute>
          } />
          
          <Route path="/register" element={
            <PublicRoute>
              <RegisterUser />
            </PublicRoute>
          } />

          {/* Public routes that don't redirect */}
          <Route path="/activate" element={<AccountConfirm />} />
          
          {/* 404 page */}
          <Route path="*" element={<NotFound />} />
        </Routes>
      </Router>
    </AuthProvider>
  );
}

export default App;
