import { useState, useEffect } from 'react';
import { Plus, Cpu, Wifi, WifiOff, MapPin, Key, Trash2, X, RefreshCw, Activity } from 'lucide-react';
import { deviceService } from '../../services/api';
import toast from 'react-hot-toast';
import { format } from 'date-fns';
import { fr } from 'date-fns/locale';

function DeviceCard({ device, onDelete }) {
  const isOnline = device.status === 'online';
  return (
    <div className="card group hover:shadow-card-hover transition-all duration-300 animate-slide-up">
      <div className="flex items-start justify-between mb-4">
        <div className={`w-12 h-12 rounded-2xl flex items-center justify-center ${isOnline ? 'bg-primary-100' : 'bg-stone-100'}`}>
          <Cpu size={22} className={isOnline ? 'text-primary-600' : 'text-stone-400'} />
        </div>
        <div className="flex items-center gap-2">
          <span className={isOnline ? 'badge-online' : 'badge-offline'}>
            {isOnline ? <><Wifi size={10} /> En ligne</> : <><WifiOff size={10} /> Hors ligne</>}
          </span>
          {onDelete && (
            <button
              onClick={() => onDelete(device.id)}
              className="opacity-0 group-hover:opacity-100 text-stone-300 hover:text-red-400 transition-all"
            >
              <Trash2 size={16} />
            </button>
          )}
        </div>
      </div>

      <h3 className="font-display font-bold text-stone-900 text-lg mb-1">{device.device_name}</h3>

      <div className="space-y-2 mt-3">
        <div className="flex items-center gap-2 text-sm text-stone-500 font-body">
          <Key size={13} className="text-stone-400" />
          <span className="font-mono text-xs bg-stone-50 px-2 py-0.5 rounded-lg text-stone-600 truncate">
            {device.device_key}
          </span>
        </div>
        {device.location && (
          <div className="flex items-center gap-2 text-sm text-stone-500 font-body">
            <MapPin size={13} className="text-stone-400" />
            <span>{device.location}</span>
          </div>
        )}
        <div className="flex items-center gap-2 text-sm text-stone-500 font-body">
          <Activity size={13} className="text-stone-400" />
          <span>Ajouté le {format(new Date(device.created_at), 'd MMM yyyy', { locale: fr })}</span>
        </div>
      </div>

      {/* Live indicator */}
      {isOnline && (
        <div className="mt-4 pt-4 border-t border-stone-50 flex items-center gap-2">
          <div className="w-2 h-2 rounded-full bg-primary-500 animate-pulse" />
          <span className="text-xs text-primary-600 font-body font-medium">Données en temps réel actives</span>
        </div>
      )}
    </div>
  );
}

