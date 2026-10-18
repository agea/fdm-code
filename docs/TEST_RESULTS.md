# Verifiche locali — 12 ottobre 2026

Unreal **5.8.3**, Editor runtime Mac Development arm64, macOS 27.0.1,
M2 Pro / 32 GB / Xcode 27. Nessun cook o pacchetto standalone.

## Eseguito con esito positivo

- Compilazione reale del modulo Editor tramite `Build.sh`: **Succeeded**.
- `scripts/setup_editor.py` eseguito nel Python commandlet Unreal:
  `ER_SETUP_SUCCESS`; mappe, Blueprint, materiale, Data Asset e input salvati.
- `scripts/test-network.py`: listen server e client in processi distinti,
  collegati a `127.0.0.1:17777`. Proiettili e RPC reali, input di test pilotato
  da una sonda di sviluppo esplicitamente attivata. **19 asserzioni superate**.
- `scripts/test-admission.py`: host e tre client, quarto client rifiutato,
  ingressi successivi e disconnessione. **4 controlli superati**.
- Unreal Automation `EclipseRecoil.Menu.IPv4Validation`: **Success**.
  Indirizzi validi, IPv4 malformati, porte fuori intervallo, opzioni travel e nomi.
- Avvio grafico con Metal SM6: menu visibile; partita locale aperta direttamente
  sulla mappa con HUD, HP 100, fucile e 30/90. Posa FPS corretta e verificata
  graficamente: arma rivolta in avanti nella parte bassa dello schermo,
  mano sinistra sul foregrip e mirino libero. L'input fisico resta distinto
  dalle prove di rete senza rendering.

Log completi locali in `Saved/Testing/`, `setup-editor.log`, `automation.log` e
`play-final.log` (ignorati da Git). Estratto persistente in `VERIFICATION.txt`.

## Copertura della specifica

| ID | Stato | Evidenza / limite |
|---|---|---|
| T01 | PARZIALE | Due processi collegati e replica gameplay; visibilità reciproca da due finestre grafiche da completare |
| T02 | PARZIALE | WASD, salto e crouch con C verificati tramite input reali e log; Ctrl fisico, mira mouse e percorrenza completa da completare |
| T03 | SUPERATO | Quattro connessioni, Alpha 2 / Omega 2; quinto giocatore rifiutato |
| T04 | SUPERATO | Host: HP 100→64→28→0; RPC e proiettile client→host: 100→64 |
| T05 | PARZIALE | Blocco centrale impedisce il danno; caso canna contro parete da collaudare visivamente |
| T06 | SUPERATO | Otto Start/Stop rapidi consumano un solo colpo; raffica continua governata dal timer server |
| T07 | SUPERATO | Trenta colpi esauriscono esattamente il caricatore; zero munizioni negative |
| T08 | SUPERATO | Ricarica parziale 20+90→30+80 dopo timer |
| T09 | SUPERATO | Ricarica con riserva 20+5→25+0 |
| T10 | SUPERATO | Morte durante ricarica cancella il trasferimento; nuova vita conserva 30+90 |
| T11 | SUPERATO | Tre colpi friendly fire; TeamKills +1; Kills e punteggio squadra invariati |
| T12 | SUPERATO | Richiesta anticipata respinta; nessun respawn automatico; nuova richiesta client dopo 3 s accettata |
| T13 | PARZIALE | Nuovo Pawn non spara e resta protetto; pressione fisica tenuta durante respawn da verificare |
| T14 | PARZIALE | Bersaglio protetto ignora danni, primo colpo autorizzato toglie protezione; scadenza naturale di 4 s da verificare visivamente |
| T15 | SUPERATO | Doppio danno letale registra una sola morte; combattimento cancellato; nessun pickup |
| T16 | SUPERATO | Termine temporale con pareggio, restart dopo 10 s, limite 50 con vittoria Alpha |
| T17 | PARZIALE | Ingressi 3/4 non riavviano il match; timer e scoreboard del late join da verificare in UI |
| T18 | PARZIALE | HUD host verificato anche dopo respawn: HP 100, 30/90 e protezione sulla nuova vita; UI del client da completare |
| T19 | PARZIALE | Uscita rimuove giocatore; cancellazione ricarica verificata con morte; uscita durante attesa/ricarica da completare |
| T20 | PARZIALE | Validazione IP automatica e server pieno verificati; timeout e chiusura host tramite UI da completare |
| T21 | DA ESEGUIRE | Latenza / perdita simulate |
| T22 | RINVIATO | Shipping su secondo computer escluso dall'ambito corrente |
| T23 | PARZIALE | Provenienza template registrata; Fab e verifica per redistribuzione rinviati; nessuna distribuzione |

## Correzione emersa dai test

La prima prova della raffica continua non esauriva 30 colpi nel tempo atteso:
il timer ripetuto poteva scattare appena prima della scadenza basata sul clock
del mondo e saltare l'intero intervallo. Il fuoco ora pianifica un callback
singolo e, se anticipato, riprova solo il tempo residuo. Non anticipa la
scadenza server. La suite successiva ha superato il caso e i controlli correlati.

## Limiti

Aggiornamento dalla prova dell'utente: Ospita apre correttamente il deposito e
mostra HUD e arma. La schermata ha evidenziato l'orientamento errato del fucile;
corretti attacco a `HandGrip_R`, posizione del corpo FPS e assi degli script
Editor. `ER_VIEWMODEL_FIXED` e `ER_ROTATIONS_REPAIRED` registrati; compilazione
riuscita e suite rete ripetuta con tutte le 19 asserzioni superate. La successiva
posa statica di progetto è stata salvata (`ER_HIP_POSE_SAVED`) e verificata nella
sessione Metal del deposito; le due posizioni delle impugnature sono controllate
anche dallo script Editor. Log in `viewmodel-fix.log` e `viewmodel-play.log`.
Animazioni e dita ancora provvisorie. Non equiparare Ospita a un match iniziato: prima
dell'ingresso di Omega, la mappa resta in attesa di giocatori.

I test senza rendering non dimostrano qualità delle animazioni, navigazione
fisica con mouse o prestazioni. L'input della sonda usa le stesse RPC gameplay
ma non equivale a premere i tasti nell'Editor. Non dichiarare il prototipo
completamente collaudato finché i casi grafici sopra non sono conclusi.

## Bot di collaudo — aggiunta richiesta dall'utente

- Compilazione finale del modulo Editor: **Succeeded** (`bot-build.log`).
- `scripts/setup_bot_routes.py`: `ER_BOT_ROUTE_SAVED`; dodici punti di
  pattuglia aggiunti e salvati nella mappa esistente, senza ricostruzione.
- `scripts/test-bots.py`: **SUPERATO**, due modalità con listen server e client
  reali, rendering disattivato. **12 verifiche server + 2 verifiche client per
  modalità**, quindi 28 complessive. Controllati team, avvio partita, tutti i
  segmenti del percorso tramite sweep della capsula, movimento effettivo,
  danni da proiettile e consumo munizioni, visibilità bloccata dal muro,
  ricarica, morte/punteggio, respawn anticipato respinto, nuova vita protetta,
  fine partita e restart con bot. Il bersaglio fermo non si muove e non spara.
- Replica osservata dal secondo processo: Pawn, team, salute, morte e
  conteggio delle morti del bot. Non equivale a una prova con due finestre.
- `scripts/test-network.py` ripetuto dopo le modifiche al GameMode:
  **19 asserzioni superate**. Il respawn dei giocatori rimane manuale.
- `scripts/test-admission.py`: **4 controlli superati**, quattro umani
  Alpha 2 / Omega 2, quinto rifiutato, late join senza restart e disconnessione;
  confermata l'assenza di bot nell'hosting normale.
- `scripts/test-admission.py --bots`: **6 controlli superati**, stessi quattro
  casi più rimozione di tre bot a favore degli umani e ritorno di un bersaglio
  dopo la disconnessione. Bilanciamento finale Alpha 2 / Omega 2 confermato.
- Menu grafico Metal verificato: entrambi i nuovi pulsanti visibili e contenuti
  nella finestra. L'input UI automatizzato resta inaffidabile come nelle prove
  precedenti; il click fisico sui nuovi pulsanti resta da confermare. L'avvio
  diretto usa le stesse opzioni della partita.
- Partita grafica contro bot avviata direttamente: timer attivo, bot Omega
  registra eliminazioni reali del giocatore e compare il messaggio di rientro.
  Pressione di Spazio verificata: nuova vita con HP 100, 30/90, protezione e
  fucile FPS correttamente visibile. Sessione lasciata aperta. La qualità della
  locomozione e dell'impugnatura del bot resta da collaudare durante il gioco.

I log completi sono `Saved/Testing/bots-{patrol,target}-{host,client}.log`,
`bot-tests.log` e `bot-network-regression.log`; estratti in `VERIFICATION.txt`.

### Correzioni emerse dal collaudo

Il primo movimento bot veniva aggiornato ogni 50 ms, mentre CharacterMovement
consuma l'input a ogni frame: il bot accelerava troppo poco. Lo steering ora
fornisce input continuo; solo la scansione dei bersagli resta ogni 200 ms.
Il controllo successivo misura circa 581 cm percorsi in 2 s e 300 cm/s.

La prima prova degli ingressi con tre bersagli ha rilevato che il refill poteva
ricreare un bot tra PreLogin e l'ingresso effettivo, alterando anche il
bilanciamento finale. Il server ora riserva i posti alle connessioni ammesse,
li libera all'ingresso e fa scadere dopo 30 s le prenotazioni incomplete.

## Collaudo dei controlli nella finestra Metal

Compilazione finale: **Succeeded** (`input-build.log`). Il nuovo flag di sviluppo
`-ERInputAudit` registra il risultato degli input della finestra ogni 100 ms;
non genera input né modifica le regole ed è disattivato normalmente.

- **WASD verificati**: osservati spostamenti in tutte e quattro le direzioni e
  velocità fino a 600 cm/s. Salto verificato con Spazio: impulso positivo e
  campione a circa 128 cm sopra la quota iniziale, seguito dal ritorno a terra.
- **Crouch con C verificato**: semialtezza della capsula 88→40 cm, camera
  158→110 cm; al rilascio ritorna in piedi. Il Blueprint conserva capacità di
  crouch e limite di velocità accovacciato di 300 cm/s. Ctrl rimane associato;
  l'automazione non ha riprodotto affidabilmente il solo modificatore Ctrl.
- **Sparo e ricarica verificati**: click sinistro tenuto circa 800 ms consuma
  9 colpi (30/90→21/90); R mostra RICARICA, `reload=1` per circa 1,5 s e
  trasferisce la riserva correttamente (30/81). Al rilascio il fuoco si ferma.
- **Tab verificato**: scoreboard attivo durante la pressione e disattivato
  al rilascio. Esc apre il menu; Spazio sul pulsante Riprendi lo chiude.
- **Avvio Bersaglio fermo dal menu verificato con tastiera**: Tab seleziona
  il pulsante e Spazio lo attiva; deposito caricato con bot Target e timer.
- **Menu corretto e verificato**: il brush precedente lasciava trasparente
  la parte centrale, rendendo il testo illeggibile sulle pareti. Un WhiteBrush
  tinto scuro ora copre il viewport e mantiene leggibili testi e pulsanti.

`scripts/fix_controls.py` aggiunge solo l'alternativa C alla mappatura esistente.
I log `input-play.log`, `input-play-after.log` e `input-setup.log` sono ignorati
da Git; campioni persistenti in `VERIFICATION.txt`.
La mira con movimento relativo del mouse, Ctrl da tastiera fisica, percorso
completo della mappa e visibilità del bot restano da completare. Non sono
state ripetute le suite di combattimento: questo passaggio modifica solo
diagnostica, un binding aggiuntivo e lo sfondo del menu.


## Movimento e laboratorio — 13 ottobre 2026

Compilazione Editor **Succeeded**, asset aggiornati via Unreal Python.
`movement-assets.log` conferma le dimensioni dei venti blocchi: 200×200 cm,
altezze da 10 a 200 cm a intervalli di 10, più perimetro della mappa.

`scripts/test-movement.py`, due veri processi host/client con nullrhi:
**57 controlli superati**. Verificati capsula 185 cm, corsa 277,78 cm/s,
camminata 138,89 cm/s, accelerazione progressiva, frenata graduale, salto
misurato **59,9 cm** e ritorno al suolo entro 0,8 s. Tutti i venti blocchi
raggiunti: 10–30 cm senza salita guidata, 40–80 cm salita breve, 90–150 cm
arrampicata da terra, 160–200 cm arrampicata dopo salto. Il client remoto
verifica corsa/camminata replicate e salita sui blocchi 80, 150 e 200 cm.
Controlli negativi: un bordo alto richiede intento di arrampicata; 200 cm
non sono raggiungibili dalle spalle da fermi; un soffitto basso impedisce la
salita. Crouch 125 cm e ritorno a 185 cm, con camera e mira server coerenti.

Il primo tentativo di sonda falliva sui blocchi 160–200 cm: chiedeva il salto
subito dopo il teletrasporto, prima del contatto a terra. Attendere il normale
contatto nella preparazione della prova ha risolto il problema; nessun aumento
artificiale della quota di salto per ottenere il risultato.

Regressione `scripts/test-network.py`: **19 controlli superati** dopo il cambio
di altezza degli occhi e velocità. **28 controlli bot superati**, modalità Patrol e Target, incluse replica
remota, morte/respawn e riavvio del round. I controlli precedenti di WASD a 600 cm/s e salto
128 cm sono storici e non descrivono questa taratura.

Limiti: le sonde muovono i personaggi tramite input nativi e verificano il
risultato del motore; non attestano da sole la resa visiva o la pressione
fisica di Shift. L'arrampicata usa una traiettoria procedurale con collisioni,
ancora senza animazione dedicata di mani/corpo. Latenza artificiale e perdita
di pacchetti restano da collaudare.


Prova grafica Metal del laboratorio: mappa caricata, venti blocchi e cartelli
visibili, HUD con km/h e quota dei piedi. Input W reale osservato fino a
277,8 cm/s; al rilascio campioni 192,8→107,8→22,8→0 cm/s, quindi frenata
progressiva confermata. Salto e C registrati nella sessione grafica.
Shift+W emulato produce W ma non il modificatore nell'input Unreal, come
il precedente limite di Ctrl: non considerato un fallimento della velocità
nativa, ma la pressione fisica di Shift resta da verificare. Cartelli passati
a testo scuro per contrasto sul grigio e arancione del laboratorio.

Verifica finale Metal dopo i cartelli scuri: quote leggibili, blocco 30 cm
blu davanti allo spawn, file grigie/arancioni dietro, fucile rivolto avanti.
Laboratorio lasciato aperto con `play-movement.command`, senza diagnostica.


## Rifinitura mano / passo / mouse — 13 ottobre 2026

Compilazione Editor **Succeeded** (`presentation-build.log`). Aggiornamento
IA_Look via Unreal Python confermato: scalar 0,18 (+50%) e mappatura conservata
(`presentation-assets.log`). Avvio nel laboratorio con Metal riuscito:
materiali caricati dopo la compilazione shader, fucile e mano visibili a terra.
Sessione aggiornata lasciata aperta per il collaudo pratico dell'utente.

Modifiche cosmetiche: ramo del braccio FPS sinistro nascosto nella stessa
condizione del fucile, oscillazione leggera in CalcCamera, sensibilità input.
Non sono state ripetute le suite di combattimento/movimento: capsule, regole
server e fisica di attraversamento non sono state modificate. La qualità
percepita del passo e la sparizione/ripristino della mano durante la salita
rimangono da confermare nel gioco; nessuna attestazione basata sul solo fermo
immagine. Il mouse relativo non è misurabile in modo affidabile via CUA.

Ritocco mouse a 0,16: salvataggio Unreal Python confermato in
`sensitivity-assets.log`; nessuna modifica C++ o alla fisica.

Sensibilità ripristinata a 0,18 su richiesta dell’utente; salvataggio Unreal
confermato in `sensitivity-restore-assets.log`.

## Feedback dello sparo — 13 ottobre 2026

- Compilazione reale del modulo Editor: **Succeeded** (`shot-build.log`).
- `scripts/setup_shot_feedback.py`: `ER_FIRE_ANIMATION_VERIFIED` e
  `ER_SHOT_ASSETS_SAVED`. Campionata l'animazione a 0, 16,7, 50 e 100 ms:
  spostamenti 0 / 1,4115 / 0,5646 / 0 cm; errore dello span tra le due
  impugnature 0 cm in tutti i campioni. Ritorno alla posa originale verificato.
- `python3 scripts/test-network.py --shot-feedback`: **25 controlli superati**,
  due processi Unreal reali host/client con nullrhi e nosound. I 19 controlli
  gameplay precedenti restano superati; quattro verifiche nuove riguardano
  un solo effetto per Start/Stop rapidi, 30 effetti per un caricatore intero,
  ControlRotation server invariata e nessun nuovo effetto a caricatore vuoto.
  Due verifiche aggiuntive confermano asset caricati e prospettive locale/remota
  su entrambi i processi: 44 eventi osservati per processo.
- Avvio Metal con audio attivo e bersaglio fermo: riuscito. Ricarica con R e
  click sinistro tenuto 750 ms consumano otto colpi (30/60→22/60), un'altra
  raffica da 1,1 s consuma undici colpi (22/60→11/60). Eventi registrati
  come `animation=1 sound=1 flash=1` in `shot-play.log`; posa dopo lo sparo
  osservata con mano sinistra ancora allineata al fucile.

Correzioni emerse durante il lavoro: la prima copia di pose valutate applicava
il retargeting due volte e spostava le mani di circa 6 cm. Disabilitare il
retargeting durante la copia ha portato lo scostamento campionato a zero.
La prima prova Metal ha rilevato un pin non collegato del nodo OneMinus nel
materiale del lampo; corretto il pin e aggiunti controlli espliciti sul risultato
di ogni collegamento nello script. Gli asset originali del template restano integri.
Sessione Metal riaperta dopo la correzione: nessun fallback o errore del
materiale in `shot-play-final.log`. Deposito con bersaglio fermo, HP 100 e
30/90 verificato graficamente; sessione lasciata aperta senza flag diagnostici.

Limiti: la suite senza rendering verifica eventi e riferimenti, non l'ascolto
del suono o la leggibilità del lampo di 35 ms. La qualità percepita di audio,
rinculo e animazione resta da valutare nel gioco. Il campione audio è quello
provvisorio del template; niente audio di ricarica o nuovi effetti di impatto.
La perdita di pacchetti può omettere eventi cosmetici inaffidabili e non è
stata simulata in questa suite. Nessuna nuova dichiarazione sulle prove T21.

## Corona munizioni e dispersione — 13 ottobre 2026

- Compilazione Editor: **Succeeded** (`accuracy-build.log`).
- `python3 scripts/test-network.py --accuracy`: **15 controlli superati**,
  dodici server e tre client. Verificati cono di base, crouch, camminata,
  corsa, salto, dieci proiettili realmente lanciati con velocità nel cono
  server e direzioni differenti, limite di accumulo, raffica via RPC del
  client, recupero graduale e completo, nessun accumulo su sparo respinto.
  Il client verifica dati di base, munizioni/accumulo replicati e recupero
  con il tempo sincronizzato. Log `accuracy-{host,client}.log` e `accuracy-tests.log`.
- Regressione `test-network.py --shot-feedback`: **25 controlli superati**
  con la dispersione reale attiva (`accuracy-network-regression.log`).
- Regressione bot: **28 controlli superati**, Patrol e Target, danni,
  consumo munizioni, morte/respawn, fine round e replica remota
  (`accuracy-bot-regression.log`). Totale di queste tre suite: **68**.
- Audit HUD durante la sessione Metal: 2345 campioni, tutti con numero di
  tacche illuminate uguale alle munizioni. Osservati tutti i valori da 0 a
  30; cono da 0,108° a 0,93° e raggio da 27,2 a 36,4 px nel viewport di
  prova. Questi campioni sono osservazioni della sessione grafica, non input
  generati dalla sonda senza rendering (`accuracy-play.log`).
- Verifica grafica con CUA nella sessione finale: corona completa da 30,
  raffica reale di 750 ms porta a 22/90 e 22 tacche illuminate; raggio
  maggiore dopo la raffica. R attiva corona ambra e lettera R, senza
  riempire anticipatamente le tacche. Posa delle mani conservata.
  Dopo il completamento della ricarica, corona piena e 30/81 osservati.
  Sessione finale avviata con Metal, bersaglio fermo e senza diagnostica
  (`accuracy-play-final.log`).

Limiti: prime prove della taratura; non è ancora un bilanciamento competitivo.
La corona ha un margine fisso per rendere leggibili le tacche: segue la
dispersione effettiva ma non marca il perimetro esatto del cono sullo schermo.
Latenza/perdita artificiale non sono state simulate in questo passaggio.

## Pallini del mirino e antialiasing — 13 ottobre 2026

Richiesta di ridurre la visibilità: diametro 3 px a 720p, nessun bordo,
bianco con alpha 0,70 per i colpi rimasti e nero con alpha 0,10 per i posti
vuoti. Aggiunta sfumatura trasparente sui bordi. Nel progetto l'AA era
disattivato (`r.AntiAliasingMethod=0`); impostato TSR (`=4`) e abilitato High
DPI per il game mode Mac. Raggio, dispersione, munizioni e logica di ricarica
invariati. Compilazione Editor finale **Succeeded** (`reticle-aa-build.log`;
ERPlayerController compilato e modulo collegato).

La prima prova grafica ha rilevato un'asserzione Canvas sui triangoli senza
texture. Corretto il disegno con `GWhiteTexture`, risorsa interna del motore,
e dipendenza privata `RenderCore`. Nessuna texture esterna o asset nuovo.
Non sono ripetute le suite gameplay: modifica esclusivamente grafica.
Prova Metal finale riuscita (`reticle-aa-play-final.log`): applicata la CVar
`r.AntiAliasingMethod=4`; caricatore vuoto mostra i pallini neri al 10%, R
ricarica il fucile e mostra di nuovo i 30 pallini bianchi al 70%. Entrambi gli
stati osservati a schermo con bordi morbidi. La scena, il fucile e il testo HUD
sono stati verificati nella sessione Retina.

## Impatti sulle pareti — 13 ottobre 2026

Compilazione Editor **Succeeded** (`bullet-impact-build.log`). Il commandlet
Unreal ha creato `M_ER_BulletHole` e lo ha assegnato ai default di
`BP_ERProjectile` (`bullet-impact-assets.log`). Il materiale procedurale usa
un cratere scuro, un bordo irregolare e nove crepe radiali; nessuna texture è
stata importata. La sessione Metal (`bullet-impact-play-final.log`) ha caricato
la mappa e mostrato i primi segni sugli ostacoli colpiti. Lifetime del decal:
20 secondi, con dissolvenza negli ultimi 5. L'effetto viene replicato per gli
impatti su superfici non appartenenti a Pawn.

## Diagnosi riavvio — 13 ottobre 2026

Lo stesso `scripts/play-local.command`, eseguito fuori dal sandbox dei comandi
Codex, ha caricato `L_MainMenu` e completato l'inizializzazione Metal in 13,01 s
(`restart-diagnostic.log`). TSR, High DPI e tutte le CVar DynamicRes sono rimaste
attive; nessuna modifica al progetto è stata necessaria per questo riavvio.
Il processo è rimasto in esecuzione. I precedenti SIGABRT avvenivano durante
la registrazione Cocoa dell'app nei tentativi di lancio ristretti, prima
dell'inizializzazione del gioco. Gli errori `Viewport Size Mismatch` citati
in precedenza appartenevano invece al browser Editor avviato senza progetto;
non dimostrano la causa del rallentamento fullscreen di Respawn & Rage.
Limite: CUA ha selezionato il vecchio browser Editor e, dopo la sua chiusura,
ha restituito `noWindowsAvailable`; il nuovo riavvio è verificato dai log e
dal processo attivo, senza nuova verifica visiva del menu o della fluidità.

## Menu dimensione finestra — 13 ottobre 2026

Compilazione Editor **Succeeded** (`window-menu-build.log`). Avvio Metal tramite
Launch Services fuori dal sandbox riuscito (`window-menu-play.log`), seguito da
partita L_Depot con bersaglio fermo. Durante l'interazione della sessione, il log
ha registrato applicazioni dal menu al 60% (3072×1728) e 80% (4096×2304), entrambe
in finestra sul monitor 5120×2880; il processo è rimasto attivo. Il controllo CUA
ha identificato la finestra Respawn & Rage ma la cattura successiva ha restituito
`noWindowsAvailable`: nessuna nuova dichiarazione di verifica visiva o misurazione
FPS. Il passaggio al 90% e allo schermo intero resta da provare nella sessione.
Nessuna modifica a gameplay, replica o asset; nessuna suite di rete ripetuta.

## Rendering adattivo Retina — 13 ottobre 2026

Compilazione Editor **Succeeded** (`adaptive-render-build.log`); controllo diff e
sintassi zsh dei tre launcher superati. Prova Metal reale nel deposito con un
bersaglio fermo e `-ERPerformanceAudit` (`adaptive-render-play.log`): DynamicRes
risulta abilitato, viewport iniziale 2560×1440, scala secondaria 75%, uscita massima
1920×1080 e risoluzione primaria variabile (osservati circa 1041×586–1497×842
nel periodo campionato). Campioni stabilizzati per lo più 59–62 FPS, con cali
transitori durante interazione/cattura; nessuna garanzia di minimo 60 FPS.
CUA ha osservato overlay e menu Esc, con 60 FPS / 16,7 ms e risoluzione 3D auto.
Passaggio al fullscreen nativo con il pulsante macOS verificato dalla scomparsa
dei controlli finestra e dal viewport registrato a 5120×2880: scala secondaria
37,5%, uscita 3D massima invariata, campioni consecutivi a circa 60 FPS / 16,7 ms
con risoluzione interna intorno a 1335×751. Non è un benchmark di combattimento
né una verifica di tutti i monitor o di tutte le misure intermedie del menu.
Nel sorgente UE 5.8 `MetalViewport.cpp` la presentazione finestrata forza
`displaySyncEnabled` anche con `r.VSync=0`; il cap del launcher 90 non garantisce
quindi oltre 60 FPS su uno schermo 60 Hz. Il fullscreen nativo di questa prova
ha anch'esso mostrato circa 60, non una misura superiore. Nessuna suite gameplay
ripetuta: modifiche solo alla scala di rendering, AA e diagnostica locale.

## Reset risoluzione automatica — 13 ottobre 2026

Compilazione Editor finale **Succeeded** (`adaptive-reset-build.log`) dopo la
correzione del controllo sul puntatore IDynamicResolutionState. Diff senza
errori di whitespace. Riavvio Metal con bersaglio fermo riuscito e nuove CVar
caricate (`adaptive-reset-play.log`). Prova CUA nel menu Esc: pulsante visibile,
raggiunto da tastiera e attivato; `ER_RENDER_RESET` registrato alle 10:03:09 UTC.
Viewport rimasto 2560×1440 e stessa partita. Dopo il reset, campioni circa
59–60 FPS con scala auto: risoluzione stimata da 1032×581 a 1242×699, in recupero
graduale nell'intervallo osservato. Nessuna garanzia di minimo FPS né nuova
prova di rete; il reset riguarda solo la cronologia locale del renderer.

## Slider sensibilità mouse — 13 ottobre 2026

Compilazione Editor **Succeeded** (`mouse-slider-build.log`), diff senza errori
di whitespace. Avvio Metal con bersaglio fermo completato in 10,55 s
(`mouse-slider-play.log`). Prova CUA nel menu Esc: slider visibile centrato,
etichetta “Sensibilità mouse · 50%”; regolazione da tastiera al 51% e chiusura
della cattura hanno registrato `ER_MOUSE_SENSITIVITY slider=51% multiplier=0.914`.
Ripristino al 50% verificato nel log con moltiplicatore 0,889 e partita ripresa.
Nessuna nuova suite di rete: modifica alla scala locale della mira e alla UI;
nessuna misura di cm/360 o prova visiva della sensibilità su altri mouse.

## Bersaglio mercenario — 13 ottobre 2026

Build Editor **Succeeded** (`mercenary-build.log`), compilazione Python dello
script e `git diff --check` superati. Generazione asset riuscita
(`mercenary-assets.log`): gerarchia condivisa di 87 ossa verificata, tre LOD
24.068 / 6.915 / 3.793 vertici e 16 slot materiali. Blueprint compilato e
salvato con mesh propria, socket del fucile e scala 0,9201.

