import { useState, useEffect } from 'react';
import { Play, StopCircle, Clock, Plus, Trash2, Zap, Settings2, ToggleLeft, ToggleRight, Info, X } from 'lucide-react';
import { irrigationService, irrigationTimeService, cropsService, deviceService } from '../../services/api';
import toast from 'react-hot-toast';
import { format } from 'date-fns';
import { fr } from 'date-fns/locale';

function ModeToggle({ mode, onChange }) {
  return (
    <div className="flex gap-1 bg-stone-100 p-1 rounded-xl w-fit">
      {[['manual', '⚡ Manuel', ToggleLeft], ['auto', '🤖 Automatique', ToggleRight]].map(([key, label]) => (
        <button
          key={key}
          onClick={() => onChange(key)}
          className={`px-5 py-2 rounded-lg text-sm font-medium font-body transition-all ${
            mode === key ? 'bg-white text-stone-900 shadow-sm' : 'text-stone-500 hover:text-stone-700'
          }`}
        >
          {label}
        </button>
      ))}
    </div>
  );
}

function AddTimeModal({ cropId, onClose, onSave, existingCount }) {
  const [time, setTime] = useState('07:00');
  const [saving, setSaving] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);
    try {
      const { data } = await irrigationTimeService.create({
        crop_id: cropId, time, order_index: existingCount
      });
      if (data.success) {
        toast.success('Horaire ajouté !');
        onSave(data.data);
        onClose();
      }
    } catch (_) {
      toast.error('Erreur lors de l\'ajout de l\'horaire');
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/40 backdrop-blur-sm animate-fade-in">
      <div className="bg-white rounded-2xl shadow-2xl w-full max-w-sm animate-slide-up">
        <div className="flex items-center justify-between p-5 border-b border-stone-100">
          <h3 className="font-display font-bold text-stone-900">Ajouter un horaire</h3>
          <button onClick={onClose} className="text-stone-400 hover:text-stone-600"><X size={18} /></button>
        </div>
        <form onSubmit={handleSubmit} className="p-5 space-y-4">
          <div>
            <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Heure d'irrigation</label>
            <input
              type="time" value={time} onChange={(e) => setTime(e.target.value)}
              required className="input-field text-lg font-mono"
            />
          </div>
          <div className="flex gap-3 pt-1">
            <button type="button" onClick={onClose} className="btn-secondary flex-1 justify-center">Annuler</button>
            <button type="submit" disabled={saving} className="btn-primary flex-1 justify-center">
              {saving ? 'Ajout…' : 'Confirmer'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}

export default function Irrigation() {
  const [mode, setMode] = useState('manual');
  const [devices, setDevices] = useState([]);
  const [activeDevice, setActiveDevice] = useState(null);
  const [events, setEvents] = useState([]);
  const [crops, setCrops] = useState([]);
  const [selectedCrop, setSelectedCrop] = useState(null);
  const [duration, setDuration] = useState(300);
  const [pumping, setPumping] = useState(false);
  const [times, setTimes] = useState([]);
  const [showTimeModal, setShowTimeModal] = useState(false);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const init = async () => {
      try {
        const [devRes, cropRes] = await Promise.all([
          deviceService.getAll(),
          cropsService.getAll(),
        ]);
        if (devRes.data.success && devRes.data.data.length > 0) {
          setDevices(devRes.data.data);
          const dev = devRes.data.data[0];
          setActiveDevice(dev);
          const evRes = await irrigationService.getEvents(dev.id);
          if (evRes.data.success) setEvents(evRes.data.data.slice(0, 15));
        }
        if (cropRes.data.success) {
          setCrops(cropRes.data.data);
          if (cropRes.data.data.length > 0) setSelectedCrop(cropRes.data.data[0]);
        }
      } catch (_) {}
      setLoading(false);
    };
    init();
  }, []);

  useEffect(() => {
    if (selectedCrop?.irrigationTimes) setTimes(selectedCrop.irrigationTimes);
    else setTimes([]);
  }, [selectedCrop]);

  const handleManual = async () => {
    if (!activeDevice) return toast.error('Aucun appareil sélectionné');
    setPumping(true);
    try {
      const { data } = await irrigationService.triggerManual(activeDevice.id, {
        duration_seconds: duration, mode: 'Manual',
      });
      if (data.success) {
        toast.success(`Irrigation lancée pendant ${duration}s !`);
        const evRes = await irrigationService.getEvents(activeDevice.id);
        if (evRes.data.success) setEvents(evRes.data.data.slice(0, 15));
      }
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
      toast.error('Impossible d\'arrêter');
    }
  };

  const deleteTime = async (id) => {
    try {
      await irrigationTimeService.delete(id);
      setTimes((prev) => prev.filter((t) => t.id !== id));
      toast.success('Horaire supprimé');
    } catch (_) {
      toast.error('Erreur lors de la suppression');
    }
  };

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
          <h1 className="font-display text-2xl font-bold text-stone-900">Irrigation</h1>
          <p className="text-stone-500 text-sm font-body mt-0.5">Contrôlez et programmez l'arrosage de vos cultures</p>
        </div>
        {/* Device selector */}
        {devices.length > 0 && (
          <select
            value={activeDevice?.id || ''}
            onChange={(e) => setActiveDevice(devices.find((d) => d.id === +e.target.value))}
            className="input-field !w-auto py-2 text-sm"
          >
            {devices.map((d) => <option key={d.id} value={d.id}>{d.device_name}</option>)}
          </select>
        )}
      </div>

      {/* Mode toggle */}
      <ModeToggle mode={mode} onChange={setMode} />

      {/* MANUAL MODE */}
      {mode === 'manual' && (
        <div className="grid lg:grid-cols-2 gap-6 animate-slide-up">
          {/* Control panel */}
          <div className="card space-y-6">
            <div className="flex items-center gap-2">
              <Zap size={18} className="text-earth-500" />
              <h2 className="font-display font-bold text-stone-900 text-lg">Contrôle manuel</h2>
            </div>

            <div className="flex flex-col items-center gap-6 py-4">
              {/* Animated water drop */}
              <div className={`w-24 h-24 rounded-full flex items-center justify-center border-4 transition-all duration-500 ${
                pumping
                  ? 'border-primary-500 bg-primary-50 shadow-lg shadow-primary-100'
                  : 'border-stone-200 bg-stone-50'
              }`}>
                <svg viewBox="0 0 24 24" className={`w-10 h-10 transition-colors ${pumping ? 'fill-primary-500 drop-anim' : 'fill-stone-300'}`}>
                  <path d="M12 2C8.13 2 5 5.13 5 9c0 5.25 7 13 7 13s7-7.75 7-13c0-3.87-3.13-7-7-7z"/>
                </svg>
              </div>

              <div className="text-center">
                <p className={`font-display font-bold text-lg ${pumping ? 'text-primary-600' : 'text-stone-400'}`}>
                  {pumping ? 'Irrigation en cours…' : 'Pompe arrêtée'}
                </p>
              </div>

              {/* Duration */}
              <div className="w-full space-y-2">
                <div className="flex items-center justify-between">
                  <label className="text-sm font-medium text-stone-700 font-body">Durée d'irrigation</label>
                  <span className="font-mono font-bold text-primary-600 text-sm">{duration}s ({Math.floor(duration/60)}min {duration%60}s)</span>
                </div>
                <input
                  type="range" min={30} max={1800} step={30}
                  value={duration} onChange={(e) => setDuration(+e.target.value)}
                  className="w-full accent-primary-600"
                />
                <div className="flex justify-between text-xs text-stone-400 font-body">
                  <span>30s</span><span>15min</span><span>30min</span>
                </div>
              </div>

              {/* Quick durations */}
              <div className="flex gap-2 flex-wrap justify-center">
                {[60, 300, 600, 900].map((d) => (
                  <button key={d} onClick={() => setDuration(d)}
                    className={`px-3 py-1.5 rounded-lg text-xs font-medium font-body transition-all ${
                      duration === d ? 'bg-primary-600 text-white' : 'bg-stone-100 text-stone-600 hover:bg-stone-200'
                    }`}>
                    {d < 60 ? `${d}s` : `${d/60}min`}
                  </button>
                ))}
              </div>
            </div>

            {/* Buttons */}
            <div className="flex gap-3">
              <button onClick={handleManual} disabled={pumping || !activeDevice}
                className="btn-primary flex-1 justify-center py-3">
                <Play size={18} />
                {pumping ? 'En cours…' : 'Démarrer'}
              </button>
              <button onClick={handleStop} className="btn-danger py-3 px-5">
                <StopCircle size={18} />
              </button>
            </div>

            {!activeDevice && (
              <p className="text-xs text-amber-600 font-body text-center bg-amber-50 rounded-xl p-2">
                ⚠️ Aucun appareil connecté — ajoutez un ESP32 dans la section Appareils
              </p>
            )}
          </div>

          {/* Recent events */}
          <div className="card">
            <h2 className="font-display font-bold text-stone-900 text-lg mb-4">Historique récent</h2>
            {events.length === 0 ? (
              <div className="flex flex-col items-center justify-center h-48 text-stone-400">
                <Clock size={28} className="mb-2" />
                <p className="text-sm font-body">Aucun événement récent</p>
              </div>
            ) : (
              <div className="space-y-2 max-h-80 overflow-y-auto pr-1">
                {events.map((ev) => (
                  <div key={ev.id} className="flex items-center gap-3 py-2.5 px-3 rounded-xl hover:bg-stone-50 transition-colors">
                    <div className={`w-8 h-8 rounded-full flex items-center justify-center shrink-0 ${
                      ev.action === 'start' ? 'bg-primary-100 text-primary-600' : 'bg-stone-100 text-stone-500'
                    }`}>
                      {ev.action === 'start' ? <Play size={12} /> : <StopCircle size={12} />}
                    </div>
                    <div className="flex-1">
                      <p className="text-sm font-medium text-stone-800 font-body">
                        {ev.action === 'start' ? 'Démarrage' : 'Arrêt'} — <span className="text-stone-400 font-normal">{ev.mode}</span>
                      </p>
                      {ev.duration_seconds && <p className="text-xs text-stone-400 font-body">{ev.duration_seconds}s</p>}
                    </div>
                    <p className="text-xs text-stone-500 font-mono shrink-0">
                      {format(new Date(ev.timestamp), 'HH:mm', { locale: fr })}
                    </p>
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      )}

      {/* AUTO MODE */}
      {mode === 'auto' && (
        <div className="grid lg:grid-cols-2 gap-6 animate-slide-up">
          {/* Crop selector + schedule */}
          <div className="card space-y-5">
            <div className="flex items-center gap-2">
              <Settings2 size={18} className="text-primary-600" />
              <h2 className="font-display font-bold text-stone-900 text-lg">Programmation automatique</h2>
            </div>

            <div className="flex gap-3 p-4 bg-primary-50 border border-primary-100 rounded-2xl">
              <Info size={16} className="text-primary-600 shrink-0 mt-0.5" />
              <p className="text-xs text-primary-700 font-body leading-relaxed">
                En mode automatique, le système déclenche l'irrigation selon les horaires programmés 
                et le seuil d'humidité de la culture sélectionnée.
              </p>
            </div>

            {/* Culture selector */}
            <div>
              <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">
                Culture active
              </label>
              {crops.length === 0 ? (
                <p className="text-sm text-stone-400 font-body">Aucune culture disponible — ajoutez-en une dans "Mes Cultures"</p>
              ) : (
                <select
                  value={selectedCrop?.id || ''}
                  onChange={(e) => setSelectedCrop(crops.find((c) => c.id === +e.target.value))}
                  className="input-field"
                >
                  {crops.map((c) => <option key={c.id} value={c.id}>{c.name}</option>)}
                </select>
              )}
            </div>

            {/* Selected crop info */}
            {selectedCrop && (
              <div className="grid grid-cols-3 gap-3">
                {[
                  { label: 'Seuil humidité', value: `${selectedCrop.humidity_threshold}%` },
                  { label: 'Besoin eau',      value: `${selectedCrop.water_need}ml/j` },
                  { label: 'Arrosages/j',    value: selectedCrop.irrigations_per_day },
                ].map(({ label, value }) => (
                  <div key={label} className="bg-stone-50 rounded-xl p-3 text-center">
                    <p className="font-mono font-bold text-stone-900 text-sm">{value}</p>
                    <p className="text-xs text-stone-400 font-body mt-0.5">{label}</p>
                  </div>
                ))}
              </div>
            )}

            {/* Schedule times */}
            <div>
              <div className="flex items-center justify-between mb-3">
                <h3 className="text-sm font-medium text-stone-700 font-body">Horaires d'arrosage</h3>
                <button
                  onClick={() => setShowTimeModal(true)}
                  disabled={!selectedCrop}
                  className="text-xs flex items-center gap-1 text-primary-600 hover:text-primary-700 font-body font-medium disabled:opacity-50"
                >
                  <Plus size={14} /> Ajouter
                </button>
              </div>

              {times.length === 0 ? (
                <div className="flex flex-col items-center justify-center h-24 bg-stone-50 rounded-xl text-stone-400">
                  <Clock size={20} className="mb-1" />
                  <p className="text-xs font-body">Aucun horaire programmé</p>
                </div>
              ) : (
                <div className="space-y-2">
                  {times.map((t) => (
                    <div key={t.id} className="flex items-center gap-3 bg-stone-50 rounded-xl px-4 py-3">
                      <Clock size={15} className="text-primary-500" />
                      <span className="font-mono font-bold text-stone-900">{t.time}</span>
                      <span className="text-xs text-stone-400 font-body">Séquence #{t.order_index + 1}</span>
                      <button
                        onClick={() => deleteTime(t.id)}
                        className="ml-auto text-stone-300 hover:text-red-400 transition-colors"
                      >
                        <Trash2 size={14} />
                      </button>
                    </div>
                  ))}
                </div>
              )}
            </div>
          </div>

          {/* Right column: status */}
          <div className="space-y-4">
            <div className="card">
              <h3 className="font-display font-bold text-stone-900 mb-4">État du système automatique</h3>
              <div className="space-y-3">
                <div className="flex items-center justify-between py-2 border-b border-stone-50">
                  <span className="text-sm text-stone-600 font-body">Appareil actif</span>
                  <span className="text-sm font-medium text-stone-900 font-body">{activeDevice?.device_name || '—'}</span>
                </div>
                <div className="flex items-center justify-between py-2 border-b border-stone-50">
                  <span className="text-sm text-stone-600 font-body">Culture sélectionnée</span>
                  <span className="text-sm font-medium text-stone-900 font-body">{selectedCrop?.name || '—'}</span>
                </div>
                <div className="flex items-center justify-between py-2 border-b border-stone-50">
                  <span className="text-sm text-stone-600 font-body">Horaires configurés</span>
                  <span className="text-sm font-medium text-stone-900 font-body">{times.length}</span>
                </div>
                <div className="flex items-center justify-between py-2">
                  <span className="text-sm text-stone-600 font-body">Prochain arrosage</span>
                  <span className="text-sm font-medium text-primary-600 font-mono">
                    {times.length > 0 ? times[0].time : '—'}
                  </span>
                </div>
              </div>
            </div>

            <div className="card bg-gradient-to-br from-primary-600 to-primary-800 text-white">
              <div className="flex items-center gap-2 mb-3">
                <Zap size={16} className="text-primary-200" />
                <h3 className="font-display font-bold text-white">Mode automatique actif</h3>
              </div>
              <p className="text-primary-100 text-sm font-body leading-relaxed">
                Le système surveille en temps réel l'humidité du sol et déclenche l'irrigation 
                automatiquement quand le seuil de <strong className="text-white">{selectedCrop?.humidity_threshold || '—'}%</strong> est atteint.
              </p>
            </div>
          </div>
        </div>
      )}

      {showTimeModal && selectedCrop && (
        <AddTimeModal
          cropId={selectedCrop.id}
          existingCount={times.length}
          onClose={() => setShowTimeModal(false)}
          onSave={(t) => setTimes((prev) => [...prev, t])}
        />
      )}
    </div>
  );
}
