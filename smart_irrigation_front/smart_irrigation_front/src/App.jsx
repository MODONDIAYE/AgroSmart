import { Routes, Route, Navigate } from 'react-router-dom';
import { useAuth } from './context/AuthContext';
import Layout from './components/Layout/Layout';
import Login from './pages/Auth/Login';
import Register from './pages/Auth/Register';
import Dashboard from './pages/Dashboard/Dashboard';
import Crops from './pages/Crops/Crops';
import Irrigation from './pages/Irrigation/Irrigation';
import Notifications from './pages/Notifications/Notifications';
import Devices from './pages/Devices/Devices';
import Settings from './pages/Settings/Settings';
import Chatbot from './pages/Chatbot/Chatbot';
import Simulator from './pages/Simulator/Simulator';

function PrivateRoute({ children }) {
  const { user } = useAuth();
  return user ? children : <Navigate to="/login" replace />;
}

function PublicRoute({ children }) {
  const { user } = useAuth();
  return user ? <Navigate to="/dashboard" replace /> : children;
}

export default function App() {
  return (
    <Routes>
      <Route path="/login"    element={<PublicRoute><Login /></PublicRoute>} />
      <Route path="/register" element={<PublicRoute><Register /></PublicRoute>} />

      <Route path="/" element={<PrivateRoute><Layout /></PrivateRoute>}>
        <Route index element={<Navigate to="/dashboard" replace />} />
        <Route path="dashboard"     element={<Dashboard />} />
        <Route path="cultures"      element={<Crops />} />
        <Route path="irrigation"    element={<Irrigation />} />
        <Route path="notifications" element={<Notifications />} />
        <Route path="settings"      element={<Settings />} />
        <Route path="assistant"     element={<Chatbot />} />
        <Route path="simulateur"   element={<Simulator />} />
        <Route path="appareils"     element={<Devices />} />
      </Route>

      <Route path="*" element={<Navigate to="/dashboard" replace />} />
    </Routes>
  );
}