`scripts/test-bots.py` **PASS**, 28 controlli complessivi nei modi Patrol e
Target, inclusi danno/proiettile/munizioni, immobilità del bersaglio, morte,
punteggio, reload, respawn protetto, fine/riavvio round e replica al client.
Log `Saved/Testing/bots-target-host.log` conferma BP_ERTrainingTarget sia alla
prima creazione sia al respawn e al riavvio round; dettagli della suite in
`mercenary-bot-tests.log` e `Saved/Testing/bots-*.log`.

Avvio del gioco Metal riuscito (`mercenary-play.log`). Osservazione CUA:
mercenario con materiali corretti, posa col fucile a due mani e piedi a terra;
braccia FPS del giocatore invariate. Overlay osservato a 60 FPS / 16,7 ms e
risoluzione automatica 1374×773. Campioni per lo più 59–60 FPS, con un calo
transitorio durante l'avvio parallelo dei test e successivo recupero. Non è
un benchmark con molti personaggi né una garanzia di minimo 60 FPS; le altre
animazioni e tutte le combinazioni modulari non sono state verificate.

### Due bersagli e colori di squadra

Build Editor finale **Succeeded** (`mercenary-teams-build.log`), script asset
completato (`mercenary-teams-assets.log`), py_compile, sintassi zsh del launcher
e diff whitespace superati. Suite bot ripetuta: **28 PASS** Patrol/Target e
client remoto (`mercenary-teams-bot-tests.log`). Il client registra la selezione
dei 16 slot della livrea Omega anche dopo respawn e nuovo round.

Prova Metal con ERBots=2 avviata (`mercenary-teams-play.log`): due join, uno
Omega e uno Alpha, e applicazione di entrambe le livree a 16 slot verificata
nel log. CUA ha osservato la partita aperta; l'ispezione ravvicinata di entrambe
le livree non è stata completata, poiché la visuale veniva spostata durante
la prova. Nessun benchmark aggiuntivo o verifica rete a quattro giocatori.

## Mercenari per i bot mobili — 13 ottobre 2026

Build Editor **Succeeded** (`mercenary-patrol-build.log`), py_compile degli
script e diff whitespace superati. Suite `test-bots.py` **PASS** nei due modi:
28 controlli gameplay/rete e nuove verifiche della classe mercenario sul server
e della livrea Omega a 16 slot sul server/client (`mercenary-patrol-bot-tests.log`).
La pattuglia percorre 481,9 cm nella finestra del probe; sparo con danno reale,
reload, morte, respawn e nuovo round superati. Le copie dei materiali sono
riapplicate sul client dopo respawn e nuovo round.

Prova Metal aperta con due bot Patrol (`mercenary-patrol-play.log`): entrambi
usano BP_ERTrainingTarget e ricevono la livrea della squadra assegnata, Omega
in questa sessione. Nessun errore di compatibilità animazione/skeleton trovato.
CUA ha osservato la partita e il combattimento in corso, ma i bot non erano
inquadrati nelle catture: qualità visiva della camminata e delle mani in
movimento non certificata da questa osservazione. Nessun nuovo benchmark FPS.

## Risposta mouse progressiva — 13 ottobre 2026

Build Editor finale **Succeeded** (`mouse-response-build.log`), diff whitespace
superato. Test Unreal `Respawn & Rage.Input.MouseResponse` **Success**
(`mouse-response-tests.log`): guadagni lento/rapido 0,75/1,8, curva monotona
e limitata su velocità 0–4000 counts/s, stesso guadagno a 30/45/60/90/144 FPS,
stesso risultato per direzioni di pari modulo, delta nullo senza movimento,
slider 0% lineare e tempo nullo gestito.

Avvio Metal con bersagli fermi riuscito (`mouse-response-play.log`). CUA ha
osservato il menu Esc con Sensibilità e Accelerazione entrambe al 50%, testo
esplicativo visibile e successiva partita ripresa. Diagnostica di input reale:
correzione (0,126; 0,025) con gain 0,750; movimento (4,750; -0,227) con gain
1,750; successivo delta piccolo subito tornato a gain 0,750. La sensazione
ergonomica e la taratura per DPI/device restano da valutare dall'utente.
Regolazione effettiva/persistenza dello slider a un valore diverso non
verificate in questa prova. Nessuna suite rete ripetuta: varia solo il
trattamento locale dell'input, la normale replica della mira resta invariata.

## Mercenario nella vista FPS — 13 ottobre 2026

Build Editor finale **Succeeded** (`mercenary-fps-build.log`). Script asset
finale completato (`mercenary-fps-assets.log`), py_compile e diff whitespace
superati. Il primo tentativo con la posa mannequin evidenziava circa 9 cm di
errore della presa sinistra; la generazione finale usa pose dedicate allo
skeleton proprio, evitando il doppio retargeting. Misura finale nello spazio
dell'impugnatura destra: presa sinistra (0; 28,522; 3,003) cm, errore <0,0001 cm
alla precisione registrata. Campioni sparo a 0/16,7/50/100 ms: span invariato,
recoil massimo 1,4115 cm e ritorno alla posa iniziale.

`test-network.py --shot-feedback` **PASS**, 25 verifiche compreso feedback host
e client in entrambe le prospettive (44 eventi ciascuno), danno/munizioni,
reload, morte/respawn e round. Dettagli `mercenary-fps-network-tests.log` e
`Saved/Testing/network-*.log`. Materiali FPS Alpha/Omega a 16 slot e quattro
sezioni visibili registrati sul client anche dopo respawn.

Avvio Metal completato (`mercenary-fps-play.log`). CUA ha osservato braccia in
pelle del mercenario, guanto blu Alpha, dita sulla presa del fucile e nessun
torso/testa FPS visibile. Spari locali reali registrano animazione/suono/lampo
validi; overlay osservato 60 FPS / 16,7 ms con 3D auto 1332×749. Questa singola
cattura non certifica ogni angolo della camera, la resa Omega o l'animazione di
arrampicata, e non è un benchmark. Il gioco resta aperto per la prova utente.

## Rifinitura delle proporzioni FPS — 13 ottobre 2026

Screenshot utente confrontato con le pose conservate. Ispezione Unreal delle
dita (`fps-hands-inspect.log`): index_01_l a 10,54 cm dal polso nel mercenario
contro 6,45 cm nella posa Manny. Rigenerazione finale completata senza errori
(`mercenary-fps-refine-assets.log`), py_compile e diff whitespace superati.
Mani 0,72, braccia con scala trasversale 0,6, orientamento impugnature Manny e
nuovo polo del gomito. Verifica fisica delle prese nel riferimento rigido
del fucile: (0;28,522;3,003) cm con errore <0,0001 alla precisione registrata;
span invariato e recoil 1,4115 cm, ritorno idle a 100 ms.

Prova Metal finale aperta (`mercenary-fps-refine-play.log`). CUA ha verificato
che la mano non si estende più lungo il lato dell'arma e che l'avambraccio
compare sotto il fucile con volume ridotto rispetto alla prima prova. Fucile
a scala assoluta 1; corpo/world dei bot invariati. Non è un confronto pixel
per pixel con una nuova cattura del mannequin né una garanzia estetica su ogni
angolo. Nessuna compilazione C++ o suite rete ripetuta: cambiati solo pose FPS
generate e default del componente arma, con controlli delle impugnature.

## Mannequin FPS ripristinato per riferimento — 13 ottobre 2026

Script Unreal completato senza errori (`mannequin-fps-restore-assets.log`):
BP_ERCharacter e BP_ERTrainingTarget tornano a SKM_Manny_Simple, posa/sparo
precedenti, offset e materiali originali; filtri sezioni e palette FPS rimossi.
py_compile e diff whitespace superati. Avvio Metal riuscito
(`mannequin-fps-reference-play.log`); CUA conferma avambraccio bianco del
mannequin e mano guantata sotto il fucile nella vista precedente. Bot world
conservati. Gioco lasciato aperto per lo screenshot dell'utente; nessuna nuova
compilazione C++ o suite gameplay richiesta per questo ripristino artistico.

Controllo successivo su richiesta dell'utente (`mannequin-fps-audit.log`):
entrambi i Blueprint salvati hanno mesh Manny, posa/sparo precedenti, offset
(-10,0,-152), yaw -90, scala componente e ossa delle braccia/mani a 1, nessun
override materiale/palette/filtro FPS. Arma trasformazione locale neutra e
absolute_scale false; camera FOV 90, posizione (0,0,77,5), scala 1. Audit
**PASS**. `git status` conferma mesh/skeleton mannequin, mesh rifle e asset
A_ER_Rifle_HipPose identici alla versione tracciata precedente al mercenario.
I valori corrispondono alla vista personalizzata del 12 ottobre documentata
in DEVELOPMENT.md; nessuna prova di equivalenza con un diverso screenshot
storico o con la posa ADS originale del template.

## Mercenario FPS allineato al riferimento utente — 13 ottobre 2026

Build Editor **Succeeded** (`fps-reference-build.log`) per la diagnostica
ERFPSAudit; generazione asset completata (`mercenary-fps-reference-assets.log`),
py_compile e diff whitespace superati. Test delle prese nei quattro campioni
di sparo: span invariato, recoil 1,4115 cm e ritorno idle a 100 ms.

Confronto Metal reale tra mannequin e mercenario, non solo asset in memoria:
rifle camera Manny (13,8795;11,6230;-24,0359) cm
(`fps-reference-runtime.log`), mercenario (13,8802;11,6230;-24,0560) cm
(`mercenary-fps-reference-play.log`). Scarto posizione circa 0,02 cm, scala
arma 1. Gomito mercenario camera (17,1027;0,9956;-24,3697): vicino all'asse
centrale e sotto il bordo inferiore; slot UpperArm escluso dal rendering FPS.
Presa sinistra circa 28,53 cm avanti alla destra, coerente con il foregrip.

CUA ha osservato la mano/guanto blu sotto il fucile, avambraccio che entra dal
centro inferiore, meno pelle visibile del riferimento Manny e bicipite assente.
Gioco lasciato aperto. Non verificati tutti gli angoli, palette Omega o high
climb in questa sessione; nessuna suite rete ripetuta, poiché sono cambiati
solo posa/default FPS e una diagnostica opt-in senza effetto sul gameplay.

## Danni direzionali e corpi persistenti — 13 ottobre 2026

Build Editor finale **Succeeded** (`damage-death-build.log`), asset configurati
con PA_MER_ME originale (`damage-death-assets.log`), py_compile e diff whitespace
superati. SoundWave proprio mono 44,1 kHz / 160 ms importato e attenuation
spaziale salvata; nessuna valutazione uditiva certificata dai test headless.

Suite listen-server/client **PASS**, 30 verifiche complessive
(`damage-death-network-tests.log`, `Saved/Testing/network-*.log`). Nuove prove:
ragdoll creato, corpo valido dopo il respawn, pelvis caduto a terra; client
riceve feedback con direzione/suono e corpo replicato assestato. Le posizioni
pelvis osservate sono intorno a 20–24 cm dal suolo; server e client differiscono
leggermente come previsto dalla simulazione cosmetica locale. Protezione,
danno reale, ricarica, 30 colpi, feedback sparo, morte, respawn e round passano.

La prima esecuzione con partita grafica parallela ha fallito tre controlli
temporizzati della raffica durante un calo del frame rate del server
(`damage-death-loaded-host.log` / client). Ottimizzati corpi con pausa animazione,
LOD medio e tick sospeso dopo assestamento; seconda esecuzione senza carico
grafico parallelo superata, mantenendo le stesse scadenze e regole di sparo.
Le due differenze non consentono di attribuire il miglioramento a una sola causa.

Avvio Metal finale riuscito (`damage-death-play.log`) con un bot Patrol per la
prova utente. La sessione grafica precedente ha registrato colpi diagonali,
feedback audio configurato, morti e corpi assestati; CUA ha osservato il menu
della partita. Non certificati aspetto della caduta a tutti gli angoli, qualità
percepita del suono, dodici corpi contemporanei o equivalenza delle pose ragdoll
tra peer. Nessuna garanzia FPS aggiuntiva; il gioco resta aperto.

## Arma contro le pareti (13 ottobre)

Build Editor finale riuscita (`viewmodel-wall-build.log`, 10.87 s).
Prova grafica Metal con `-ERViewmodelWallTest` nella mappa L_Depot: giocatore
a 36 cm dalla parete più vicina, scala renderizzata 0.20; braccia, fucile e
fiammata tutti classificati FirstPerson. Log `viewmodel-wall-play.log`:
`ER_VIEWMODEL_WALL_TEST clearance=23.04 scale=0.20 body=1 rifle=1 flash=1 result=PASS`.
Schermata `Saved/Screenshots/MacEditor/ScreenShot00000.png` acquisita tramite
Unreal Shot e ispezionata: fucile intero, mirino anteriore e volata visibili
contro la parete. Nessuna modifica ad asset, rete o collisioni fisiche;
`git diff --check` superato. Non verificati tutti gli angoli, le pose di fuoco
o la vicinanza simultanea a pavimento/soffitto. Partita normale riaperta senza
parametro di test per la prova utente.

## Arsenale Low Poly (13 ottobre)

Build Editor finale riuscita (`arsenal-build.log`, 10,23 s). Importazione
Unreal di 16 FBX e T_Gun.TGA completata (`low-poly-import.log`), con 47 asset
nel percorso Weapons/LowPoly e 22 pose proprie. Dati/Blueprint salvati, marker
`ER_ARSENAL_READY primaries=7 sidearm=1 utilities=3 project_local=1` nel log
`arsenal-assets.log`. Sorgenti copiati nel progetto, SHA-256 in source.json.

`scripts/test-arsenal.py`: 16 controlli PASS, zero FAIL, host/client Unreal
reali con nullrhi/nosound. Verificati cinque slot, undici profili con mesh e
pose, selezione tramite RPC, semiautomatico che consuma un colpo quando tenuto
premuto, munizioni della principale conservate, ricarica pistola, slot invalido
respinto, scelta differita al respawn, M249 100+200 dopo rientro, sei proiettili
per una cartuccia shotgun, razzo a 2200 cm/s, danno HE effettivo al lanciatore,
Stun senza danno HP, fumo presente per 12 s, mesh M249 sul client e flash locale
remoto, detonazioni HE/smoke/stun dopo miccia. Log `arsenal-tests.log` e
Saved/Testing/arsenal-host.log / arsenal-client.log. Ripetuto dopo le ultime
correzioni del danno esplosivo con direzione basata sul punto di esplosione.

Suite precedente del combattimento `test-network.py --shot-feedback`: 30
controlli PASS (`arsenal-network-tests.log`), inclusi munizioni, protezione,
ricarica, raffica, ostacoli, friendly fire, morte, corpi persistenti, respawn,
fine round e feedback locale/remoto. Eseguita dopo l'integrazione del loadout,
prima delle ultime rifiniture esclusivamente visive e del nuovo point damage
esplosivo, quest'ultimo coperto dalla ripetizione della suite arsenale.

La prima galleria Metal ha rivelato WorldGrid e armi corte fuori inquadratura.
Corretto override del template e assegnazione persistente: la proprietà legacy
Materials di SkeletalMesh è transitoria in UE 5.8, Python non espone SetMaterials;
aggiunto helper editor ristretto a LowPoly. `arsenal-material-inspection.log`
conferma in un processo distinto texture T_Gun e M_LowPolyGun su AK74, M1911 e
RGD5. Corrette posa/proporzioni delle armi corte e presa della pistola.

Galleria finale via Unreal Shot in Saved/Screenshots/Arsenal/Weapon_00..10.png.
Verifiche visive statiche con rendering Metal; caricamento iniziale shader
richiede una breve attesa. Nessuna certificazione di audio ascoltato, animazioni
di ricarica/lancio dedicate, tutti gli angoli di contatto, bilanciamento,
prestazioni con quattro giocatori o occlusione esplosiva in ogni geometria.
Fumo cosmetico non interrompe la vista dei bot. Diff e sintassi Python verificati.

Ispezionati tutti gli undici riquadri della galleria definitiva (contact-sheet.png):
palette visibile, otto armi e tre granate presenti, riserve corrette, M1911 sopra
la mano e armi corte dentro l'inquadratura. Ottiche M4/M107 presenti; dettaglio
di dita, lunghezza dell'avambraccio e animazioni restano pose di prototipo.

Sessione di verifica chiusa; avvio normale nel menu L_MainMenu, senza flag di
probe/galleria, per scegliere la principale e avviare allenamento o LAN.
Log di avvio: arsenal-normal-play.log.

## Filtro rotella (13 ottobre)

Build Editor riuscita (`wheel-build.log`, 25,30 s). Suite arsenale a due peer
con 18 controlli PASS (`wheel-tests.log`): al test precedente sono aggiunti
impulsi del client ogni 50 ms per 400 ms, che cambiano un solo slot, e un
nuovo gesto opposto dopo la quiete, che torna di un solo slot. Il test chiama
lo stesso handler della rotella e verifica lo slot sul server; non riproduce
un dispositivo fisico. Tasti numerici continuano a usare SelectSlot senza
filtro temporale. Nessun asset modificato; `git diff --check` superato.

## Granata HE / Bomb Crack Ground (13 ottobre)

Conversione Blender 3.6.5 riuscita: 1.416 mesh originali, 128 schegge volanti
selezionate, 1.693 vertici / 2.874 triangoli, 92 frame a 30 Hz. Esclusi suolo
fermo e lastre basse. Importazione tramite Unreal riuscita, animazione 3,033 s
eseguita a velocità 1,5 per 2,022 s. `grenade-asset-check.log` verifica in un
processo distinto 130 ossa (128 frammenti più radici) e i due materiali salvati.
Lettura runtime indipendente da Downloads; FBX e ZIP originali conservati.

Build Editor finale riuscita (`grenade-build.log`, 13,51 s). Suite arsenale a
due peer con 21 controlli PASS (`grenade-tests.log`): danno HE autorevole,
fuse di HE/fumo/stun, effetto animato presente su host e client, attore locale
senza collisione che sopravvive al proiettile, rimozione dopo l'animazione,
oltre alle verifiche esistenti di armi, ammo, slot e rotella.

Galleria Metal in `Saved/Screenshots/Grenade/Burst_01..05.png`: frammenti
visibili e in moto, nessuna lastra di pavimento, ritorno alla scena pulita.
La prima prova compila tre shader; le catture PNG provocano anche pause
sincrone e NON misurano il costo ordinario dell'effetto. Nessun errore shader
o materiale nei log di importazione/rendering.

Prova separata senza catture (`grenade-performance.log`, 1280x720, Metal,
risoluzione interna dinamica, t.MaxFPS 90, una detonazione): 179 fotogrammi,
media 11,53 ms, massimo 84,43 ms, un frame oltre 33,34 ms alla prima comparsa;
HUD circa 68 FPS subito dopo e 89–90 FPS durante la continuazione. La misura
comprende la fuse e il termine della sequenza; non certifica otto esplosioni
simultanee, quattro giocatori, altri Mac o assenza di scatti di prima inizializzazione.
Il precache PSO è richiesto in anticipo, ma non elimina quel primo picco su
questa configurazione. Nessuna verifica di nuovo audio: l'archivio non ne contiene.

Sintassi dei tre script Python e `git diff --check` superati. Nessuna mappa o
geometria modificata; tutti gli uasset creati/modificati tramite Unreal.
Suite a due peer ripetuta dopo il precaricamento/default finali: ancora 21
PASS e zero FAIL. Avvio normale da L_MainMenu in `grenade-normal-play.log`,
senza flag di galleria o misurazione. HE selezionabile col tasto 3.

## Fumogena / Bullet Interactive Smoke (13 ottobre)

Progetto sorgente 5.3 migrato con AssetTools di Unreal in Respawn & Rage 5.8.3.
NiagaraFluids presente nell'installazione Mac e attivato; undici package
necessari importati, senza demo/maps. Asset propri creati con Unreal:
NS_ER_InteractiveSmoke, BP_ERSmokeCloud, default aggiornato su BP_ERProjectile.
`smoke-setup.log` registra l'Engine 5.8.3 e i parametri, sorgente archiviata
con SHA-256 in SourceAssets/InteractiveSmoke. Nessuna patch binaria degli asset.

Build finale riuscita (`smoke-build.log`, 16,22 s, nessun warning/error).
Suite a due peer con 25 PASS e zero FAIL (`smoke-tests.log`): nube replicata
su host/client, collisione disabilitata, rimozione dopo 12 s, posizioni di
carving da un colpo del server e ricezione sul client remoto, oltre alle
verifiche già presenti per HE/stun, armi, ammo, selezione e rotella. Il client
registra ER_SMOKE_REMOTE_CUT events=1. La disconnessione conclusiva del client
è attesa: il test chiude esplicitamente l'host. Nullrhi non verifica la densità GPU.

Galleria Metal salvata in Saved/Screenshots/Smoke/Burst_01..06.png. La prima
prova aveva solo sprite: sg.ShadowQuality=1 disabilita r.VolumetricFog e la
mappa non contiene ExponentialHeightFog. Corretto con supporto temporaneo a
runtime, senza modificare la mappa. Galleria successiva
`smoke-gallery-volume.log`: nube volumetrica densa visibile, movimento/puff,
interazione e rimozione finale; nessun errore materiale, shader o Niagara.
La compilazione iniziale comprende oltre cento shader e la cattura PNG
provoca pause: quegli FPS non sono una misura ordinaria dell'effetto.

Prima misura senza PNG: 766 frame, media 13,88 ms, massimo 715,37 ms alla
prima attivazione. Corretto con preriscaldamento locale senza densità durante
il caricamento del primo loadout. Misura finale
`smoke-performance-prewarm.log`: 818 frame, media 12,57 ms (~79,5 FPS),
massimo 20,14 ms, zero frame oltre 33,34 ms. Una nube a 1280x720, Metal,
t.MaxFPS 90, risoluzione interna dinamica circa 1056–1202 px durante la nube;
HUD circa 76–82 FPS e ritorno a 90 dopo la rimozione. Il picco iniziale ora
avviene nel caricamento; non certificata la fluidità con quattro nubi/giocatori,
altri Mac, né il dettaglio della densità identico fra peer o un late join.

Sintassi Python e `git diff --check` superati. Nessuna nuova registrazione audio.
Il fumo non interrompe la visibilità dei bot; la reazione delle celle è locale
e cosmetica, mentre durata/posizione della nube e colpi restano autorevoli.
Gioco riaperto nel menu normale (`smoke-normal-play.log`), senza flag delle
prove; fumogena nello slot 4 e lancio con click sinistro.

### 13 ottobre — Twinblast Alpha / Drongo Omega

- Build Editor Mac Development: **Succeeded**, ultimo log
  `Saved/Testing/team-appearance-build-final.log` (8,19 s).
- Verifica in un processo Unreal nuovo: **6/6 PASS** per riferimenti mesh,
  animazioni, asset fisici, socket, numero dei materiali, parti nascoste e
  shader non emissivi nei default di player, bot e cadavere. Log
  `team-appearance-saved-verify-final.log`, ER_TEAM_SAVED_COMPLETE.
- `python3 scripts/test-network.py --shot-feedback`: **PASS**. Due processi
  reali, listen server/client. Mesh, classe animazione, materiali e parti
  nascoste di entrambe le squadre verificati anche dal client (fase 2, con
  entrambe le squadre presenti). Cadavere Omega corrispondente al vivo,
  aspetto conservato al respawn; cadaveri Alpha e Omega replicati. Test
  precedenti di danno, munizioni, protezione, friendly fire, round e feedback
  dei colpi superati. Log `team-appearance-network-summary.log`,
  `network-host.log`, `network-client.log`.
- `python3 scripts/test-bots.py`: **PASS** con gli asset definitivi, Patrol e
  Target. Classi e personaggi su host/client, parti nascoste Omega, movimento,
  sparo reale, copertura, ricarica, morte, scoring e nuova vita protetta.
  Log `team-appearance-bots-final-summary.log` e `bots-{patrol,target}-*.log`.
- Prova grafica reale Metal, 1600x1000, sg.ShadowQuality=1: **PASS** per
  caricamento e compilazione dei materiali attivi. Nessun fallback "Failed
  to compile Material" nel log finale. Ispezionata
  `Saved/Testing/Team-Characters.png`: volto umano/vestiario oliva Alpha;
  Omega scuro, tubi sopra la testa e decorazioni flottanti rimossi. Log
  `team-appearance-gallery-final.log`, ER_TEAM_GALLERY_COMPLETE.
- Rimossi attraverso Unreal 25 materiali di prova propri, dopo controllo
  delle referenze effettive. Nessun asset Paragon eliminato; log
  `team-appearance-cleanup.log`.
- Sintassi dei quattro script Python modificati/aggiunti: **PASS**.

Le prime prove hanno rilevato Blueprint retargettati non salvati, ossa q_weapon
assenti in ActionHero e un input Desaturation senza nome. Tutti corretti prima
delle verifiche finali. I test nullrhi/nosound certificano stato e replica,
non la qualità grafica/audio. La gallery Metal verifica la posa ADS ferma;
non certifica ogni animazione, ogni arma, il late join o il costo con quattro
giocatori. Realismo parziale: geometria delle protesi Twinblast e proporzioni
Drongo originali; braccia FPS ancora mercenario con palette aggiornata.

### 13 ottobre — correzione ragdoll dopo screenshot utente

Difetto riprodotto in Metal: armi/accessori Paragon riapparivano e corpi
incompleti producevano deformazioni. La verifica precedente di stato/flag non
certificava il ragdoll visibile. Fisica propria a 16 corpi/15 articolazioni,
profilo dei vincoli salvato esplicitamente, LOD0 e posa valutata fino allo stop;
ossa nascoste nuovamente collassate dopo il blend fisico.

- Build Editor Mac Development: **Succeeded**, `corpse-final-build.log`, 8,63 s.
- `setup_team_ragdolls.py`: **PASS**, entrambi gli asset propri persistenti e
  BP_ERCorpse con ERCorpseMeshComponent. `corpse-assets-final.log`.
- Processo Unreal nuovo: **6/6 PASS**, mesh/materiali/animazioni e riferimenti
  ai nuovi asset fisici su player, bot e cadavere.
  `corpse-saved-assets-verify.log`, ER_TEAM_SAVED_COMPLETE.
- Gallery reale Metal: **6/6 PASS**, Alpha e Omega a circa 2, 7 e 12 secondi
  dalla creazione. Verificati profili dei vincoli, 16 corpi/15 articolazioni,
  lunghezze di braccia/gambe e scala zero realmente pubblicata delle ossa
  nascoste, anche dopo lo stop. Ispezionati i PNG; nessuna arma fantascientifica
  riapparsa o estrusione lunga dello screenshot iniziale.
  `corpse-gallery-final-pass.log`, ER_CORPSE_GALLERY_COMPLETE.
  Immagini `Team-Corpses-{1,2,3}.png` in Saved/Testing.
- `test-network.py --shot-feedback`: **PASS**, listen server/client reali.
  Nuovi controlli sulla posa Omega durante la fisica e Alpha dopo lo stop;
  morte, replica dei cadaveri, sopravvivenza al respawn, caduta a terra,
  salute/munizioni, protezione, scoring, round e feedback superati.
  `corpse-network-final-pass-summary.log`, `network-{host,client}.log`.
- Sintassi dei tre script di generazione/verifica: **PASS**.

Diagnostica: foot_l/foot_r sono anche socket Paragon con offset 9–13 cm. I primi
controlli con GetSocketLocation segnalavano erroneamente variazioni della
lunghezza della tibia. Corretto il controllo con GetBoneTransform; la soluzione
finale conserva il blend standard del motore. Test nullrhi non certificano
la grafica; quella è stata verificata separatamente in Metal. Non misurato
il costo massimo con dodici cadaveri né tutte le possibili pose/impatti.

Pulizia finale: eliminati dai tre default Blueprint i gruppi canister_pos e
grenade_pos di Drongo, responsabili del piccolo contenitore fluttuante vicino
ai piedi nell'inquadratura dall'alto. Log `corpse-loose-props.log`.
Ripetuta la gallery Metal dopo la pulizia: **6/6 PASS** e
ER_CORPSE_GALLERY_COMPLETE in `corpse-gallery-clean-final.log`; ispezionato
`Team-Corpses-3.png`, contenitore rimosso e arti integri per entrambi i team.

### 14 ottobre — dieci skin selezionabili

- Build Editor Mac Development: **Succeeded**, `skins-final-build.log`, 9,84 s.
- Generazione finale con dettagli neutri: **PASS**, `skins-setup-neutral.log`,
  ER_SKINS_SETUP_COMPLETE. Dieci mesh/materiali/ragdoll propri e casco/zaino.
- Processo Unreal nuovo: **30/30 PASS**, tutte le skin nei default di player,
  bersaglio e cadavere; catalogo del controller valido. `skins-saved-final.log`.
