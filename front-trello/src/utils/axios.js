import axios from 'axios';
import { getAuthHeader, removeCookies } from './cookies';

// Create multiple API instances for different services
const createApiInstance = (baseURL) => {
  const api = axios.create({ baseURL });

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

  return api;
};

// API instances for each service
export const userApi = createApiInstance('http://localhost:8080');
export const projectApi = createApiInstance('http://localhost:8081');
export const taskApi = createApiInstance('http://localhost:8082');

// Default export for backward compatibility (user service)
export default userApi; 