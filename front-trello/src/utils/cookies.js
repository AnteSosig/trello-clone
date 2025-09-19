import Cookies from 'js-cookie';

export const getCookie = (name) => {
  return Cookies.get(name);
};

export const removeCookies = () => {
  Cookies.remove('token');
  Cookies.remove('role');
  Cookies.remove('sessionExpiration');
};

export const isSessionValid = () => {
  const expiration = Cookies.get('sessionExpiration');
  if (!expiration) return false;
  
  return new Date(expiration) > new Date();
};

export const getAuthHeader = () => {
  const token = Cookies.get('token');
  return token ? { 'Authorization': `Bearer ${token}` } : {};
};

/**
 * Sets a secure cookie with enhanced security options
 * @param {string} name - Cookie name
 * @param {string} value - Cookie value
 * @param {number} expirationInDays - Expiration in days
 */
export const setSecureCookie = (name, value, expirationInDays) => {
  const cookieOptions = {
    expires: expirationInDays,
    sameSite: 'strict',
    secure: window.location.protocol === 'https:', // Only secure if HTTPS
    path: '/'
  };
  
  // Note: httpOnly cannot be set from JavaScript for security reasons
  // This should be set by the server
  Cookies.set(name, value, cookieOptions);
}; 