function AddDeviceModal({ onClose, onSave }) {
  const [form, setForm] = useState({ device_name: '', device_key: '', location: '' });
  const [saving, setSaving] = useState(false);

  const handleChange = (e) => setForm({ ...form, [e.target.name]: e.target.value });

  const generateKey = () => {
    const key = 'ESP32-' + Math.random().toString(36).substring(2, 10).toUpperCase();
    setForm({ ...form, device_key: key });
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);
    try {
      const { data } = await deviceService.create(form);
      if (data.success) {
        toast.success(`Appareil "${form.device_name}" ajouté !`);
        onSave(data.data);
        onClose();
      }
    } catch (err) {
      toast.error(err.response?.data?.message || 'Erreur lors de l\'ajout');
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/40 backdrop-blur-sm animate-fade-in">
      <div className="bg-white rounded-2xl shadow-2xl w-full max-w-md animate-slide-up">
        <div className="flex items-center justify-between p-6 border-b border-stone-100">
          <h2 className="font-display font-bold text-stone-900 text-xl">Ajouter un appareil</h2>
          <button onClick={onClose} className="text-stone-400 hover:text-stone-600"><X size={20} /></button>
        </div>
        <form onSubmit={handleSubmit} className="p-6 space-y-4">
          <div>
            <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Nom de l'appareil</label>
            <input type="text" name="device_name" value={form.device_name} onChange={handleChange}
              placeholder="Ex: ESP32 Serre principale" required className="input-field" />
          </div>
          <div>
            <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Clé de l'appareil</label>
            <div className="flex gap-2">
              <input type="text" name="device_key" value={form.device_key} onChange={handleChange}
                placeholder="Clé unique ESP32" required className="input-field font-mono" />
              <button type="button" onClick={generateKey}
                className="btn-secondary px-3 shrink-0" title="Générer une clé">
                <RefreshCw size={15} />
              </button>
            </div>
            <p className="text-xs text-stone-400 font-body mt-1">
              Cette clé doit correspondre à celle configurée sur l'ESP32.
            </p>
          </div>
          <div>
            <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Emplacement <span className="text-stone-400">(optionnel)</span></label>
            <input type="text" name="location" value={form.location} onChange={handleChange}
              placeholder="Ex: Champ Nord, Parcelle A…" className="input-field" />
          </div>

          {/* ESP32 info */}
          <div className="flex gap-3 p-3 bg-stone-50 rounded-xl">
            <Cpu size={15} className="text-stone-400 shrink-0 mt-0.5" />
            <p className="text-xs text-stone-500 font-body leading-relaxed">
              Assurez-vous que votre ESP32 est connecté au Wi-Fi et configuré avec la même clé que ci-dessus avant de valider.
            </p>
          </div>

          <div className="flex gap-3 pt-2">
            <button type="button" onClick={onClose} className="btn-secondary flex-1 justify-center">Annuler</button>
            <button type="submit" disabled={saving} className="btn-primary flex-1 justify-center">
              {saving ? 'Enregistrement…' : 'Ajouter l\'appareil'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}

export default function Devices() {
  const [devices, setDevices] = useState([]);
  const [loading, setLoading] = useState(true);
  const [showModal, setShowModal] = useState(false);

  useEffect(() => {
    deviceService.getAll()
      .then(({ data }) => { if (data.success) setDevices(data.data); })
      .catch(() => {})
      .finally(() => setLoading(false));
  }, []);

  const handleDelete = async (id) => {
    if (!confirm('Supprimer cet appareil ? Toutes les données associées seront perdues.')) return;
    try {
      await deviceService.update(id, { deleted: true });
      setDevices((prev) => prev.filter((d) => d.id !== id));
      toast.success('Appareil supprimé');
    } catch (_) {
      toast.error('Impossible de supprimer l\'appareil');
    }
  };

  const onlineCount = devices.filter((d) => d.status === 'online').length;

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <svg className="animate-spin h-7 w-7 text-primary-600" viewBox="0 0 24 24" fill="none">
          <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"/>
          <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8H4z"/>
        </svg>
      </div>
    );
  }

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <h1 className="font-display text-2xl font-bold text-stone-900">Appareils</h1>
          <p className="text-stone-500 text-sm font-body mt-0.5">
            {devices.length > 0
              ? `${devices.length} appareil${devices.length > 1 ? 's' : ''} — ${onlineCount} en ligne`
              : 'Aucun appareil configuré'}
          </p>
        </div>
        <button onClick={() => setShowModal(true)} className="btn-primary">
          <Plus size={18} /> Ajouter un appareil
        </button>
      </div>

      {/* Summary cards */}
      {devices.length > 0 && (
        <div className="grid grid-cols-3 gap-4">
          {[
            { label: 'Total', value: devices.length, color: 'bg-stone-100 text-stone-600' },
            { label: 'En ligne', value: onlineCount, color: 'bg-primary-100 text-primary-700' },
            { label: 'Hors ligne', value: devices.length - onlineCount, color: 'bg-stone-100 text-stone-500' },
          ].map(({ label, value, color }) => (
            <div key={label} className="card text-center py-4">
              <p className={`inline-block font-mono font-bold text-2xl px-3 py-1 rounded-xl ${color}`}>{value}</p>
              <p className="text-stone-500 text-sm font-body mt-2">{label}</p>
            </div>
          ))}
        </div>
      )}

      {/* Devices grid */}
      {devices.length === 0 ? (
        <div className="card flex flex-col items-center py-20 text-center animate-fade-in">
          <div className="w-20 h-20 rounded-2xl bg-stone-100 flex items-center justify-center mb-5">
            <Cpu size={36} className="text-stone-400" />
          </div>
          <h3 className="font-display font-bold text-stone-700 text-xl mb-2">Aucun appareil configuré</h3>
          <p className="text-stone-400 text-sm font-body max-w-sm mb-8 leading-relaxed">
            Connectez votre premier microcontrôleur ESP32 pour commencer à collecter des données 
            et contrôler votre irrigation.
          </p>
          <button onClick={() => setShowModal(true)} className="btn-primary">
            <Plus size={18} /> Ajouter mon premier ESP32
          </button>
        </div>
      ) : (
        <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-4">
          {devices.map((device) => (
            <DeviceCard key={device.id} device={device} onDelete={handleDelete} />
          ))}
        </div>
      )}

      {showModal && (
        <AddDeviceModal
          onClose={() => setShowModal(false)}
          onSave={(d) => setDevices((prev) => [d, ...prev])}
        />
      )}
    </div>
  );
}