- `test-team-skins.py`: **PASS**, ER_SKINS_RENDER_COMPLETE. Gallery Metal
  10/10 e ragdoll 30/30 (cinque coppie, due team, tre tempi ciascuno).
  Log `skins-render-summary.log`, `skins-corpses-{0..4}.log`; screenshot
  `Skin-Corpses-{indice}-{campione}.png`. Verificati rig 16/15, lunghezze degli
  arti, accessori nascosti e attrezzatura propria senza collisione.
- `test-network.py --skins --shot-feedback`: **PASS**,
  `skins-network-final-summary.log`. Preferenze non predefinite, indici RPC
  invalidi respinti, scelta pendente fino al respawn, vecchia skin conservata
  dal cadavere e nuova applicata alla nascita. Mesh/animazioni/parti nascoste
  replicate e regressione gameplay/feedback superate su host/client reali.
- Sintassi dei quattro script Python di setup/verifica/test: **PASS**.

La prima gallery ha evidenziato un personaggio parzialmente coperto dalla
parete della mappa e caschi troppo bassi; regolati inquadratura e attacchi.
La prima generazione lasciava alcuni materiali istanziati originali; aggiunta
la gestione del parent proprio e una palette distinta per stoffa e dettagli.
Su feedback visivo successivo, ridotti e ribassati i caschi di Pattuglia,
Supporto, Incursore e Miliziano; render finale **10/10 PASS** in
`skins-head-fit-gallery.log`, tavole `Skins-Alpha.png` e `Skins-Omega.png`.
Il primo avvio del comando di ispezione con path progetto relativo è fallito;
usati path assoluti. Un avvio durante il link del modulo non ha trovato fdm;
eseguito nuovamente dopo la build. Nessuno di questi tentativi è contato come
verifica superata. Il realismo resta limitato da proporzioni/protesi originali;
non misurati costo massimo di dodici corpi o ogni possibile animazione.

Su ulteriore feedback visivo, rimosso l'accessorio casco aggiunto alle skin
Alpha Pattuglia (3) e Supporto (5). Per Supporto sostituita anche la mesh
ShadowOps con TwinBlast a volto scoperto, mantenendo zaino e palette. Galleria
Metal aggiornata **10/10 PASS** in `skins-alpha-head-cleanup-gallery.log`;
verificata la tavola `Skins-Alpha.png`.


### 14 ottobre — avambracci FPS della skin e frame originale

- Editor Mac Development: **Succeeded**, `Saved/Testing/fps-forearms-build.log`,
  9,85 s per la build finale del codice.
- Generazione tramite Unreal: **PASS**, `fps-forearms-assets.log`,
  ER_FPS_FOREARMS_READY: dieci mesh proprie, riferimenti salvati su player e
  bersaglio, posa/offset FPS originali mantenuti.
- Gallery Metal con i materiali reali: **10/10 PASS** sul confronto di entrambe
  le mani, delle ossa lowerarm/hand e del fucile nel frame camera, tolleranza
  0,01. `fps-forearms-gallery.log`, ER_FPS_FOREARM_GALLERY_COMPLETE.
  Ispezionate tutte le dieci immagini FPS-Forearms-{Alpha,Omega}-{1..5}.png
  e la tavola FPS-Forearms-Contact.png; riferimento in FPS-Forearms-Reference.png.
  Viene mostrata soltanto la parte distale degli arti, nell'inquadratura
  originale dell'AK74, senza il torso o le estrusioni del tentativo precedente.
- Sintassi Python dei due script FPS e di setup_team_skins.py: **PASS**;
  `git diff --check`: **PASS**.

Diagnostica: il primo helper eliminava triangoli senza rimuovere i poligoni
importati, causando un assert dell'Editor; corretto con la rimozione dei
poligoni. Alcuni LOD Scavenger non hanno descrizione importata: ora usano il
LOD0 come sorgente per la riduzione. Le pose esistenti non pilotano i twist
Paragon come quelli del loro rig originale: le influenze sono riportate al
segmento lowerarm. I bounds della mesh ritagliata causavano culling del
viewmodel in posa FPS; mantenuti quelli del vecchio mercenario. Le immagini
con materiale neutro e le gallery precedenti non sono contate come verifica
visiva finale. Nell'avvio diretto della mappa è stato inoltre corretto
BeginPlay, che sovrascriveva l'arma già assegnata da InitializeLoadout con il
fallback Rifle A. La gallery finale usa effettivamente AK74 e la sua posa.

Verifica dei package in un processo Unreal nuovo: **10/10 PASS**, undici armi
per skin, 550 campioni idle/fire e due grip per campione. Posizioni, rotazioni
e scale confrontate con SK_ER_Mercenary_Target; frame del componente e bounds
conservati. `fps-forearms-saved-verify.log`, ER_FPS_FOREARMS_SAVED_COMPLETE.

`test-network.py --skins --shot-feedback`: **PASS**, host/client reali,
`fps-forearms-network-summary.log`. Scelte skin/indici invalidi, selezione al
respawn e identità del cadavere, entrambi i team, munizioni/scoring, feedback
sparo locale/remoto e danno proprietario superati. L'avvio mantiene AK74
invece di sostituirlo con il fallback, anche prima dell'inizio del match.
Questi processi nullrhi non certificano la grafica, verificata nella gallery
Metal separata.


## Ten stationary skin targets — 14 October 2026

Editor Mac arm64 build: **Succeeded**, 7.44 seconds, `ten-targets-build.log`.
`test-target-skins.py`: **PASS**, real listen host and remote client, ten
live target pawns, Alpha/Omega skin masks both 31 (all indices 0–4),
appearance meshes match, zero movement, full magazines/no firing. A second
human joins without removing targets. `ten-targets-summary.log`,
`ten-targets-host.log`, `ten-targets-client.log`. These are nullrhi checks.

`test-bots.py`: **PASS**, Patrol and Target, actual damage/ammo, stationary
behavior, death/scoring, protected respawn, round reset and remote replication.
Updated its stale base-mesh path assertion to also accept the existing skin
subdirectories. `ten-targets-bot-regression.log`. Python compile and
`git diff --check`: PASS. Opened a Metal game in Target mode with ERBots=10;
`ten-targets-play.log` confirms ten spawned pawns and their selected meshes.


## Repository cleanup/export and dependency audit — 14 October 2026

Read-only Unreal commandlet: **PASS**, ER_ASSET_AUDIT_COMPLETE packages=4938
runtime=929 authoring=970. Full transitive reference graph, explicit original
skin sources, protected external actors/objects and two non-indexed packages.
Saved/Testing/asset-audit.log; report Saved/AssetAudit/asset-audit.json and
versionable docs/ASSET_AUDIT_SUMMARY.json. Audit does not prove safe deletion;
no asset removal, cook or post-removal gameplay validation was performed.

Cleanup/export tests: **6 PASS**, disposable Git projects verify dry run,
tracked/indexed protection, active Unreal refusal, symlink exclusion, retention
of assets/settings/autosaves/audit/images and exclusion of assets/history from
code export. repository-tools-tests.log. Python syntax and git diff --check
passed. Real project dry-run: approx 64 MiB logs, 1403 MiB all cleanup profiles;
no deletion executed. git lfs prune --dry-run: 28 objects / 816 KB eligible,
4847 retained; no prune performed. No gameplay source or asset binary changed
by this maintenance task.

Created and verified BuildArtifacts/code-only: **105 text files / 0.76 MiB**,
all manifest SHA-256 values match; no Content, SourceAssets or .git directory.
The exported ignore file also excludes restored private assets. No upload.


## Mercenary removal readiness — 14 October 2026

Editor build: **Succeeded**, 21.25 seconds, detach-mercenary-build.log.
Unreal migration: ER_MERCENARY_DETACHED animations=24 samples=88
grip_error_cm=0.000000; two sockets and rotation/scale compared before/after.
No vendor mesh/material copied into the new FPS folder; fitted skeleton/poses
retain provenance. Blueprint/component/weapon/preview references replaced
through Unreal. Legacy setup scripts moved out of current authoring roots.

Fresh asset audit: packages=4963 runtime=823 authoring=892, **zero** reachable
packages or transitive references to either Mercenary tree. Full referencer
check (including unused packages): **272 packages, zero external referencers**,
mercenary-removal-ready.log/json. No actual deletion performed.

Built a hardlinked read-only fixture at BuildArtifacts/no-mercenary-test with
Content/Mercenary_Male_WaveforceStudio and EclipseRecoil/Characters/Mercenary
physically absent. Copied source/config/native binaries; did not reuse the
original Intermediate or asset registry. In this fixture:

- verify_no_mercenary.py: **PASS**, all saved ten skins, eleven weapons,
  550 idle/fire samples with two grips, Blueprint fallbacks and zero legacy refs.
- test-network.py --skins --shot-feedback: **PASS**, two actual peers, skin
  selection/RPC validation, death/corpse settling, respawn, ammo/score and shots.
- test-target-skins.py: **PASS**, all five skins per team, host and client.
- Metal FPS gallery: **10/10 PASS**, normal materials, captures and fixed
  camera/component/weapon/socket frame. All ten posed geometry bounds match
  the pre-migration gallery exactly. Inspected Alpha 3 and Omega 4 captures.

Logs copied to Saved/Testing/no-mercenary-*.log; fixture contains the captures
and full host/client logs. Package backup is Saved/AssetAudit/MercenaryDetachBackup.
The first migration attempts needed corrections for a hash enumeration guard
and Python material override access; the completed migration and final fresh
process checks above pass. Python syntax and git diff --check also passed.


## Actual repository cleanup — 14 October 2026

Removed **272 Mercenary + 3399 unused Paragon packages** after separate
verified archives. All outside referencers checked before deletion; all
current runtime/authoring inputs retained. Standard legacy World deletion
hit a UE 5.8 CleanupAfterSuccessfulDelete crash; final Editor helper deletes
only unloaded, archived Paragon packages with no outside referencers, then
updates AssetRegistry. cleanup-editor-build.log: **Succeeded**, 30.73 seconds.
paragon-prune-final.log: **ER_PARAGON_PRUNED packages=3399 bytes=2511291628**.
No active package was modified by this pruning operation.

Final fresh audit: **1292 packages, runtime=823, authoring=892**.
verify_no_mercenary.py passes 10 skins/11 weapons/550 grip samples, complete
fallback and transitive dependency checks. final-cleanup-assets.log.
Actual host/client test-network.py --skins --shot-feedback: **PASS**, scoring,
ammo, both team corpses/settling, skin validation/selection, death/respawn
and authorized shot feedback. final-cleanup-network-summary.log.
All ten targets on host/client: **PASS**, final-cleanup-target-summary.log.
Metal world and FPS galleries: **10/10 each**, no material compile errors;
inspected both complete world lineup captures. final-cleanup-*-gallery.log.

Cleanup utility tests: **7 PASS**, including allowing the Finder
UnrealEditorServices helper while blocking actual Editor/compiler processes.
Log/screenshots/DDC/asset-registry/shader-debug cleanup executed; settings,
autosaves and working native build retained. Cache LFS moved outside project:
**4927 objects / 5835165108 bytes, every SHA-256 verified, deleted_objects=0**.
Only local lfs.storage changed, no remote/history/index edits by these tools.
Project: ~4.1 GiB; Content ~2.7 GiB; .git ~66 MiB. Backups and cache are local
and external to the project, so these sizes are not a claim of disk space freed
by moving data. Python syntax and git diff --check pass.


## 14 October 2026 — Code/asset submodule separation

New checkout: `/Users/agea/git/respawn-and-rage/core`. The original private repository
and working tree remain available; no original remote history was rewritten.
All 1,336 Content/SourceAssets files were copied into three independent private
repositories and verified SHA-256 against the original tree. The public parent
contains only allowlisted text and three pinned gitlinks, with 22 generated
directory mounts preserving every `/Game` package path.

Validation in the new checkout:

- Editor Mac arm64 Development compile: **Succeeded**, 39.77 seconds. Existing
  deprecated Unreal API warning remains; no new compile error.
- Mount check: **22/22**, zero unresolved LFS pointers.
- Unreal saved-asset verification: **10 skins, 11 weapons, 550 grip samples ×
  2 hands**, preserved fitted frame and no Mercenary dependency.
- Asset Registry: **1,292 packages, runtime 823, authoring 892**, unchanged.
- `test-network.py --skins --shot-feedback`: **PASS**, host/client combat,
  replicated appearances, corpse identity/settling, respawn, invalid skin
  rejection and authorized shot feedback.
- `unittest discover -s scripts/tests`: **15 tests PASS**, including
  conflicts/overwrites, missing LFS data, logical mounted paths and rejection
  of binary assets anywhere in public history (even deleted later).

Evidence: `Saved/AssetAudit/submodule-build.log`, `submodule-assets.log`,
`submodule-network-summary.log`, `submodule-unit-tests.log`; network details
in `Saved/Testing/network-{host,client}.log`. These generated logs stay local.
Windows junction setup is implemented but has not been tested on Windows.
A fresh remote clone will be checked after repository creation/upload.

- Metal FPS gallery on the mounted checkout: **10/10 frame checks**, gallery
  complete; Alpha Support and Omega Smuggler screenshots inspected.
  Evidence: `Saved/AssetAudit/submodule-fps-gallery.log` and
  `Saved/Testing/FPS-Forearms-*.png`. No asset files were modified.


## 14 October 2026 — GitHub publication and fresh clone

The user confirmed creation/upload of all four repositories at action time.
`respawn-and-rage/core` is public; `assets-characters`, `assets-weapons`,
`assets-maps` are private. Each private main branch matches its pinned
gitlink, including all LFS objects. The old archivio privato originale remains private.

- Anonymous GitHub API check: code HTTP **200**, private assets and old
  repository HTTP **404**. Private creation was also verified in the signed-in
  UI; all three asset commits were confirmed over authenticated SSH.
- A fresh HTTPS code clone with credential helpers disabled: **PASS**.
- Fresh clone switched to SSH for private dependency authentication, then
  `setup-assets.py --init`: **22/22 mounts**, no unresolved LFS pointers.
- Downloaded private payload: **1,336 files**, every SHA-256 matches the
  manifest; `git lfs fsck` **PASS** for each repository. No local LFS cache
  was seeded into this remote-clone test.
- Public history verifier: **PASS**; all branches contain text and the
  three private gitlinks, with no asset blobs or LFS pointers.

Evidence: `Saved/AssetAudit/remote-submodule-init.log` and
`remote-clone-verification.json`. Detailed machine-local receipts stay
outside the public Git history. See `REPOSITORY_SPLIT.json` for the summary.

- Unreal verification on the freshly downloaded clone: **PASS**, unchanged
  1,292 packages / runtime 823 / authoring 892, ten FPS skins and 550 two-hand
  grip samples; **all three maps loaded** (MainMenu, Depot, MovementLab).
  Evidence: `Saved/AssetAudit/remote-clone-unreal.log`. The already compiled
  Mac Editor module was copied into the test clone; this check verifies
  downloaded assets and map loading, not a second independent compilation.
- The disposable remote-clone checkout and its duplicate LFS cache were
  removed only after checking all worktrees clean and retaining receipts/logs.
  The active checkout, private LFS caches and original history are retained.


## Sprint, postura prona e transizioni — 14 ottobre 2026

- Compilazione reale Editor: **Succeeded**, `Saved/Testing/locomotion-build.log`.
- `scripts/test-movement.py`: **73 controlli server + 1 proprietario/proxy
  superati**, `ER_MOVEMENT_COMPLETE PASS`. Conservati i log completi in
  `locomotion-full-movement-host.log`, `locomotion-full-movement-client.log`
  e il riepilogo `locomotion-network.log`, sotto Saved/Testing.
  Comprendono corsa/camminata/inertia, salto, venti altezze del laboratorio,
  arrampicata remota, accovacciamento graduale, sprint a 18 km/h, limite 5 s,
  recupero 10 s senza riavvio tenendo W, interruzione anticipata, prona remota,
  altezza capsula, camera/mira, volume colpibile oltre la capsula, soffitto
  bloccante e rialzata dopo rimozione del soffitto.
- Le ultime cinque asserzioni usano **InputKey di Unreal e i binding effettivi**:
  doppio W, rilascio W, doppio C, C per rialzarsi, doppio Ctrl. Non sono input
  fisici del sistema operativo. La prova via automazione nativa è rimasta
  inconclusiva per timeout della connessione alla finestra; la sessione di
  laboratorio temporanea è stata chiusa. Nessun collaudo OS dichiarato.
- `scripts/test-network.py --accuracy`: **15 controlli superati**,
  `ER_ACCURACY_COMPLETE result=PASS`, log `locomotion-accuracy.log`.
  Include dispersione accovacciata, camminata/corsa/salto, proiettili campionati
  dal server, recupero raffica e replica dati al proprietario.
- `-ERPostureGallery`: sessione grafica **Metal**, completata; ispezionate
  `Posture-FPS.png`, `Posture-Alpha.png`, `Posture-Omega.png` in Saved/Testing.
  Vista a terra, entrambe le squadre pancia in giù, testa sollevata e fucile
  orientato in avanti. Corretto l'iniziale sollevamento delle gambe usando
  la catena di riferimento e abbassando l'origine visuale. Non si dichiara
  una nuova animazione mocap di discesa/risalita o strisciamento.
- Il campione di accelerazione della vecchia sonda cercava un singolo intervallo
  80–120 ms, che poteva essere saltato con frame lunghi durante l'avvio. Ora
  verifica un campione effettivo fra fermo e velocità di regime. Anche la
  sonda del doppio W attende l'accelerazione prevista prima di misurare la corsa.
- `py_compile` dello script modificato e `git diff --check`: superati.
  Nessun asset binario modificato; nessun commit/push. Test con due processi
  locali senza latenza/perdita artificiale. Collisione di movimento resta la
  capsula verticale; il volume lungo il corpo prono è solo per colpi/visibilità.

La ripetizione completa sulla build finale, dopo la preservazione dello smoothing
nativo e degli intenti correnti durante `ClientUpdatePositionAfterServerUpdate`,
ha confermato nuovamente **73 + 1 PASS**, nessun FAIL. Questo corregge il caso
intermittente in cui il replay di un saved move antecedente al gesto poteva
rilasciare il nuovo intento di sprint/prona. Le scadenze correnti del server
restano preservate dopo il replay. I log completi sopra indicati sono stati
aggiornati a questa esecuzione finale.


## Hitscan e quattro decal PNG — 14 ottobre 2026

- Build `RespawnAndRageEditor Mac Development`, arm64, UE 5.8.3: **Succeeded**.
  `Saved/Testing/hitscan-build.log`. Nuovo helper Niagara Editor; le binding
  usano una API deprecata in 5.7 ancora supportata in 5.8 (warning, non errori).
- Generazione tramite Unreal: `ER_HITSCAN_EFFECTS_SAVED`, asset Niagara,
  texture RGBA e quattro material instance salvati, assegnati ai Blueprint.
  `Saved/Testing/hitscan-assets.log`; `setup-assets.py --check`: **23 mounts**,
  nessun puntatore LFS irrisolto. PNG privata intatta, 1254x1254, alpha 0..255.
- `scripts/test-network.py --shot-feedback`: **PASS**, due processi Unreal
  reali, host/client. Il danno del primo colpo è già 64 HP prima del ritorno
  della richiesta di sparo; nessun attore AERProjectile Bullet. Passano
  protezione, replica, cadenza, caricatore vuoto, danno client→host, friendly
  fire, blocco della parete, death/respawn, score e feedback direzionale.
  `Saved/Testing/hitscan-network.log` e `network-{host,client}.log`.
- `scripts/test-network.py --accuracy`: **PASS**, dieci trace realmente
  campionate entro il cono server, direzioni diverse, nessun attore Bullet;
  cammino/corsa/salto, limite/recupero bloom e replica owner.
  `Saved/Testing/hitscan-accuracy.log` e `accuracy-{host,client}.log`.
- `scripts/test-arsenal.py`: **PASS**, sei trace del fucile a pompa e una sola
  cartuccia consumata; regressioni loadout, slot, RPG, HE/smoke/stun, nube
  e carving autorizzato verificati su due peer.
  `Saved/Testing/hitscan-arsenal.log` e `arsenal-{host,client}.log`.

Le prove di rete usano nullrhi/nosound: verificano logica e presenza del
feedback, senza giudicare la resa grafica o il suono udibile. La prima prova
ha rilevato capsule ignorate da Visibility: corretto usando il canale
Projectile, verificato ripetendo con successo la suite completa.
La scena Metal temporanea conferma quattro immagini distinte con alpha
sulle superfici; la PNG non è stata modificata per nascondere i bordi
originali. Non sono state salvate modifiche alla mappa del deposito.

Verifica grafica finale **PASS**, `-ERHitscanGallery`, renderer Metal,
1280x720 con due bersagli fermi. `Saved/Testing/hitscan-gallery.log` e
`Saved/Testing/Hitscan-Effects.png`: scia dorata sottile dalla volata
al punto colpito, burst compatto orientato fuori dalla parete e quattro
decal diverse con alpha. Il campione cosmetico della scia è mantenuto
350 ms soltanto nella gallery per il readback; gli spari normali usano 55 ms.
Il muzzle flash già esistente è conservato, con durata ordinaria 35 ms;
questa cattura differita non verifica il suo picco. Nessun benchmark FPS
è stato eseguito e il readback dello screenshot non misura il costo di gioco.

La scia usa bounds espliciti dell'intero segmento e l'emitter è collegato
al grafo tramite FNiagaraEditorUtilities::AddEmitterToSystem (la sola
AddEmitterHandle non crea i nodi che lo eseguono). Il burst d'impatto usa
spazio locale per scalare sia moto sia sprite, con materiale proprio.
Controlli finali: Python py_compile e git diff --check **PASS**.


## Bagliore discreto e bossolo nero — 14 ottobre 2026

- Build Editor arm64 **Succeeded** (`Saved/Testing/subtle-shots-build.log`).
- Generazione asset Unreal **PASS**: nuovo impatto a sprite singolo, materiali
  attenuati per scia/flash, BP_ER_SpentCasing con Cube e materiale nero opaco;
  riferimenti salvati nel Blueprint del personaggio.
  `Saved/Testing/subtle-shots-assets.log`.
- `test-network.py --shot-feedback` **PASS**, due peer reali nullrhi/nosound:
  hitscan immediato, danni, munizioni, protezione, cadenza, round, respawn e
  feedback invariati. `ER_CASING_EJECT` osservato su host e client con
  replicated=0, sim=1, pawn=0, projectile=0 (ECR_Ignore).
  `Saved/Testing/subtle-shots-network.log`, network-host/client.log.
- `-ERHitscanGallery -ERShotAudit`, Metal 1280x720: **PASS** per traccia
  autorizzata e quattro decal; bossolo con mesh e fisica locale, collisioni
  pawn/proiettili ignorate, discesa superiore a 50 cm e pavimento entro 5 cm
  dopo 850 ms, poi eliminazione per lifetime di tre secondi.
  `Saved/Testing/subtle-shots-gallery.log`.
- Ispezionata `Saved/Testing/Hitscan-Effects.png`: nessun ventaglio luminoso,
  bagliore piccolo/morbido e scia molto tenue. La gallery mantiene la sola
  scia 350 ms per il readback; impatto ordinario 35 ms, muzzle flash 18 ms.
  `Casing-OnFloor.png` è una vista del pavimento, non una prova leggibile
  del minuscolo bossolo: contatto e cleanup sono verificati dalla sonda.

La prima cattura è stata invalidata da un clic di focus iniziale che aveva
consumato la cadenza cosmetica del bossolo; isolati input/counter nella sola
gallery. La misura di caduta viene eseguita prima del readback per non
confondere una pausa di acquisizione con il tempo fisico. Nessuna modifica
alla logica di danno né alla mappa salvata. Python py_compile e
git diff --check **PASS**. Non misurata la qualità audio (nosound).


## Ripristino visibilità traccianti — 14 ottobre 2026

Build Editor arm64 **Succeeded**; aggiornato tramite Unreal soltanto il
materiale M_ER_Tracer, emissione 4/2,2/0,65 e alpha 0,7. Larghezza runtime
ripristinata a 0,12 cm. Generatore aggiornato; impatto/muzzle flash tenui
e bossoli conservati. Log `Saved/Testing/visible-tracer-{build,assets}.log`.

Gallery Metal 1280x720 **PASS**, immagine Hitscan-Effects.png ispezionata:
scia dorata chiaramente leggibile e nessun ritorno del ventaglio luminoso
all'impatto. Passano anche fisica locale, contatto col pavimento e rimozione
del bossolo nella sonda esistente. Log visible-tracer-gallery.log.
Il campione della scia nella gallery dura 350 ms per il readback; durata
ordinaria 55 ms e cadenza ogni tre spari. Nessuna modifica alla logica
di sparo; non ripetute le suite gameplay già superate per questa regolazione
cosmetica. Python py_compile e git diff --check **PASS**.


## Traccianti dalla punta durante il movimento — 14 ottobre 2026

- Build Editor arm64 **Succeeded** (`Saved/Testing/muzzle-tracer-build.log`).
- Unreal **PASS**: salvati MuzzleOffset delle sette armi Bullet dal centro
  della geometria frontale, senza modificare mesh/grip. Il campo previous
  nel primo log usa una proxy Struct già modificata: non rappresenta il valore
  precedente. `muzzle-tracer-assets.log` conferma i sette valori finali.
- Materiale M_ER_Tracer aggiornato tramite Unreal **PASS**: rimossa solo
  la sfumatura lungo il segmento, mantenuti colore/alpha/sfumatura trasversale.
  `muzzle-tracer-cap.log`.
- Gallery Metal 1280x720, `-ERHitscanGallery -ERTracerMotion -ERTracerAudit`
  **PASS**. Raffica laterale a 166,7 cm/s, POV dello stesso frame (camera_age
  circa zero), origine 0,75 cm oltre la punta. Tra due frame consecutivi
  l'origine segue il movimento mentre l'endpoint del server resta fisso.
  Traccianti ordinari visibili uno/due frame a 60 FPS, durata circa 20 ms;
  solo il campione per readback è mantenuto 350 ms.
  `muzzle-tracer-motion.log`, `Hitscan-Effects.png` ispezionata visivamente:
  scia dalla volata e bossolo scuro in caduta, senza ventaglio d'impatto.
- La stessa sonda conferma cadenza in movimento, bossolo locale con collisioni
  player/shot ignorate, caduta da Z=168,545 a Z=4,177 cm e contatto col pavimento,
  poi cleanup a tre secondi. Percorso temporaneo spostato su pavimento libero
  per evitare un appoggio su oggetti della mappa. Nessuna mappa salvata.
- `test-network.py --shot-feedback` **PASS**, due peer nullrhi/nosound,
  `ER_NETWORK_PROBE_COMPLETE result=PASS` in `muzzle-tracer-network.log`.
  Le prime prove avevano superato il tempo previsto per svuotare il caricatore:
  eliminati Niagara/bossoli inutili nei processi senza rendering, senza cambiare
  cadenza, danno o assert gameplay. La rete verifica feedback/danni/munizioni,
  non la resa grafica dei traccianti, verificata separatamente con Metal.

Python py_compile dei tre generatori modificati e git diff --check **PASS**.
Qualità audio non verificata (nosound); nessun benchmark prestazioni.


## Distruzione a blocchi, Milestone 1 — 14 ottobre 2026

Progetto corretto: /Users/agea/git/respawn-and-rage/core/RespawnAndRage.uproject; modulo fdm;
UE 5.8.3 CL 58210709, Editor Mac Development arm64, Apple M2 Pro.
Specifica importata integralmente in docs/PROCEDURAL_DESTRUCTION_SPEC.md
(cmp con il file Downloads **PASS**). Nessuna modifica al prototipo precedente.

- Build Editor **Succeeded**, Saved/Testing/destruction-build.log. Corrette
  durante la compilazione tre incompatibilità C++ (TObjectPtr.Get, infinity
  della libreria standard e loop iterator con break incondizionato). Restano
  gli avvisi preesistenti di deprecazione Niagara; nessun errore finale.
- Generazione tramite Unreal **PASS**, destruction-assets.log. Salvati una
  nuova mappa L_DestructionLab, tre materiali e DA_ER_BlockMaterials nel mount
  privato Assets/maps. Seconda esecuzione preserva geometria/materiali e
  configura soltanto la luce di riempimento gestita. Prima creazione: quattro
  errori Asset Registry dovuti a load di asset ancora assenti, poi creati e
  salvati correttamente; il generatore ora controlla l'esistenza prima del load
  e l'ultima esecuzione non riporta questi errori.
