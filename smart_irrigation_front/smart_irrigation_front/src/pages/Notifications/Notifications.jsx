import { useState, useEffect } from 'react';
import { Bell, BellOff, Check, CheckCheck, Droplets, AlertTriangle, Info, Zap } from 'lucide-react';
import { notificationService } from '../../services/api';
import toast from 'react-hot-toast';
import { format, formatDistanceToNow } from 'date-fns';
import { fr } from 'date-fns/locale';

const TYPE_CONFIG = {
  low_water:   { icon: Droplets,      color: 'text-water-600',   bg: 'bg-water-50',   border: 'border-water-100',   label: 'Eau faible' },
  warning:     { icon: AlertTriangle, color: 'text-amber-600',   bg: 'bg-amber-50',   border: 'border-amber-100',   label: 'Avertissement' },
  info:        { icon: Info,          color: 'text-stone-500',   bg: 'bg-stone-50',   border: 'border-stone-100',   label: 'Information' },
  irrigation:  { icon: Zap,           color: 'text-primary-600', bg: 'bg-primary-50', border: 'border-primary-100', label: 'Irrigation' },
};

function getConfig(type) {
  return TYPE_CONFIG[type] || TYPE_CONFIG['info'];
}

export default function Notifications() {
  const [notifications, setNotifications] = useState([]);
  const [loading, setLoading] = useState(true);
  const [filter, setFilter] = useState('all'); // 'all' | 'unread'

  useEffect(() => {
    notificationService.getAll()
      .then(({ data }) => { if (data.success) setNotifications(data.data); })
      .catch(() => {})
      .finally(() => setLoading(false));
  }, []);

  const markRead = async (id) => {
    try {
      await notificationService.markAsRead(id);
      setNotifications((prev) => prev.map((n) => n.id === id ? { ...n, is_read: true } : n));
    } catch (_) { toast.error('Erreur'); }
  };

  const markAllRead = async () => {
    try {
      await notificationService.markAllRead();
      setNotifications((prev) => prev.map((n) => ({ ...n, is_read: true })));
      toast.success('Toutes les notifications marquées comme lues');
    } catch (_) { toast.error('Erreur'); }
  };

  const displayed = filter === 'unread'
    ? notifications.filter((n) => !n.is_read)
    : notifications;

  const unreadCount = notifications.filter((n) => !n.is_read).length;

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
    <div className="space-y-6 max-w-2xl">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <h1 className="font-display text-2xl font-bold text-stone-900">Notifications</h1>
          <p className="text-stone-500 text-sm font-body mt-0.5">
            {unreadCount > 0 ? `${unreadCount} non lue${unreadCount > 1 ? 's' : ''}` : 'Tout est à jour'}
          </p>
        </div>
        {unreadCount > 0 && (
          <button onClick={markAllRead} className="btn-secondary py-2">
            <CheckCheck size={16} /> Tout marquer comme lu
          </button>
        )}
      </div>

      {/* Filter tabs */}
      <div className="flex gap-1 bg-stone-100 p-1 rounded-xl w-fit">
        {[['all', 'Toutes', notifications.length], ['unread', 'Non lues', unreadCount]].map(([key, label, count]) => (
          <button
            key={key}
            onClick={() => setFilter(key)}
            className={`px-4 py-2 rounded-lg text-sm font-medium font-body transition-all ${
              filter === key ? 'bg-white text-stone-900 shadow-sm' : 'text-stone-500 hover:text-stone-700'
            }`}
          >
            {label}
            <span className={`ml-1.5 text-xs px-1.5 py-0.5 rounded-full ${filter === key ? 'bg-primary-100 text-primary-700' : 'bg-stone-200 text-stone-500'}`}>
              {count}
            </span>
          </button>
        ))}
      </div>

      {/* Notifications list */}
      {displayed.length === 0 ? (
        <div className="card flex flex-col items-center py-16 text-center animate-fade-in">
          <div className="w-16 h-16 rounded-2xl bg-stone-100 flex items-center justify-center mb-4">
            <BellOff size={28} className="text-stone-400" />
          </div>
          <h3 className="font-display font-bold text-stone-700 text-lg mb-2">
            {filter === 'unread' ? 'Aucune notification non lue' : 'Aucune notification'}
          </h3>
          <p className="text-stone-400 text-sm font-body max-w-xs">
            {filter === 'unread'
              ? 'Vous avez lu toutes vos notifications.'
              : 'Les alertes de votre système d\'irrigation apparaîtront ici.'}
          </p>
        </div>
      ) : (
        <div className="space-y-2 animate-slide-up">
          {displayed.map((notif) => {
            const cfg = getConfig(notif.type);
            const Icon = cfg.icon;
            return (
              <div
                key={notif.id}
                className={`flex items-start gap-4 p-4 rounded-2xl border transition-all duration-200 ${
                  notif.is_read
                    ? 'bg-white border-stone-100'
                    : `${cfg.bg} ${cfg.border}`
                }`}
              >
                <div className={`w-9 h-9 rounded-xl flex items-center justify-center shrink-0 ${notif.is_read ? 'bg-stone-100' : cfg.bg}`}>
                  <Icon size={18} className={notif.is_read ? 'text-stone-400' : cfg.color} />
                </div>
                <div className="flex-1 min-w-0">
                  <div className="flex items-start justify-between gap-2">
                    <p className={`text-sm font-medium font-body ${notif.is_read ? 'text-stone-600' : 'text-stone-900'}`}>
                      {notif.message || notif.title || 'Nouvelle notification'}
                    </p>
                    {!notif.is_read && (
                      <div className="w-2 h-2 rounded-full bg-primary-500 mt-1.5 shrink-0" />
                    )}
                  </div>
                  {notif.body && (
                    <p className="text-xs text-stone-400 font-body mt-0.5 line-clamp-2">{notif.body}</p>
                  )}
                  <div className="flex items-center gap-3 mt-2">
                    <span className={`text-xs font-body px-2 py-0.5 rounded-full ${notif.is_read ? 'bg-stone-100 text-stone-400' : `${cfg.bg} ${cfg.color}`}`}>
                      {cfg.label}
                    </span>
                    <span className="text-xs text-stone-400 font-body">
                      {formatDistanceToNow(new Date(notif.created_at), { addSuffix: true, locale: fr })}
                    </span>
                  </div>
                </div>
                {!notif.is_read && (
                  <button
                    onClick={() => markRead(notif.id)}
                    title="Marquer comme lu"
                    className="shrink-0 w-8 h-8 flex items-center justify-center rounded-lg text-stone-400 hover:text-primary-600 hover:bg-white transition-all"
                  >
                    <Check size={15} />
                  </button>
                )}
              </div>
            );
          })}
        </div>
      )}
    </div>
  );
}
