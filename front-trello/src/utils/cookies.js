import Cookies from 'js-cookie';

export const getCookie = (name) => {
  return Cookies.get(name);
};

export const removeCookies = () => {
  Cookies.remove('token');
  Cookies.remove('role');
  Cookies.remove('sessionExpiration');

  console.log('Cookies after removal:', document.cookie);
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