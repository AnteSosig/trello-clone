// Security utility functions for XSS protection and input validation

/**
 * Sanitizes user input by removing potentially dangerous characters
 * @param {string} input - The input string to sanitize
 * @returns {string} - The sanitized string
 */
export const sanitizeInput = (input) => {
  if (typeof input !== 'string') return input;
  
  return input
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#x27;')
    .replace(/\//g, '&#x2F;')
    .trim();
};

/**
 * Validates and sanitizes form data
 * @param {Object} formData - The form data object
 * @returns {Object} - The sanitized form data
 */
export const sanitizeFormData = (formData) => {
  const sanitized = {};
  
  for (const [key, value] of Object.entries(formData)) {
    if (typeof value === 'string') {
      sanitized[key] = sanitizeInput(value);
    } else if (Array.isArray(value)) {
      sanitized[key] = value.map(item => 
        typeof item === 'string' ? sanitizeInput(item) : item
      );
    } else {
      sanitized[key] = value;
    }
  }
  
  return sanitized;
};

/**
 * Safely encodes URL parameters
 * @param {string} param - The parameter to encode
 * @returns {string} - The URL-encoded parameter
 */
export const encodeURLParam = (param) => {
  if (typeof param !== 'string') return '';
  return encodeURIComponent(param);
};

/**
 * Validates email format
 * @param {string} email - The email to validate
 * @returns {boolean} - True if valid email format
 */
export const validateEmail = (email) => {
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  return emailRegex.test(email);
};

/**
 * Validates username format (alphanumeric and underscores only)
 * @param {string} username - The username to validate
 * @returns {boolean} - True if valid username format
 */
export const validateUsername = (username) => {
  const usernameRegex = /^[a-zA-Z0-9_]{3,20}$/;
  return usernameRegex.test(username);
};

/**
 * Validates project name (no special characters except spaces, hyphens, underscores)
 * @param {string} projectName - The project name to validate
 * @returns {boolean} - True if valid project name
 */
export const validateProjectName = (projectName) => {
  const projectRegex = /^[a-zA-Z0-9\s\-_]{1,50}$/;
  return projectRegex.test(projectName);
};

/**
 * Validates password strength
 * @param {string} password - The password to validate
 * @returns {Object} - Validation result with isValid boolean and errors array
 */
export const validatePassword = (password) => {
  const errors = [];
  
  if (password.length < 8) {
    errors.push('Password must be at least 8 characters long');
  }
  
  if (!/[A-Z]/.test(password)) {
    errors.push('Password must contain at least one uppercase letter');
  }
  
  if (!/[a-z]/.test(password)) {
    errors.push('Password must contain at least one lowercase letter');
  }
  
  if (!/[0-9]/.test(password)) {
    errors.push('Password must contain at least one number');
  }
  
  if (!/[!@#$%^&*()_+\-=\[\]{};':"\\|,.<>\/?]/.test(password)) {
    errors.push('Password must contain at least one special character');
  }
  
  return {
    isValid: errors.length === 0,
    errors
  };
};

/**
 * Creates a safe API URL with encoded parameters
 * @param {string} baseUrl - The base URL
 * @param {Object} params - The parameters to encode
 * @returns {string} - The safe URL with encoded parameters
 */
export const createSafeApiUrl = (baseUrl, params = {}) => {
  const url = new URL(baseUrl);
  
  Object.entries(params).forEach(([key, value]) => {
    if (value !== null && value !== undefined) {
      url.searchParams.append(key, encodeURLParam(String(value)));
    }
  });
  
  return url.toString();
};

/**
 * Rate limiting helper for API calls
 * @param {Function} apiCall - The API function to rate limit
 * @param {number} delay - Delay in milliseconds
 * @returns {Function} - The rate-limited function
 */
export const rateLimit = (apiCall, delay = 1000) => {
  let lastCall = 0;
  
  return async (...args) => {
    const now = Date.now();
    const timeSinceLastCall = now - lastCall;
    
    if (timeSinceLastCall < delay) {
      await new Promise(resolve => setTimeout(resolve, delay - timeSinceLastCall));
    }
    
    lastCall = Date.now();
    return apiCall(...args);
  };
};
