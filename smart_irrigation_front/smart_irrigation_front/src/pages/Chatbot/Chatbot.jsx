import { useState, useEffect, useRef } from 'react';
import { Send, MessageSquare, Bot, User, Sparkles, RefreshCw } from 'lucide-react';

const initialMessages = [
  { 
    role: 'assistant', 
    text: 'Bonjour ! 👋 Je suis votre assistant AgroSmart. Je connais tout sur votre système d\'irrigation. Que souhaitez-vous savoir ?',
    suggestions: ['Comment ajouter un ESP32 ?', 'Voir mes capteurs', 'Programmer l\'arrosage']
  },
];

const knowledgeBase = [
  {
    tags: ['bonjour', 'salut', 'hello', 'aide'],
    answer: 'Bonjour ! Je peux vous aider à gérer vos appareils, surveiller l’humidité, ou programmer vos cycles d’arrosage. 🌱',
  },
  {
    tags: ['appareil', 'esp32', 'device', 'ajouter'],
    answer: 'Pour configurer un nouvel appareil :\n1. Allez dans la section **Appareils**.\n2. Cliquez sur "Ajouter".\n3. Entrez la clé unique de votre ESP32.',
  },
  {
    tags: ['capteur', 'humidité', 'température', 'donnée', 'sensor'],
    answer: 'Vos graphiques en temps réel se trouvent sur le **Tableau de bord**. Si aucune donnée ne s\'affiche, vérifiez la connexion Wi-Fi de votre boîtier.',
  },
  {
    tags: ['irrigation', 'arrosage', 'programme', 'planifier'],
    answer: 'Dans l\'onglet **Irrigation**, vous pouvez définir des seuils automatiques (ex: arroser si humidité < 30%) ou des plages horaires précises.',
  },
  {
    tags: ['culture', 'plante', 'tomate', 'maïs'],
    answer: 'Chaque plante a des besoins différents. Vous pouvez choisir une culture prédéfinie dans l\'onglet **Cultures** pour appliquer automatiquement les paramètres optimaux.',
  }
];

export default function Chatbot() {
  const [messages, setMessages] = useState(initialMessages);
  const [input, setInput] = useState('');
  const [isTyping, setIsTyping] = useState(false);
  const scrollRef = useRef(null);

  // Scroll automatique vers le bas
  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [messages, isTyping]);

  const findResponse = (question) => {
    const query = question.toLowerCase();
    const match = knowledgeBase.find(item => 
      item.tags.some(tag => query.includes(tag))
    );
    
    return match ? match.answer : "Je n'ai pas bien compris. Essayez de me parler de vos 'capteurs', 'irrigation' ou 'appareils'.";
  };

  const handleSend = async (text) => {
    const userText = text || input;
    if (!userText.trim()) return;

    // Ajouter le message utilisateur
    setMessages(prev => [...prev, { role: 'user', text: userText }]);
    setInput('');
    setIsTyping(true);

    // Simuler un délai de réflexion "IA"
    setTimeout(() => {
      const response = findResponse(userText);
      setMessages(prev => [...prev, { role: 'assistant', text: response }]);
      setIsTyping(false);
    }, 800);
  };

  return (
    <div className="max-w-2xl mx-auto space-y-4">
      <div className="flex justify-between items-center px-2">
        <div>
          <h1 className="text-2xl font-bold flex items-center gap-2">
            <Sparkles className="text-primary-600" size={24} /> 
            Assistant AgroSmart
          </h1>
          <p className="text-stone-500 text-sm">Expert en agriculture connectée</p>
        </div>
        <button 
          onClick={() => setMessages(initialMessages)}
          className="p-2 hover:bg-stone-100 rounded-full transition-colors"
          title="Réinitialiser"
        >
          <RefreshCw size={18} className="text-stone-400" />
        </button>
      </div>

      <div className="bg-white border border-stone-200 rounded-3xl shadow-sm overflow-hidden flex flex-col h-[600px]">
        {/* Zone de messages */}
        <div ref={scrollRef} className="flex-1 overflow-y-auto p-6 space-y-6 bg-stone-50/50">
          {messages.map((msg, i) => (
            <div key={i} className={`flex ${msg.role === 'user' ? 'justify-end' : 'justify-start'}`}>
              <div className={`flex gap-3 max-w-[85%] ${msg.role === 'user' ? 'flex-row-reverse' : ''}`}>
                <div className={`w-8 h-8 rounded-full flex items-center justify-center shrink-0 ${
                  msg.role === 'assistant' ? 'bg-primary-600 text-white' : 'bg-stone-200 text-stone-600'
                }`}>
                  {msg.role === 'assistant' ? <Bot size={16} /> : <User size={16} />}
                </div>
                
                <div className="space-y-3">
                  <div className={`p-4 rounded-2xl text-sm leading-relaxed ${
                    msg.role === 'assistant' 
                    ? 'bg-white border border-stone-200 text-stone-800 shadow-sm' 
                    : 'bg-primary-600 text-white shadow-md'
                  }`}>
                    <p className="whitespace-pre-line">{msg.text}</p>
                  </div>

                  {/* Suggestions (uniquement pour le dernier message assistant) */}
                  {msg.suggestions && i === messages.length - 1 && (
                    <div className="flex flex-wrap gap-2">
                      {msg.suggestions.map((s, idx) => (
                        <button 
                          key={idx} 
                          onClick={() => handleSend(s)}
                          className="text-xs bg-white border border-primary-200 text-primary-700 px-3 py-1.5 rounded-full hover:bg-primary-50 transition-colors"
                        >
                          {s}
                        </button>
                      ))}
                    </div>
                  )}
                </div>
              </div>
            </div>
          ))}

          {isTyping && (
            <div className="flex gap-3">
              <div className="w-8 h-8 rounded-full bg-primary-600 text-white flex items-center justify-center">
                <Bot size={16} />
              </div>
              <div className="bg-white border border-stone-200 p-4 rounded-2xl shadow-sm">
                <div className="flex gap-1">
                  <span className="w-1.5 h-1.5 bg-stone-400 rounded-full animate-bounce"></span>
                  <span className="w-1.5 h-1.5 bg-stone-400 rounded-full animate-bounce [animation-delay:0.2s]"></span>
                  <span className="w-1.5 h-1.5 bg-stone-400 rounded-full animate-bounce [animation-delay:0.4s]"></span>
                </div>
              </div>
            </div>
          )}
        </div>

        {/* Formulaire */}
        <div className="p-4 bg-white border-t border-stone-100">
          <form onSubmit={(e) => { e.preventDefault(); handleSend(); }} className="relative flex items-center gap-2">
            <input
              value={input}
              onChange={(e) => setInput(e.target.value)}
              placeholder="Posez votre question ici..."
              className="w-full pl-4 pr-12 py-4 bg-stone-100 border-none rounded-2xl focus:ring-2 focus:ring-primary-500 focus:bg-white transition-all text-sm outline-none"
            />
            <button 
              type="submit" 
              disabled={!input.trim()}
              className="absolute right-2 p-2.5 bg-primary-600 text-white rounded-xl hover:bg-primary-700 disabled:opacity-50 disabled:bg-stone-300 transition-all shadow-lg shadow-primary-200"
            >
              <Send size={18} />
            </button>
          </form>
        </div>
      </div>
    </div>
  );
}