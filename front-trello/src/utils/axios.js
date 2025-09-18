import axios from 'axios';
import { getAuthHeader, removeCookies } from './cookies';

const api = axios.create({
  baseURL: 'http://localhost:8080'
});

api.interceptors.request.use(config => {
  const headers = getAuthHeader();
  config.headers = {
    ...config.headers,
    ...headers
  };
  return config;
});

api.interceptors.response.use(
  response => response,
  error => {
    if (error.response?.status === 401) {
      removeCookies();
      window.location.href = '/login';
    }
    return Promise.reject(error);
  }
);

export default api; 