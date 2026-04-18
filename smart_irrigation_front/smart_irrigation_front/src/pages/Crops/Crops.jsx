import { useState, useEffect } from 'react';
import { Plus, Sprout, Trash2, Droplets, Clock, Leaf, X, Info } from 'lucide-react';
import { cropsService } from '../../services/api';
import toast from 'react-hot-toast';

function CropCard({ crop, onDelete, isPredefined }) {
  return (
    <div className="card group hover:shadow-card-hover transition-all duration-300 animate-slide-up">
      <div className="flex items-start justify-between mb-4">
        <div className="flex items-center gap-3">
          <span className="text-3xl">{crop.icon || '🌱'}</span>
          <div>
            <h3 className="font-display font-bold text-stone-900">{crop.name}</h3>
            <span className={`text-xs font-body px-2 py-0.5 rounded-full ${isPredefined || crop.is_predefined ? 'bg-water-100 text-water-700' : 'bg-earth-100 text-earth-700'}`}>
              {isPredefined || crop.is_predefined ? 'Prédéfinie' : 'Personnalisée'}
            </span>
          </div>
        </div>
        {!isPredefined && onDelete && (
          <button
            onClick={() => onDelete(crop.id)}
            className="opacity-0 group-hover:opacity-100 text-stone-300 hover:text-red-400 transition-all"
          >
            <Trash2 size={16} />
          </button>
        )}
      </div>
      <div className="grid grid-cols-3 gap-3">
        <div className="bg-stone-50 rounded-xl p-3 text-center">
          <Droplets size={16} className="text-water-500 mx-auto mb-1" />
          <p className="font-mono font-bold text-stone-900 text-sm">{crop.water_need}<span className="text-xs font-body text-stone-400">ml</span></p>
          <p className="text-xs text-stone-400 font-body">Besoin eau</p>
        </div>
        <div className="bg-stone-50 rounded-xl p-3 text-center">
          <Leaf size={16} className="text-primary-500 mx-auto mb-1" />
          <p className="font-mono font-bold text-stone-900 text-sm">{crop.humidity_threshold}<span className="text-xs font-body text-stone-400">%</span></p>
          <p className="text-xs text-stone-400 font-body">Seuil humidité</p>
        </div>
        <div className="bg-stone-50 rounded-xl p-3 text-center">
          <Clock size={16} className="text-earth-500 mx-auto mb-1" />
          <p className="font-mono font-bold text-stone-900 text-sm">{crop.irrigations_per_day}</p>
          <p className="text-xs text-stone-400 font-body">Arrosages/j</p>
        </div>
      </div>
    </div>
  );
}

