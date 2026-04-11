import { useState, useEffect } from 'react';
import { Cpu, Sparkles, PlusCircle, RefreshCw } from 'lucide-react';
import { deviceService, sensorService } from '../../services/api';
import toast from 'react-hot-toast';

export default function Simulator() {
  const [devices, setDevices] = useState([]);
  const [selectedDevice, setSelectedDevice] = useState(null);
  const [rows, setRows] = useState(5);
  const [readings, setReadings] = useState([]);
  const [loading, setLoading] = useState(true);
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    deviceService.getAll()
      .then(({ data }) => {
        if (data.success) {
          setDevices(data.data);
          if (data.data.length > 0) {
            setSelectedDevice(data.data[0]);
          }
        }
      })
      .catch(() => {})
      .finally(() => setLoading(false));
  }, []);

  useEffect(() => {
    if (!selectedDevice) {
      setReadings([]);
      return;
    }

    sensorService.getAll(selectedDevice.id)
      .then(({ data }) => {
        if (data.success) {
          setReadings(data.data);
        }
      })
      .catch(() => {
        setReadings([]);
      });
  }, [selectedDevice]);

  const createDemoDevice = async () => {
    setSubmitting(true);
    try {
      const key = `SIM-${Math.random().toString(36).substring(2, 10).toUpperCase()}`;
      const { data } = await deviceService.create({
        device_name: 'Appareil de simulation',
        device_key: key,
        location: 'Simulation',
      });
      if (data.success) {
        setDevices((prev) => [data.data, ...prev]);
        setSelectedDevice(data.data);
        toast.success('Appareil de simulation créé.');
      }
    } catch (err) {
      toast.error(err.response?.data?.message || 'Impossible de créer l’appareil.');
    } finally {
      setSubmitting(false);
    }
  };

  const generateData = async () => {
    if (!selectedDevice) {
      toast.error('Sélectionnez d’abord un appareil.');
      return;
    }

    setSubmitting(true);
    try {
      const { data } = await sensorService.generate(selectedDevice.id, { rows });
      if (data.success) {
        toast.success(`Génération terminée : ${data.generated} lectures ajoutées.`);
        const refresh = await sensorService.getAll(selectedDevice.id);
        if (refresh.data.success) {
          setReadings(refresh.data.data);
        }
      }
    } catch (err) {
      toast.error(err.response?.data?.message || 'Impossible de générer les données.');
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div className="space-y-6">
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <h1 className="font-display text-2xl font-bold text-stone-900">Simulateur de capteurs</h1>
          <p className="text-stone-500 text-sm font-body mt-0.5">
            Créez un appareil de test et générez des lectures aléatoires pour afficher des données dans le tableau de bord.
          </p>
        </div>
        <div className="flex items-center gap-2">
          <span className="badge-online">
            <Sparkles size={12} /> Simulation
          </span>
        </div>
      </div>

      <div className="grid lg:grid-cols-[1.2fr_0.8fr] gap-4">
        <div className="card space-y-4">
          <div className="flex items-center gap-3">
            <div className="w-11 h-11 rounded-2xl bg-primary-100 text-primary-700 flex items-center justify-center">
              <Cpu size={20} />
            </div>
            <div>
              <p className="text-sm font-medium text-stone-900">Appareil de simulation</p>
              <p className="text-xs text-stone-400">Aucun appareil réel nécessaire pour la démo.</p>
            </div>
          </div>

          {loading ? (
            <div className="text-stone-500">Chargement des appareils…</div>
          ) : devices.length === 0 ? (
            <div className="space-y-3">
              <p className="text-sm text-stone-600">Aucun appareil trouvé.</p>
              <button
                onClick={createDemoDevice}
                disabled={submitting}
                className="btn-primary w-full"
              >
                <PlusCircle size={16} /> Créer un appareil de simulation
              </button>
            </div>
          ) : (
            <div className="space-y-4">
              <div>
                <label className="block text-sm text-stone-500 mb-2">Sélectionner un appareil</label>
                <select
                  value={selectedDevice?.id || ''}
                  onChange={(e) => setSelectedDevice(devices.find((d) => d.id === +e.target.value))}
                  className="input-field w-full"
                >
                  {devices.map((device) => (
                    <option key={device.id} value={device.id}>
                      {device.device_name} — {device.device_key}
                    </option>
                  ))}
                </select>
              </div>

              <div>
                <label className="block text-sm text-stone-500 mb-2">Lignes de données à générer</label>
                <input
                  type="number"
                  min={1}
                  max={50}
                  value={rows}
                  onChange={(e) => setRows(Number(e.target.value))}
                  className="input-field w-24"
                />
              </div>

              <button
                onClick={generateData}
                disabled={submitting}
                className="btn-primary w-full"
              >
                {submitting ? 'Génération en cours…' : 'Générer des données aléatoires'}
              </button>
            </div>
          )}
        </div>

        <div className="card space-y-4 bg-stone-50">
          <div>
            <p className="text-xs uppercase tracking-[0.16em] text-stone-400">Mode de simulation</p>
            <h2 className="font-display text-lg font-bold text-stone-900 mt-2">Données aléatoires</h2>
          </div>
          <ul className="space-y-3 text-sm text-stone-600">
            <li className="flex items-start gap-2">
              <span className="mt-1 text-primary-600">•</span>
              Génère des lectures de capteurs réalistes pour un appareil.
            </li>
            <li className="flex items-start gap-2">
              <span className="mt-1 text-primary-600">•</span>
              Utilise les capteurs : humidité, température, eau et litres.
            </li>
            <li className="flex items-start gap-2">
              <span className="mt-1 text-primary-600">•</span>
              Les données apparaîtront ensuite dans le tableau de bord.
            </li>
          </ul>
          <div className="rounded-2xl bg-white p-4 border border-stone-100">
            <p className="text-xs uppercase tracking-[0.18em] text-stone-400">Conseil</p>
            <p className="text-sm text-stone-600 mt-2">
              Créez d’abord un appareil de test puis générez des lectures. Vous pouvez refaire la génération autant de fois que nécessaire.
            </p>
          </div>
        </div>
      </div>

      {readings.length > 0 && (
        <div className="card animate-slide-up">
          <div className="flex items-center justify-between mb-4">
            <div>
              <h2 className="font-display text-lg font-bold text-stone-900">Lectures récentes</h2>
              <p className="text-sm text-stone-500 font-body mt-0.5">
                Les dernières données créées dans la base pour l’appareil sélectionné.
              </p>
            </div>
            <button
              onClick={() => selectedDevice && sensorService.getAll(selectedDevice.id).then(({ data }) => data.success && setReadings(data.data))}
              className="btn-secondary px-3 py-2"
            >
              <RefreshCw size={16} /> Actualiser
            </button>
          </div>
          <div className="overflow-x-auto">
            <table className="min-w-full text-left text-sm font-body">
              <thead>
                <tr className="text-stone-500 border-b border-stone-200">
                  <th className="px-3 py-3">Heure</th>
                  <th className="px-3 py-3">Capteur</th>
                  <th className="px-3 py-3">Valeur</th>
                  <th className="px-3 py-3">Unité</th>
                </tr>
              </thead>
              <tbody>
                {readings.slice(0, 20).map((row) => (
                  <tr key={row.id} className="border-b border-stone-100 hover:bg-stone-50">
                    <td className="px-3 py-3 text-stone-600">{new Date(row.created_at).toLocaleString('fr-FR', { day: '2-digit', month: '2-digit', hour: '2-digit', minute: '2-digit' })}</td>
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
    </div>
  );
}