- Automation **PASS**, destruction-automation.log e destruction-report/index.json:
  **2 succeeded, 0 failed, 0 not run**. Storage: determinismo stanza/ID, HP
  differenti per tre materiali, nessuna sovrapposizione iniziale agli angoli,
  rifiuto dimensioni/danno invalido e limite allocazione. CollisionAndIdentity:
  veri ISM e scena Chaos, rimozione di tutti i 504 blocchi in ordine casuale
  seedato, trace/ID dei superstiti corretti, assenza di collisione ai blocchi
  rimossi, capsula 34×92,5 cm fermata prima e libera dopo l'abbattimento,
  pavimento ancora solido; cinque rigenerazioni senza componenti/istanze duplicate.
- Sessione standalone nullrhi **PASS**, destruction-game.log:
  ER_BLOCK_PROBE_COMPLETE result=PASS. Dieci controlli: spawn sostenuto, match
  per un giocatore, comandi console danno/distruzione, blocco adiacente corretto,
  sparo autorizzato con consumo munizioni, movimento del personaggio reale,
  muro integro bloccante, breccia 150×200 cm percorribile e rigenerazione con
  ritorno sicuro al centro. Il probe usa la mappa e il BP_ERCharacter reali.
- Metal 1280×720 **PASS**, destruction-gallery.log, stesso probe gameplay.
  Destruction-Blocks.png e Destruction-Breach.png ispezionate: rimozioni visibili,
  breccia continua e pavimento conservato. Tarata la sola luce di riempimento
  della nuova mappa: 2500 sovraesponeva la stanza, intensità finale 12 rende
  leggibili i tre materiali con il launcher a GI bassa. Nessun default rendering
  globale modificato. Immagini/log sotto Saved/Testing. Non verificata la qualità
  audio (nosound) né la sensazione di input umano.
- Regressione due peer LAN su loopback, test-network.py --shot-feedback:
  **PASS** nell'esecuzione isolata, destruction-network.log e network-{host,client}.log;
  ER_NETWORK_PROBE_COMPLETE result=PASS, 43 eventi feedback per peer,
  controlli danno/munizioni/cadenza/ricarica/respawn/copertura/corpi superati.
  La prima esecuzione insieme alla gallery Metal aveva **FAIL** nei tre controlli
  temporali dello svuotamento caricatore/feedback (clip non vuoto a 4,2 s);
  conservata in destruction-network-concurrent{,-host,-client}.log. Ripetuta
  senza processi grafici, con lo stesso codice e senza allentare gli assert: PASS.
  Questo verifica una regressione del gioco esistente, non la replica dei blocchi.
- Python py_compile dei due nuovi script, zsh -n del launcher e git diff --check
  **PASS**. Nessun commit/push, cook, pacchetto o pubblicazione effettuato.

Misure indicative della singola stanza: 504 blocchi, 4 chunk, 7 ISM; nella
gallery finale generazione 2,59 ms e un aggiornamento campionato 0,21 ms.
Comprendono lavoro sincrono di istanze/collisioni; non sono benchmark FPS o
misure complete dei costi asincroni del rendering/physics.

Milestone 0 soddisfatta dall'infrastruttura UE esistente; implementazione e
verifiche automatiche/grafiche della Milestone 1 completate. Restano collaudo
umano con mouse/tastiera e profiling prolungato. Distruzione abilitata solo
in NM_Standalone con authority: multiplayer/late join non implementati.
Milestone 2 (penetrazione ed esplosioni ai blocchi) non iniziata.


## Milestone 2 — penetrazione ed esplosioni ai blocchi, 14 ottobre 2026

Stesso core, UE 5.8.3 CL 58210709, Editor Mac Development arm64/M2 Pro.

- Build Editor **Succeeded**, Saved/Testing/destruction-m2-build.log.
- Automation **PASS**: Storage, CollisionAndIdentity e BallisticsAndExplosion,
  destruction-automation.log / destruction-m2-automation-summary.log.
  Report destruction-report/index.json: 2 succeeded + 1 succeededWithWarnings,
  0 failed, 0 notRun. L'unico warning riguarda DestroyActor del mondo temporaneo
  senza WorldContext della fixture; le verifiche di gioco/collisione passano.
- BallisticsAndExplosion usa scene fisiche/trace reali: cartongesso attraversato
  senza rimuovere il blocco, HP 30→25 e bersaglio 500→491,5 con colpo 10/energia 100;
  energia residua 85. Mattoni/cemento fermano 100; energia 200 attraversa il mattone
  e lascia il bersaglio a 496. Verificati spessore normale/obliquo/scalato, uscita
  da dentro il volume, distanza che finisce nel muro, angolo radente, limite di
  attraversamenti, ricerca spaziale vicino alla faccia e aggregazione duplicati.
- Esplosione automatica di test: 104 candidati, 98 rimossi, solo 2 gruppi
  ricostruiti; capsula piena attraversa la breccia, pavimento conservato.
  Test a due stanze: 106 candidati coperti; cemento esposto distrutto ma
  cartongesso dietro protetto dalla stessa esplosione, colpito da quella seguente.
  Prima versione **FAIL** su ampiezza/passaggio della breccia: le tracce ai centri
  facevano schermare i blocchi della stessa parete. Corretto il campionamento
  sulla faccia esposta; i test passano senza ridurre le verifiche. Primo log
  conservato in destruction-m2-self-occlusion.log.
- Script Unreal **PASS**, destruction-m2-assets.log: migrati una volta i soli
  nuovi campi del catalogo, creati tre materiali decal propri e tre bersagli,
  salvata soltanto L_DestructionLab nel mount maps. Art/HP/geometria precedenti
  preservati; nessun nuovo asset Fab o binario nel codice pubblico.
- Standalone --combat nullrhi **PASS**, destruction-game-combat.log:
  otto controlli gameplay, ER_BLOCK_PROBE_COMPLETE result=PASS. Colpo debug
  e vera cartuccia raggiungono il bersaglio dietro cartongesso ancora solido;
  mattoni/cemento bloccano l'energia normale; esplosione console crea breccia
  percorsa dal vero personaggio; una HE fisica con miccia rimuove 72 blocchi
  aggiornando due gruppi; rigenerazione ripristina pareti e bersagli a 500 HP.
- Metal 1280×720 --combat --render **PASS**, destruction-gallery-combat.log.
  Destruction-Penetration.png / Destruction-Explosion.png ispezionate: 504
  istanze ancora vive dopo penetrazione (blocco HP 7, bersaglio 460,9 dopo il
  colpo debug e il colpo arma), poi 116 blocchi rimossi dall'esplosione console
  con due aggiornamenti di gruppo e passaggio libero. Nessun errore shader
  dei nuovi materiali nel log. Tempi campionati: generazione circa 2,5 ms,
  aggiornamento dell'esplosione circa 0,45 ms. Non sono benchmark FPS o costi
  asincroni completi. Non verificata la qualità audio (nosound).
- Vecchia sonda --game Milestone 1 **PASS**, destruction-game.log, dieci
  controlli. Il test di sparo prepara il blocco a 1 HP per verificare una
  distruzione reale con il nuovo danno ridotto del cartongesso.
- LAN --shot-feedback **PASS**, destruction-m2-network.log: danni, copertura,
  cadenza minima, munizioni, ricarica, morte/respawn, mesh, corpi e feedback.
  Primo tentativo **FAIL** nei tre assert di fine caricatore: un colpo ancora
  pendente dopo 4,2 s. La sola sonda ora attende ammo=0 con timeout 8 s; tutti
  gli assert esatti su eventi/munizioni/cadenza restano. Nessuna regola arma
  cambiata. Primo log conservato in destruction-m2-network-timing{,-host}.log.
- LAN --accuracy **PASS**, destruction-m2-accuracy.log: cone reali, postura,
  movimento, raffica/recupero e dati owner replicati.
- LAN test-arsenal.py **PASS**, destruction-m2-arsenal.log: pallini, selezione
  slot, munizioni/respawn, HE/smoke/stun, FX locali/remoti, cleanup e carve remoto.
  Nel primo tentativo il probe host aveva PASS, ma il wrapper falliva il solo
  marker remoto smoke: letto una sola volta al cambio fase 9. La sola sonda
  ora osserva l'evento fino al cleanup (ancora FAIL se manca); evento remoto
  events=1 realmente osservato nel nuovo log. Log precedente in
  destruction-m2-arsenal-timing{,-client}.log. Nessuna modifica al fumo runtime.
- Python py_compile, zsh -n e git diff --check **PASS**. Spec originale ancora
  identica al file importato (cmp PASS). Nessun commit, push, cook o packaging.

Milestone 2 implementata e verificata localmente. Limiti: standalone, riparo
a un campione, nessuna onda di esplosione che aggira i muri; penetrazione
hitscan limitata a 16 attraversamenti. Usare scala uniforme sugli attori
stanza e configurare le dimensioni tramite Config. Restano taratura umana,
profiling prolungato, generazione di più stanze (Milestone 3) e replica blocchi.


## 14 ottobre 2026 — libreria PBR e Milestone 3

Unreal 5.8.3, Mac arm64 M2 Pro 32GB; nessun cook/packaging/pubblicazione.
Build Editor **Succeeded**, destruction-m3-build.log. Tutti i sette run finali
sono terminati con codice 0; riepilogo in Saved/Testing/destruction-m3-validation.json.

- ZIP originale **PASS**: dieci set, 84 file del manifest verificati SHA256/byte
  prima dell'estrazione. Archivio conservato con hash
  7858347e98cd15adc774709c9769c239e8c5fa2f515cec573b556bce90409360.
- Import Unreal **PASS**, destruction-m3-textures.log: 41 texture, dieci materiali
  PBR e un vetro parametrico. Catalogo aggiornato solo nelle Surface; sorgenti
  originali/ZIP/manifest/crediti nel mount privato maps.
- Audit Unreal in sola lettura **PASS**, destruction-m3-assets-audit.log:
  tutte le texture 2048×2048, colore sRGB, normali DX con compressione normal-map,
  maschere lineari; hash/provenienza presenti. Graph PBR con texture e collegamenti
  BaseColor/Normal/Roughness, world-space normal abilitato; vetro traslucido,
  catalogo e L_ProceduralLab salvati. L'audit legge le espressioni del graph: i
  resource shader GetUsedTextures non sono disponibili con NullRHI.
- Automation **PASS**, destruction-m3-automation-summary.log /
  destruction-automation.log: Storage, CollisionAndIdentity,
  BallisticsAndExplosion, ProceduralLayout e ProceduralCollision. Report:
  **4 succeeded + 1 succeededWithWarnings, 0 failed, 0 notRun**. Il warning
  ereditato è DestroyActor in una fixture temporanea senza WorldContext.
- ProceduralLayout verifica 25 combinazioni (6–10 stanze × seed 0,1,42,7919,-17):
  ripetibilità BlockID/transform/materiale, diversità dei seed, assenza di ID/
  volumi duplicati, numero di aperture/percorsi e limiti di configurazione.
  Verificate anche stanze minime, corridoio/rami minimi e porte da 100/200 cm.
- ProceduralCollision usa Chaos reale con seed 42,1,7919: capsule 34/92,5 cm
  attraversano ogni percorso stanza→ramo→hall e tutto il corridoio centrale.
  Trace su tutte le porte/finestre, spawn libero, 120 rimozioni casuali per seed
  con identità/assenza di collisione, tre rigenerazioni senza duplicare componenti,
  pavimento conservato e rigenerazione invalida che preserva le istanze vive.
  Ai raccordi a T il test usa il lato esposto: un raggio avviato dentro il blocco
  perpendicolare deve correttamente colpire quel vicino.
- Standalone --procedural **PASS**, destruction-m3-game-summary.log /
  destruction-game-procedural.log: dodici controlli con personaggio/arma reali.
  Spawn a (0,625,94,65), passaggio attraverso entrambe le porte, cambio console
  seed7919/10 stanze, arma che rimuove il blocco mirato, esplosione batch e
  camminata nella breccia fino all'asfalto, richiesta invalida preservata e reset.
  PlayerStart mobili risolvono il precedente spawn iniziale fermo alla posizione
  salvata: i PlayerStart statici impedivano lo spostamento runtime.
- Metal 1280×720 --procedural --render **PASS**, destruction-m3-render-summary.log
  / destruction-gallery-procedural.log. Procedural-Room.png,
  Procedural-Corridor.png e Procedural-Breach.png ispezionate: texture continue,
  aperture pulite, soffitti/pavimenti presenti, basamento asfaltato visibile dopo
  la breccia. Ridotto il fill del laboratorio da 20 a 3 per leggere meglio PBR.
  Nessun errore shader dei nuovi materiali; non verificato l'audio (nosound).
- Campioni Metal finali, senza claim FPS: seed42/8 stanze = 8.018 blocchi,
  66 chunk, 78 gruppi, generazione 37,29 ms; seed7919/10 stanze = 10.075 blocchi,
  70 chunk, 94 gruppi, generazione 54,82 ms e rigenerazione 54,17 ms.
  Un blocco rimosso: 0,49 ms / un gruppo; esplosione: 124 candidati, 115 rimossi,
  due gruppi aggiornati, LastUpdate 0,91 ms. LastUpdate copre mutazione/ISM/
  collisioni sincrone, esclude query/occlusione/FX e costi asincroni successivi.
- Laboratorio M2 --combat **PASS**, destruction-m3-combat-summary.log:
  penetrazione/bersagli, resistenza dei materiali, breccia percorsa, vera HE
  con miccia e rigenerazione. Le texture nuove non modificano le regole di danno.
- LAN --shot-feedback **PASS**, destruction-m3-network-summary.log:
  munizioni/cadenza/ricarica, danni/copertura, morte/respawn/corpi, asset e
  feedback host/remoto. Nessuna generazione o distruzione blocchi in LAN.
- LAN --accuracy **PASS**, destruction-m3-accuracy-summary.log:
  postura/movimento/coni reali, raffica/recupero e dati owner replicati.
- LAN test-arsenal.py **PASS**, destruction-m3-arsenal-summary.log:
  selezione slot/ammo/respawn, HE/smoke/stun, effetti host/remoto e carve del fumo.
- setup-assets.py --check **PASS**: 24 mount, nessun puntatore LFS irrisolto.
  Python py_compile, zsh -n, git diff --check pubblico e privato **PASS**.

Milestone 3 implementata e verificata localmente. Layout su due file con
corridoio comune; varianti di packing/grafo, vetri fisici, nuovi profili
balistici e replica dei blocchi restano estensioni successive. Nessun commit
o push eseguito; tutti i binari/sorgenti texture sono nel submodule privato maps.


## 14 ottobre 2026 — Milestone 4: stabilità e profiling

UE 5.8.3 CL58210709, Mac arm64 M2 Pro / 32GB / macOS27.
Implementazione e verifica locali, nessun nuovo asset binario, commit/push,
cook o packaging. Profiling dettagliato e ricevuta in DESTRUCTION_BENCHMARK.json;
confronto e metodologia pubblicati nel README.

- Build base/ottimizzata **Succeeded**, destruction-m4-baseline-build.log e
  destruction-m4-build.log. Warning deprecazioni Niagara già presenti.
- Automation **PASS**: Storage, CollisionAndIdentity, BallisticsAndExplosion,
  ProceduralLayout, ProceduralCollision e StressAndBatchedIdentity. Log
  destruction-m4-automation-summary.log / destruction-automation.log; report:
  **5 succeeded + 1 succeededWithWarnings, 0 failed, 0 notRun**. Warning ereditato
  della fixture WorldContext temporanea; nessun errore del sistema blocchi.
- Stress automatico **PASS**, ER_BLOCK_STRESS_AUTOMATION cycles=18:
  dieci stanze, chunk200/400/800, sei seed per chunk. Rimozione del90% con
  richieste triplicate: esattamente gli ID unici e un update per gruppo sporco,
  zero upload dei superstiti. Trace su ogni superstite dopo gli swap, campioni
  dei volumi rimossi privi di collisione, handle nativi/mapping coerenti.
  Rigenerazioni identiche ripristinano corpi/allocazioni esclusive e non
  accumulano componenti; GC dopo ogni ciclo.
- Benchmark base **PASS**, destruction-m4-baseline-summary.log /
  destruction-benchmark-baseline-nullrhi.{log,csv}: sei casi,54cicli includendo
  sei warmup esclusi,2.544 righe misurate. Algoritmo precedente full-group
  ClearInstances/AddInstance per superstite, con la stessa strumentazione.
- Benchmark ottimizzato NullRHI **PASS**, destruction-m4-optimized-summary.log /
  destruction-benchmark-optimized-nullrhi.{log,csv}: stessi54cicli e2.544righe.
  Ogni ciclo:16rimozioni singole, batch256 con duplicati,32query verificate
  contro riferimento lineare dei blocchi vivi, esplosione250/2400, distruzione
  completa, trace in tutti i9.907/10.075volumi precedenti, pavimento/capsula
  conservati, verifica corpi eGC. Oltre539mila rimozioni per run, nessuna
  collisione fantasma, handle recovery o crash.
- Dopo il benchmark: raffica vera da30cartucce **PASS**,9blocchi rimossi nel
  run senza rendering; nessun FX cosmetico creato con NullRHI.
- Benchmark/stress Metal **PASS**, destruction-m4-metal-summary.log /
  destruction-benchmark-optimized-metal.{log,csv}, stessi54cicli e2.544righe.
  Raffica vera30colpi,10blocchi rimossi; Destruction-Stress-Burst.png ispezionata:
  foro visibile e coerente, texture superstiti continue. Finestra1280×720 con
  dynamic resolution attiva: questa prova non certifica FPS o rendering3D nativo.
- FX Metal **PASS**: budget0/24/1000 con200spawn ciascuno, ogni spawn controllato
  contro il clamp0..64. Emettitori/decal reali creati; campione massimo64/64.
  Impatti lontani respinti;32richieste di esplosione producono al massimo8burst
  (otto realmente attivi). Rigenerazione azzera gli effetti posseduti. EndPlay
  distrugge burst e dis-registra i nuovi componenti decal/Niagara effettivi,
  senza limitarsi a svuotare le weak reference.
- Trace Unreal Insights **creata**, destruction-benchmark-optimized-metal.utrace
  (circa17MiB), canali CPU/frame/bookmark. Scope native di generazione,
  remove/group, query, batch, penetrazione ed esplosione. Nessuna analisi
  manuale completa GPU/heap o profiling prolungato dichiarato.
- Risultati pooled dei due seed, chunk400: single p95 **1,211→0,032ms**,
  batch256 mediana **43,901→1,821ms**, generazione **45,318→43,470ms**,
  query250 p95 **0,0115→0,0101ms**, esplosione250 mediana **1,243→0,516ms**,
  memoria tracciata **5,676→3,779MiB**. In Metal: single p950,038ms,
  batch256 mediana1,858ms, generazione48,289ms. Non si confronta CPU NullRHI
  prima con Metal dopo; le due tabelle sono distinte.
- Tempi: wall time delle chiamate sul game thread, incluse operazioni fisiche
  sincrone in ISM; esclusi verifica, GC, logging/CSV, snapshot memoria, thread
  render/GPU e fisica differita. Clear/remove/add non sono puro tempo Chaos.
  TrackedBytes = allocazioni logiche/layout/mapping più report esclusivi UE
  degli ISM (pareti/pavimenti/soffitti); escluse risorse condivise, heap totali
  e memoria GPU. I body count riguardano questi ISM, non il basamento non-ISM.
  RSS del processo registrata separatamente, nessun assert di uguaglianza su
  cache/pool engine. Warmup e percentili/count sono riportati nella ricevuta.
- Regressioni tutte **PASS**, destruction-m4-regression.json (sette codici0):
  --game del laboratorio M1 (dieci controlli), --combat, --combat --render,
  --procedural --render (dodici controlli), LAN --shot-feedback, --accuracy
  e test-arsenal.py. Log summary destruction-m4-{room,combat,combat-metal,
  procedural-metal,network,accuracy,arsenal}-summary.log. Verificati
  penetrazione, bersagli, HE con miccia, breccia percorsa, reset, dati owner,
  ammo/cadenza, morte/respawn/corpi, feedback e smoke carve remoto.
- Python py_compile e git diff --check **PASS**; 24mount privati verificati
  senza puntatori LFS irrisolti. Specifica importata preservata.

Default chunk400/blocco50×20×25 conservati: chunk200 aumenta i gruppi circa
93→420 e il costo di generazione; chunk800 aumenta il mapping di un update
singolo. Le ottimizzazioni non cambiano danni/materiali/occlusione o l'authority.
Milestone4 completata; sistema ancora standalone, crolli/replica successivi.


## Milestone 5 — supporti e detriti Chaos (14 ottobre 2026)

Build Mac arm64 Development riuscita su UE5.8.3 CL58210709. Nuovi test
`StructuralSupport` e `CollapseAndIdentity`; suite finale: otto successi,
zero fallimenti/non eseguiti (report: sei senza warning, due con warning per
DestroyActor nei mondi fixture senza world context). Il probe della partita
salvata non usa questi mondi temporanei. Warning di deprecazione preesistenti
nel codice Niagara/forearms nella build funzionale; build finale del probe
compila e collega senza nuovi warning.

| Verifica | Esito osservato |
|---|---|
| Grafo | Contatti di faccia, esclusione spigoli/vertici, architrave collegato lateralmente, ancoraggi espliciti/indistruttibili, corner clip e rigetto rotazioni non assiali: PASS |
| Distacco | Anello distrutto di 16 blocchi: nove isolati staccati, resto ancorato conservato, nessuna collisione/query/danno sulle parti staccate, mapping nativo valido a ogni lotto: PASS |
| Danno durante il job | Rimozione aggiuntiva durante il flood: completamento e secondo controllo della revisione senza starvation: PASS |
| Budget | Limiti di lavoro/distacco configurati e hard, cap hard64 globali e hard8 spawn tra due stanze, overflow senza blocchi sospesi: PASS |
| Fisica effettiva | Nove detriti selezionati (rimozione4/spawn4 per passo nel fixture) cadono con Chaos; player/proiettili ignorati; fisica disattivata e actor scaduti: PASS |
| Frame lenti | Intervallo reale a 5 FPS, poi ritorno a60: almeno un delta>100 ms e massimo un callback di supporto per frame, NullRHI e Metal: PASS |
| Pulizia | Regenerate ripristina storage/ISM/supporti e cancella timer/detriti; EndPlay elimina i propri detriti; pavimento preservato: PASS |
| Piano intero | Seed7919, dieci stanze, 10.075 blocchi: eliminati827 alla base, staccati9.248 superiori, zero istanze muro residue, budget default rispettato: PASS |
| Regressioni | Stanza, combattimento NullRHI/Metal, piano procedurale Metal, LAN con shot-feedback, accuracy e arsenal: tutti exit0 |
| Stress M4 con M5 | Due seed×tre chunk×nove cicli=54, NullRHI e Metal: identità/collisioni, 30 colpi reali, FX/culling, rigenerazione/EndPlay: PASS |
| Profiling finale | Stesso stress Metal da54 cicli e2.544 righe; subtotali logical/layout/mapping/ISM/support sommano esattamente al tracked; trace salvata: PASS |

Misure sullo stesso Mac M2 Pro/32 GiB, chunk400 e blocchi50×20×25 cm. Collasso
completo: 88 callback, support_bytes427.567. Un singolo run per RHI (nessun
warmup escluso per il collasso); p95 riguarda i callback del run, non una
popolazione di run indipendenti.

| RHI | Costruzione grafo | Callback p95 | Callback max | Completamento |
|---|---:|---:|---:|---:|
| NullRHI | 6,733 ms | 0,512 ms | 0,630 ms | 4,424 s |
| Metal | 6,763 ms | 0,680 ms | 0,851 ms | 4,430 s |

Il timer CPU include traversal/inizializzazione, rimozioni ISM/mapping e spawn
selettivo. Esclude integrazione differita Chaos, render thread e GPU; il tempo
di completamento usa il clock del mondo. La deadline1,5 ms vale solo per il
flood: ISM/spawn successivi hanno limiti di quantità, senza promessa di deadline
complessiva. Non è una garanzia FPS o multipiattaforma; dynamic resolution resta
attiva. `SupportBytes` misura array posseduti/weak reference, non heap totali
Chaos/detriti, risorse condivise, bucket temporanei di build o memoria GPU.

Stress Metal finale, pool dei due seed a chunk400: generazione mediana54,741 ms,
rimozione singola p95 0,039 ms, batch256 mediana1,942 ms, tracked4,094 MiB,
grafo iniziale mediano326.722 byte. Il workload distrugge tutti i blocchi nel
callback prima del timer di supporto: queste misure descrivono i costi ordinari
e il grafo in generazione, mentre il probe di collasso misura i distacchi.

Durante l'ultima verifica del CSV è stato rilevato un probe nativo compilato
prima dell'aggiunta delle colonne support/debris: i controlli del gioco erano
PASS, ma il parser rifiutava lo schema precedente. Ricompilazione del probe e
nuovo stress completo hanno confermato lo schema e tutti i subtotali; esito
finale exit0. Diagnostica iniziale conservata nel runner schema, esiti finali
in destruction-m5-final.json.

Artefatti locali in Saved/Testing: destruction-automation.log e destruction-report/,
destruction-game-collapse.log, destruction-gallery-collapse.log,
Collapse-Before.png e Collapse-Falling.png (ispezionate: breccia e frammenti
in caduta), destruction-m5-regression.json (nove exit0), destruction-m5-final.json,
destruction-benchmark-m5-final-metal.csv / -summary.json / .utrace. Receipt
pubblico [COLLAPSE_BENCHMARK.json](COLLAPSE_BENCHMARK.json) con metodo, limiti,
metriche/hash sorgenti e CSV; DESTRUCTION_BENCHMARK.json conserva la M4.

Limiti intenzionali: connettività alla base rigida locale, nessun calcolo di
carichi; solo campione cosmetico di blocchi simulato, restante overflow rimosso;
soffitti e pavimenti statici; shader world-position riapplica la texture durante
il moto; rete/late join, più piani e materiali gameplay aggiuntivi restano M6.

## Milestone 6 — replica, late join e tre piani (15 ottobre 2026)

Build Mac arm64 Development su UE5.8.3 CL58210709: riuscita. Suite finale undici
successi, zero fallimenti/non eseguiti: nove senza warning, due con gli stessi
warning dei mondi fixture senza world context. Nuovi MultiFloorLayout,
MultiFloorCollision e NetworkTransactions. Verificati seed per piano, sette
materiali, ID distinti/pavimenti disgiunti, clearance capsule con step-up,
ancoraggi superiori, pagine fuori ordine/future/parziali, transazioni atomiche,
payload invalidi, mismatch protocollo e reset epoch. Gameplay precedente,
identità/Chaos e18 cicli di stress automation restano PASS.

Migrazione Unreal del catalogo: added4/filled0/total7; bindings Wood/Stone/Metal/
Glass sulla libreria PBR esistente. Rerun added0/filled0/total7 e SHA della
uasset invariato. Dati/art delle tre righe originarie preservati, nessuna nuova
texture o modifica alla geometria salvata delle mappe.

| Prova multiplayer effettiva | Esito |
|---|---|
| Server + primo client | Stessa geometria/HP/stato, mapping nativo completo e collisioni reali campionate: PASS |
| Authority | Mutazione, rigenerazione ed esplosione dirette sul client rifiutate; suo RPC di fuoco posseduto consuma ammo e distrugge il blocco predisposto: PASS |
| Breccia | Secondo colpo client attraverso la breccia colpisce host (HP100→64); client attraversa col movimento reale, conferma server: PASS |
| Crollo | Authority rileva nove isolati; gli stessi stati Detached/collisioni sono applicati sui client: PASS |
| Late join | Secondo remoto entra dopo danni/crollo e riceve lo stato corrente (nessun replay dei vecchi detriti): PASS |
| Epoch | Rigenerazione da due a tre piani e successivo reset; snapshot vecchi esclusi, tutti i remoti ripristinano stato e collisioni: PASS |
| Scale | Primo client sale e scende due piani usando la movement replication ordinaria; conferma server/client: PASS |
| Snapshot denso | Ulteriore remoto entra dopo rimozione di tutte le17.427 parti di parete; stato completo applicato tramite137 pagine: PASS |
| Esplosione finale | 146 candidati,5 coperti,141 danneggiati e53 rimossi; hash/HP/collisioni corrispondono su tutti i remoti: PASS |

Due run finali indipendenti del probe esteso: NullRHI con3% incoming packet loss
+40ms incoming lag su ciascun remoto (valori confermati in tutti i log) e Metal
su host/primo remoto. Entrambi completati PASS con host listen e tre remoti,
quattro slot umani totali. In ogni run: epoch1 seed42/due piani/11.529 blocchi/
91 pagine; epoch2 seed7919/tre piani/17.427 blocchi/137 pagine; epoch3 reset
stesso layout. Dati di stato confrontati integralmente tramite digest; geometria
verificata con mapping completo e trace reale ogni13 record. È una prova locale,
non una certificazione su internet, dedicated server o piattaforme differenti.

