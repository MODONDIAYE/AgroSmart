import { useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { Eye, EyeOff, Droplets, UserPlus } from 'lucide-react';
import { useAuth } from '../../context/AuthContext';

export default function Register() {
  const { register, loading } = useAuth();
  const navigate = useNavigate();
  const [form, setForm] = useState({ username: '', email: '', phone: '', password: '' });
  const [showPwd, setShowPwd] = useState(false);

  const handleChange = (e) => setForm({ ...form, [e.target.name]: e.target.value });

  const handleSubmit = async (e) => {
    e.preventDefault();
    const ok = await register(form);
    if (ok) navigate('/dashboard');
  };

  return (
    <div className="min-h-screen flex bg-stone-50">
      {/* Left decorative panel */}
      <div className="hidden lg:flex flex-col justify-between w-1/2 bg-gradient-to-br from-earth-600 via-primary-700 to-primary-600 p-12 relative overflow-hidden">
        <div className="absolute top-0 left-0 w-full h-full opacity-10">
          <div className="absolute -top-24 -right-24 w-96 h-96 rounded-full bg-white" />
          <div className="absolute bottom-0 left-0 w-72 h-72 rounded-full bg-white" />
        </div>
        <div className="relative flex items-center gap-3">
          <div className="w-10 h-10 rounded-xl bg-white/20 backdrop-blur flex items-center justify-center">
            <Droplets className="text-white" size={22} />
          </div>
          <span className="font-display font-bold text-white text-xl">AquaSmart</span>
        </div>
        <div className="relative space-y-4">
          <h1 className="font-display text-4xl font-bold text-white leading-tight">
            Rejoignez la<br />révolution agricole.
          </h1>
          <p className="text-primary-100 text-base font-body leading-relaxed max-w-sm">
            Créez votre compte gratuitement et commencez à optimiser votre irrigation dès aujourd'hui.
          </p>
          <ul className="space-y-2 pt-4">
            {['Capteurs ESP32 connectés', 'Alertes en temps réel', 'Cultures prédéfinies ou personnalisées'].map((item) => (
              <li key={item} className="flex items-center gap-2 text-primary-100 text-sm font-body">
                <span className="w-5 h-5 rounded-full bg-white/20 flex items-center justify-center text-white text-xs">✓</span>
                {item}
              </li>
            ))}
          </ul>
        </div>
        <p className="relative text-primary-200 text-xs font-body">© 2025 ISM – ETSE Licence 3</p>
      </div>

      {/* Right form panel */}
      <div className="flex-1 flex items-center justify-center p-6">
        <div className="w-full max-w-md animate-slide-up">
          <div className="lg:hidden flex items-center gap-2 mb-8">
            <div className="w-8 h-8 rounded-xl bg-primary-600 flex items-center justify-center">
              <Droplets className="text-white" size={16} />
            </div>
            <span className="font-display font-bold text-stone-900 text-lg">AquaSmart</span>
          </div>

          <h2 className="font-display text-3xl font-bold text-stone-900 mb-1">Créer un compte</h2>
          <p className="text-stone-500 text-sm font-body mb-8">Remplissez le formulaire pour commencer.</p>

          <form onSubmit={handleSubmit} className="space-y-4">
            <div className="grid grid-cols-2 gap-4">
              <div className="col-span-2 sm:col-span-1">
                <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Nom d'utilisateur</label>
                <input
                  type="text" name="username" value={form.username} onChange={handleChange}
                  placeholder="jean_dupont" required minLength={3}
                  className="input-field"
                />
              </div>
              <div className="col-span-2 sm:col-span-1">
                <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Téléphone <span className="text-stone-400">(optionnel)</span></label>
                <input
                  type="tel" name="phone" value={form.phone} onChange={handleChange}
                  placeholder="+221 77 000 00 00"
                  className="input-field"
                />
              </div>
            </div>

            <div>
              <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Adresse e-mail</label>
              <input
                type="email" name="email" value={form.email} onChange={handleChange}
                placeholder="vous@exemple.com" required
                className="input-field"
              />
            </div>

            <div>
              <label className="block text-sm font-medium text-stone-700 font-body mb-1.5">Mot de passe</label>
              <div className="relative">
                <input
                  type={showPwd ? 'text' : 'password'} name="password" value={form.password} onChange={handleChange}
                  placeholder="Minimum 6 caractères" required minLength={6}
                  className="input-field pr-11"
                />
                <button type="button" onClick={() => setShowPwd(!showPwd)}
                  className="absolute right-3 top-1/2 -translate-y-1/2 text-stone-400 hover:text-stone-600">
                  {showPwd ? <EyeOff size={18} /> : <Eye size={18} />}
                </button>
              </div>
            </div>

            <button
              type="submit" disabled={loading}
              className="btn-primary w-full justify-center py-3 text-base mt-2"
            >
              {loading ? (
                <span className="flex items-center gap-2">
                  <svg className="animate-spin h-4 w-4" viewBox="0 0 24 24" fill="none">
                    <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"/>
                    <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8H4z"/>
                  </svg>
                  Création…
                </span>
              ) : (
                <><UserPlus size={18} /> Créer mon compte</>
              )}
            </button>
          </form>

          <p className="mt-6 text-center text-sm text-stone-500 font-body">
            Déjà inscrit ?{' '}
            <Link to="/login" className="text-primary-600 font-medium hover:text-primary-700 transition-colors">
              Se connecter
            </Link>
          </p>
        </div>
      </div>
    </div>
  );
}
