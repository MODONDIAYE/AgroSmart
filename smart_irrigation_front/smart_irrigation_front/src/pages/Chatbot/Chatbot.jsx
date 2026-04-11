import { useState } from 'react';
import { Send, MessageSquare } from 'lucide-react';

const initialMessages = [
  { role: 'assistant', text: 'Bonjour ! Je suis votre assistant AgroSmart. Posez-moi une question sur l’irrigation, les cultures, les capteurs ou les notifications.' },
];

function createAssistantReply(question) {
  const normalized = question.toLowerCase();

  if (/\b(bonjour|salut|hello|hi|coucou)\b/.test(normalized)) {
    return 'Bonjour ! Je peux vous aider à vérifier l’état de vos appareils, vos données capteurs, vos programmes d’irrigation ou vos notifications.';
  }

  if (normalized.includes('capteur') || normalized.includes('humidité') || normalized.includes('température') || normalized.includes('eau')) {
    return 'Les données des capteurs sont visibles dans le tableau de bord. Si le graphique est vide, vérifiez que votre ESP32 envoie bien les lectures et que l’appareil est bien en ligne.';
  }

  if (normalized.includes('irrigation') || normalized.includes('arroser') || normalized.includes('arrosage')) {
    return 'Vous pouvez piloter l’arrosage depuis le tableau de bord et configurer les horaires dans la page Irrigation. Je peux aussi vous dire si votre appareil est en ligne.';
  }

  if (normalized.includes('culture') || normalized.includes('crop') || normalized.includes('plante')) {
    return 'Vous pouvez affecter une culture à un appareil depuis la page Appareils. Si cela ne fonctionne pas, assurez-vous que la page a bien chargé les cultures disponibles et réessayez.';
  }

  if (normalized.includes('notification') || normalized.includes('alerte') || normalized.includes('message')) {
    return 'Les notifications s’affichent dans la page Notifications. Si elle est vide, cela signifie qu’il n’y a pas encore d’alertes pour le moment.';
  }

  if (normalized.includes('contact') || normalized.includes('assistance') || normalized.includes('aide')) {
    return 'Pour nous contacter, rendez-vous dans Paramètres : vous y trouverez nos coordonnées téléphoniques et notre adresse e-mail pour obtenir de l’assistance.';
  }

  return 'J’ai bien reçu votre question. Vous pouvez me demander des informations sur l’irrigation, les capteurs, l’état des appareils, les cultures ou les notifications.';
}

export default function Chatbot() {
  const [messages, setMessages] = useState(initialMessages);
  const [input, setInput] = useState('');

  const handleSend = (e) => {
    e.preventDefault();
    const trimmed = input.trim();
    if (!trimmed) return;

    const userMessage = { role: 'user', text: trimmed };
    const assistantReply = {
      role: 'assistant',
      text: createAssistantReply(trimmed),
    };

    setMessages((prev) => [...prev, userMessage, assistantReply]);
    setInput('');
  };

  return (
    <div className="space-y-6">
      <div>
        <h1 className="font-display text-2xl font-bold text-stone-900">Assistant IA</h1>
        <p className="text-stone-500 text-sm font-body mt-1">Discutez avec votre assistant pour optimiser l’irrigation et mieux comprendre vos données.</p>
      </div>

      <div className="card p-6">
        <div className="flex items-center gap-3 mb-5">
          <div className="w-11 h-11 rounded-2xl bg-primary-100 text-primary-700 flex items-center justify-center">
            <MessageSquare size={18} />
          </div>
          <div>
            <h2 className="text-base font-semibold text-stone-900">Chat en temps réel</h2>
            <p className="text-sm text-stone-500 font-body">Posez une question et obtenez des réponses immédiates sur votre installation.</p>
          </div>
        </div>

        <div className="space-y-4 mb-4 max-h-[430px] overflow-y-auto pr-2">
          {messages.map((message, index) => (
            <div key={index} className={`rounded-3xl p-4 ${message.role === 'assistant' ? 'bg-stone-100 self-start' : 'bg-primary-600 text-white self-end'} max-w-xl`}>
              <p className={`text-sm ${message.role === 'assistant' ? 'text-stone-700' : 'text-white'}`}>{message.text}</p>
            </div>
          ))}
        </div>

        <form onSubmit={handleSend} className="flex items-center gap-3">
          <input
            value={input}
            onChange={(e) => setInput(e.target.value)}
            placeholder="Posez une question..."
            className="input-field flex-1"
          />
          <button type="submit" className="btn-primary px-4 py-3 flex items-center gap-2">
            <Send size={16} /> Envoyer
          </button>
        </form>
      </div>
    </div>
  );
}
