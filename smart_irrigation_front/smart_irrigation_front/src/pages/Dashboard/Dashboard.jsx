import { useState, useEffect, useCallback } from 'react';
import {
  Thermometer, Droplets, Sun, Gauge, RefreshCw,
  Play, StopCircle, Clock, Activity, Wifi, WifiOff
} from 'lucide-react';
import {
  AreaChart, Area, XAxis, YAxis, CartesianGrid,
  Tooltip, ResponsiveContainer, Legend
} from 'recharts';
import { sensorService, irrigationService, deviceService } from '../../services/api';
import { useAuth } from '../../context/AuthContext';
import toast from 'react-hot-toast';
import { format } from 'date-fns';
import { fr } from 'date-fns/locale';

const REFRESH_INTERVAL = 30000; // 30s

function StatCard({ icon: Icon, label, value, unit, color, sub }) {
  return (
    <div className="stat-card animate-slide-up">
      <div className="flex items-start justify-between">
        <div className={`w-10 h-10 rounded-xl flex items-center justify-center ${color}`}>
          <Icon size={20} className="text-white" />
        </div>
      </div>
      <div>
        <p className="text-2xl font-display font-bold text-stone-900">
          {value !== null && value !== undefined ? value : '—'}
          {value !== null && value !== undefined && <span className="text-base font-body font-normal text-stone-400 ml-1">{unit}</span>}
        </p>
        <p className="text-sm text-stone-500 font-body">{label}</p>
        {sub && <p className="text-xs text-stone-400 font-body mt-0.5">{sub}</p>}
      </div>
    </div>
  );
}