function AddCropModal({ onClose, onSave }) {
  const [form, setForm] = useState({
    name: '', water_need: 500, humidity_threshold: 40,
    min_water_level: 20, irrigations_per_day: 2, description: ''
  });
  const [saving, setSaving] = useState(false);

  const handleChange = (e) => {
    const { name, value } = e.target;
    setForm({ ...form, [name]: name === 'name' || name === 'description' ? value : Number(value) });
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);
    try {
      const { data } = await cropsService.create(form);
      if (data.success) {
        toast.success(`Culture "${form.name}" ajoutée !`);
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
          <h2 className="font-display font-bold text-stone-900 text-xl">Nouvelle culture</h2>
          <button onClick={onClose} className="text-stone-400 hover:text-stone-600"><X size={20} /></button>
        </div>
        <form onSubmit={handleSubmit} className="p-6 space-y-4">
          <div>
            <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Nom de la culture</label>
            <input type="text" name="name" value={form.name} onChange={handleChange}
              placeholder="Ex: Manioc, Arachide…" required className="input-field" />
          </div>
          <div>
            <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Description <span className="text-stone-400">(optionnel)</span></label>
            <input type="text" name="description" value={form.description} onChange={handleChange}
              placeholder="Notes sur la culture…" className="input-field" />
          </div>
          <div className="grid grid-cols-2 gap-4">
            <div>
              <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Besoin en eau (ml/j)</label>
              <input type="number" name="water_need" value={form.water_need} onChange={handleChange}
                min={50} max={5000} required className="input-field" />
            </div>
            <div>
              <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Seuil humidité (%)</label>
              <input type="number" name="humidity_threshold" value={form.humidity_threshold} onChange={handleChange}
                min={10} max={90} required className="input-field" />
            </div>
          </div>
          <div className="grid grid-cols-2 gap-4">
            <div>
              <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Niveau eau min. (%)</label>
              <input type="number" name="min_water_level" value={form.min_water_level} onChange={handleChange}
                min={5} max={50} required className="input-field" />
            </div>
            <div>
              <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Arrosages/jour</label>
              <input type="number" name="irrigations_per_day" value={form.irrigations_per_day} onChange={handleChange}
                min={1} max={10} required className="input-field" />
            </div>
          </div>
          <div className="flex gap-3 pt-2">
            <button type="button" onClick={onClose} className="btn-secondary flex-1 justify-center">Annuler</button>
            <button type="submit" disabled={saving} className="btn-primary flex-1 justify-center">
              {saving ? 'Enregistrement…' : 'Ajouter la culture'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}

export default function Crops() {
  const [predefinedCrops, setPredefinedCrops] = useState([]);
  const [customCrops, setCustomCrops] = useState([]);
  const [loading, setLoading] = useState(true);
  const [showModal, setShowModal] = useState(false);
  const [tab, setTab] = useState('predefined'); // 'predefined' | 'custom'

  useEffect(() => {
    cropsService.getAll()
      .then(({ data }) => {
        if (data.success) {
          setPredefinedCrops(data.data.filter((c) => c.is_predefined));
          setCustomCrops(data.data.filter((c) => !c.is_predefined));
        }
      })
      .catch(() => {})
      .finally(() => setLoading(false));
  }, []);

  const handleDelete = async (id) => {
    if (!confirm('Supprimer cette culture ?')) return;
    try {
      await cropsService.delete(id);
      setCustomCrops((prev) => prev.filter((c) => c.id !== id));
      toast.success('Culture supprimée');
    } catch (_) {
      toast.error('Impossible de supprimer');
    }
  };

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <h1 className="font-display text-2xl font-bold text-stone-900">Mes Cultures</h1>
          <p className="text-stone-500 text-sm font-body mt-0.5">Gérez vos cultures et leurs besoins en eau</p>
        </div>
        <button onClick={() => setShowModal(true)} className="btn-primary">
          <Plus size={18} /> Ajouter une culture
        </button>
      </div>

      {/* Info banner */}
      <div className="flex gap-3 p-4 bg-water-50 border border-water-100 rounded-2xl animate-slide-up">
        <Info size={18} className="text-water-600 shrink-0 mt-0.5" />
        <p className="text-sm text-water-700 font-body">
          Les cultures <strong>prédéfinies</strong> ont des paramètres établis selon les besoins agronomiques standards. 
          Vous pouvez aussi créer vos propres cultures avec des paramètres personnalisés.
        </p>
      </div>

      {/* Tabs */}
      <div className="flex gap-1 bg-stone-100 p-1 rounded-xl w-fit">
        {[['predefined', 'Prédéfinies', predefinedCrops.length], ['custom', 'Mes cultures', customCrops.length]].map(([key, label, count]) => (
          <button
            key={key}
            onClick={() => setTab(key)}
            className={`px-4 py-2 rounded-lg text-sm font-medium font-body transition-all ${
              tab === key ? 'bg-white text-stone-900 shadow-sm' : 'text-stone-500 hover:text-stone-700'
            }`}
          >
            {label}
            <span className={`ml-2 text-xs px-1.5 py-0.5 rounded-full ${tab === key ? 'bg-primary-100 text-primary-700' : 'bg-stone-200 text-stone-500'}`}>
              {count}
            </span>
          </button>
        ))}
      </div>

      {/* Crops grid */}
      {tab === 'predefined' && (
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
          {predefinedCrops.length > 0 ? predefinedCrops.map((crop) => (
            <CropCard key={crop.id} crop={crop} isPredefined={true} />
          )) : (
            <div className="card col-span-full p-10 text-center text-stone-500">
              Aucune culture prédéfinie n’a été trouvée. Exécutez le seeder ou ajoutez-en une manuellement.
            </div>
          )}
        </div>
      )}

      {tab === 'custom' && (
        <>
          {loading ? (
            <div className="flex items-center justify-center h-40">
              <svg className="animate-spin h-6 w-6 text-primary-600" viewBox="0 0 24 24" fill="none">
                <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"/>
                <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8H4z"/>
              </svg>
            </div>
          ) : (
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
              {customCrops.map((crop) => (
                <CropCard key={crop.id} crop={crop} isPredefined={false} onDelete={handleDelete} />
              ))}
            </div>
          )}
        </>
      )}

      {showModal && (
        <AddCropModal
          onClose={() => setShowModal(false)}
          onSave={(c) => setCustomCrops((prev) => [c, ...prev])}
        />
      )}
    </div>
  );
}
