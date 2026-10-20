# Teko per Respawn & Rage

**Nota:** il download diretto dei file font è risultato non disponibile in questa sessione. Questo ZIP contiene licenza, credits e script per ottenere i font originali (non contiene ancora i TTF).

## Scaricare i font
Eseguire `python3 download_teko.py` con una connessione Internet. Verrà salvato `Fonts/Teko-Variable.ttf`, scaricato dal repository ufficiale Google Fonts.

## Unreal Engine
Importare `Fonts/Teko-Variable.ttf` nel Content Browser e creare un asset Font per l'UI (UMG/Slate). Se la versione di Unreal non gestisce correttamente le istanze variabili, generare pesi statici usando fonttools oppure scaricare il pacchetto statico da Google Fonts.

## Credits e obblighi di distribuzione
Quando distribuisci il gioco con Teko incorporato, mantieni copyright e licenza integrale OFL in un file consultabile, come `ThirdPartyLicenses/Teko-OFL.txt`. `CREDITS.txt` è una dicitura pratica facoltativa e non sostituisce OFL. Il font è utilizzabile commercialmente, senza obbligo di aprire il codice del gioco.

Font originale: https://fonts.google.com/specimen/Teko
Repository: https://github.com/google/fonts/tree/main/ofl/teko