In Metal il client ha attraversato la breccia fino a Z74,650 (asfalto20cm sotto
il pavimento), salito fino a Z734,650 e ridisceso a Z94,650. Percorsi e correzioni
sono del CharacterMovement reale, non teleport di prova attraverso le scale.
I teleport authority sono usati solo per predisporre gli scenari/reset. Durante
lo sviluppo la capsula ha rilevato un ingresso troppo basso e un ritorno vicino
alla scala superiore: corrette geometria/aperture. Il primo timeout LAN era la
tolleranza5cm del test post-teleport: lo stato dei blocchi era già identico;
correzione nativa e tolleranza20cm del probe hanno risolto l'attesa senza
introdurre RPC client di posizione.

Immagine Procedural-Multifloor.png ispezionata: tre livelli e superfici PBR
visibili. Il capture ha mostrato12FPS/85,4ms e scena803×452 dinamica in output
1280×720, con processi Unreal simultanei sulla stessa macchina. Questo carico
condiviso e il dato istantaneo non stabiliscono le prestazioni di una partita
isolata, né una promessa FPS. Non si dichiara una garanzia grafica o di banda.

Tutti gli undici comandi finali exit0: automation, network-loss, network-metal,
combat NullRHI, combat Metal, procedural Metal, collapse Metal, stress Metal,
LAN precedente con shot-feedback, accuracy e arsenal. Collasso conserva le
prove reali di gravità, cap globali, settle/scadenza e pulizia. Stress finale
Metal:54 cicli, due seed×tre chunk×warmup+otto campioni,2.544 righe e trace;
30 colpi reali, FX/culling, rigenerazione/EndPlay. Tutti i subtotali CSV includono
ora network_bytes e sommano al tracked; essendo questo stress standalone, le
pagine network non esistono e non misura il costo multiplayer.

Stress standalone a chunk400, pool dei due seed: generazione mediana52,656 ms,
rimozione singola p95 0,032 ms, batch256 mediana1,719 ms, query250 p95 0,010 ms,
explosion250 mediana0,447 ms, tracked4,094 MiB. Timer CPU sincroni, esclusi
Chaos differito/render/GPU; actor/canali e shared/GPU heap esclusi dalle
allocazioni possedute. I receipt M4/M5 restano osservazioni storiche.

Artefatti locali Saved/Testing: destruction-m6-build.log,
destruction-m6-materials.log e -materials-rerun.log, destruction-report/,
destruction-m6-validation.json (undici exit0), destruction-network-loss-*.log /
-report.json e destruction-network-metal-*.log / -report.json, immagine dei tre
piani, destruction-benchmark-m6-metal.csv / -summary.json / .utrace. Receipt
pubblico [DESTRUCTION_NETWORK_RESULTS.json](DESTRUCTION_NETWORK_RESULTS.json)
con protocolli/limiti, epoch/report dei peer, hash sorgenti/catalogo/CSV/log,
metodo e limiti.

Scope: un actor di mondo, max50k record e391 pagine, fino a tre piani con scale
statiche e branch>=250. Verifica di connettività, senza calcolo dei carichi/crollo
dei solai, streaming/packing arbitrario o NavMesh dinamica. I nuovi valori dei
materiali sono tuning gameplay iniziale. Binaries privati; nessun packaging,
commit o pubblicazione eseguiti in questa milestone.

## Menu/sessioni — 15 ottobre 2026

- Compilazione RespawnAndRageEditor Mac Development arm64 riuscita.
- Verifica grafica Metal del menu a tre colonne: partita, personaggio/armi,
  impostazioni; finestra indicata al 70% (2419×1564 iniziale sul Retina 2×).
- `python3 scripts/test-network.py`: PASS con host/client reali, incluse
  ricariche finite nel deposito, danno, respawn, punteggi e replica personaggi.
- `scripts/test-session.py` è il nuovo collaudo opt-in per avvio con un solo
  umano, squadre esplicite, bot per squadra, movimento senza sparo, bind UDP
  wildcard e tre ricariche consecutive da riserva zero nei laboratori.
- Durante il collaudo corretto lo spawn attorno ai PlayerStart: la ricerca
  del pavimento ignora i pawn e scarta appoggi con dislivello oltre 40cm.
  Il timeout client del probe è 60s per consentire il caricamento Editor.

Risultati conclusivi dei cinque scenari `test-session.py`: PASS (deposito
locale con bersagli fermi; deposito listen con bot solo movimento; movimento,
distruzione e procedurale listen con client reale). In ogni server verificato
UDP wildcard su 0.0.0.0, avvio solo, team Omega host/Alpha client, 2 bot Alpha
e 1 Omega. Nei tre laboratori riuscite tre ricariche consecutive da riserva
azzerata. Bot fermi: spostamento 0cm; bot movimento: avanzamento osservato
senza consumo del caricatore. `test-bots.py`: PASS Patrol e Target, inclusi
sparo, copertura, ricarica, morte/respawn, fine/riavvio round e replica client.
Gli spawn dei bot sono esclusi dal riposizionamento degli spawn umani procedurali.

Ripetizione finale `test-session.py --map L_ProceduralLab`: PASS dopo la
correzione dei punti di spawn individuali; build finale riuscita.


## 15 October 2026 — Killhouse reference template

- Read-only analysis of Workshop526448928 legacy ZIP: de_killhouse.bsp/BSP21,
  1584 brushes,95 static props,20 team spawns; Source converter reused without
  changes to the original csgopen checkout. Measurement/provenance JSON private.
- Editor arm64 Development build: PASS. New DA_Killhouse and L_Killhouse saved
  through Unreal in Assets/maps. Initial missing concrete-floor material fallback
  was corrected to the existing concrete_wall_006 material; final setup is clean.
- `python3 scripts/test-destruction.py`: PASS,12/12 including MapTemplate. Saved
  template tested at seeds0/1/42/7919/-17/INT32_MAX: deterministic IDs/transforms,
  unique records,20 physically clear supported spawns, circulation sweeps, real
  hit identity/removal, stable component count on regeneration and invalid-data
  rejection. Existing storage/combat/multifloor/network/support tests also pass.
- `python3 scripts/test-map-template.py --render --network`: PASS. Actual Metal
  host, initial remote and late remote after damage. Initial phases1/3/4 and late
  phases3/4 match full state digests, native mappings and sampled static/logical
  collision. Real pawn walks the side lane and climbs the watch platform stairs.
  Authorized rifle shot plus exposed-face explosion remove25 logical blocks;
  seed42→7919 resets damage and advances epoch1→2 coherently on both clients.
  No blocks detached in this exposure; general collapse remains verified by the
  existing automation suite. Original probe assumptions (single weakened cell
  despite weapon spread, blast origin coplanar with a wall and too-short client
  loading windows) were corrected before the successful run.
- Inspected actual Metal captures: industrial shell, floor, bunker partitions,
  containers, central stairs and overhead cutaway are visible. These show current
  substitute geometry/PBR surfaces, not imported Factory meshes or measured FPS.
- Existing regressions: `test-network.py`, `test-network.py --accuracy`,
  `test-arsenal.py`: all PASS with actual local host/client processes.
- Repeated setup: PASS, template/map preserved byte-for-byte; hashes and concise
  receipt in MAP_TEMPLATE_RESULTS.json. Capture/log details in Saved/Testing.
  No packet-loss run for this template, distribution or publication performed.

- Final `python3 scripts/test-map-template.py --render` after the HUD polish:
  PASS in standalone as well. Captures refreshed with the ordinary gameplay HUD;
  `-ERBlockDebug` opts into template block diagnostics. Inspected Spawn/Breach/
  Overview images; overhead cutaway hides only the ceiling for that debug capture.


## 15 October2026 — Factory art and balanced Killhouse revision2

- Source FactoryEnvironmentCollect completed:8.7GB content, UE5.6 source project.
  Read-only bounds/material inspection; selected migration via an isolated
  project, Unreal rename/save under private Art/FactoryEnvironment. Final closure
  15 root meshes/70 packages/546,899,871 bytes. `verify_factory_assets.py`: PASS,
  all70 packages load, dependency/material references resolve in the private
  namespace or Engine. No source Factory package saved. Early rename-registry
  dependency lag and stale staging conflicts were resolved with explicit closure
  migration and a fresh selection-keyed staging project before final validation.
- Mac Editor arm64 Development build: PASS. Template upgrade validates103
  distinct seeds before save. DA_ER_BlockMaterials SHA256 remains
  88ad4b5875c772a2c39ccacdbb3ed037ba8ad4e6ab3bdee4bc92b76cba9fee03.
- `test-destruction.py`: PASS12/12. MapTemplate verifies paired logical/fixed
  geometry and normalized spawns, deterministic IDs and exact damage-cell seams,
  six representative physical seeds, clear circulation, rear door access,
  field-facing firing windows, internal stairwell/headroom and walkable roof
  exits on both bases. A first overly literal floating-point panel grid failed
  pairing checks; centred generation corrected it before the final passing run.
- `test-map-template.py --render --network`: PASS. Metal listen host, initial
  peer and late peer agree on full state digests/mappings and sampled collision.
  Actual pawn traverses the side lane, enters Alpha from the rear/climbs its roof,
  then does the same for Omega, and climbs the central platform. Authorized shot
  and exposed-face explosion remove17/3366 blocks; seed42→7919 advances epoch1→2
  and restores the layout coherently. Initial phases1/3/4 and late phases3/4 PASS.
- `test-map-template.py --render`: PASS in standalone too, exercising both bases
  and central stairs with normal gameplay HUD. Inspected final Spawn, AlphaRoof,
  OmegaRoof and Overview captures. Solid corrugated modules replace an early
  open-frame selection for the shed. Overview is a deliberate roof cutaway;
  ordinary gameplay has the fixed roof. Observed render frames are not an FPS
  benchmark or guarantee. Fixed collision uses authored box approximations.
- Existing `test-network.py`, `test-network.py --accuracy`, `test-arsenal.py`:
  all PASS with real local host/client processes.
- Repeated setup+asset verification: PASS. Template, map and destruction catalog
  preserved byte-for-byte, including later art edits. Hashes in
  Saved/Testing/factory-preservation-before.json. Current concise receipt is
  MAP_TEMPLATE_RESULTS.json. No packet-loss run for this art revision, packaging,
  public asset upload or license verification claimed.


## 15 October 2026 — Killhouse revision3: solid surfaces and procedural composition

- Editor arm64 Development build: PASS. DA_Killhouse revision3 saved through
  Unreal after native validation of516 distinct seeds; map and material catalog
  preserved. No additional vendor package imported.
- `test-destruction.py`: PASS12/12. MapTemplate now samples512 composition seeds:
  248 arena dimensions,450 module compositions,511 base frames/proportions, four
  room counts,3052..7064 logical blocks. Six physical seeds verify symmetry,
  deterministic IDs, clear spawns/full outer loop, standing room access, front
  windows, stairs/roof exits and solid roof samples matching visible cube geometry.
  Invalid range/open-art slab inputs reject atomically.
- The first stair fixture incorrectly tested a capsule only7.5cm above each
  30cm tread, intersecting the next riser during step-up. Use the existing lab's
  extra20cm clearance for tread checks; real pawn movement remains unmodified.
- `test-map-template.py --render --network`: PASS. Real pawn crosses both bases,
  rear entrances and roofs, and the watch platform. Authorized shot/explosion
  removes43 blocks. Initial phases1/3/4 and late phases3/4 match full state,
  mappings and sampled collision. Seed42->7919 changes3750x7400 to3800x8700cm,
  epoch1->2, and resets damage.
- `test-map-template.py --gallery`: PASS. Three compositions rendered at7919,7,113;
  final down-facing roof captures inspect solid decks/treads and guarded opening.
  Actual captures inspected. No isolated FPS/CPU/GPU or memory guarantee inferred.
- Existing `test-network.py`, `test-network.py --accuracy`, `test-arsenal.py`: PASS.
- Read-only preservation rerun + `verify_factory_assets.py`: PASS70 packages;
  template, map and destruction catalog hashes unchanged. Details/hashes in
  MAP_TEMPLATE_RESULTS.json and Saved/Testing/variation-preservation.json.

No packet-loss sweep, packaging or publication in this revision.


## 15 October 2026 — Narrow Killhouse roof parapets

- Editor arm64 Development build: PASS. FindLedge now samples near the contacted
  face and follows a bounded continuous top interval. The100cm-high/20cm-thick
  parapets receive a supported landing; broad obstacles retain the old distance.
  Jump velocity,30/80/150cm thresholds, standing clearance, swept movement and
  saved-move prediction remain unchanged. No asset binary modified.
- `test-destruction.py`: PASS12/12. MapTemplate now checks48 ledge queries:
  four sides x two bases x six seeds. Explicit climb input required, height
  relative to roof verified and landing centre lies within the20cm top strip.
- `test-parapets.py`: PASS16 server-verified cases (eight host/eight remote),
  plus8 autonomous-client confirmations. Uses the ordinary Space input binding
  and movement. Server only places the initial roof fixture; no target teleport.
  Final feet settle102.1..102.2cm above the roof on all four sides of both bases.
- The initial graphical run stopped when its viewport closed after three local
  cases; its first mounted screenshot was inspected. A headless attempt found
  the idle host occupying the final remote landing, correctly rejected by body
  clearance. Park the host off the roof before remote trials; complete rerun PASS.
- `test-movement.py`: PASS, including20 block heights, real remote80/150/200cm
  traversal, jump envelope, ceiling rejection, posture/sprint and actual InputKey
  bindings. `test-network.py --accuracy`: PASS on server and owner.
- Map/template/catalog SHA256 match the revision3 preservation receipt exactly.
  Detailed logs: Saved/Testing/parapet-{host,client}.log, parapet-report.json,
  movement-{host,client}.log and accuracy-{host,client}.log.


## 15 October 2026 — Movement-only bot demonstrations

- Editor arm64 Development build: PASS. Server AI now requests ordinary jumps,
  nearby obstacle traversal, crouch, prone, crawl and standing again. Posture
  cycles last22s and use explicit roster offsets. Movement physics and assets
  remain unchanged; traversal still obeys normal collision/height checks.
- `test-bot-motion.py`: PASS with two real bots (one per team), listen server
  and remote LAN observer in the existing MovementLab. Each bot independently
  reached evidence mask127 on both peers: upward jump, actual traversal, full
  crouch, prone capsule with horizontal visible world mesh, crawling, standing
  after prone and mounting the80/140cm fixtures. No shots/ammo consumption.
  Team appearance must be ready before observation and both bots ready at report.
  Only initial test lives/positions are reset; subsequent actions use normal AI.
- `test-bots.py`: PASS for Patrol/Target, including remote team appearance,
  death/respawn and round restart. `test-session.py --map L_Depot`: PASS on the
  final module for local stationary and movement listen sessions, bot movement,
  no firing, roster and real remote join/team selection.
- Verification is headless gameplay/replicated state and mesh-transform evidence;
  no new rendered animation capture or NavMesh/pathfinding guarantee claimed.
  Details: BOT_MOTION_RESULTS.json and Saved/Testing/bot-motion-{host,client}.log.


## 15 October2026 — Dust Mirror desert reference template

- Source2 analysis: Workshop3117367319 verified as Dust2(Mirror),561 entries
  inside the nested main VPK,15 spawn entities/team. CRC-verified read, entity
  export and geometry inspection with official Source2Viewer-CLI20.1. Original
  CS2 materials are outside this Workshop package; no Source2 mesh/texture
  imported into Unreal. Private analysis/assembly manifests retain provenance.
- Selected MOUT_Arabian migration through Unreal:27 vendor packages, source
  mount read-only, intermediate namespace rename/save and final private mount.
  The imported Substance instances had missing parents: four owned PBR parents
  rebuilt from their existing color/normal/roughness/AO/metallic maps. World
  tiling and normal transforms follow the existing procedural material approach.
  Final dependency/load verification: PASS31 packages, including all four parents.
- Editor Mac arm64 Development build: PASS.515 candidate seeds validate before
  save. The first physical test caught a continuous parapet blocking the stair
  mouth; revision2 opens both opposing entrances. A test key also normalized
  signed zero before the final symmetry check.
- Destruction suite: PASS13/13. DustTemplate checks512 seed builds with cover
  variation and six physical seeds, paired fixed geometry, clear starts, both
  orthogonal crossing levels,320cm/0cm support, narrow gates that pass at centre
  and block at their leaves, and both solid stairways/headroom. Existing
  Killhouse, storage, combat, collision, support/collapse and network tests pass.
- test-dust-template.py --render --network: PASS. Actual pawn passes both timber
  gates, walks under the bridge, climbs the stairs and crosses the upper route.
  An authorized real shot damages project wood; exposed-face explosion/collapse
  preserves mappings. Seed42->7919 restores cover and advances epoch1->2. Initial
  peer phases1/6/7 and late peer6/7 match full state digests and sampled collision.
  The probe keeps snapshot phase independent from movement phase so an initial
  peer can still verify after map loading. Geometry was final; subsequent PBR
  parent repair changes appearance only.
- Existing test-network.py, test-network.py --accuracy and test-arsenal.py: PASS.
- Setup preservation: PASS. Repeated template, lighting and PBR migrations skip
  completed versions; map/template/catalog/Killhouse hashes match saved receipt.
  Destruction catalog SHA256 remains
  88ad4b5875c772a2c39ccacdbb3ed037ba8ad4e6ab3bdee4bc92b76cba9fee03.
- Limits: simplified balanced60x80m adaptation, not every original DustII route.
  Seed variation currently changes paired cover offsets; crossing/gates and arena
  dimensions are invariant. Fixed art collision is authored box approximation.
  No performance benchmark, packaging, publication or acquisition-license review.

- Final `test-dust-template.py --render` after PBR repair: PASS. Actual local
  traversal/combat/seed reset repeats successfully. Inspected final Doors, Upper
  and Overview captures: stone/wood/metal maps render instead of the parentless
  material fallback; outdoor sky and light reach the procedural geometry.
  Full-map exterior beyond the arena remains unbuilt. Summary receipt:
  DUST_TEMPLATE_RESULTS.json; final private package hashes in
  SourceAssets/ArabianEnvironment/final_manifest.json.


### Dust town revision3 — 15 October2026

- Replaces the earlier fixed symmetric arena with the user's reference-plan
  town: seven named routes, six courts, solid building parcels, a bent Short A
  crossing CT_A at320cm, and three inward-open timber gates under curved arches.
- Editor Mac arm64 Development build: PASS. Setup validates515 candidate seeds.
  Destruction automation:13/13 PASS. DustTemplate validates512 unique town plans,
  202 arena proportions,42..46 parcels and seven variable street widths, plus
  six physical seeds (0,1,42,7919,-17,INT_MAX). Tests cover full route capsule
  sweeps, spawns, centre gate passage/blocked leaves, stairs and both levels.
- `test-dust-template.py --render --network`: PASS. Actual pawn crosses all
  three doors, walks CT under Short A, climbs and follows the complete bent
  upper route to A, fires an authorized shot and causes exposed-face blast
  damage/collapse. Seed42->7919 resets cover and advances epoch1->2. Initial
  peer phases1/6/7 and late peer6/7 match state digests and sampled collision.
  Digests:1=3896086461,6=1825454632,7=3603509753.
- Existing ordinary LAN, accuracy and arsenal regressions: PASS.
- One-time art migration adds three owned meshes and five town materials.
  Final material-only projection repair connects all17 texture samples in four
  owned PBR copies to the actual Unreal `UVs` input. Eight-package asset, bounds,
  dependency and UV-connection verification: PASS. A new rendered gate capture
  checks the final material repair; the earlier full gameplay/network run used
  the same final geometry and gameplay, before this shader-only correction.
- Completed setup reruns preserve artist changes. Protected Killhouse map and
  template, Dust map/template and shared catalog hashes match the saved receipt;
  the projection repair changes only the four owned town materials. Catalog SHA256:
  88ad4b5875c772a2c39ccacdbb3ed037ba8ad4e6ab3bdee4bc92b76cba9fee03.
- Limits: semantic adaptation of the supplied plan; seeds reshape geometry while
  preserving its route graph. Buildings are solid closed parcels. Curved arch
  collision uses24 strips and leaves use oriented plank proxies. No performance
  benchmark or publication. Receipts/logs: `Saved/Testing/dust-town-*`,
  `dust-network-metal-report.json`; summary: `DUST_TEMPLATE_RESULTS.json`.

- Final `test-dust-template.py --render` after the town UV repair and the
  probe's delayed gate capture: PASS. Actual three-gate traversal, lower route,
  full bent upper route, authorized shot, blast/collapse and seed rebuild all
  repeated successfully. Inspected final wooden-gate capture confirms vertical
  timber grain, inward-open leaves, iron braces and the arched surround.


### Dust town revision4 — compact plan and opposite-swing doors,15 October2026

- Editor Mac arm64 Development builds: PASS. Compact migration validates515
  candidate seeds before saving66..75x73..84m dimensions and42 patrol actors.
  Approximately13% reduction per axis,24% less area at the range midpoints.
- Destruction suite:13/13 PASS. DustTemplate:512 unique compact plans,187 arena
  proportions,40..46 building parcels and seven sampled gate-street widths.
  Six physical seeds verify all street routes, standing clearance/spawns, stairs,
  crossing heights and the diagonal door passage in both directions. Nine
  frontal standing-eye rays per door from each side are blocked, as is straight
  frontal capsule motion. The safe six-point dogleg remains open.
- Rendered host traversal passes all three opposite-swing gates, CT underpass,
  complete bent Short A, authorized firing, blast/collapse and seed replacement.
  Initial network attempt exposed a probe timing defect: the underpass sweep
  hit the replicated host pawn while it was crossing. Environment checks now
  explicitly ignore all pawns. The initial failure/report is retained in
  Saved/Testing/dust-compact-first-{host,initial,network-report} files.
- Ordinary LAN, accuracy and arsenal regressions: PASS.
- All four completed setup scripts rerun without modifying13 protected packages
  (town art, Dust map/template, Killhouse map/template and shared catalog).
  Receipt: dust-compact-rerun.log, ER_DUST_COMPACT_RERUN_PASS.
- Limits: same semantic reference-plan graph and closed building parcels;
  static half-open doors with approximated plank/arch collision. No performance
  benchmark or publication. Final network/bot outcome is recorded below.

- Final `test-dust-template.py --render --network`: PASS after the probe fix.
  Initial peer1/6/7 and late peer6/7 agree; digests1=3399067073,6=4238296378,
  7=3855062917, epochs1->2. Rendered frontal capture shows overlapping timber
  leaves, while the real pawn completes all three diagonal passages.

- Bot integration: PASS for Patrol and Target modes, including team appearance,
  remote health/death/score and round restart. Final capture-only change holds
  the frontal view for one extra second and ignores live mouse look before
  the probe starts its dogleg;
  map geometry and gameplay are identical to the successful full LAN run.

- Final capture-only preview: PASS, DUST_TOWN_VISUAL_CAPTURE_READY. Inspected
  DustTown-OppositeSwingGates.png with fixed frontal view: opposite-swing timber
  leaves hide the standing-eye sightline. This preview does not repeat the
  completed full traversal/LAN test. Final source/asset diff checks: PASS.

## Phase Omega / Belica Alpha — 15 ottobre 2026

Importazione locale completata con cinque nuove varianti per personaggio,
aggiunte agli indici 5–9 delle rispettive squadre. Catalogo totale: venti skin.
Originali completi conservati nel submodule privato characters, agli stessi
percorsi /Game: SHA-256 dei 3.276 file originali invariati. `setup-assets.py
--check`: 26 mount corretti, nessun puntatore LFS irrisolto.

- Compilazione reale RespawnAndRageEditor Mac Development arm64: **Succeeded**. Restano
  avvisi di API deprecate già presenti in ERFirstPersonForearms/ERShotEffects;
  nessun errore di compilazione nel risultato finale.
- Verifica Editor dei riferimenti Character/TrainingTarget/Corpse: dieci skin
  per squadra, nomi/mesh distinti, animazioni, materiali, socket e rig salvati.
  Verifica FPS: venti mesh, undici armi, 1.100 campioni, entrambe le impugnature
  coincidenti con il rig di riferimento; posizione/scala della camera conservate.
- Gallerie Metal: venti silhouette in pagine da cinque e venti visuali FPS
  (`ER_FPS_FOREARM_FRAME_PASS` ×20), oltre a prima persona e due pose prone
  dei nuovi personaggi. Anteprime locali: Phase-Belica-{Alpha,Omega}-Final.png.
- Ragdoll finali: cinque coppie nuove e una coppia originale, tre campioni
  per coppia, **36 controlli superati**, comprese ossa/giunti, accessori nascosti,
  assestamento e concordanza tra posa fisica e copia visibile Phase. PNG finali
  in Saved/Testing/Final-Corpses-{5..9,0}-{1..3}.png, ispezionate realmente.
  I primi controlli numerici sulle sole ossa non bastavano a valutare l'aspetto:
  la verifica visiva ha portato a posa visibile esplicita e limiti articolari
  Phase più contenuti, evitando gambe e polsi eccessivamente ripiegati.
- `test-target-skins.py`: venti bersagli verificati sul listen host e sul client,
  dieci per squadra; mask Alpha/Omega 1023 su entrambi, armi e immobilità corrette.
- `test-network.py --phase-belica` e `--skins`: **PASS** nella versione finale,
  34 controlli server per esecuzione più conferme client. Selezione valida,
  rifiuto indici fuori catalogo, applicazione al respawn e skin precedente
  conservata dal cadavere; danno, protezione, ammo e feedback restano verificati.
- Regressioni eseguite durante l'integrazione: bot **PASS**, precisione **PASS**,
  arsenale **PASS**. Nessun esito headless viene usato come prova di qualità
  visiva o audio; le immagini sopra provengono da processi Metal reali.

Le migrazioni usano marker per conservare i successivi ritocchi dell'artista.
Il riavvio degli script già applicati è stato verificato senza rigenerare gli
asset. Il renderer Phase aggiunge una componente cosmetica senza collisioni,
aggiornata dopo la fisica in un batch; fisica e copia visibile vengono congelate
all'assestamento dopo cinque secondi. Restano sedici corpi/quindici giunti,
WorldStatic soltanto, massimo dodici cadaveri e novanta secondi di durata.
Non è stato eseguito un benchmark del costo delle copie di posa.

Rimangono le limitazioni delle geometrie Paragon, soprattutto gonne/giacche corte
Phase e armature futuristiche Belica. Sono stati adattati materiali, accessori,
rig e pose; non sono stati modellati corpi o uniformi sostitutivi. Originali e
derivati restano privati; nessuna verifica indipendente di ricevute/licenze Fab,
packaging o pubblicazione è stata effettuata in questa richiesta.

### 15 ottobre — correzione testa Phase Omega 9

Il bordo isolato proveniva dalla calotta del casco con winding rivolto verso
l'interno. Creata una copia PhaseEngineer con facce esterne corrette, materiale
opaco a due facce e bordo rialzato; aggiornati Character/TrainingTarget/Corpse
senza modificare il casco condiviso delle altre skin.

- Compilazione Editor Mac Development arm64: **Succeeded** (build correzione
  33,65s; successivo aggiornamento galleria 8,94s).
- Verifica fresca Editor e doppia esecuzione migrazione: **PASS**, cataloghi
  coincidenti, migrazione idempotente; venti mesh FPS/undici armi/1.100 campioni
  con entrambe le impugnature allineate.
- Metal reale: dettagli testa frontale/lato/retro **PASS**, immagini realmente
  ispezionate (`Head-Omega-9-{0,90,180}.png`). La calotta è intera e la fronte
  non presenta il precedente bordo isolato. Anche la galleria pagina1 completa
  passa dieci controlli; anteprime Phase-Belica-{Alpha,Omega}-Final.png aggiornate.
- `git diff --check`, parsing dei quattro script interessati e mount asset:
  **PASS**, ventisei mount e nessun puntatore LFS irrisolto.

Log locali: build-phase-helmet-fix.log, build-phase-helmet-gallery-final.log,
verify-phase9-fix.log, phase9-head-fixed-report.log e phase9-catalog-report.log.
Nessuna modifica alla logica di rete/gameplay e nessuna nuova esecuzione LAN
richiesta per questa correzione cosmetica.

### 15 ottobre — nomi omogenei e pulizia dei personaggi approvati

