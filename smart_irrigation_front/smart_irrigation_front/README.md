# 🌿 AquaSmart — Frontend React

Interface web du système d'irrigation intelligent basé sur ESP32.

## Stack technique
- **React 18** + **Vite**
- **Tailwind CSS** (design personnalisé)
- **React Router v6** (navigation SPA)
- **Axios** (appels API REST)
- **Recharts** (graphiques historique capteurs)
- **react-hot-toast** (notifications UI)
- **date-fns** (formatage des dates en français)

## Structure du projet
```
src/
├── context/
│   └── AuthContext.jsx       # Gestion session / token Sanctum
├── services/
│   └── api.js                # Tous les appels API Laravel
├── components/
│   └── Layout/
│       └── Layout.jsx        # Sidebar + navigation principale
├── pages/
│   ├── Auth/
│   │   ├── Login.jsx
│   │   └── Register.jsx
│   ├── Dashboard/
│   │   └── Dashboard.jsx     # Tableau de bord (capteurs temps réel)
│   ├── Crops/
│   │   └── Crops.jsx         # Gestion cultures (prédéfinies + custom)
│   ├── Irrigation/
│   │   └── Irrigation.jsx    # Mode Manuel + Mode Automatique
│   ├── Notifications/
│   │   └── Notifications.jsx
│   └── Devices/
│       └── Devices.jsx       # Gestion appareils ESP32
├── App.jsx
├── main.jsx
└── index.css
```

## Installation

### 1. Prérequis
- Node.js >= 18
- Backend Laravel (`Smart_Irrigation_Mobile_App_V2R`) en fonctionnement sur `http://localhost:8000`

### 2. Cloner / extraire le projet

### 3. Installer les dépendances
```bash
npm install
```

### 4. Configurer l'API
```bash
cp .env.example .env
# Modifier VITE_API_URL si votre backend tourne sur un autre port
```

### 5. Lancer le serveur de développement
```bash
npm run dev
# → http://localhost:3000
```

### 6. Build production
```bash
npm run build
# Le dossier dist/ peut être servi via Nginx ou Apache
```

## Configuration CORS (Laravel)
Dans `config/cors.php` du backend, assurez-vous que :
```php
'allowed_origins' => ['http://localhost:3000'],
```

## Routes API attendues
| Méthode | Endpoint | Contrôleur |
|---------|----------|------------|
| POST | `/api/login` | AuthController |
| POST | `/api/register` | AuthController |
| POST | `/api/logout` | AuthController |
| GET | `/api/crops` | CropsController |
| POST | `/api/crops` | CropsController |
| DELETE | `/api/crops/{id}` | CropsController |
| GET | `/api/devices` | DeviceController |
| POST | `/api/devices` | DeviceController |
| GET | `/api/devices/{id}/sensors/latest` | SensorController |
| GET | `/api/devices/{id}/sensors` | SensorController |
| GET | `/api/devices/{id}/events` | IrrigationEventController |
| POST | `/api/devices/{id}/irrigate` | IrrigationEventController |
| POST | `/api/devices/{id}/stop` | IrrigationEventController |
| POST | `/api/irrigation-times` | IrrigationTimeController |
| DELETE | `/api/irrigation-times/{id}` | IrrigationTimeController |
| GET | `/api/notifications` | NotificationController |
| PUT | `/api/notifications/{id}/read` | NotificationController |
| PUT | `/api/notifications/read-all` | NotificationController |

## Membres du groupe
- Mariama DIALLO
- Ndèye Maimouna DIOP
- Modou NDIAYE
- Ibrahima TALL
- Ndiaye Coumba Mbaye

**Responsable : M. Robert — ISM ETSE (Licence 3) — 2025/2026**