export default function Dashboard() {
  const { user } = useAuth();
  const [devices, setDevices]   = useState([]);
  const [activeDevice, setActiveDevice] = useState(null);
  const [sensors, setSensors]   = useState(null);
  const [history, setHistory]   = useState([]);
  const [readings, setReadings] = useState([]);
  const [events,  setEvents]    = useState([]);
  const [loading, setLoading]   = useState(true);
  const [pumping, setPumping]   = useState(false);
  const [refreshing, setRefreshing] = useState(false);

  const loadDevices = useCallback(async () => {
    try {
      const { data } = await deviceService.getAll();
      if (data.success && data.data.length > 0) {
        setDevices(data.data);
        if (!activeDevice) setActiveDevice(data.data[0]);
      }
    } catch (_) {}
  }, [activeDevice]);

  const loadSensorData = useCallback(async (deviceId) => {
    if (!deviceId) return;
    try {
      const [latestRes, historyRes, readingsRes, eventsRes] = await Promise.all([
        sensorService.getLatest(deviceId),
        sensorService.getHistory(deviceId, { limit: 24 }),
        sensorService.getAll(deviceId),
        irrigationService.getEvents(deviceId),
      ]);

      if (latestRes.data.success) setSensors(latestRes.data.data);
      if (historyRes.data.success) setHistory(historyRes.data.data);
      if (readingsRes.data.success) setReadings(readingsRes.data.data);
      if (eventsRes.data.success) setEvents(eventsRes.data.data.slice(0, 10));
    } catch (_) {
      setReadings([]);
    }
  }, []);

  const refresh = useCallback(async () => {
    setRefreshing(true);
    await loadSensorData(activeDevice?.id);
    setRefreshing(false);
  }, [activeDevice, loadSensorData]);

  useEffect(() => {
    loadDevices().finally(() => setLoading(false));
  }, []);

  useEffect(() => {
    if (activeDevice?.id) {
      loadSensorData(activeDevice.id);
      const timer = setInterval(() => loadSensorData(activeDevice.id), REFRESH_INTERVAL);
      return () => clearInterval(timer);
    }
  }, [activeDevice]);

  const handleManualIrrigate = async () => {
    if (!activeDevice) return;
    try {
      setPumping(true);
      await irrigationService.triggerManual(activeDevice.id, { duration_seconds: 300, mode: 'Manual' });
      toast.success('Irrigation démarrée manuellement !');
      setTimeout(() => loadSensorData(activeDevice.id), 1500);
    } catch (_) {
      toast.error('Impossible de démarrer l\'irrigation');
    } finally {
      setPumping(false);
    }
  };

  const handleStop = async () => {
    if (!activeDevice) return;
    try {
      await irrigationService.stopIrrigation(activeDevice.id);
      toast.success('Irrigation arrêtée');
    } catch (_) {
      toast.error('Impossible d\'arrêter l\'irrigation');
    }
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="flex flex-col items-center gap-3">
          <svg className="animate-spin h-8 w-8 text-primary-600" viewBox="0 0 24 24" fill="none">
            <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"/>
            <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8H4z"/>
          </svg>
          <p className="text-stone-500 font-body text-sm">Chargement des données…</p>
        </div>
      </div>
    );
  }

  const chartData = history.map((r) => ({
    heure: format(new Date(r.timestamp), 'HH:mm', { locale: fr }),
    'Humidité sol': r.soil_humidity ? Math.round(r.soil_humidity) : null,
    'Température': r.temperature ? Math.round(r.temperature * 10) / 10 : null,
    'Niveau eau': r.water_level ? Math.round(r.water_level) : null,
  }));

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <h1 className="font-display text-2xl font-bold text-stone-900">
            Bonjour, {user?.username} 👋
          </h1>
          <p className="text-stone-500 text-sm font-body mt-0.5">
            {format(new Date(), "EEEE d MMMM yyyy", { locale: fr })}
          </p>
        </div>
        <div className="flex items-center gap-2">
          {/* Device selector */}
          {devices.length > 1 && (
            <select
              value={activeDevice?.id || ''}
              onChange={(e) => setActiveDevice(devices.find((d) => d.id === +e.target.value))}
              className="input-field !w-auto py-2 text-sm"
            >
              {devices.map((d) => (
                <option key={d.id} value={d.id}>{d.device_name}</option>
              ))}
            </select>
          )}
          <button onClick={refresh} disabled={refreshing}
            className="btn-secondary py-2 px-3">
            <RefreshCw size={16} className={refreshing ? 'animate-spin' : ''} />
            <span className="hidden sm:inline">Actualiser</span>
          </button>
        </div>
      </div>

      {/* Device status */}
      {activeDevice && (
        <div className="card flex flex-col sm:flex-row sm:items-center gap-4 animate-slide-up">
          <div className="flex items-center gap-3">
            <div className={`w-3 h-3 rounded-full ${activeDevice.status === 'online' ? 'bg-primary-500 animate-pulse' : 'bg-stone-300'}`} />
            <div>
              <p className="font-medium text-stone-900 font-body">{activeDevice.device_name}</p>
              <p className="text-xs text-stone-500 font-body font-mono">{activeDevice.device_key}</p>
            </div>
            <span className={activeDevice.status === 'online' ? 'badge-online' : 'badge-offline'}>
              {activeDevice.status === 'online' ? <><Wifi size={10} /> En ligne</> : <><WifiOff size={10} /> Hors ligne</>}
            </span>
          </div>
          {activeDevice.location && (
            <p className="text-sm text-stone-500 font-body sm:ml-4">📍 {activeDevice.location}</p>
          )}
          {/* Manual control */}
          <div className="sm:ml-auto flex gap-2">
            <button onClick={handleManualIrrigate} disabled={pumping}
              className="btn-primary py-2">
              <Play size={15} />
              {pumping ? 'En cours…' : 'Irriguer'}
            </button>
            <button onClick={handleStop} className="btn-danger py-2">
              <StopCircle size={15} /> Arrêter
            </button>
          </div>
        </div>
      )}

      {/* Stat cards */}
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-4">
        <StatCard icon={Droplets}    label="Humidité du sol"   value={sensors?.soil_humidity ? Math.round(sensors.soil_humidity) : null} unit="%" color="bg-water-500"   sub="Dernière lecture" />
        <StatCard icon={Thermometer} label="Température air"    value={sensors?.air_temperature ? Math.round(sensors.air_temperature * 10) / 10 : null} unit="°C" color="bg-earth-500" sub="Capteur BME280" />
        <StatCard icon={Gauge}       label="Niveau d'eau"      value={sensors?.water_level ? Math.round(sensors.water_level) : null} unit="%" color="bg-primary-600" sub="Réservoir" />
        <StatCard icon={Activity}    label="Flux total"         value={sensors?.liters ? Math.round(sensors.liters) : null} unit="L" color="bg-stone-500" sub="Dernière lecture" />
      </div>

      {/* Chart */}
      {chartData.length > 0 && (
        <div className="card animate-slide-up">
          <div className="flex items-center justify-between mb-6">
            <h2 className="font-display font-bold text-stone-900 text-lg">Historique des capteurs</h2>
            <span className="text-xs text-stone-400 font-body">24 dernières heures</span>
          </div>
          <ResponsiveContainer width="100%" height={260}>
            <AreaChart data={chartData} margin={{ top: 5, right: 10, left: -20, bottom: 0 }}>
              <defs>
                <linearGradient id="gradH" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%" stopColor="#3b96f5" stopOpacity={0.3}/>
                  <stop offset="95%" stopColor="#3b96f5" stopOpacity={0}/>
                </linearGradient>
                <linearGradient id="gradT" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%" stopColor="#d4801f" stopOpacity={0.3}/>
                  <stop offset="95%" stopColor="#d4801f" stopOpacity={0}/>
                </linearGradient>
              </defs>
              <CartesianGrid strokeDasharray="3 3" stroke="#f0f0f0" />
              <XAxis dataKey="heure" tick={{ fontSize: 11, fontFamily: '"DM Sans"' }} />
              <YAxis tick={{ fontSize: 11, fontFamily: '"DM Sans"' }} />
              <Tooltip contentStyle={{ borderRadius: 12, fontFamily: '"DM Sans"', fontSize: 13, border: '1px solid #e7e5e4' }} />
              <Legend wrapperStyle={{ fontFamily: '"DM Sans"', fontSize: 13 }} />
              <Area type="monotone" dataKey="Humidité sol"    stroke="#3b96f5" fill="url(#gradH)" strokeWidth={2} dot={false} />
              <Area type="monotone" dataKey="Température" stroke="#d4801f" fill="url(#gradT)" strokeWidth={2} dot={false} />
            </AreaChart>
          </ResponsiveContainer>
        </div>
      )}

      {readings.length > 0 && (
        <div className="card animate-slide-up">
          <div className="flex items-center justify-between mb-4">
            <div>
              <h2 className="font-display font-bold text-stone-900 text-lg">Lectures brutes</h2>
              <p className="text-sm text-stone-500 font-body mt-0.5">Toutes les lignes de sensor_data chargées depuis la base.</p>
            </div>
          </div>
          <div className="overflow-x-auto">
            <table className="min-w-full text-left text-sm font-body">
              <thead>
                <tr className="text-stone-500 border-b border-stone-200">
                  <th className="px-3 py-3">Horodatage</th>
                  <th className="px-3 py-3">Capteur</th>
                  <th className="px-3 py-3">Valeur</th>
                  <th className="px-3 py-3">Unité</th>
                </tr>
              </thead>
              <tbody>
                {readings.map((row) => (
                  <tr key={row.id} className="border-b border-stone-100 hover:bg-stone-50">
                    <td className="px-3 py-3 text-stone-600">{format(new Date(row.created_at), 'd MMM HH:mm', { locale: fr })}</td>
                    <td className="px-3 py-3 text-stone-700">{row.sensor_name}</td>
                    <td className="px-3 py-3 text-stone-700">{row.value}</td>
                    <td className="px-3 py-3 text-stone-500">{row.unit}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* Recent events */}
      {events.length > 0 && (
        <div className="card animate-slide-up">
          <h2 className="font-display font-bold text-stone-900 text-lg mb-4">Événements récents</h2>
          <div className="space-y-2">
            {events.map((ev) => (
              <div key={ev.id} className="flex items-center gap-3 py-2 border-b border-stone-50 last:border-0">
                <div className={`w-7 h-7 rounded-full flex items-center justify-center shrink-0 ${(ev.action === 1 || ev.action === 'start') ? 'bg-primary-100 text-primary-600' : 'bg-red-100 text-red-500'}`}>
                  {(ev.action === 1 || ev.action === 'start') ? <Play size={12} /> : <StopCircle size={12} />}
                </div>
                <div className="flex-1 min-w-0">
                  <p className="text-sm font-medium text-stone-800 font-body capitalize">
                    Irrigation {(ev.action === 1 || ev.action === 'start') ? 'démarrée' : 'arrêtée'}
                    <span className="ml-2 text-xs font-normal text-stone-400 font-body">[{ev.mode}]</span>
                  </p>
                  {ev.duration_seconds && (
                    <p className="text-xs text-stone-400 font-body">Durée : {ev.duration_seconds}s</p>
                  )}
                </div>
                <div className="text-right shrink-0">
                  <p className="text-xs text-stone-500 font-mono">
                    {format(new Date(ev.timestamp), 'HH:mm', { locale: fr })}
                  </p>
                  <p className="text-xs text-stone-400 font-body">
                    {format(new Date(ev.timestamp), 'd MMM', { locale: fr })}
                  </p>
                </div>
              </div>
            ))}
          </div>
        </div>
      )}

      {/* Empty state */}
      {!activeDevice && (
        <div className="card flex flex-col items-center justify-center py-16 text-center animate-fade-in">
          <div className="w-16 h-16 rounded-2xl bg-stone-100 flex items-center justify-center mb-4">
            <Wifi size={28} className="text-stone-400" />
          </div>
          <h3 className="font-display font-bold text-stone-700 text-lg mb-2">Aucun appareil connecté</h3>
          <p className="text-stone-400 text-sm font-body max-w-xs">
            Ajoutez votre premier appareil ESP32 depuis la section <strong>Appareils</strong> pour commencer.
          </p>
        </div>
      )}
    </div>
  );
}
