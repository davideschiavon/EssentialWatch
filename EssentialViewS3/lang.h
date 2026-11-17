// EssentialWatch - user interface languages (English, French, Spanish)
//
// The English text is the key: T("Settings") returns the text in the language chosen in the settings
// (cfg.lang). A text missing from the table stays in English. The characters used here must exist in the
// fonts (tools/make_fonts.py, EXTRA): the letters that the Essentialsail fonts lack come from DejaVu Sans.
#pragma once
#include <string.h>

enum Lang : uint8_t { L_EN = 0, L_FR = 1, L_ES = 2, L_COUNT = 3 };
static const char *const LANG_CODES[L_COUNT] = {"en", "fr", "es"};
static const char *const LANG_NAMES[L_COUNT] = {"English", "Français", "Español"};

struct Tr { const char *en, *fr, *es; };

static const Tr TR[] = {
  // settings menu
  {"WiFi network", "Réseau WiFi", "Red WiFi"},
  {"WiFi password", "Mot de passe WiFi", "Contraseña WiFi"},
  {"NMEA server IP", "IP serveur NMEA", "IP servidor NMEA"},
  {"Port", "Port", "Puerto"},
  {"Depth alarm", "Alarme profondeur", "Alarma profundidad"},
  {"Collision CPA", "CPA collision", "CPA colisión"},
  {"Collision TCPA", "TCPA collision", "TCPA colisión"},
  {"Language", "Langue", "Idioma"},
  {"Theme", "Thème", "Tema"},
  {"Screen timeout", "Veille écran", "Apagado pantalla"},
  {"Raise to wake", "Lever pour réveiller", "Levantar para activar"},
  {"MOB messages", "Messages MOB", "Mensajes MOB"},
  {"MOB server IP", "IP serveur MOB", "IP servidor MOB"},
  {"MOB owner name", "Nom du porteur", "Nombre del dueño"},
  {"MOB pairing code", "Code d'appairage MOB", "Código de emparejamiento MOB"},
  {"Paired", "Appairé", "Emparejado"},
  {"Failed", "Échec", "Falló"},
  {"Never", "Jamais", "Nunca"},
  {"On", "Activé", "Activado"},
  {"Off", "Désactivé", "Desactivado"},
  {"Day", "Jour", "Día"},
  {"Elegant Black", "Noir élégant", "Negro elegante"},
  {"Night", "Nuit", "Noche"},
  {"Diagnostics", "Diagnostic", "Diagnóstico"},
  {"Save and restart", "Enregistrer et redémarrer", "Guardar y reiniciar"},
  {"Restart", "Redémarrer", "Reiniciar"},
  {"Factory reset", "Réinitialisation usine", "Restablecer fábrica"},
  {"Power off", "Éteindre", "Apagar"},
  {"Exit without saving", "Quitter sans enregistrer", "Salir sin guardar"},
  {"(none)", "(aucun)", "(ninguna)"},
  {"(empty)", "(vide)", "(vacío)"},
  {"SETTINGS", "RÉGLAGES", "AJUSTES"},
  {"Tap the keys below.  After editing, 'Save and restart'.", "Touchez les touches.  Puis « Enregistrer et redémarrer ».",
   "Pulse las teclas.  Después, «Guardar y reiniciar»."},
  // keys
  {"UP", "HAUT", "ARRIBA"},
  {"DOWN", "BAS", "ABAJO"},
  {"BACK", "RETOUR", "ATRÁS"},
  // editor
  {"Server port", "Port du serveur", "Puerto del servidor"},
  {"Depth alarm (m)", "Alarme profondeur (m)", "Alarma profundidad (m)"},
  {"Collision CPA (nm)", "CPA collision (nm)", "CPA colisión (nm)"},
  {"Collision TCPA (min)", "TCPA collision (min)", "TCPA colisión (min)"},
  {"Characters: ", "Caractères : ", "Caracteres: "},
  {"UP/DOWN: character (hold = fast)   OK: add   BACK: delete", "HAUT/BAS : lettre (long = vite)  OK : ajout  RETOUR : effacer",
   "ARRIBA/ABAJO: letra (largo = rápido)  OK: añadir  ATRÁS: borrar"},
  {"OK hold = confirm      BACK hold = cancel", "OK long = valider      RETOUR long = annuler", "OK largo = confirmar     ATRÁS largo = cancelar"},
  // WiFi scan
  {"SELECT WIFI NETWORK", "CHOISIR LE RÉSEAU WIFI", "ELEGIR RED WIFI"},
  {"No networks found", "Aucun réseau trouvé", "No se encontraron redes"},
  {"Choose the boat WiFi, then OK to enter its password.", "Choisissez le WiFi du bateau, puis OK pour le mot de passe.",
   "Elija el WiFi del barco y pulse OK para la contraseña."},
  {"Scanning WiFi...", "Recherche WiFi...", "Buscando WiFi..."},
  // WiFi state (diagnostics)
  {"setup mode (no boat WiFi set)", "mode config (pas de WiFi bateau)", "modo config. (sin WiFi del barco)"},
  {"connected", "connecté", "conectado"},
  {"SSID not found (5 GHz network?)", "SSID introuvable (réseau 5 GHz ?)", "SSID no encontrado (¿red 5 GHz?)"},
  {"connect failed (wrong password?)", "échec (mauvais mot de passe ?)", "fallo (¿contraseña errónea?)"},
  {"connection lost", "connexion perdue", "conexión perdida"},
  {"disconnected / connecting", "déconnecté / connexion", "desconectado / conectando"},
  {"idle", "inactif", "inactivo"},
  {"unknown", "inconnu", "desconocido"},
  // diagnostics
  {"DIAGNOSTICS", "DIAGNOSTIC", "DIAGNÓSTICO"},
  {"Network: ", "Réseau : ", "Red: "},
  {"Server: ", "Serveur : ", "Servidor: "},
  {"Connect tries: ", "Tentatives : ", "Intentos: "},
  {"Sentences: ", "Phrases : ", "Frases: "},
  {"Bad checksum: ", "Checksum erroné : ", "Checksum erróneo: "},
  {"   other ", "   autres ", "   otras "},
  {"Last: ", "Dernière : ", "Última: "},
  {"Setup WiFi: ", "WiFi config : ", "WiFi config.: "},
  {"  pass ", "  mdp ", "  clave "},
  {"No WiFi: 2.4 GHz networks only.", "Pas de WiFi : réseaux 2,4 GHz seulement.", "Sin WiFi: solo redes de 2,4 GHz."},
  {"No sentences: wrong port, UDP-only or busy server.", "Pas de phrases : port erroné, UDP ou serveur occupé.",
   "Sin frases: puerto erróneo, solo UDP o servidor ocupado."},
  // factory reset
  {"FACTORY RESET", "RÉINITIALISATION", "RESTABLECER"},
  {"All settings will be erased:", "Tous les réglages seront effacés :", "Se borrarán todos los ajustes:"},
  {"boat WiFi, NMEA server, alarm,", "WiFi bateau, serveur NMEA, alarme,", "WiFi del barco, servidor NMEA, alarma,"},
  {"name, orientation, theme,", "nom, orientation, thème,", "nombre, orientación, tema,"},
  {"admin password (back to admin).", "mot de passe admin (retour à admin).", "contraseña admin (vuelve a admin)."},
  {"The device restarts in setup mode:", "L'appareil redémarre en mode config :", "El equipo se reinicia en modo config.:"},
  {"open WiFi", "WiFi ouvert", "WiFi abierto"},
  {"OK hold = erase everything and restart.  BACK = cancel.", "OK long = tout effacer et redémarrer.  RETOUR = annuler.",
   "OK largo = borrar todo y reiniciar.  ATRÁS = cancelar."},
  // first setup
  {"WELCOME - FIRST SETUP", "BIENVENUE - CONFIGURATION", "BIENVENIDO - CONFIGURACIÓN"},
  {"1. On your phone or PC join the", "1. Sur le téléphone ou PC, rejoignez", "1. En el móvil o PC conéctese a la"},
  {"    open WiFi network:", "    le réseau WiFi ouvert :", "    red WiFi abierta:"},
  {"2. The setup page opens by itself,", "2. La page de config s'ouvre seule,", "2. La página de config. se abre sola,"},
  {"    or open  http://192.168.4.1", "    ou ouvrez  http://192.168.4.1", "    o abra  http://192.168.4.1"},
  {"3. Login: admin    Password: admin", "3. Identifiant : admin   Mot de passe : admin", "3. Usuario: admin    Contraseña: admin"},
  {"4. Choose the boat WiFi and the", "4. Choisissez le WiFi du bateau et le", "4. Elija el WiFi del barco y el"},
  {"    NMEA server, then Save.", "    serveur NMEA, puis Enregistrer.", "    servidor NMEA, luego Guardar."},
  {"OK opens the settings here.  BACK = dashboard.", "OK ouvre les réglages ici.  RETOUR = tableau de bord.",
   "OK abre los ajustes aquí.  ATRÁS = panel."},
  // man overboard page
  {"MAN OVERBOARD", "HOMME À LA MER", "HOMBRE AL AGUA"},
  {"Hold to raise the alarm", "Appui long = alarme", "Mantener = alarma"},
  {"Not set up: use the web page", "Non configuré : page web", "Sin configurar: página web"},
  {"No WiFi", "Pas de WiFi", "Sin WiFi"},
  {"Tablet connected", "Tablette connectée", "Tableta conectada"},
  {"Tablet not answering", "Tablette sans réponse", "Tableta sin respuesta"},
  {"Delivered", "Livré", "Entregado"},
  {"NOT DELIVERED", "NON LIVRÉ", "NO ENTREGADO"},
  {"Cancelling...", "Annulation...", "Cancelando..."},
  {"Delivering...", "Envoi...", "Enviando..."},
  {"Hold again to confirm cancel", "Appui long : confirmer", "Mantener: confirmar"},
  {"Hold to cancel the alarm", "Appui long : annuler", "Mantener: cancelar"},
  {"SIREN - tap to mute", "SIRÈNE - toucher = silence", "SIRENA - tocar = silencio"},
  {"Siren muted", "Sirène coupée", "Sirena silenciada"},
  {"last %d s ago", "il y a %d s", "hace %d s"},
  // messages
  {"Saved", "Enregistré", "Guardado"},
  {"Restarting...", "Redémarrage...", "Reiniciando..."},
  {"Powering off...", "Extinction...", "Apagando..."},
  {"Alarm snoozed for 5 minutes", "Alarme en pause 5 minutes", "Alarma en pausa 5 minutos"},
  // dashboard
  {"BOOT = SNOOZE 5 MIN", "BOOT = PAUSE 5 MIN", "BOOT = PAUSA 5 MIN"},
  {"ALARM SNOOZED  %d:%02d", "ALARME EN PAUSE  %d:%02d", "ALARMA EN PAUSA  %d:%02d"},
  {"SIGNAL LOST", "SIGNAL PERDU", "SEÑAL PERDIDA"},
  {"NO TIME", "PAS D'HEURE", "SIN HORA"},
  {"HEADING", "CAP", "RUMBO"},
  {"AWS  apparent  kn", "AWS  apparent  kn", "AWS  aparente  kn"},
  {"TWS  true  kn", "TWS  réel  kn", "TWS  real  kn"},
  {"Apparent wind", "Vent apparent", "Viento aparente"},
  {"True wind", "Vent réel", "Viento real"},
  {"SOG  speed over ground", "SOG  vitesse fond", "SOG  velocidad fondo"},
  {"DEPTH  below transducer", "PROFONDEUR  sous la sonde", "PROFUNDIDAD  bajo la sonda"},
  {"   keel ", "   quille ", "   quilla "},
  {"NO AIS TARGETS NEARBY", "AUCUNE CIBLE AIS PROCHE", "SIN BLANCOS AIS CERCA"},
  {"NO OWN POSITION", "PAS DE POSITION", "SIN POSICIÓN PROPIA"},
  {"No boat in collision course or within 3 nm", "Aucun bateau en collision ni à moins de 3 nm",
   "Ningún barco en colisión ni a menos de 3 nm"},
  {"Needs GPS position from NMEA (RMC / GLL / GGA)", "Position GPS NMEA requise (RMC / GLL / GGA)",
   "Requiere posición GPS NMEA (RMC / GLL / GGA)"},
  {"CPA %s in %d min    %.1f kn", "CPA %s dans %d min    %.1f kn", "CPA %s en %d min    %.1f kn"},
  {"Not approaching    %.1f kn", "S'éloigne    %.1f kn", "Se aleja    %.1f kn"},
};

static const char *const THEME_NAMES[3] = {"Day", "Elegant Black", "Night"};   // index = cfg.theme

// Text in the current language (the English text when the table has no translation)
static const char *T(const char *en) {
  if (cfg.lang == L_EN || cfg.lang >= L_COUNT) return en;
  for (const Tr &t : TR)
    if (!strcmp(t.en, en)) return cfg.lang == L_FR ? t.fr : t.es;
  return en;
}

static const char *const DAYS[L_COUNT][7] = {
  {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
  {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"},
  {"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"},
};
static const char *const MONTHS[L_COUNT][12] = {
  {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"},
  {"janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre"},
  {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre"},
};

// Language code ("en", "fr", "es") -> index; -1 if unknown
static int langFromCode(const char *c) {
  for (int i = 0; i < L_COUNT; i++)
    if (!strcmp(c, LANG_CODES[i])) return i;
  return -1;
}
