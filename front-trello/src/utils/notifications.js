import { getAuthHeader } from './cookies';

const API_BASE_URL = 'https://localhost:8443';

// Transform backend notification format to frontend format
const transformNotification = (notification) => {
  return {
    id: notification.notification_id,
    user_id: notification.user_id,
    type: notification.type,
    title: notification.title,
    message: notification.message,
    metadata: notification.metadata,
    is_read: notification.status === 1,
    created_at: notification.created_at,
    read_at: notification.read_at
  };
};

// Fetch notifications for the current user
export const fetchNotifications = async (unreadOnly = false) => {
  try {
    // The notification service will automatically use the authenticated user's ID from the JWT token
    // We don't need to specify user_id as a parameter since the service extracts it from the token
    const url = `${API_BASE_URL}/notification/notifications${unreadOnly ? '?unread_only=true' : ''}`;
    const response = await fetch(url, {
      method: 'GET',
      headers: {
        'Content-Type': 'application/json',
        ...getAuthHeader()
      }
    });

    if (!response.ok) {
      throw new Error(`Failed to fetch notifications: ${response.status}`);
    }

    const data = await response.json();
    
    // Transform the notifications to match frontend expectations
    const transformedData = Array.isArray(data) 
      ? data.map(transformNotification) 
      : [];
    
    // Sort by created_at descending (most recent first)
    transformedData.sort((a, b) => b.created_at - a.created_at);
    
    return transformedData;
  } catch (error) {
    console.error('Error fetching notifications:', error);
    throw error;
  }
};

// Mark a notification as read
export const markNotificationAsRead = async (notificationId) => {
  try {
    const response = await fetch(`${API_BASE_URL}/notification/notifications/${notificationId}/read`, {
      method: 'PATCH',
      headers: {
        'Content-Type': 'application/json',
        ...getAuthHeader()
      }
    });

    if (!response.ok) {
      throw new Error(`Failed to mark notification as read: ${response.status}`);
    }

    return await response.json();
  } catch (error) {
    console.error('Error marking notification as read:', error);
    throw error;
  }
};

// Create a notification (for managers)
export const createNotification = async (userId, type, title, message, metadata = null) => {
  try {
    const response = await fetch(`${API_BASE_URL}/notification/notifications`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        ...getAuthHeader()
      },
      body: JSON.stringify({
        user_id: userId,
        type,
        title,
        message,
        metadata
      })
    });

    if (!response.ok) {
      throw new Error(`Failed to create notification: ${response.status}`);
    }

    return await response.json();
  } catch (error) {
    console.error('Error creating notification:', error);
    throw error;
  }
};

// Helper function to format notification timestamps
export const formatNotificationTime = (timestamp) => {
  // Convert seconds to milliseconds if needed
  const date = new Date(timestamp * 1000);
  const now = new Date();
  const diffInMinutes = Math.floor((now - date) / (1000 * 60));

  if (diffInMinutes < 1) {
    return 'Just now';
  } else if (diffInMinutes < 60) {
    return `${diffInMinutes} minute${diffInMinutes > 1 ? 's' : ''} ago`;
  } else if (diffInMinutes < 1440) {
    const hours = Math.floor(diffInMinutes / 60);
    return `${hours} hour${hours > 1 ? 's' : ''} ago`;
  } else {
    const days = Math.floor(diffInMinutes / 1440);
    return `${days} day${days > 1 ? 's' : ''} ago`;
  }
};

// Get notification icon based on type
export const getNotificationIcon = (type) => {
  // Convert to uppercase for comparison
  const typeUpper = type?.toUpperCase();
  
  switch (typeUpper) {
    case 'PROJECT_MEMBER_ADDED':
    case 'TASK_MEMBER_ADDED':
    case 'MEMBER_ADDED':
    case 'PROJECT_CREATED':
      return '👥';
    case 'PROJECT_MEMBER_REMOVED':
    case 'TASK_MEMBER_REMOVED':
    case 'MEMBER_REMOVED':
      return '👤';
    case 'PROJECT_UPDATED':
      return '📋';
    case 'TASK_ASSIGNED':
      return '✅';
    case 'TASK_STATUS_CHANGED':
    case 'TASK_COMPLETED':
      return '🎉';
    case 'USER_REGISTERED':
      return '🎊';
    default:
      return '📢';
  }
};

// Get notification color based on type
export const getNotificationColor = (type) => {
  // Convert to uppercase for comparison
  const typeUpper = type?.toUpperCase();
  
  switch (typeUpper) {
    case 'PROJECT_MEMBER_ADDED':
    case 'TASK_MEMBER_ADDED':
    case 'MEMBER_ADDED':
    case 'USER_REGISTERED':
    case 'PROJECT_CREATED':
      return 'bg-green-100 border-green-200 text-green-800';
    case 'PROJECT_MEMBER_REMOVED':
    case 'TASK_MEMBER_REMOVED':
    case 'MEMBER_REMOVED':
      return 'bg-red-100 border-red-200 text-red-800';
    case 'PROJECT_UPDATED':
      return 'bg-blue-100 border-blue-200 text-blue-800';
    case 'TASK_ASSIGNED':
      return 'bg-yellow-100 border-yellow-200 text-yellow-800';
    case 'TASK_STATUS_CHANGED':
    case 'TASK_COMPLETED':
      return 'bg-purple-100 border-purple-200 text-purple-800';
    default:
      return 'bg-gray-100 border-gray-200 text-gray-800';
  }
};
