import React from 'react';
import { useAuth } from '../contexts/AuthContext';

// Component for conditional rendering based on roles
export const RoleBasedRender = ({ 
  allowedRoles = [], 
  children, 
  fallback = null,
  requireAll = false 
}) => {
  const { hasRole, user } = useAuth();

  if (!user) {
    return fallback;
  }

  let hasPermission = false;

  if (requireAll) {
    // User must have ALL specified roles
    hasPermission = allowedRoles.every(role => hasRole(role));
  } else {
    // User must have AT LEAST ONE of the specified roles
    hasPermission = allowedRoles.some(role => hasRole(role));
  }

  return hasPermission ? children : fallback;
};

// Shorthand components for common role checks
export const ManagerOnly = ({ children, fallback = null }) => (
  <RoleBasedRender allowedRoles={['MANAGER']} fallback={fallback}>
    {children}
  </RoleBasedRender>
);

export const UserOnly = ({ children, fallback = null }) => (
  <RoleBasedRender allowedRoles={['USER']} fallback={fallback}>
    {children}
  </RoleBasedRender>
);

export const AuthenticatedOnly = ({ children, fallback = null }) => {
  const { isAuthenticated } = useAuth();
  return isAuthenticated ? children : fallback;
};

// Component for showing different content based on user's role
export const RoleSwitch = ({ 
  managerContent = null, 
  userContent = null, 
  fallback = null 
}) => {
  const { user, isManager } = useAuth();

  if (!user) return fallback;

  if (isManager()) return managerContent;
  if (user.role === 'USER') return userContent;
  
  return fallback;
};

// Hook for getting role-based permissions
export const usePermissions = () => {
  const { user, hasRole, isManager, isUser } = useAuth();

  const permissions = {
    // Project permissions
    canCreateProjects: isManager(),
    canEditProjects: isManager(),
    canDeleteProjects: isManager(),
    canManageMembers: isManager(),
    canViewProjects: isUser() || isManager(),

    // Task permissions
    canCreateTasks: isManager(),
    canEditTasks: isManager(),
    canDeleteTasks: isManager(),
    canAssignTasks: isManager(),
    canViewTasks: isUser() || isManager(),
    canUpdateTaskStatus: (taskMembers, taskCreator) => {
      if (isManager()) return true;
      if (!user) return false;
      return taskMembers?.includes(user.id) || taskCreator === user.id;
    },

    // User permissions
    canViewUsers: isUser() || isManager(),
    canManageUsers: isManager(),
  };

  return {
    permissions,
    hasRole,
    user,
    isManager: isManager(),
    isUser: isUser()
  };
};
