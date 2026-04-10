import axios from 'axios';

const API_BASE = import.meta.env.VITE_API_URL || 'http://localhost:8000/api';

const api = axios.create({
  baseURL: API_BASE,
  headers: { 'Content-Type': 'application/json', 'Accept': 'application/json' },
});

// Inject token on every request
api.interceptors.request.use((config) => {
  const token = localStorage.getItem('auth_token');
  if (token) config.headers.Authorization = `Bearer ${token}`;
  return config;
});

// Handle 401 globally
api.interceptors.response.use(
  (res) => res,
  (err) => {
    if (err.response?.status === 401) {
      localStorage.removeItem('auth_token');
      localStorage.removeItem('auth_user');
      window.location.href = '/login';
    }
    return Promise.reject(err);
  }
);

/* ── AUTH ─────────────────────────────────────────── */
export const authService = {
  login:    (data) => api.post('/login',    data),
  register: (data) => api.post('/register', data),
  logout:   ()     => api.post('/logout'),
};

/* ── CROPS ────────────────────────────────────────── */
export const cropsService = {
  getAll: ()     => api.get('/crops'),
  create: (data) => api.post('/crops', data),
  delete: (id)   => api.delete(`/crops/${id}`),
};

/* ── DEVICES ──────────────────────────────────────── */
export const deviceService = {
  getAll:  ()     => api.get('/devices'),
  getById: (id)   => api.get(`/devices/${id}`),
  create:  (data) => api.post('/devices', data),
  update:  (id, data) => api.put(`/devices/${id}`, data),
};

/* ── SENSOR READINGS ──────────────────────────────── */
export const sensorService = {
  getLatest:  (deviceId)          => api.get(`/devices/${deviceId}/sensors/latest`),
  getHistory: (deviceId, params)  => api.get(`/devices/${deviceId}/sensors`, { params }),
};

/* ── IRRIGATION EVENTS ────────────────────────────── */
export const irrigationService = {
  getEvents:   (deviceId) => api.get(`/devices/${deviceId}/events`),
  triggerManual: (deviceId, data) => api.post(`/devices/${deviceId}/irrigate`, data),
  stopIrrigation: (deviceId)      => api.post(`/devices/${deviceId}/stop`),
};

/* ── IRRIGATION TIMES ─────────────────────────────── */
export const irrigationTimeService = {
  create:  (data) => api.post('/irrigation-times', data),
  delete:  (id)   => api.delete(`/irrigation-times/${id}`),
};

/* ── NOTIFICATIONS ────────────────────────────────── */
export const notificationService = {
  getAll:     ()   => api.get('/notifications'),
  markAsRead: (id) => api.put(`/notifications/${id}/read`),
  markAllRead: ()  => api.put('/notifications/read-all'),
};

export default api;