Venti nomi nel formato `personaggio · ruolo`, stessi indici, cataloghi
Character/TrainingTarget/Corpse concordanti. Rimossi tramite Unreal 3.210 package,
2.329.822.505 byte (2,170 GiB), in due lotti da 3.136 e 74. Il primo lotto è stato
provato prima in una fixture senza quei file; il secondo segue la sola rimozione
di riferimenti editor e di ombre a capsule disattivate, mantenendo la resa attiva.

- Compilazioni Editor Mac arm64 Development: **Succeeded**, 30,80s e 17,58s.
  Nessun nuovo errore C++; rimangono gli avvisi di deprecazione già documentati.
- Editor fresco finale: **PASS** sulle 20 skin e sui tre cataloghi; 1.100 campioni
  di grip, 11 armi, entrambe le mani, rig e framing conservati. Migrazioni già
  applicate saltate senza rigenerare asset. Caricate tutte le 7 mappe del progetto.
- Metal reale:20 silhouette e20 visuali FPS **PASS**; quattro gallerie finali e
  contatto FPS realmente ispezionati. Dopo l'ultimo lotto ripetute entrambe le
  pagine di silhouette. Due coppie di ragdoll (indici 0 e 8, quattro personaggi),
  tre campioni per coppia: 12 controlli **PASS**, incluse pose assestate e
  concordanza del renderer Phase. Le due immagini finali dei cadaveri sono
  state ispezionate. Nessuna pretesa di nuova verifica di tutti e dieci i ragdoll.
- `test-target-skins.py`: host e client 20 bersagli, mask 1023 per squadra **PASS**.
  Primo tentativo fallito: l'host terminava prima dei 5s del client appena entrato
  a freddo. La finestra del solo probe passa da 20 a 35s; successivo test passa su
  entrambi. Nessuna modifica ai tempi delle partite ordinarie.
- `test-network.py --phase-belica` e `--skins`: **PASS** (selezione, replica,
  rifiuto indici invalidi, respawn, cadavere con skin precedente e fisica).
- Audit fresco dopo le cancellazioni: nessun ulteriore package personaggio
  eliminabile; 34 sono conservati per referencer esterni. Rig/pose FirstPerson
  esplicitamente esclusi dalla rimozione. Mount 26, nessun puntatore LFS irrisolto.
- Archivio finale: 3.314 membri verificati SHA256 e digest dell'archivio completo.
  Tutti i 3.210 file rimossi sono coperti; 103 asset prima delle modifiche e la
  provenienza sono recuperabili. 1.198 file conservati restano identici SHA256;
  aggiornamento della sola documentazione di provenienza registrato a parte.
  Fixture e archivio completo ridondante eliminati. Nessun prune Git/LFS.

Ricevuta versionabile: CHARACTER_CLEANUP_RESULT.json. Log: Saved/AssetAudit/
character-cleanup-{build,final-build,delete,delete-final}.log e Saved/Testing/
character-cleanup-{final-verify,graphics-report,final-galleries-report,network-report}.log.
La riduzione dichiarata riguarda i file attivi del progetto; il backup privato
esterno occupa spazio. Nessun cook/packaging, benchmark o pubblicazione eseguito.


## Requested Sketchfab arsenal — 15 October 2026

- Real Mac arm64 Editor compilation succeeded on UE5.8.3. The imported Italian
  request, English reference and downloaded worksheet snapshot are retained.
- Native `EclipseRecoil.Weapons.ReferenceBallistics`: Success, including Source
  distance units, compounded attenuation, armor depletion and full penetration.
- `test-reference-weapons.py`: PASS,38 server checks, all10 owning-client key
  selections observed on the server and all10 remote model observations. Includes
  six shotgun pellets/one shell, timed reload/reserve transfer, holster cancel,
  non-consuming short knife query, model-derived physical launcher projectile and
  an actual AK-47 hit matching the worksheet's distance/armor formula.
- `test-reference-weapons.py --render`: PASS,45 server checks and17 fresh Metal
  captures (ten idle viewmodels plus seven actual mid-reload poses). The rendered
  AR15/SMG direction and grip corrections use fresh V2 skeleton/clip packages;
  Interchange's mesh-only reimport had retained older animation packages. All17
  final captures were inspected in Saved/Testing/ReferenceWeapons. The fitted
  hand retarget clamps some reaches; this does not certify authored hand/finger
  contact throughout every animation frame. No full-body/world reload retarget
  or grenade-set throw animation is claimed.
- `test-network.py --shot-feedback`: PASS on the explicit original-arsenal
  fixture, including combat, death, respawn, corpse/team replication and feedback.
- `test-arsenal.py`: PASS on the legacy fixture, including primary selection
  at respawn, shotgun pellets, physical rockets, HE/smoke/stun fuses, replicated
  debris/smoke, shot carving, wheel gating and effect cleanup.
- `test-network.py --accuracy`: PASS with the new default catalog;15 logged
  server/owner checks cover real spread, burst recovery, rejected empty shots and
  replicated owner state.
- Saved-asset audit: PASS, nine original ZIP SHA-256 receipts, ten data bindings,
  fitted FPS skeleton compatibility, weapon clip skeletons, mechanical reload
  motion and blueprint catalog references. Receipt: REFERENCE_WEAPON_RESULTS.json.
  Includes first-generation private derivatives retained for comparison; active
  catalog geometry is ten viewmodels plus the source GL_slug1 projectile.
- `setup-assets.py --check`: PASS,27 mounts and no unresolved LFS pointers.

The native automation log contains UE's built-in startup error-test messages
before the requested test; its actual ReferenceBallistics result is Success.
The initial graphical run encountered first-equip shader stalls; the final
probe waits for the owning client's actual equip/capture acknowledgment and
shader completion. Ordinary gameplay keeps its existing resolution policy;
the gallery alone fixes primary resolution to100%. These captures are visual
checks, not a performance benchmark. Existing character material-usage warnings
were observed while loading the unchanged skin packages.

## Weapon grip, idle and material correction — 15 October 2026

- Editor arm64 Development build: PASS. No gameplay aim/camera/capsule changes.
- `verify_sketchfab_weapons.py`: PASS. Every mechanical idle bone remains fixed;
  fitted forearm transverse scales are .82/.95; all seven compressed hand reloads
  reproduce their recalculated palm trajectories within 0.0001 cm at sampled keys.
  These are palm-target checks, not proof of authored finger contact throughout.
- `test-reference-weapons.py --render`: PASS, 51 server checks, all ten real
  client key selections, six world support-hand contact checks, seven reloads,
  actual armor/range damage and physical Milkor projectile. All 17 fresh Metal
  captures were inspected in Saved/Testing/ReferenceWeapons. Milkor is textured
  and has no scope; the shotgun now displays its supplied blue/wood material.
  Active weapon materials compile successfully on Metal on both peers.
- Native `EclipseRecoil.Weapons.ReferenceBallistics`: Success.
- `test-target-skins.py`: PASS on host and client, 20 stationary targets, both
  complete skin masks and all ten weapon slots. All 12 two-handed targets meet
  the fore-end contact segment within 1 cm on both peers. Full ammo is checked on
  the server; weapon ammo intentionally uses owner-only replication. The probe
  now waits for the client's explicit validation instead of a fixed host delay.
- `test-bots.py`: PASS for Patrol and Target on the explicit legacy fixture,
  including replicated team meshes, firing/idle behavior, reload, death, respawn
  and round restart. The new default exhibition catalog is covered separately
  by the twenty-target test above.
- `test-network.py --shot-feedback`: PASS on the legacy fixture.
- `test-arsenal.py`: PASS on the legacy fixture, including payloads, fuses,
  replicated HE/smoke, shot carving, selection and cleanup.
- `test-network.py --accuracy`: PASS on the new default catalog, including real
  spread, burst recovery, rejected empty shots and replicated owner state.
  Final build/audit/automation logs and the regression receipt are retained in
  Saved/Testing/weapon-presentation-*.

The first render exposed missing OneMinus inputs in supplied glossiness-derived
material graphs; those were repaired and the final graphical regression rejects
any active weapon material compilation failure. Source 24/60 Hz gun clips are now
resampled to the same 30 Hz grid as hands, avoiding Interchange's fractional-frame
key-count ensure. Equipping refreshes stationary cached world poses. Shorter rigs
can move their support palm along a 10 cm fore-end segment while retaining arm
lengths. Fingers remain the fitted rig's existing poses. The static grenade set
still has no authored throw animation.

## Actual palm surfaces and centered pistol — 15 October 2026, later revision

The user's follow-up screenshots showed that the earlier nominal HandGrip/segment
checks did not establish contact with the visible palm. This revision measures
each skin's palm geometry and raycasts the idle-deformed rifle triangles.

- Final Mac arm64 Development Editor build: PASS.
- `test-reference-weapons.py --render`: PASS. Eighteen fresh Metal captures,
  including a separate Alpha pistol view, were inspected. The running FPS pose
  now also checks that the pistol elbow is below the wrist and toward the center.
  Five rifle support palms meet their sampled surfaces; Milkor retains its
  previous support setup. All ten slots, seven reloads and combat checks pass.
- Saved-asset audit: PASS. Forty owned world/FPS meshes have both palm sockets;
  the saved pistol handle meets its palm attachment and the elbow is lower.
  Compressed reload trajectories still pass. The sniper explicitly matches the
  AWP row: damage115, interval1.46s, ArmorRatio1.95, RangeModifier0.99.
- Native `EclipseRecoil.Weapons.ReferenceBallistics`: Success.

An intermediate capture exposed runtime skeleton translation retargeting
discarding the pistol's hidden-shoulder translation. The final FPS pistol
instance retains the fitted animation translations; switching weapons restores
the existing retarget behavior. The final screenshots verify this correction
with both Alpha and Omega forearms. Finger poses remain those of the fitted rig;
these checks do not certify authored finger contact throughout every frame.
Logs and the final regression receipt are in Saved/Testing/weapon-palm-*.

`test-target-skins.py`: PASS on host and client with20 stationary targets,
both ten-skin masks and all ten weapon slots. All12 two-handed targets meet
their configured surface/segment within1cm. Server-side full ammo and the
non-owner ammo replication boundary remain intact.

`test-network.py --accuracy`: PASS on an isolated unchanged rerun, including
owner ammo/bloom replication. The first run failed
`stopped_burst_recovers_gradually` during a slower host observation window;
the other precision checks passed. Both attempts are retained in the regression
receipt and weapon-palm-first-* logs. No spread/recovery gameplay setting was
changed to obtain the rerun result.

`test-network.py --shot-feedback` and `test-arsenal.py`: PASS on the explicit
legacy fixture, including both feedback perspectives, damage/death/respawn,
team corpses, payload fuses, replicated HE/smoke, carving and effect cleanup.

## Pistol angle restored and closer framing — 15 October 2026, latest revision

The user rejected the steep centered pistol arm. Restored the original fitted
arm/wrist pose and previous weapon orientation, retaining measured palm contact.
The pistol view offset moves6cm toward the camera. Rifle/world presentation
and combat settings retain the previously tested behavior.

- Editor arm64 build: PASS.
- `test-reference-weapons.py --render`: PASS, including the relaxed running
  forearm check, ten slot selections, seven reloads and combat checks. Inspected
  fresh Alpha/Omega pistol idle captures and the pistol mid-reload capture.
- `verify_sketchfab_weapons.py`: PASS, including previous weapon angle, measured
  handle contact and compressed reload trajectories. No further gameplay changes.
  Logs: Saved/Testing/weapon-pistol-relaxed-*.

## Pistol cant, muzzle alignment and thumb grip — 15 October 2026

- Editor arm64 build: PASS. The editor-only geometry query can measure an
  evaluated idle pose; ordinary combat and source statistics are unchanged.
- `test-reference-weapons.py --render`: PASS. Fresh Alpha/Omega pistol idle and
  pistol reload captures inspected. Runtime checks pass for the3.5mm palm embed,
  the thumb tip at the opposite grip surface and the relaxed forearm. Actual
  projected muzzle positions at1280x720: pistol(755.5,485.6), assault(739.4,470.1).
  Camera depths are40.0cm and74.8cm respectively. Ten slots, seven reloads and
  the existing combat checks pass too.
- `verify_sketchfab_weapons.py`: PASS, including actual absolute weapon scale,
  new wrist angle, saved thumb fit, measured muzzle framing and compressed reload
  trajectories in the new motion frame. Logs/receipt: Saved/Testing/weapon-pistol-thumb-*.

The first comparison exposed an incorrect inherited-scale assumption in the
offline fit: FPS hands are scaled while the rifle component retains absolute
scale1. The final pistol attachment compensates its translation, and pistol
retargeting uses actual weapon scale1 with the canted basis. The new runtime
checks verify contacts against the real component transform, rather than only
comparing offline targets. Other weapon trajectories retain their prior setup.

## Pistol viewer reference — 15 October 2026, final image fit

The user's20:06 viewer screenshot supersedes the earlier assault-tip framing.
The final fit keeps an upright grip, a46cm muzzle depth and a lower-right
screen anchor. Camera-relative pitch4.045°/yaw-4.149°/roll0° converge the barrel
on the central aim ray at150cm; authoritative aiming and shot origin are unchanged.
The palm and thumb now meet the upper grip5cm above the earlier contact height,
keeping the3.5mm embed and original hand geometry/segment lengths.

- Editor arm64 build: PASS (10.95s). Pistol-only Unreal import: PASS.
- `test-reference-weapons.py --render`: PASS (106.0s), with18 fresh Metal
  captures, ten slots, seven reloads and the existing server combat checks.
  Runtime pistol muzzle projects to(744.96,462.60) at1280x720. Convergence,
  zero roll, palm contact, thumb contact and relaxed forearm checks pass.
  Inspected both Alpha/Omega pistol idle views and the pistol reload capture.
- `verify_sketchfab_weapons.py`: PASS (24.4s), including compressed saved
  pistol motion, source statistics, catalog bindings, forty palm-socket meshes
  and original archive hashes. Python syntax and scoped diff whitespace: PASS.

Logs/receipt: Saved/Testing/weapon-pistol-reference-*.
Final preview: Saved/Testing/ReferenceWeapons/Pistol_Alpha_Reference.png.
The private source reference is retained unchanged in
SourceAssets/SketchfabWeapons/References/pistol-viewer-2026-10-15.png.
This matches composition approximately using the existing skins' hand shapes;
it does not replace their geometry with the viewer's glove.


## 15 ottobre 2026 — Respawn & Rage: trasferimento e nuovo nome

- `scripts/build-editor.command`: **PASS**, target RespawnAndRageEditor, Mac
  Development arm64, UE 5.8.3. Compilazione/link completati; presenti avvisi di
  deprecazione Unreal nei preesistenti helper di mesh/Niagara.
- `python3 -m unittest discover -s scripts/tests -v`: **15/15 PASS**.
- `python3 scripts/setup-assets.py --check`: **PASS**, 27 mount e nessun
  puntatore LFS irrisolto dopo il trasferimento del progetto e delle cache.
- `python3 scripts/test-network.py --shot-feedback --skins`: **PASS** con due
  processi Unreal effettivi. Verificati rete, danno, respawn, corpi, selezione
  skin e asset/feedback di sparo per host e client. Output locale:
  `Saved/Testing/rebrand-network-result.log`.
- `zsh -n` sui tre launcher e `git diff --check`: **PASS**.
- Avvio reale `scripts/play.command`: menu Metal e titolo finestra
  **Respawn & Rage**, sottotitolo **Serious guns. Questionable players**
  verificati visivamente. Cattura locale `Saved/Testing/RespawnAndRage-menu.png`.

Le namespace interne delle classi e degli asset sono state mantenute per
compatibilità; nessun package binario è stato rinominato o modificato.
Il prototipo precedente resta nella sua cartella, con remote e contenuti invariati.


## 16 ottobre 2026 — Set personaggi a tre viste

Compilazione Editor Mac arm64 riuscita. Eseguito `scripts/capture-character-set.py`
con rendering Metal: processo terminato con codice 0 e marker
`ER_CHARACTER_SHOTS_COMPLETE count=36`. Verificati 36 PNG a 1600×1200:
Twinblast, Belica, Drongo e Phase, skin 0/2/4 e 5/7/9, fronte/lato/retro.
Arma casuale mantenuta per ogni terna, registrata in `manifest.txt`.
Controllate visivamente le quattro tavole riepilogative e le inquadrature singole
durante la regolazione del fondale. Output in `Saved/Screenshots/CharacterSet`,
archivio `Saved/Screenshots/Personaggi-36-screenshot.zip`.
Il log segnala usage flag materiali mancanti su alcune mesh; nessuna modifica
ai package asset e nessun test di rete richiesto per la sola modalità di cattura.


## 16 ottobre 2026 — Splash, logo e icona

Compilazione `RespawnAndRageEditor Mac Development arm64` riuscita.
Avvio reale `-game` con Metal e cattura nativa `shot showui`: logo visibile
nel menu, proporzioni e trasparenza preservate. Evidenza locale:
`Saved/Testing/Branding-Menu.png`, log `Saved/Testing/branding-capture.log`.
La prima verifica mostrava un rettangolo bianco; corretto usando una
`FSlateDynamicImageBrush`, poi ricompilato e verificato nella cattura finale.
SHA-256 degli originali copiati identici ai tre file forniti in Downloads;
PNG/ICNS/ICO generati riaperti e verificati con Pillow. `zsh -n` sul launcher
e `git diff --check` superati. Solo `Content/Splash` è ammesso da Git;
i mount Content degli asset e SourceAssets rimangono ignorati.
Splash gioco/Editor e icone native configurate secondo i percorsi UE 5.8
verificati nel sorgente locale; splash transitoria e Dock non certificati
visivamente dalla cattura del viewport. Nessun packaging o test di rete
eseguito per questa modifica di presentazione.


## 16 ottobre 2026 — Sfondo della finestra e menu trasparente

Compilazione Editor Mac arm64 riuscita. Avvio reale Metal su L_MainMenu
con `-NoSplash`: sfondo visibile a finestra intera, anche dopo il cambio
di dimensione applicato dalle preferenze, pannelli neri semitrasparenti e
testi chiari verificati a schermo. Cattura finale tramite UI:
`Saved/Testing/Branding-Background.png`; log `branding-background.log`.
La cattura `shot showui` del primissimo frame precede la normalizzazione
del layout SScaleBox, quindi non è usata come evidenza del layout finale.
Il menu iniziale e quello durante la partita condividono lo stesso ShowMenu;
nessun test di gameplay/rete aggiunto per questa modifica grafica.
`git diff --check`, verifica JPEG con Pillow e sintassi zsh launcher superati.


## 16 ottobre 2026 — Menu senza logo e avvio al 70%

Compilazione Editor Mac arm64 riuscita. Avvio reale Metal con dimensioni
volutamente piccole `-ResX=1280 -ResY=720 -ERInitialWindowPercent=70`:
log `Saved/Testing/menu-window70.log` registra `ER_DISPLAY size=70%
resolution=2419x1564 fullscreen=0` e viewport effettivo 2419×1564.
Verifica visiva conferma selettore Finestra 70%, rimozione del logo
sovrapposto e mantenimento dello sfondo con pannelli trasparenti.
Evidenza: `Saved/Testing/Menu-NoLogo-70.png`. Sintassi zsh launcher
e `git diff --check` superati. Nessun test di combattimento/rete richiesto.


## 16 ottobre 2026 — Verifica pubblicazione degli asset del marchio

Il pre-push locale respingeva il commit c4a9426 perché il vecchio controllo
ammetteva esclusivamente testo e gitlink. Aggiunta eccezione per undici
immagini/icone del marchio, con percorso e SHA-256 esatti, README del marchio
e sorgenti Objective-C++ `.mm`. Nessuna eccezione generica per binari.
Sei test `test_public_tree.py` superati: include marchio esatto accettato,
contenuto sostituito/LFS rifiutato, immagini extra e symlink rifiutati,
asset rimossi ma ancora nella cronologia e binari mascherati da sorgenti.
`verify-public-tree.py`: PASS su tutti i 15 commit e 2827 voci approvate.
`git diff --check` superato. Nessun push eseguito durante questa correzione.


## Recupero sprint proporzionale — 16 ottobre 2026

- Editor Mac Development arm64: compilazione riuscita (25,99 s).
- `test-movement.py --posture-only`: PASS con host e client Unreal reali;
  limite sprint 5 s / recupero 10 s, recupero pari al doppio della durata
  effettiva dopo interruzione per postura e rilascio W tramite InputKey,
  nessun riavvio tenendo W e replica postura owner/proxy.
- `test-network.py --accuracy`: PASS, inclusi dati e recupero bloom owner.
- `test-movement.py` completo: FAIL su tre controlli dei blocchi da 200 cm
  (raggiungimento/soglia host e raggiungimento client remoto). Tutti i
  controlli sprint e postura sono passati anche in questa esecuzione.
  Nessuna modifica alla logica di arrampicata in questo intervento.
- Log in `Saved/Testing`: `movement-{host,client}.log` per la verifica
  mirata, `sprint-full-movement-{host,client}.log` per il test completo,
  `accuracy-{host,client}.log` per la regressione precisione.


## Trasparenza protezione alla nascita — 16 ottobre 2026

- Editor Mac Development arm64: compilazione riuscita.
- `setup_spawn_protection.py`: PASS; 222 associazioni di materiali per
  Blueprint e 326 varianti/parent generati tramite Unreal nel mount characters.
  Texture e parametri ereditati, maschere preservate, opacità moltiplicata per
  0,5. Corretto l'uso SkeletalMesh sulle copie dei materiali originariamente
  destinati solo ad accessori statici/sezioni nascoste.
- `test-network.py --phase-belica`: PASS, incluse nuove verifiche materiali
  su spawn, primo attacco, respawn e owner/proxy remoto; regressioni skin,
  cadaveri, munizioni, danno e respawn completate.
- `test-network.py --accuracy`: PASS, inclusi bloom, colpi respinti e stato
  owner replicato.
- `-ERProtectionGallery` con rendering Metal: quattro acquisizioni 1280×720
  Alpha/Omega, protetti/normali, ispezionate visivamente. Corpo e zaino sono
  trasparenti durante protezione; materiali originali ripristinati dopo la
  scadenza. Arma invariata. Nessun errore di compilazione shader osservato.
- Immagini: `Saved/Testing/Protection-{Alpha,Omega}-{Half,Normal}.png`.
  Log setup/build/render in `/tmp/respawn-protection-*`; log LAN/precisione
  in `Saved/Testing/{network,accuracy}-{host,client}.log`.


### Verifica dopo segnalazione sui bot — 16 ottobre 2026

Aggiunto `-ERProtectionRespawnGallery`: usa quattro veri BP_ERTrainingTarget,
li uccide e lascia eseguire il respawn normale dopo 3 s. Non prolunga la
protezione dei bot. A +0,5 s dal respawn verifica IsProtected e 48 slot dei
materiali; a +4,5 s verifica scadenza e ripristino. PASS su L_Depot e
L_Killhouse con ERAlphaBots=4/EROmegaBots=0/ERBotMode=Target.

Il Blueprint character e quello training salvati hanno entrambi 222
associazioni e nessun materiale skin mancante. Il difetto segnalato non è
stato riprodotto in questi avvii nuovi. Non è stata modificata la durata
dell'invulnerabilità né la logica di attacco. La camera della verifica evita
le pareti prima dell'acquisizione nella stanza. Log:
`/tmp/respawn-protection-{bots,killhouse}-gallery.log`; immagini
`Saved/Testing/Protection-Bots-Respawn-{Half,Normal}.png`.


## Indicatori sopra la testa rimossi — 16 ottobre 2026

Editor Mac Development arm64: PASS (28,78 s). TeamMarker viene nascosto
in BeginPlay su ogni peer, con visibilità e ombre disattivate anche per
i Blueprint già salvati. Rimossi creazione/aggiornamento del materiale
dinamico del marker. `git diff --check`: PASS. Nessun test gameplay
aggiuntivo eseguito per questa modifica esclusivamente visiva.


## 16 ottobre 2026 — HE, Milkor, rimbalzi e fumogena

- Build reale RespawnAndRageEditor Mac Development arm64 UE 5.8.3: **Succeeded**
  (`Saved/Testing/grenade-build.log`). Asset creati/salvati tramite Unreal:
  12 package Epic rinominati nel mount privato Effects/Grenade/Epic (circa
  5,9 MiB), P_ER_CompactHE e M_ER_ContainedSmoke, default BP collegati.
  Gli originali ContentExamples sono stati solo letti; ricevuta con 12 SHA-256
  in Assets/weapons/SourceAssets/EpicContentExamples/migration.json.
- `python3 scripts/test-grenades.py --render`: **PASS**, host Metal e secondo
  processo LAN. Contatti reali di HE e LaunchedHE prima della miccia senza
  detonazione all'impatto, rotazione server e variazione dello stesso attore
  osservata sul client, due burst remoti dopo miccia, cleanup dei proiettili.
  Griglia smoke realmente riempita sul lato sorgente, lato opposto vuoto con
  parete chiusa, passaggio dopo apertura della finestra, celle del muro pieno
  e sotto il pavimento vuote; durata/cleanup replicati verificati.
  Tutti i sette controlli del runner sono true in grenades-render-results.json.
- Ispezionate le sei catture Metal fresche in Saved/Testing/Grenades: rimbalzo,
  lampo HE, successiva polvere, muro chiuso, finestra aperta e camera dentro
  la nube. Shader M_ER_ContainedSmoke compilato senza fallback/errori;
  opacità con canale alpha esplicito, bordo morbido, profondità opaca e
  camera interna verificati visivamente. Un problema iniziale di visibilità
  dei nuovi componenti e la lettura dell'alpha sono stati corretti prima di
  queste catture. Il test dell'arsenale senza renderer verifica il template
  assegnato, anziché richiedere particelle attive con -nullrhi.
- `scripts/test-arsenal.py`: **PASS**, inclusi HE/smoke/stun, danno server,
  burst su entrambi i peer, smoke persistente, tagli da colpi autorizzati e cleanup.
- `scripts/test-network.py`: **PASS**, inclusi protezione, danni, copertura,
  respawn, score, modelli e materiali owner/proxy.
- `scripts/test-network.py --accuracy`: **PASS**.
- `scripts/test-reference-weapons.py`: **PASS**, tutti i dieci slot remoti e
  proiettile Milkor fisico con geometria propria.
- `scripts/test-destruction.py --combat`: **PASS**, incluse miccia HE reale,
  danni ai blocchi e rigenerazione; nessuna modifica ai danni/cover introdotta.
  Riepilogo di queste quattro regressioni in Saved/Testing/grenade-regressions.json.
- `scripts/verify-public-tree.py`: **PUBLIC_TREE_PASS**, 16 commit/3094 entry;
  asset privati mantenuti nei gitlink. Python compile e git diff --check: PASS.

Limiti: contenimento smoke cosmetico a celle circa38×38×35cm, conservativo
presso pareti e aperture strette; non è una simulazione fluidodinamica e non
modifica la visione dei bot. Il nuovo materiale non altera le impostazioni
globali del fog. Non misurate prestazioni/FPS di una partita completa; nessun
packaging o pubblicazione eseguito.


## 16 ottobre 2026 — danno 5/15m, armatura e cook

- Build Editor Mac Development arm64 reale: **Succeeded**, ultima compilazione
  in Saved/Testing/grenade-cook-build.log. Migrazione Unreal dei tre profili
  salvati HE legacy, HE Sketchfab e Milkor: 140 danni, 500/1500cm e ArmorRatio 1.6.
  Nessun asset artistico modificato da setup_grenade_combat.py; ricevuta privata
  SourceAssets/EpicContentExamples/combat.json.
- Automation `EclipseRecoil.Weapons.ReferenceBallistics`: **Success**. Aggiunti
  casi di letalità con 100AP nel nucleo, consumo AP, coda a10m, limite esatto
  15m e assenza del vecchio danno minimo al bordo; regressioni bullet invariate.
  Log grenade-cook-weapons-automation.log. Il log Editor riporta anche generici
  `Condition failed` durante l'inizializzazione, precedenti all'avvio del test;
  il test richiesto termina con Result={Success} ed eventi senza errori.
- `test-grenades.py --combat --render`: **PASS**, host Metal e client LAN,
  input reali LeftMouseButton press/release tramite Enhanced Input. Consumo
  munizione al click, proiettile armato trattenuto, mano e modello FPS nascosti,
  cambio arma/ricarica rifiutati durante il cook. Rilascio dello stesso attore,
  nessun reset della scadenza, origine dietro/sopra la testa, velocità verticale
  positiva. Sweep verificato con parete dietro la testa e soffitto basso.
  Verificati anche arming/release remoti e mano nascosta sul client.
- Danno reale server su 16 bersagli nativi, otto per payload HE/Milkor:
  con 100AP, bersagli a 4m e 5m morti; a 7,5m/10m/14m rimangono circa 37/72/98,88HP;
  a 15m 100HP. Copertura piena: 100HP; parziale: danno ridotto. I campioni sono
  eseguiti prima della modifica strutturale.
