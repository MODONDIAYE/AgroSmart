import { useState } from 'react';
import { Outlet, NavLink, useNavigate } from 'react-router-dom';
import {
  LayoutDashboard, Sprout, Droplets, Bell, Cpu,
  Settings, MessageSquare, LogOut, Menu, X, ChevronRight, Wifi,
  Activity
} from 'lucide-react';
import { useAuth } from '../../context/AuthContext';

const navItems = [
  { to: '/dashboard',     icon: LayoutDashboard, label: 'Tableau de bord' },
  { to: '/cultures',      icon: Sprout,           label: 'Mes Cultures' },
  { to: '/irrigation',    icon: Droplets,          label: 'Irrigation' },
  { to: '/simulateur',    icon: Activity,          label: 'Simulateur' },
  { to: '/appareils',     icon: Cpu,               label: 'Appareils' },
  { to: '/notifications', icon: Bell,              label: 'Notifications' },
  { to: '/settings',      icon: Settings,          label: 'Paramètres' },
  { to: '/assistant',     icon: MessageSquare,     label: 'Assistant IA' },
];

export default function Layout() {
  const { user, logout } = useAuth();
  const [open, setOpen] = useState(false);
  const navigate = useNavigate();

  const handleLogout = async () => {
    await logout();
    navigate('/login');
  };

  return (
    <div className="flex h-screen bg-stone-50 overflow-hidden">
      {/* Mobile overlay */}
      {open && (
        <div
          className="fixed inset-0 z-20 bg-black/30 backdrop-blur-sm lg:hidden"
          onClick={() => setOpen(false)}
        />
      )}

      {/* Sidebar */}
      <aside className={`
        fixed lg:static inset-y-0 left-0 z-30 w-64 flex flex-col
        bg-white border-r border-stone-100 shadow-card
        transform transition-transform duration-300 ease-out
        ${open ? 'translate-x-0' : '-translate-x-full lg:translate-x-0'}
      `}>
        {/* Logo */}
        <div className="flex items-center gap-3 px-6 py-5 border-b border-stone-100 bg-mint-50">
          <div className="w-9 h-9 rounded-3xl bg-gradient-to-br from-mint-300 to-forest-700 flex items-center justify-center shadow-sm">
            <svg viewBox="0 0 24 24" className="w-5 h-5 fill-white">
              <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 14H9V8h2v8zm4 0h-2V8h2v8z"/>
              <path d="M12 2a10 10 0 0 0-7.07 17.07C6.4 17.4 9 15.5 9 12c0-1.5.5-3 2-4 .7-.5 1-.7 1-1s-.3-.5-1-1c-1.5-1-2-2.5-2-4 2 0 4 1 5 3 1-2 3-3 5-3 0 1.5-.5 3-2 4-.7.5-1 .7-1 1s.3.5 1 1c1.5 1 2 2.5 2 4 0 3.5 2.6 5.4 4.07 7.07A10 10 0 0 0 12 2z"/>
            </svg>
          </div>
          <div>
            <p className="font-display font-bold text-stone-900 text-base leading-none">AgroSmart</p>
            <p className="text-[10px] text-stone-400 font-body mt-0.5">Irrigation Intelligente</p>
          </div>
          <button className="ml-auto lg:hidden text-stone-400" onClick={() => setOpen(false)}>
            <X size={18} />
          </button>
        </div>

        {/* Nav */}
        <nav className="flex-1 px-3 py-4 space-y-1 overflow-y-auto">
          {navItems.map(({ to, icon: Icon, label }) => (
            <NavLink
              key={to}
              to={to}
              onClick={() => setOpen(false)}
              className={({ isActive }) => `
                flex items-center gap-3 px-3 py-2.5 rounded-xl text-sm font-body font-medium
                transition-all duration-200 group
                ${isActive
                  ? 'bg-primary-600 text-white shadow-sm'
                  : 'text-stone-600 hover:bg-stone-100 hover:text-stone-900'}
              `}
            >
              {({ isActive }) => (
                <>
                  <Icon size={18} className={isActive ? 'text-white' : 'text-stone-400 group-hover:text-stone-600'} />
                  <span className="flex-1">{label}</span>
                  {isActive && <ChevronRight size={14} className="text-white/60" />}
                </>
              )}
            </NavLink>
          ))}
        </nav>

        {/* User card */}
        <div className="px-3 py-4 border-t border-stone-100 bg-mint-50">
          <div className="flex items-center gap-3 px-3 py-2.5 rounded-3xl bg-white shadow-sm">
            <div className="w-8 h-8 rounded-full bg-gradient-to-br from-mint-400 to-forest-700 flex items-center justify-center text-white text-xs font-bold font-body shrink-0">
              {user?.username?.[0]?.toUpperCase() || 'U'}
            </div>
            <div className="flex-1 min-w-0">
              <p className="text-sm font-medium text-stone-800 font-body truncate">{user?.username}</p>
              <p className="text-xs text-stone-400 font-body truncate">{user?.email}</p>
            </div>
            <button
              onClick={handleLogout}
              title="Se déconnecter"
              className="text-stone-400 hover:text-red-500 transition-colors"
            >
              <LogOut size={16} />
            </button>
          </div>
        </div>
      </aside>

      {/* Main content */}
      <div className="flex-1 flex flex-col min-w-0 overflow-hidden">
        {/* Top bar (mobile) */}
        <header className="lg:hidden flex items-center gap-3 px-4 py-3 bg-white border-b border-stone-100 shadow-sm">
          <button onClick={() => setOpen(true)} className="text-forest-700">
            <Menu size={22} />
          </button>
          <span className="font-display font-bold text-forest-900">AgroSmart</span>
          <div className="ml-auto flex items-center gap-1.5 text-xs text-forest-600 font-body font-medium">
            <Wifi size={14} />
            En ligne
          </div>
        </header>

        {/* Page content */}
        <main className="flex-1 overflow-y-auto p-4 lg:p-8 animate-fade-in">
          <Outlet />
        </main>
      </div>
    </div>
  );
}
