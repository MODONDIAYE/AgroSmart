import { useState, useEffect } from 'react';
import { ShieldCheck, Bell, Zap, CheckCircle, User, Phone, Mail, Wifi, Cpu } from 'lucide-react';
import { useAuth } from '../../context/AuthContext';
import { deviceService } from '../../services/api';

export default function Settings() {
  const { user } = useAuth();
  const [devices, setDevices] = useState([]);
  const [settings, setSettings] = useState({
    autoIrrigation: true,
    notifications: true,
    dailyReport: false,
  });
  const [saved, setSaved] = useState(false);

  useEffect(() => {
    deviceService.getAll()
      .then(({ data }) => { if (data.success) setDevices(data.data); })
      .catch(() => {});
  }, []);

  const toggle = (key) => {
    setSettings((prev) => ({ ...prev, [key]: !prev[key] }));
    setSaved(false);
  };

  const handleSave = (e) => {
    e.preventDefault();
    setSaved(true);
  };

  return (
    <div className="space-y-6">
      <div>
        <h1 className="font-display text-2xl font-bold text-stone-900">Paramètres</h1>
        <p className="text-stone-500 text-sm font-body mt-1">Affinez le comportement de votre système, consultez votre profil et contactez-nous si besoin.</p>
      </div>

      <div className="grid gap-6 xl:grid-cols-3">
        <div className="card p-6 space-y-4">
          <div className="flex items-center gap-3">
            <div className="w-12 h-12 rounded-2xl bg-primary-100 text-primary-700 flex items-center justify-center">
              <User size={20} />
            </div>
            <div>
              <h2 className="font-semibold text-stone-900">Profil utilisateur</h2>
              <p className="text-sm text-stone-500">Détails de connexion et informations personnelles.</p>
            </div>
          </div>
          <div className="space-y-2 text-sm text-stone-600">
            <div className="flex items-center gap-2">
              <strong>Nom :</strong>
              <span>{user?.username || 'Utilisateur'}</span>
            </div>
            <div className="flex items-center gap-2">
              <strong>Email :</strong>
              <span>{user?.email || 'Non renseigné'}</span>
            </div>
            {user?.phone && (
              <div className="flex items-center gap-2">
                <strong>Téléphone :</strong>
                <span>{user.phone}</span>
              </div>
            )}
          </div>
        </div>

        <div className="card p-6 space-y-4">
          <div className="flex items-center gap-3">
            <div className="w-12 h-12 rounded-2xl bg-slate-100 text-slate-700 flex items-center justify-center">
              <Cpu size={20} />
            </div>
            <div>
              <h2 className="font-semibold text-stone-900">État du dispositif</h2>
              <p className="text-sm text-stone-500">Résumé de vos appareils connectés.</p>
            </div>
          </div>
          <div className="grid gap-3 text-sm text-stone-600">
            <div className="flex items-center justify-between">
              <span>Total d’appareils</span>
              <span className="font-semibold text-stone-900">{devices.length}</span>
            </div>
            <div className="flex items-center justify-between">
              <span>En ligne</span>
              <span className="font-semibold text-primary-700">{devices.filter((d) => d.status === 'online').length}</span>
            </div>
            <div className="flex items-center justify-between">
              <span>Hors ligne</span>
              <span className="font-semibold text-stone-600">{devices.filter((d) => d.status !== 'online').length}</span>
            </div>
          </div>
        </div>

        <div className="card p-6 space-y-4">
          <div className="flex items-center gap-3">
            <div className="w-12 h-12 rounded-2xl bg-emerald-100 text-emerald-700 flex items-center justify-center">
              <Phone size={20} />
            </div>
            <div>
              <h2 className="font-semibold text-stone-900">Nous contacter</h2>
              <p className="text-sm text-stone-500">Assistance disponible par téléphone ou par e-mail.</p>
            </div>
          </div>
          <div className="space-y-3 text-sm text-stone-600">
            <div className="flex items-center gap-2">
              <Phone size={16} className="text-primary-600" />
              <a href="tel:+33123456789" className="text-primary-700">+33 1 23 45 67 89</a>
            </div>
            <div className="flex items-center gap-2">
              <Mail size={16} className="text-primary-600" />
              <a href="mailto:support@agrosmart.fr" className="text-primary-700">support@agrosmart.fr</a>
            </div>
            <div className="flex items-center gap-2">
              <Wifi size={16} className="text-primary-600" />
              <span>Support ouvert du lundi au vendredi</span>
            </div>
          </div>
        </div>
      </div>

      <form onSubmit={handleSave} className="grid gap-6 lg:grid-cols-2">
        {[
          {
            key: 'autoIrrigation',
            icon: Zap,
            title: 'Irrigation automatique',
            description: 'Active l’arrosage automatique selon les seuils et les horaires configurés.',
          },
          {
            key: 'notifications',
            icon: Bell,
            title: 'Notifications',
            description: 'Recevez des alertes par e-mail ou dans l’application lorsque des seuils sont dépassés.',
          },
          {
            key: 'dailyReport',
            icon: ShieldCheck,
            title: 'Rapport quotidien',
            description: 'Recevez un résumé journalier des actions d’irrigation et de l’état des capteurs.',
          },
        ].map(({ key, icon: Icon, title, description }) => (
          <div key={key} className="card flex flex-col justify-between p-6">
            <div>
              <div className="flex items-center gap-3 mb-4">
                <div className="w-11 h-11 rounded-2xl bg-primary-100 text-primary-700 flex items-center justify-center">
                  <Icon size={18} />
                </div>
                <div>
                  <h2 className="text-sm font-semibold text-stone-900">{title}</h2>
                  <p className="text-xs text-stone-400 font-body mt-1">{description}</p>
                </div>
              </div>
              <button
                type="button"
                onClick={() => toggle(key)}
                className={`inline-flex items-center gap-2 px-4 py-2 text-sm font-medium rounded-full transition ${settings[key] ? 'bg-primary-600 text-white' : 'bg-stone-100 text-stone-600 hover:bg-stone-200'}`}
              >
                {settings[key] ? 'Activé' : 'Désactivé'}
              </button>
            </div>
          </div>
        ))}

        <div className="card p-6 col-span-full">
          <h2 className="font-display text-lg font-bold text-stone-900 mb-3">Sauvegarde</h2>
          <p className="text-sm text-stone-500 font-body mb-5">Enregistrez vos préférences pour le système.</p>
          <button type="submit" className="btn-primary">
            Enregistrer les paramètres
          </button>
          {saved && (
            <p className="mt-4 inline-flex items-center gap-2 text-sm text-emerald-600">
              <CheckCircle size={16} /> Paramètres mis à jour.
            </p>
          )}
        </div>
      </form>
    </div>
  );
}