- Morte portatore: la granata già armata cade senza rimborso/reset; verificato
  abbassamento fisico di oltre 10cm nei primi 250ms, detonazione al termine
  residuo e cleanup. Overcook 2,5s in mano: portatore con 100HP/100AP ucciso e
  stato cook ripulito. La velocità massima del drop conserva il budget di
  movimento invece di limitare la caduta alla velocità iniziale nulla.
- Tre catture Metal fresche ispezionate in Saved/Testing/Grenades:
  Cook00_Ready.png, Cook01_Held.png, Cook02_Released.png. Cinque controlli true
  in grenade-combat-render-results.json.
- Regressioni finali **PASS**: test-arsenal.py, test-network.py, --accuracy,
  test-reference-weapons.py, test-destruction.py --combat e test-grenades.py
  baseline (rimbalzi/fuse/effetti/smoke/coperture/cleanup). Ricevute:
  grenade-cook-regressions.json e grenade-cook-effects-regressions.json.
  Tre avvii iniziali erano falliti per modulo temporaneamente assente durante
  una ricompilazione concorrente; rilanciati dopo la build e passati.
- Python compile e git diff --check: PASS. Nessun packaging/pubblicazione.

Il modello resta un'approssimazione gameplay: decadimento quadratico, tre
raggi di esposizione e armatura uniforme; nessuna simulazione individuale
delle schegge o protezione anatomica. La miccia non compensa la latenza
del rilascio: decide il tempo server. La distruzione strutturale resta nel
proprio raggio 3,5m, distinto dal raggio di ferimento 15m.

### Taratura granate 3,5s / 2–6m — 16 ottobre

- Build editor Mac arm64: PASS (`grenade-tuning-build.log`).
- Migrazione asset Unreal: PASS, tre profili HE/Milkor a 140 danni,
  nucleo 200cm e limite 600cm; sei profili granata con miccia 3,5s
  (`grenade-tuning-assets.log`, ricevuta privata `combat.json`).
- Automation `EclipseRecoil.Weapons.ReferenceBallistics`: PASS, nucleo,
  decadimento quadratico e zero al limite (`grenade-tuning-weapons-automation.log`).
- `test-grenades.py --combat`: PASS su host/client, profili salvati,
  danni HE e Milkor con armatura, coperture piene/parziali, cook dal click,
  rilascio senza reset, caduta alla morte ed esplosione in mano.
  Tutti i quattro controlli della ricevuta `grenade-combat-results.json` true;
  log riepilogo `grenade-tuning-combat-result.log`.
- Python compile e `git diff --check`: PASS. Nessuna modifica visiva in
  questa taratura; la verifica grafica precedente resta documentata sopra.

Questa taratura sostituisce i precedenti 5/15m e 2,5s. Con armatura piena
il nucleo scoperto infligge 112HP; a 4m 28HP, a 6m zero.

### Fumogena irregolare 3m — 16 ottobre

- Build editor arm64 PASS (`smoke-shape-build-final.log`); migrazione Unreal
  smoke-only PASS (`smoke-shape-assets.log`).
- `test-grenades.py --render` PASS: volume 600x600x300cm, fase casuale,
  limite 3m, origine sopra il pavimento, parete chiusa, finestra aperta,
  celle solide escluse, scadenza12s, replica e shader. Tutti i sette controlli
  true in `grenades-render-results.json`. Ispezionate le catture aggiornate
  `04_OpenWindow.png` e `05_InsideSmoke.png` (occlusione scena e vista interna).
- Regressioni arsenal/reference/network/accuracy/block-combat PASS:
  `smoke-shape-regressions.json`, tutti cinque true.
- Python compile e git diff --check PASS. Le deformazioni sono visive entro
  il volume massimo; il contenimento resta una griglia conservativa16x16x8,
  non una simulazione fluida. Nessuna modifica alla durata o al danno.

### Lancio ridotto e bagliore/schegge — 16 ottobre

- Build editor arm64 PASS (`explosion-refinement-build-final.log`).
- Migrazioni Unreal PASS: setup_grenade_combat.py salva Milkor1800cm/s,
  HE/smoke/stun880cm/s; setup_explosion_refinement.py salva solo gli asset
  dell'esplosione (fuoco disabilitato, lampo65ms, tre sbuffi brevi, mesh schegge).
- `test-grenades.py --render` PASS: velocità salvate,3..6 schegge a terra,
  zero in aria, rimbalzi/replica/micce, fumo3m e pareti/finestre, shader e
  catture fresche. Ispezionata02_HE.png: piccoli frammenti, niente fiammata.
- Combat/cook, reference, arsenal, network, accuracy, block-combat PASS.
  `explosion-refinement-regressions.json`: sette controlli true, incluso render.
- Corrette due condizioni della fixture: il controllo iniziale del Milkor
  leggeva la vecchia Arsenal[7] (pistola), ora legge ReferenceArsenal[7];
  il controllo cleanup arsenal cadeva1ms prima della lifespan3,05s, aggiunto
  margine200ms. Rilanciati entrambi i test e passati. La prima build aveva
  overload FRandRange ambiguo con interi, corretto usando argomenti float.
- Python compile e git diff --check PASS. Danno2/6m e miccia3,5s invariati;
  schegge cosmetiche temporanee, non un cratere persistente nella geometria.

### Fumogena4m e interazioni — 16 ottobre

- Build editor arm64 PASS (`smoke-interaction-build.log`). Migrazioni Unreal
  smoke e dotazione/velocità PASS (`smoke-interaction-assets.log`,
  `smoke-interaction-stock.log`). Milkor1530cm/s, granate manuali880cm/s,
  HE/smoke10 ciascuna nel catalogo attuale (legacy stun anche10).
- `test-grenades.py --render` PASS: nuovo raggio4m/altezza4m, dotazione10,
  velocità ridotta, parete chiusa e finestra aperta, replica/lifetime12s,
  shader e catture aggiornate. Un vero projectile attraversa la nube e apre
  la scia; la sua detonation multicast apre il vuoto più ampio. Entrambe le
  verifiche PASS. Sette controlli true in `grenades-render-results.json`;
  ispezionata `05_InsideSmoke.png` con apertura temporanea e clipping scena.
- Contorni mobili: fase casuale replicata e tempo nube sincronizzato nelle
  tre oscillazioni lente del materiale; crescita2,2s e fade4s nel Tick.
- Combat/cook, reference, arsenal, network, accuracy, block-combat PASS;
  `smoke-interaction-regressions.json`, sei controlli true.
- Python compile e git diff --check PASS. Le scie sono cosmetiche e limitate
  a quattro per nube; nessuna simulazione fluida, deviazione balistica o cambio
  al danno. Durata complessiva invariata12s, dissolvenza negli ultimi4s.

### Spalla, cook in terza persona e spostamento fumo — 16 ottobre

- Build arm64 PASS (`grenade-shoulder-build-final.log`); migrazione profili
  PASS (`grenade-shoulder-assets.log`), Milkor1750cm/s e origine(-14,24,-4)cm.
- Native `EclipseRecoil.Weapons.ReferenceBallistics` PASS
  (`grenade-shoulder-weapons.log`).
- `test-grenades.py --combat --render` finale PASS (`grenade-shoulder-grip.log`):
  cook reale host/client, mano FPS nascosta, modello terza persona visibile,
  blend posa pronto-lancio, mano sopra spalla, mesh granata vicino alla mano,
  origine sotto livello occhi, miccia originale, coperture/danno, death drop
  e overcook. Tutti cinque controlli true; quattro catture fresche, inclusa
  Cook03_RemoteReady.png, ispezionata dopo il rilassamento del braccio libero.
  Il log ER_COOK_WORLD_GRIP conferma HandGrip_R su hand_r e centro del modello
  circa8cm dal polso (offset presa), non alla cintura.
- Baseline `--render` PASS: fumo4m, passaggi/pareti, wake e nuova apertura
  esplosiva300cm/2,5s, shader, lifetime/cleanup. Ricevuta
  `grenade-shoulder-visuals.json` true per entrambe le modalità.
- Arsenal/reference/network/accuracy/block-combat PASS, cinque true in
  `grenade-shoulder-regressions.json`. Nuovo codice pose inattivo con armi normali.
- Il primo tentativo richiedeva include CameraActor; la luce opt-in della
  fixture richiede FindComponentByClass<PointLightComponent> in questa UE.
  Corrette entrambe le compilazioni. Una ripetizione grafica aveva ricevuto
  look input mentre teneva la granata: la fixture ora ripristina la mira
  nota immediatamente prima del rilascio, senza cambiare il lancio gameplay.
  Ripetizione finale PASS. Python compile e git diff --check PASS.

### Fumogene del colore squadra — 16 ottobre

- Build editor arm64 PASS (`smoke-team-build.log`). Nessuna rigenerazione
  degli asset necessaria: il colore è un valore runtime replicato.
- `test-grenades.py --render` PASS (`smoke-team-render.log`): due veri
  projectile smoke con miccia, team Alpha/Omega assegnati prima dell'armamento
  e ripristinati prima della detonazione; le nubi conservano entrambi i colori.
  Il client riceve esattamente(.2,.65,1) e(1,.25,.1), confermato dai log
  ER_SMOKE_REMOTE_TEAM. Otto controlli true in `grenades-render-results.json`.
- Ispezionata05_InsideSmoke.png: tinta azzurra presente nel volume renderizzato,
  mantenuti clipping, contenimento, interazioni e durata. Alpha e Omega sono
  colori assoluti condivisi, non colori relativi al team dell'osservatore.
- Arsenal/reference/network/accuracy/block-combat PASS:
  `smoke-team-regressions.json`, cinque true.
- Python compile e git diff --check PASS. Le esplosioni HE mantengono il
  proprio fumo neutro; solo il payload fumogena usa il colore del lanciatore.

### Fumo scuro e crescita meno uniforme — 16 ottobre

- Build arm64 PASS (`smoke-dark-build.log`); rigenerazione Unreal del solo
  materiale/Blueprint fumo PASS (`smoke-dark-assets.log`).
- `test-grenades.py --render` PASS (`smoke-dark-render.log`), otto controlli
  true: entrambe le tinte scure replicate, shader, contenimento, radius4m,
  interazioni e cleanup. Client log: Alpha(.08,.26,.4), Omega(.4,.1,.04).
  Ispezionate03_ClosedWall.png e05_InsideSmoke.png aggiornate.
- Crescita materiale con ritardi direzionali continui fino0,48s, raggiunge
  comunque100% a2,2s; lobi mobili leggermente più marcati e limite4m.
- `test-network.py` PASS (`smoke-dark-network.log`). Python compile e
  git diff --check PASS. Nessun cambio a densità, durata o danni; per questa
  sola taratura visiva verifiche concentrate su shader, fumo e replica.

### Fumo denso e detonazione delle granate colpite — 16 ottobre

- Build editor arm64 PASS (`grenade-shot-build-final.log`); native Weapons
  ReferenceBallistics PASS (`grenade-shot-weapons.log`). Migrazione smoke-only
  finale PASS (`grenade-shot-assets-final.log`).
- Combat --render PASS (`grenade-shot-combat-render.log`): veri spari arma
  server contro granata armata sospesa e granata in mano al portatore. Entrambe
  detonano prima della miccia; parete solida blocca lo sparo. Il cook si chiude
  senza doppio lancio. Preservati danni/coperture, posa e death drop/overcook.
- Baseline --render finale PASS (`grenade-shot-baseline-final.log`): fumo
  più denso, foro bullet ancora aperto dopo1,4s e richiuso dopo2s; esplosione
 400cm fino a fine nube, raggio ancora>=380cm dopo3s e dopo cinque scie nuove.
  Il materiale finale libera il nucleo all'80% del raggio delle esplosioni.
  Otto controlli true; sei catture fresche inclusa06_HE_Dispersed.png,
  ispezionata: finestra/fondo nuovamente leggibili, pochi residui marginali.
- Arsenal/reference/network/accuracy/block-combat PASS, cinque true in
  `grenade-shot-regressions.json`; entrambe le modalità grafiche true in
  `grenade-shot-visuals.json`. Ultima sola rifinitura shader riverificata
  con baseline grafica, senza ulteriori cambi al C++.
- Python compile e git diff --check PASS. Solo danno puntuale positivo da
  proiettile sul server attiva la granata; nessuna detonazione da melee/radiale.

### Reazione a catena con coperture — 16 ottobre

- Build arm64 PASS (`grenade-chain-build.log`), native Weapons/ReferenceBallistics
  PASS (`grenade-chain-weapons.log`). Nessuna migrazione asset/audio.
- `test-grenades.py --combat --render` PASS (`grenade-chain-combat.log`): sparo
  iniziale su HE, Milkor armato a4,5m, seconda HE a9m (richiede secondo passaggio),
  granata in mano esposta e HE protetta dietro una parete. Propagazione riuscita
  a più passaggi, cook chiuso, bersaglio coperto ancora attivo. Next-tick/pending
  verificati, nessuna detonazione ricorsiva. Tutti cinque controlli true e
  quattro catture fresche; ispezionata Cook03_RemoteReady.png.
- Arsenal/reference/network/accuracy/block-combat/baseline PASS:
  `grenade-chain-regressions.json`, sei true. Baseline conferma fumo, tinte,
  micce indipendenti delle fixture a danno zero e aperture persistenti.
- `git diff --check` PASS. Audio solo ricercato: Freesound qubodup162363
  (pagina indica CC0, registrazione breve/distanza) e Sonniss GameAudioGDC
  con licenza corrente consultata; nessun file scaricato o importato.

### Freesound grenade audio — 16 October 2026

- Editor arm64 Development build succeeded (`grenade-audio-build.log`).
- Unreal Python import/save completed: mono0.601s PCM,5m inner radius plus75m
  falloff,8-voice concurrency; `ER_GRENADE_AUDIO_READY` in
  `Saved/Testing/grenade-audio-assets.log`.
- `python3 scripts/test-grenades.py --render --audio`:10/10 checks passed,
  including real host active audio component, remote asset binding, HE/Milkor
  fuse/bounce, smoke containment/team tint, shader and fresh Metal captures.
  Receipt: `Saved/Testing/grenades-render-audio-results.json`.
- `python3 scripts/test-grenades.py --combat --render --audio`:7/7 checks passed,
  including gameplay/cook, remote state, active host audio, remote asset and
  captures. Multi-hop chain, covered target and held grenade checks passed.
  Receipt: `Saved/Testing/grenade-combat-render-audio-results.json`.
- CoreAudio mixer initialized and active components were observed; this validates
  Unreal playback initiation, not a subjective listening/mix assessment. Remote
  peer intentionally runs without audio output to avoid duplicate speaker playback.
- Python syntax and git diff whitespace checks passed. Source/credit/hashes and
  derived audio packages remain in the private weapons submodule.

## Protezione respawn grigia senza copie — 16 ottobre 2026

- Build Editor Mac arm64 Development: PASS (11.11s, configurazione finale).
- setup_spawn_protection.py: eliminate326 varianti/parent traslucidi (circa16.3MB
  di pacchetti); nessun nuovo asset. Seconda esecuzione idempotente:0 eliminati,
 0 creati. Rimossi anche i soli vecchi inserimenti staged della cartella ritirata,
  assente da HEAD, per non conservare le copie eliminate nel prossimo commit.
- Base finale: materiale engine LevelColorationLitMaterial, lit/opaco con Color
  grigio e supporto universale engine. BasicShapeMaterial è stato scartato dopo
  avere osservato richieste di compilazione per SkeletalMesh/Clothing. Nessuna
  modifica o salvataggio nei materiali engine o nelle skin originali.
- test-network.py --phase-belica: PASS; spawn grigio, ripristino dopo attacco,
  respawn, owner/proxy remoto, selezione skin, cadaveri, danno e munizioni.
- ERProtectionGallery Metal: completata; quattro confronti Alpha/Omega
  Gray/Normal ispezionati visivamente, resa opaca uniforme e materiali originali
  ripristinati. Nessun warning di usage per la base engine definitiva o errore
  shader osservato. Restano warning preesistenti di usage su materiali originali
  nascosti di alcuni personaggi, estranei al nuovo materiale.
- Log Saved/Testing/protection-gray-{build,removal,assets,network,gallery,bots}.log;
  screenshot Protection-{Alpha,Omega}-{Gray,Normal}.png. Nessun benchmark FPS
  dichiarato: la riduzione riguarda copie su disco e passaggi di trasparenza.
- ERProtectionRespawnGallery Metal: PASS con4 bot veri,48 slot verificati
  protetti a+0.5s e ripristinati a+4.5s dopo respawn; screenshot grigio ispezionato.

## XM1014 individual pellets and anatomy — 16 October 2026

- Final Editor Mac arm64 Development build: PASS (8.30s).
- setup_shotgun.py: saved active DA_Shotgun only;6x20,ArmorRatio1.6,
  RangeModifier.70,anatomical opt-in,3.2deg pellet pattern/.25deg aim.
- EclipseRecoil.Weapons.ReferenceBallistics: SUCCESS, including96 total
  armored chest damage/12AP cost, head/stomach/leg multipliers,14 damage
  per pellet at12.7m, deterministic seeded bounded jitter without center pellet.
- test-reference-weapons.py: PASS with real host/client and all10 remote slots.
  Real close shotgun shot consumed one shell and registered6 anatomical hits
  (lowerarm_l on this pose, same x1 as chest). Each pellet had19.77..19.79
  pre-armor damage after short-range falloff; target survived. Actual world box
  blocked the anatomical query. Point-damage integration independently verified
  all five bone cases with/without AP, helmet bypass, AP depletion,3 chest pellets
  for60 damage and6 for a kill. This distinguishes real aimed hits from the
  controlled point-event checks; no exact CS2 pattern or bone-mesh fidelity claim.
- test-network.py --accuracy: PASS, including owner ammo/bloom replication
  and server-clock recovery. test-arsenal.py: PASS. test-destruction.py --combat:
  PASS, including block penetration, actual HE fuse and regeneration.
- Receipts: Saved/Testing/shotgun-{build,assets,ballistics,reference,accuracy,
  arsenal,block-combat}.log. Python syntax and whitespace checks passed.
- test-network.py --phase-belica: PASS in configurazione finale, incluse
  protezione grigia owner/proxy, cancellazione dopo attacco, selezione skin,
  cadaveri, danno, munizioni e respawn. Receipt: shotgun-network.log.

## English UI and generic weapon labels — 16 October 2026

- Editor Mac arm64 Development build: PASS (28.30s).
- setup_english_presentation.py: PASS;22 existing weapon data labels and20 skin
  names across3 Blueprint defaults saved. Exact active slot order verified:
  Knife,Pistol,SMG,Shotgun,Assault Rifle,Sniper Rifle,Marksman Rifle,
  Grenade Launcher,HE Grenade,Smoke Grenade. Stats and asset IDs unchanged.
- Fresh-process verify_team_skins.py: ER_SKINS_SAVED_COMPLETE,60 appearance
  records passed, checking translated names plus original meshes, materials,
  accessories and corpse copies.
- EclipseRecoil.Menu.IPv4Validation: SUCCESS, including Recruit fallback and
  safe player-name/address handling.
- test-admission.py: PASS:4-player cap, balanced teams, late joins, disconnect
  and English Server full message observed by the rejected real client.
  Bot label translation is a source change; this run did not use --bots.
- Actual Metal main menu1600x1000 captured and inspected: English headings,
  selectors, skin names, settings, controls and requested generic weapon keys;
  no clipped labels observed. Screenshot Saved/Testing/English-MainMenu.png.
- Existing Italian UI literal audit, Python syntax and whitespace checks passed.
  Logs: Saved/Testing/english-presentation-{build,assets,skins,menu-tests,
  menu-render,admission}.log; saved label receipt english-presentation-assets.json.

## Rifle scopes — 16 October 2026

- Editor Mac arm64 Development: PASS with `scripts/build-editor.command -DisableUnity`.
  The default unity build exposed an existing anonymous-namespace `Count` collision
  between ERSmokeCloud.cpp and ERTeamGallery.cpp; no unrelated code was changed.
- `setup_scopes.py`: PASS; saved only DA_Assault, DA_Carbine and DA_Sniper in
  the private weapons mount. Runtime owner observations confirm the new profiles.
- Native `EclipseRecoil.Weapons.ReferenceBallistics`: Success (one test).
- `test-scopes.py --render`: PASS on the final module, two actual peers and19
  remote input phases. Server checks: Assault unchanged spread/two-click toggle;
  actual early Marksman shot retains >5deg cone and exits scope; optical cycles,
  settled0deg/40FOV, second zoom10FOV,3/6deg hip spread, shot retention/exit,
  reload clearing and disabled knife scope. All19 client level/FOV observations
  independently matched expectations. Captures `Saved/Testing/Scopes/Scope_100..118.png`.
  Inspected final Assault ADS (101), Sniper optic (104), Marksman second zoom (109)
  and Marksman hip (107): gun-only centred Assault, both black crosslines,
  circular opaque perimeter, hidden scope weapon/arms, no hip reticle/ammo ring.
  The initial captures exposed oversized Assault framing and a faint/missing
  vertical crossline; both were corrected and final captures repeated.
- `test-reference-weapons.py`: PASS, all ten real remote slot selections and
  existing ammunition, launcher, armor, XM1014 anatomy/cover checks.
- `test-network.py --accuracy`: PASS, motion/posture, real server ray sampling,
  burst, empty-shot rejection and owner bloom/clock recovery.
- `test-network.py`: PASS, including damage/cover, ammunition, teams, round reset,
  death/corpse and respawn rules.
- `test-arsenal.py`: server gameplay checks passed on the first run; the runner
  still expected the retired display label M249 and rejected the correctly
  replicated Machine Gun / SK_M249 /100-round loadout. Updated that assertion
  to use presentation_labels.LEGACY_NAMES; final full repeat PASS.

Logs: Saved/Testing/scopes-{build,setup,native}.log, scopes-render-{host,client}.log,
reference-weapons-{host,client}.log, accuracy-{host,client}.log,
network-{host,client}.log and arsenal-{host,client}.log. No packaging/publication.

### Assault aperture centring — 16 October 2026, follow-up reference

Editor Mac arm64 Development build PASS (`-DisableUnity`,88.18s).
Read the preserved derived Assault FBX through Blender without saving it;
rear-aperture centre is weapon-local(6.07019,0,18.71037)cm. Native framing now
places that anchor at camera(14,0,0)cm instead of placing the muzzle below centre.
This preserves fitted grip transforms and the gun-only ADS view while increasing
its on-screen size to match the newer reference. No weapon data/art changes.

The owning client's actual Metal camera projection reports
`ER_SCOPE_IRON_CENTER result=PASS screen=640.000,360.000 error=0.0000`
at1280x720. Inspected the new `Saved/Testing/Scopes/Scope_101.png`: aperture
centred, front post visible through it, closer rifle, no floating/cropped hands.
Full `test-scopes.py --render` repeat PASS: all19 owner states/FOVs, actual
server scope/accuracy checks, fresh Metal captures and new centre assertion.
Prior combat/network/arsenal
regressions remain the earlier results above; this adjustment changes presentation.

## Grenade range / rebound / structure and launcher ADS — 16 October 2026

- Editor Mac arm64 Development: PASS (`-DisableUnity`,58.89s final build).
  First attempt read a grenade-probe file during its write; rebuilt after the
  source was complete. Final source has no compiler errors.
- Existing weapon data migrated through setup_grenade_combat.py/setup_scopes.py:
  hand horizontal multiplier1.2; launcher scoped multiplier1.5, iron-style75FOV
  and its own measured aiming anchor. Retained2/6m player damage and3.5s fuses.
- `test-grenades.py --render`: PASS, actual bounce/contact/tumble, runtime
  bounciness.18..25 and friction.4, saved range settings, fuse/explosion/smoke,
  remote effects/team colors and fresh Metal captures. Inspected01_Bounce.png.
- `test-grenades.py --combat --render`: PASS, actual input-driven hand release
  has1.2x horizontal velocity and the original vertical velocity; cook/release,
  same fuse, held pose, damage/armor/cover, shootable grenades and chain tests.
  Inspected Cook02_Released.png.
- `test-destruction.py --combat --render`: PASS. Same intact physical room and
  blast origin: previous100 structural damage destroyed100 blocks; the actual
  HE fuse supplies300 structural damage and destroyed106. Material/cover rules
  still apply; this is damage x3, not a claim of triple destroyed-block count.
  Existing penetration, blocked shots, rear targets, pawn breach and reset passed.
- `test-network.py --accuracy`: PASS on the changed module.
- Initial `test-scopes.py --render`: PASS,26 actual owning-client phases;
  real launched projectile uses1.5x horizontal velocity with original vertical
  component in ADS, and ordinary velocity after exit. Refined the launcher's
  anchor from the rear feature to the upper front feature to expose the target
  above the gun. Final repeat and remaining regression results recorded below.

No new meshes/materials, saved map edits, packaging or publication.

Final aiming repeat: `test-scopes.py --render` PASS with26 owner states/FOVs,
all fresh captures, real launcher velocity checks and both iron anchors at
(640,360),0px error. The probe now ticks at TG_PostUpdateWork so its camera
cache and weapon transform describe the same rendered frame; an earlier
pre-physics sample mixed frames and reported4.56px despite the centred capture.
Final build including that probe fix: PASS,60.17s. Inspected final Scope_120.png:
centred launcher, target visible above the front aiming feature, ordinary hip
frame restored on exit. `test-reference-weapons.py` and `test-arsenal.py`: PASS.

Final regressions: `test-network.py` PASS after a fresh startup (one earlier
attempt exited before listening); `EclipseRecoil.Weapons.ReferenceBallistics`
Success,1 test. Saved logs in Saved/Testing/grenade-range-{build,setup,scopes,native}.log
plus the standard grenade baseline/combat, destruction, reference, accuracy,
network and arsenal host/client logs. Both direct physics boosts and all
remaining network/weapon regressions completed successfully.

## Continuous aim / movement and crouch focus — 16 October 2026

Editor Mac arm64 Development PASS (`-DisableUnity`,70.27s final build).
setup_scopes.py saved the four existing aiming data assets. Launcher drop2.5cm,
upward rig pitch3deg; Assault centre retained. Iron rig/FOV/sensitivity ease
in/out over.2s at normal frame rates; integration capped50ms per frame so a
stall cannot skip the complete animation. No art/animation regeneration.

`test-scopes.py --render`: final PASS,26 real owner input phases, actual server
launch/shot checks, fresh steady and intermediate Metal captures. New server
checks passed: moving standing Marksman never finishes focus, actual stopping
requires recovery, grounded crouch-walking gives zero cone, crouch focus finishes
within30ms, crouching Sniper reduces recoil/recovery delay, walking Sniper has
large true spread plus blur. Crouch benefits use the shared server posture/velocity
formula; no new movement RPC or client-selected shot direction.

Four intermediate phases101/102/120/122 observed blends between.05 and.95 with
FOV between75 and90. Inspected Transition_101.png and final Scope_120.png.
Alignment: Assault(640,360); launcher(640,413.333) at1280x720, matching its
intentional drop. First graphical run missed the initial Assault intermediate
frame after a stall; added the integration cap and repeated all captures.

`test-network.py --accuracy`: final PASS, including real movement/posture cones,
server traces, burst, owner replication/clock recovery and rejected empty fire.
The first concurrent run missed timing-sensitive burst checks; the sequential
repeat passed without changing their assertions. `test-reference-weapons.py`:
PASS, all ten remote slots and armor/reload/projectile/XM1014/cover regressions.
Native Weapons result is recorded below.

Requested mechanics are implemented as game-specific rules, without asserting
that the supplied Counter-Strike description is an exact simulation reference.

Native `EclipseRecoil.Weapons.ReferenceBallistics`: Success,1 test. Final
sequential command chain exited0. Logs: Saved/Testing/scope-motion-{build,setup,test,native}.log
and the standard accuracy/reference host/client logs. No packaging/publication.

### Launcher arc and player contacts — 16 October

- Editor build with `-DisableUnity`: PASS (54.27s).
- `test-grenades.py --combat --render`: PASS, two real peers. Four actual
  swept contacts (launcher HE, hand HE, smoke, stun) each registered a bounce;
  only launcher contact removed35HP from the unarmored target. Post-contact
  horizontal rebound stayed below80cm/s from1000cm/s incoming speed. All
  retained their fuse. Explicit repeated-contact callback checks applied no
  further damage. Existing cook, armor, cover, shooting and chain tests passed.
- `test-scopes.py --render`: PASS, all26 replicated states, iron alignment,
  intermediate transition captures, actual launcher shots above the reticle
  with six-degree elevation, scoped horizontal velocity x1.5 and ordinary
  velocity restored on scope exit.

Logs: `Saved/Testing/grenade-combat-render-{host,client}.log`,
`Saved/Testing/scopes-render-{host,client}.log`; build log copied as
`Saved/Testing/launcher-contact-build.log`. No asset regeneration required.

`test-reference-weapons.py`: PASS, all remote slots, real launcher projectile
spawn/speed, rifle armor/reload and shotgun anatomical/cover checks.

### Scoped launcher origin — 16 October

- Editor `-DisableUnity`: PASS.
- `test-scopes.py`: PASS, two real peers, actual fired projectile origin matches
  raised muzzle with FOV correction (measured delta0.000cm), distinct from hip
  origin; upward launch and scoped horizontal range checks pass.
- `test-scopes.py --render`: PASS on final run, all26 replicated states, fresh
  captures, alignment and intermediate transitions. Raised muzzle origin delta
  0.000cm in final Metal run. Initial graphical run failed the later-pose muzzle
  comparison; diagnostic headless and graphical reruns matched exactly with no
  gameplay changes between them. Diagnostic origin log retained for visibility.
- Existing eye-to-muzzle sweep/fallback remains in place; no new near-wall
  gameplay fixture was added in this revision.

Logs: `Saved/Testing/scopes-host.log`, `scopes-render-host.log` and matching
client logs. Build retained as `Saved/Testing/launcher-origin-build.log`.

## Audio Sonniss HE/fumogena — 17 ottobre 2026
- Editor Mac Development: Succeeded (28,83s).
- setup_grenade_audio.py eseguito in Unreal: exit 0, script successfully,
  0 errors/0 warnings; SoundWave HE 2,361s, smoke 6s, mono PCM44,1kHz.
- scripts/test-grenades.py --render --audio: exit 0, tutti i 12 check PASS.
  Host conferma started=1, spatial=1, concurrency=1 per entrambi; client
  headless/nosound conferma asset e percorso remoto (non ascolto remoto).
  Gameplay probe PASS, colori remoti e cleanup12s PASS, capture fresche.
- WAV smoke verificato: 264600 campioni, fade lineare 4,5..6s, ultimo campione0.
  Verifica strumentale della riproduzione; nessuna valutazione uditiva dichiarata.

## F1 / HE senza sfera — 17 ottobre 2026
- Blender bake HE: F1 estratta dal FBX originale, 3608 vertici / due ossa.
- setup_f1_grenade.py: import e salvataggio Unreal completati (exit0).
  Primo tentativo fermato dalla clip idle senza suffisso atteso; corretta
  la gestione del modello statico, migrazione rieseguita con successo.
  Controlli: FlashMesh nullo, audio Sonniss preservato, burst e chips presenti.
- test-grenades.py --render --audio dopo la migrazione: exit0, 12/12 PASS;
  HE e smoke started=1 sul host, controlli remoti e capture fresche PASS.
- test-reference-weapons.py --render: exit0, probe PASS, tutti gli slot
  remoti PASS, shader Metal e capture modelli/reload PASS.
- Ispezionate Slot_08.png (F1 impugnata, texture e replica visibili) e
  Grenades/02_HE.png (detriti, nessuna mesh sferica). La cattura HE è
  successiva al flash breve; la luce resta invariata nel codice runtime.

## Preset danni materiali/blocchi/granate — 17 ottobre 2026
- Editor Mac Development: Succeeded. Corretto un errore preesistente
  `-Wshadow` (unity build, `Count` in ERTeamGallery vs ERSmokeCloud) che
  bloccava anche la baseline.
- setup_block_damage_preset.py: commandlet exit0, `DA_ER_BlockMaterials`
  salvato (maps). Il test DamagePreset rilegge l'asset: HP/moltiplicatori della
  tabella per gli ID 0–6, surface e decal 0–2 invariati.
- test-destruction.py: 16/16 Success (nuovi DamagePreset,
  DamageLocalAndShielding, GlassPane). Granata su parete a quattro strati:
  310 candidati, 144 occlusi, 84 attenuati, 1,14ms.
- --combat: PASS 3/3. Il colpo reale di fucile rimuove 1 cella di cartongesso,
  o 2 quando la dispersione cade su una giunzione. La granata HE reale rimuove
  60 celle, tutte entro 200cm (contro 94 dell'esplosione lineare legacy).
- --procedural, --collapse, test-destruction-stress.py, test-grenades.py
  (base e --combat), test-map-template.py, test-dust-template.py: PASS.
- test-destruction-network.py (normale e --loss): FAIL su un solo check
  preesistente, `remote_hitscan_through_replicated_breach`: l'host perde 27,8
  invece di 36 con un colpo diretto che non attraversa blocchi. Fallisce
  identico anche sulla baseline senza queste modifiche. Lo script interrompe
  la run al primo FAIL; con una copia temporanea che non interrompe, tutte le
  18 fasi client e gli altri check host passano: late join dopo
  distruzione/crollo, dense join, esplosione coerente su tre client.
- Non verificato: celle da 5cm (rinviate), resa grafica Metal di questo preset.

## Prototipo celle 5cm su L_DestructionLab — 17 ottobre 2026
- Editor Mac Development: Succeeded (nuove dipendenze GeometryCore,
  GeometryFramework, NetCore).
- test-cell-destruction.py: 4/4 Success (Cells.StorageAndCalibration,
  MeshAndCollision, BallisticsAndBlast, NetworkTransactions). Facce e box
  verificati per forza bruta dopo 3000 rimozioni casuali: nessuna faccia
  interna, box che coprono esattamente le celle vive. Mattone 20cm a
  trasmissione equivalente (0,933/cm): capsula accovacciata dopo 2 granate,
  in piedi dopo 4. Delta di rete per la granata su cartongesso: 13 chunk,
  606 byte per 7937 celle rimosse.
- test-cell-destruction.py --game: PASS (personaggio reale nella breccia del
  cartongesso, mattone chiuso dopo 1 granata e attraversato accovacciato dopo 3,
  cemento danneggiato senza rimozioni, legno e lamiera aperti, spoletta HE reale
  entro 200cm, passaggio celle↔blocchi e rigenerazioni senza componenti residui).
  Colpo reale di fucile: 2 celle (una per lastra), bersaglio dietro 500→468,1.
  Generazione 77.580 celle / 52 chunk 5,5ms; granate 4,3–7,3ms ciascuna.
- measure-cell-scaling.py: 0,14ms per chunk (607.500 celle / 388 chunk in 56ms).
- Regressioni blocchi: test-destruction.py 16/16 Success (20/20 con le celle),
  --combat, --collapse, --procedural PASS. --game FAIL su un solo check,
  `console_aim_damage`, che attende ancora `HP=20` (cartongesso a 30 HP prima del
  preset; ora 20−10 = 10). Non dipende dal codice celle: è un'aspettativa da
  aggiornare dopo il preset.
- test-cell-destruction.py --render (Metal): PASS. Ispezionate Cells-Overview,
  DrywallBreach, BrickBreach, WoodEast/WoodWest e Blocks-Overview. Le prime catture
  hanno mostrato facce scure: avvolgimento dei triangoli invertito rispetto alla
  convenzione Unreal ed elemento normale condiviso fra i 4 angoli del quad
  (triangoli senza normali). Corretti entrambi; MeshAndCollision ora verifica
  avvolgimento e normali di ogni triangolo. Illuminazione uguale alla modalità
  blocchi; resta una linea leggera sul legno al bordo fra due chunk.
- Non verificato: LAN reale in modalità celle (protocollo 7 testato solo in
  processo), mappe procedurali/template a celle (fase 2), crolli.

## Celle 5cm in LAN reale (protocollo 7) — 17 ottobre 2026
- Nuovo probe opt-in `ERCellNetworkProbe` e script `test-cell-network.py`: host
  listen su `L_DestructionLab?Cells=1` (`-ERCellNetworkProbe`), un client
  iniziale e un client che entra dopo granate e attraversamenti. Il probe si
  registra da solo (nessuna modifica a ERMatch/ERPlayerController). Ogni
  client remoto ha un attore `AERCellNetworkReporter` per i report verso l'host.
- test-cell-network.py: exit0, 21/21 controlli dello script, host
  `ER_CELL_NET_PROBE_COMPLETE result=PASS`, nessun FAIL.
  - Digest di stato identici su host e client dopo ogni transazione. Cartongesso:
    7937 celle rimosse, 13 chunk, 606 byte di maschere. Mattone 20cm, granate
    1–3: +2570, +3138, +2290 celle (19 chunk, 1390 byte in tutto).
  - Collisione: 40.140 colonne di celle per controllo, ognuna tracciata due
    volte (componente del chunk e query di scena). 0 differenze dai bit vivi,
    su host e client, a ogni fase.
  - Gate: nessun componente della stanza collide prima del primo snapshot
    completo (0 violazioni; tick in attesa nell'ultima run: iniziale 3, tardivo
    12). Il probe non pretende più almeno un tick in attesa: descrittore e shard
    possono arrivare nello stesso frame, prima di qualsiasi tick (visto una volta
    con gated_ticks=0). Ogni tick in attesa controlla tutti i componenti. Gli stati
    applicati dai client (10 per l'iniziale, 6 per il tardivo) coincidono tutti
    con uno stato pubblicato dall'host per la stessa epoca e sequenza: nessun
    manifest parziale. La stanza del lab usa un solo shard (52 chunk).
  - Personaggio reale del client: breccia nel cartongesso attraversata (server
    Y=−325,9). Breccia nel mattone dopo 3 granate attraversata accovacciato
    (server X=318,4, client X=315,6).
  - Late join: lo shard porta solo i 19 chunk modificati su 52, 1390 byte di
    maschere (una bitmask completa del lab costerebbe 9698 byte). Byte ricevuti
    al primo snapshot pronto: iniziale 6461 (lab intatto), tardivo 9993
    (+3532, comprende anche gli altri attori).
  - Epoche: `ERCellMode 0` → epoca 2 a blocchi (504 istanze sui client).
    `ERCellMode 1` → epoca 3 a celle pulite, 0 chunk inviati. Granata → 13 chunk.
    `ERBlockRegenerate` → epoca 4 pulita. Granata sul legno: 8590 celle,
    18 chunk, 769 byte. Tutto coerente su entrambi i client.
- test-cell-network.py --loss (3% di perdita in ingresso, 40ms di lag sui
  client): exit0, 21/21. 1–4 pacchetti persi per client; gate 3/11 tick; byte
  al primo snapshot 11096 / 13436.
- Bug trovato e corretto (ERBlockRoomCells.cpp, `BuildCellComponents`): i
  componenti dei chunk sono Movable e creati a runtime, ma non erano
  net-addressable. Quando il pawn sta su un chunk (il bordo rimasto della
  breccia nel mattone), le correzioni di CharacterMovement arrivano relative a
  quel componente. Il client non riusciva a risolverlo e scartava ogni
  correzione: `ClientAdjustPosition_Implementation could not resolve the new
  relative movement base actor, ignoring server correction!` (53 volte).
  Risultato: client oltre la breccia (X=315), server fermo contro il muro
  (X=244, Y=−60, Z=110). Correzione: `SetNetAddressable()`. I nomi
  `Cells_P<id>_C<chunk>` sono deterministici su host e client per lo stesso
  layout. Lo script ora fallisce se quel warning ricompare.
- Regressioni dopo la correzione: test-cell-destruction.py 4/4 Success,
  --game PASS.
- Non verificato: test-cell-network.py --render (Metal). Non rieseguite le
  suite a blocchi: la correzione tocca solo i componenti delle celle. I gruppi
  ISM a blocchi sono anch'essi Movable, creati a runtime e non net-addressable.
  Nei log di test-destruction-network.py il warning non compare, ma su quella
  mappa nessuna correzione con base ISM è stata osservata.

## Gruppi ISM a blocchi come base di movimento in LAN — 17 ottobre 2026
- Domanda: i gruppi ISM della modalità a blocchi (`AERBlockRoom::Regenerate`,
  `NewObject<UInstancedStaticMeshComponent>(this)`, Movable, nomi automatici)
  hanno lo stesso bug dei chunk a celle? Sì.
- Perché i log precedenti erano puliti: nel probe nessun pawn remoto stava mai
  sopra un blocco. Pavimenti, scale e fondazione di `AERProceduralMap` sono
  default subobject con nome stabile. La breccia toglieva le righe 0–7, quindi
  l'attraversamento avveniva sul pavimento.
- Riproduzione (ERBlockNetworkProbe.cpp, test-destruction-network.py):
  - La breccia nel muro 5 ora toglie le righe 1–8 e lascia la fila W5:C*:R0
    (25cm, sotto il passo di 30cm). Il client iniziale la scavalca.
  - A fine fase 3 il server lo teletrasporta sopra W5:C4:R0 (`Place`, cioè
    `ForceClientAdjustment`). In fase 5 client e server verificano che
    `CurrentFloor` risolva proprio quel blocco (`block_movement_base`,
    `authoritative_remote_rests_on_block_group`).
  - `CollisionParity` ora ignora i personaggi: un pawn fermo nella breccia
    non conta come blocco.
  - Lo script passa `p.NetShowCorrections 1` e fallisce se un log contiene
    "could not resolve the new relative movement base".
- Prima della correzione: host `FNetGUIDCache::SupportsObject:
  ...ERProceduralMap_0.InstancedStaticMeshComponent_20 NOT Supported` a ogni
  correzione. Client `ClientAdjustPosition_Implementation could not resolve the
  new relative movement base actor, ignoring server correction!` 47 volte.
  Il client restava sul pavimento (X=363,7, Z=74,65), il server sul blocco
  (X=235, Z=119,5), con errore costante di 96,8cm. Fase 5 FAIL.
- Correzione (solo ERBlockRoom.cpp, `Regenerate`):
  - nomi deterministici `Blocks_<cx>_<cy>_<cz>_M<materiale>`, uguali su host e
    client per lo stesso layout (chiave chunk+materiale, unica per stanza);
  - `SetNetAddressable()`;
  - prima di `DestroyComponent`, il gruppo morto viene rinominato
    (`DeadBlocks_N`), così un nuovo Regenerate con lo stesso layout non
    ricrea un oggetto con un nome ancora occupato. Il probe lo esercita: fase
    9→10 rigenera lo stesso layout.
- Dopo la correzione:
  - test-destruction-network.py: exit0, 21/21 (18 di prima + 3 controlli
    `unresolved_movement_base_corrections=0`), `ER_BLOCK_NET_PROBE_COMPLETE
    result=PASS epoch=3`. Client e server fermi su `Blocks_0_1_0_M0` a
    X=235, Z=119,5. 0 "NOT Supported", 0 correzioni scartate.
  - --loss: exit0, 21/21, PASS, 0 correzioni scartate.
  - test-destruction.py: exit0, 20/20 Success (Cells inclusi).
  - test-cell-network.py: primo run exit1. Fallisce solo il controllo
    `gated_ticks>0` della fase 1 del client iniziale (snapshot applicato prima
    del primo tick in attesa), mentre parità, digest e tutte le altre fasi
    passano. È un controllo racy del probe a celle, che il suo autore sta
    correggendo. Secondo run identico: exit0, 21/21, PASS. 0 correzioni
    scartate (anche nell'epoca 2 a blocchi).
- Non verificato: --render (Metal). Stesso schema, non toccato qui:
  `AERProceduralMap::ConfigureTemplateFloors` crea `FixedMeshes`
  (NewObject, Movable, BlockAll) senza nome né `SetNetAddressable`. Sulle mappe
  template i pawn ci camminano sopra. Inoltre `ClearCells` e
  `BuildCellComponents` riusano i nomi `Cells_P<id>_C<chunk>` senza liberarli
  prima (finora senza errori osservati).

## FixedMeshes delle mappe template come base di movimento in LAN — 17 ottobre 2026
- Domanda: i gruppi `FixedMeshes` di `AERProceduralMap::ConfigureTemplateFloors`
  (`NewObject<UInstancedStaticMeshComponent>(this)`, Movable, BlockAll per i
  pezzi non visivi, nomi automatici) hanno lo stesso bug dei gruppi a blocchi?
  Sì. A Dust town quasi tutta la città fissa (muri in pietra, scale, ponte di
  Short A) è fatta di questi gruppi BlockAll.
- Indizio già presente: anche nei log di test-dust-template.py --network del
  15 ottobre l'host scriveva `FNetGUIDCache::SupportsObject: ...
  ERProceduralMap_0.InstancedStaticMeshComponent_4 NOT Supported` mentre il
  pawn dell'host camminava sul ponte (base replicata ai simulated proxy).
- Riproduzione (ERDustTemplateProbe.{h,cpp}, test-dust-template.py --network):
  - 3s dopo la fase 6 il server teletrasporta il primo client remoto sul
    ponte di Short A sopra `TownCrossing` (`PlaceRemote`: `TeleportTo`,
    `ForceClientAdjustment`); 3s dopo la fase 7, cioè dopo la rigenerazione
    con seed 7919, fa lo stesso con l'ultimo client entrato (Late). Il
    PlayerState scelto è replicato (`DeckPlayer`): i nomi `?Name=` vengono
    riassegnati dal game mode, quindi si sceglie per ordine di ingresso.
  - Il client scelto scrive `ER_DUST_CLIENT_BASE_PASS/FAIL` (entro 10s): a
    terra, base di movimento di proprietà della mappa, entro 40cm dal punto.
  - Lo script passa `p.NetShowCorrections 1` e fallisce se manca la riga
    BASE_PASS, se un client contiene "could not resolve the new relative
    movement base" o se l'host contiene `SupportsObject ... NOT Supported`.
- Prima della correzione: exit1. Client iniziale 242 correzioni scartate,
  Late 127. Entrambi restavano a terra vicino allo spawn (Z=94,65,
  `base=IndestructibleFloor`) mentre il server li teneva sul ponte (Z=414).
  Host 374 "NOT Supported": `InstancedStaticMeshComponent_4` (seed 42) e
  `InstancedStaticMeshComponent_15` (seed 7919, i nomi automatici avanzano a
  ogni rigenerazione).
- Correzione (solo ERProceduralMap.cpp):
  - nomi deterministici `TemplateFixed_<N>`, N = indice di creazione del
    gruppo. L'ordine segue `Layout.FixedPieces`, identico su host e client per
    lo stesso layout (stesso digest);
  - `SetNetAddressable()`;
  - in `ConfigureFloors`, prima di ricreare i gruppi, quelli vecchi vengono
    rinominati (`DeadTemplateFixed_N`) dopo `DestroyComponent`, così i nomi
    sono liberi. Il probe lo esercita: in fase 7 Late è sul `TemplateFixed_4`
    del layout rigenerato.
- Dopo la correzione:
  - test-dust-template.py --network: exit0, PASS. Initial fase 6 e Late fase 7
    `ER_DUST_CLIENT_BASE_PASS base=TemplateFixed_4` a Z=414,65 (server Z=414).
    0 correzioni scartate, 0 "NOT Supported".
  - test-dust-template.py (locale): exit0, PASS.
  - test-map-template.py --network (Killhouse, usa anch'esso `FixedMeshes`):
    exit0, PASS. 0 correzioni scartate, 0 "NOT Supported" (lo script non passa
    `p.NetShowCorrections 1`).
  - test-destruction.py: exit0, 20/20 Success.
  - test-destruction-network.py: exit0, 21/21, `ER_BLOCK_NET_PROBE_COMPLETE
    result=PASS epoch=3`, 0 correzioni scartate.
- Non verificato: --render (Metal) e --loss. Nel probe Killhouse nessun pawn
  remoto viene messo di proposito su un gruppo `FixedMeshes`.

## Celle 5cm ovunque, blocchi rimossi, crollo sulle celle — 18 ottobre 2026
Richiesta dell'utente: togliere i vecchi blocchi dappertutto e portare il crollo
sulle celle. Rimossi gruppi ISM, record/ID a blocchi, pagine protocollo 6, grafo di
supporto a blocchi, sonda e test a blocchi; le mappe generano pannelli di celle.
- Editor Mac Development: Succeeded.
- test-destruction.py: 11/11 Success (Cells.StorageAndCalibration, MeshAndCollision,
  BallisticsAndBlast, NetworkTransactions, DamagePreset, ProceduralLayout,
  ProceduralCollision, MultiFloorLayout, MultiFloorCollision, MapTemplate,
  DustTemplate). Celle: piano 10 stanze 1,71M; 3 piani 2,42–2,45M; Killhouse
  0,46–1,37M su 512 semi.
- --game (laboratorio a celle) PASS; --collapse PASS (demo: 2 isole da 551 celle,
  frammenti che cadono davvero, limiti 8/passo e 64 globali anche su due stanze;
  guscio intero da 1,69M celle staccato in 52 passi, p95 13 ms, picco 121 ms);
  --procedural PASS (la breccia percorribile è su una parete sottile: col preset
  il mattone da 20cm non si apre con un'esplosione sola).
- test-destruction-stress.py --samples 3 --label cells18oct: PASS, 414 righe.
  Chunk 160cm: generazione 151 ms, rimozione singola p95 0,33 ms, 256 celle
  sparse 34 ms, verifica di supporto 9,9 ms, granata 5,8 ms, memoria 33,5 MiB
  (25,7 MiB copie CPU delle mesh), 704 corpi.
- test-cell-network.py PASS (crollo replicato ai client); test-destruction-network.py
  PASS (3 epoche, 2,45M celle, 1162 chunk, 10 shard, late e dense join coerenti);
  test-map-template.py --network PASS; test-dust-template.py --network PASS.
  Con --loss (3% perdita, 40 ms): test-destruction-network.py e test-cell-network.py PASS.
- Non verificato in questa tornata: --render (Metal) delle mappe procedurali e template
  a celle; il laboratorio a celle era stato ispezionato su Metal il 17 ottobre.
  Corretti nelle sonde: nome del giocatore inviato una sola volta (sovrascritto da
  "Recruit") e una cella di finestra con il vetro nello stesso piano della tavola.

## Catture Metal delle mappe a celle, fessure nelle template — 18 ottobre 2026

Catture --render su Metal di mappe procedurali, Killhouse e Dust, ispezionate a vista.
- Procedurale (Room, Corridor, Breach): illuminazione e materiali corretti, aperture di
  porte e finestre corrette, bordi della breccia a gradini da 5cm su parete di legno.
- Killhouse: le misure derivano da Source (x2,54), quindi quasi nessun pezzo è un
  multiplo di 5cm. Il port troncava ogni pezzo a celle intere: le catture mostravano
  una riga di luce tra pezzi impilati (fessure da 2,5cm, attraverso cui si vede) e le
  casse più sottili di 5cm sparivano. Correzione: ogni pezzo mantiene il volume esatto
  e ogni asse ha round(L/5cm) celle uguali (FERCellWorld::FitCell). Dopo la correzione
  le casse a sinistra dello spawn ricompaiono; seme della sonda: celle da 813.792 a 815.712.
- MapTemplate ora verifica che non ci siano fessure sotto i 5cm tra pezzi affacciati
  (pannelli e pezzi fissi allineati agli assi, 6 semi). Controprova: reintroducendo il
  troncamento il test fallisce (fessure da 2,50cm sul seme 42); con la correzione passa.
- Riga di puntini chiari nella cattura del tetto Omega: non dipende dalle celle.
  Una diagnostica temporanea con raggi dai pixel (poi rimossa) mostra che i puntini
  cadono sul pavimento oltre la porta posteriore (Y≈3410), visto sotto l'architrave.
  Il raggio verso il sole da quei punti passa esattamente al giunto tra la cima della
  parete del capannone (Y 3700, fino a Z 1050) e il tetto. Le tegole del tetto
  (RoofMesh) coprivano solo l'interno dell'arena e toccavano la parete lungo uno
  spigolo, quindi la shadow map lasciava passare una lama di sole. Lato Alpha pulito
  perché il sole arriva da +Y. Correzione: le tegole coprono Size+40 come il soffitto
  di collisione, cioè anche la cima delle pareti da 20cm. Dopo la correzione la riga
  non c'è più. Una sovrapposizione di 0,2mm tra i quad (ipotesi T-junction) non
  cambiava nulla ed è stata tolta.
- Dust (WoodenGates, Underpass, ShortA, Plan 42/7919): corretta. Ha solo ~15–18k celle
  distruttibili. Le viste dall'alto Plan sono a ~21 FPS con risoluzione dinamica 512x288,
  e ShortA una volta a 20 FPS; non è stato confrontato con l'epoca a blocchi.
- Binario finale: test-destruction.py 11/11 Success; test-map-template.py --gallery PASS;
  --procedural --render PASS. Dopo la correzione delle fessure (binario con la
  sovrapposizione, che riguarda solo il render): test-dust-template.py --render PASS,
  test-dust-template.py --render --network PASS, test-map-template.py --render --network PASS.
- Dopo la correzione del tetto, binario finale senza diagnostica: test-destruction.py
  11/11 Success; test-map-template.py --gallery PASS; test-map-template.py --render
  --network PASS. Il ritaglio del tetto Omega è pulito.

## 18 October 2026 — Standalone macOS universal package

User authorized standalone packaging. UE 5.8.3 / Xcode 27.1 produced a
Development `.app` with `x86_64 arm64` verified by `lipo`. Editor module,
native game targets, cook, stage, package and archive completed. Cook reported
zero errors and one legacy `Paragon_Proto_Retarget` Rig-class warning.

The initial package exposed missing loose menu background / Dock icon files.
`fdm.Build.cs` now declares `Branding/background.jpg` and `Branding/icon.png`
as NonUFS runtime dependencies; the build tools include exactly those runtime
branding files in snapshots, preserving the authoring originals separately.
The corrected app passes `codesign --verify --deep --strict`.

Packaged-app tests used Metal offscreen and isolated user preferences, loading
L_MainMenu and L_Depot and requesting clean exit: both returned zero, initialized
the engine, loaded the requested map and had no missing Slate image resources.
These checks cover startup/loading; interactive gameplay and Intel runtime were
not exercised. Distribution notarization was not performed.

The local build helper `scripts/package-macos.py` works around the UE5.8.3/macOS27/Xcode27
closed-stdin issue by running ApplePostBuildSync outside UBA. The installed
engine was not patched. Logs, receipt and the private 3.2 GiB app are retained
in the local build workspace; the corrected app is now retained under
`../builds/macos/20261018-175348-retina/dist/RespawnAndRage.app`. Generated scratch
files were removed after verification to recover disk space.

## 18 October 2026 — Finder icon and Retina packaging correction

User screenshot exposed the engine-default Finder icon and low-DPI standalone
window. The project lacked the modern Xcode asset catalog and Info template;
the packaged `NSHighResolutionCapable` was false despite Unreal High DPI being
already enabled. Added a project AppIcon catalog with all ten 1x/2x renditions
from the existing user icon, generated by prepare-branding.py, plus a project
Info.Template.plist with NSHighResolutionCapable=true. New binary-resource hashes
are pinned individually in the public-tree audit.

Compiled the project catalog with actool and prepared a separate APFS clone of
the successful universal app, replacing its icon catalog/Info.plist and restoring
the ad-hoc signature. `codesign --verify --deep --strict` and lipo checks pass.
A paired Metal offscreen startup test at the same 70% window size records the
old viewport 1210x782 and the corrected Retina viewport 2419x1564. Both load the
menu and exit zero. The corrected 3D scene remains capped at 1670x1080 through
the existing resolution governor; UI retains the Retina backing resolution.
The previous app was retained to avoid changing the user's running process.


## 18 October 2026 — Build version in the main menu

Game and Editor target pre-build steps generate an ignored Build/GameVersion.txt
with local build date and the first eight source HEAD characters. The Mac helper
preserves the original full commit in Git-free snapshots and records the version
in its receipt. NonUFS staging carries the stamp on all platforms; packaged Mac
validation now requires an identical copy. No Git/Python is used at runtime.

- Native Mac Editor Development arm64 compilation: PASS (18.67 s). Renamed
  a test-local MaxCells variable to avoid a pre-existing unity-build shadow error.
- Incremental up-to-date build: PASS (3.02 s), stamping still executed with zero
  compilation actions.
- Checkout commit lookup, Git-free snapshot stamp, unchanged-file timestamp and
  rejection of invalid commit hashes: PASS.
- Metal offscreen Main Menu startup/clean exit: PASS, exit zero. Runtime logged
  ER_BUILD_VERSION 2026.10.18.b3e95fe5. Saved/Testing/BuildVersion/menu.png
  visually confirms the version at bottom right, on the background below the
  three menu panels; generated stamp/source identity are Git-ignored.

This check used the Editor game runtime. A new standalone package, Linux/Windows
execution and packaged-file validation have not been run for this revision;
the previously built app remains unchanged.
