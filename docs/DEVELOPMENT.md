# Respawn & Rage — sviluppo Unreal

## Ambito concordato

Richiesta del 12 ottobre 2026: partire dal progetto Unreal locale,
realizzare una versione alternativa in Unreal e riuscire a eseguirla localmente.
La guida originale è conservata in `GAMEPLAY_BASE.md`.
Queste decisioni aggiornano la guida senza cancellarne i criteri di accettazione.

- **B10–B12 rinviati:** niente packaging, distribuzione, CI, pubblicazione o ZIP.
- La compilazione del modulo Editor serve esclusivamente a eseguire il C++ locale.
- Progetto mantenuto: `RespawnAndRage.uproject`; modulo `fdm`, titolo Respawn & Rage.
- Unreal fissato a **5.8.3**. First Person Blueprint con variante Arena Shooter
  già creata dall'utente; aggiunto un modulo C++ senza sostituire il progetto.
- Blueprint del template mantenuti come contenuti di riferimento. Le loro regole,
  bot, pickup e armi aggiuntive non entrano nel GameMode Respawn & Rage.
- `BP_ERCharacter`, `BP_ERGameMode`, `BP_ERPlayerController`, `BP_ERWeapon_Rifle`,
  `BP_ERProjectile`, `DA_Rifle_A`, Enhanced Input e mappe creati via script Editor.
- Prima mappa: deposito simmetrico 56 × 36 m, blocco centrale, due corsie laterali,
  coperture, due spawn per squadra senza vista diretta tra le basi.
- Prima UI implementata in Slate/Canvas nativo per evitare collegamenti manuali;
  la sostituzione con Widget Blueprint resta una rifinitura successiva.
- Asset provvisori: mannequin, fucile e animazioni già inclusi nel template Epic.
  Lyra non è una dipendenza di questo prototipo.
- Effetti di sparo/impatti provvisori tramite debug draw, senza audio definitivo.
- Aggiornamento richiesto dall'utente: bot di collaudo opzionali e bersaglio
  fermo. L'hosting LAN normale mantiene zero bot.
- Il launcher locale limita a 60 FPS e usa GI/riflessioni basse e ombre livello 1
  per il primo collaudo Mac. Il limite non certifica prestazioni; il profiling
  e le impostazioni dell'Editor restano da verificare.
- Il menu assegna focus esplicito a nome e pulsante Riprendi. La verifica con
  gli input UI automatizzati non ha dato risultati affidabili. L'utente ha
  confermato l'apertura del deposito tramite Ospita e inviato una schermata.

## Regole implementate da verificare

Alpha/Omega; quattro giocatori; 100 HP; friendly fire; 36 danni uniformi;
fucile automatico 600 RPM; 30+90; ricarica manuale 1,5 s; hitscan immediato
con primo impatto conclusivo, traccianti Niagara e decal. Server autorevole.
Respawn su nuova pressione di sparo/salto dopo almeno 3 s; input anticipato ignorato.
Protezione 4 s interrotta dal primo colpo autorizzato. Scoreboard persistente nel
PlayerState. 50 eliminazioni nemiche / 10 minuti / nuova partita dopo 10 s.

## Stato verificato

- Ambiente rilevato e modulo Editor compilato con esito positivo.
- Mappe e asset generati e salvati realmente dall'Editor. Test gameplay con
  due processi e test di ammissione con quattro giocatori superati; copertura
  e limiti dettagliati in `TEST_RESULTS.md`.
- Git LFS 3.8.0 installato e configurato nel solo repository locale. Nessun
  commit o push effettuato; i file del progetto sono pronti per versionamento.
- Nessun repository remoto creato e nessun contenuto pubblicato.

## Correzione fucile e mani — 12 ottobre 2026

La schermata dell'utente ha confermato il difetto della prima visuale: il fucile
era ruotato e separato dalla posa delle mani. L'ispezione Editor ha mostrato che
`ue.Rotator(0, -90, 0)` produceva Pitch -90, anziché lo Yaw previsto. Gli script
ora usano sempre parametri nominati `pitch`, `yaw`, `roll`.

Il fucile FPS si attacca al socket Manny `HandGrip_R`, con trasformazione locale
neutra; il modello del fucile ha `GripPoint` nell'origine e `Muzzle` lungo +Y.
La stessa impugnatura è usata per la rappresentazione world. Il corpo FPS resta
dietro la camera (X -10 cm), evitando di visualizzare l'interno del torso.
L'attacco è ripristinato in BeginPlay anche per Blueprint
che conservano il vecchio parent Camera.

La posa ADS del template era troppo vicina alla camera e lasciava la mano
sinistra separata dal fucile. `scripts/create_rifle_pose.py` crea una copia
di progetto, `A_ER_Rifle_HipPose`: posa statica con IK delle due braccia,
lunghezze conservate e socket sinistro allineato al foregrip del modello.
Gli asset originali restano integri. Le dita e le animazioni di movimento,
sparo e ricarica rimangono provvisorie.

`scripts/fix_viewmodel.py` aggiorna il Blueprint esistente attraverso l'Editor;
`scripts/fix_spawn_rotations.py` corregge gli spawn Alpha/Omega e il sole nel
livello salvato, senza ricostruire la geometria. Compilazione riuscita e 19
controlli di rete superati dopo la correzione dell'attacco. La successiva posa
statica è stata salvata e verificata in una sessione grafica Metal: fucile in
basso a destra, rivolto in avanti, mano sinistra sul fucile e mirino libero.
Sessione locale lasciata aperta direttamente sul deposito.

## Prossimi criteri

Completare il collaudo fisico con i bot e due finestre, simulare latenza/perdita
e registrare i casi ancora parziali della tabella T01–T21. Dopo stabilizzazione,
integrare un pacchetto Fab alla volta conservando geometria e regole della mappa.

## Bot di collaudo — 12 ottobre 2026

L'utente ha approvato l'aggiunta dopo la proposta di test in solitaria. Questo
aggiorna il precedente vincolo “nessun bot” della specifica importata.

Il menu offre **Allenamento contro bot** e **Bersaglio fermo**. Entrambi avviano
il deposito con un avversario Omega; Alpha resta al giocatore locale. Il bot
mobile percorre dodici punti lungo le corsie esterne, rileva nemici entro 40 m
con una traccia di visibilità, mira con un piccolo errore e spara raffiche brevi.
La taratura iniziale era 300 cm/s per il bot e 600 cm/s per il giocatore;
è stata sostituita il 13 ottobre dalla nuova velocità comune di 10 km/h. Il bersaglio fermo non si
muove e non spara, ma conserva salute, protezione alla nascita, morte e respawn.

Tutti usano `BP_ERCharacter`, `DA_Rifle_A` e le stesse regole server di danno,
ricarica, punteggio e respawn: dopo la morte il controller bot richiede il rientro
appena trascorsi 3 s. Non viene cambiato il respawn manuale dei giocatori.
Il corpo visibile degli avversari usa la locomozione rifle del template.

`scripts/setup_bot_routes.py` aggiunge i punti alla mappa esistente senza
ricostruirla. Non è un sistema NavMesh generale: una mappa diversa richiede un
nuovo percorso; ostacoli dinamici possono far invertire la direzione del bot.
Non sono implementati ricerca strategica, coperture, navigazione di scale o
comportamenti competitivi. È un avversario di collaudo.

Le opzioni della mappa `ERBots=1` e `ERBotMode=Patrol`/`Target` attivano i bot;
il numero viene limitato a tre e il totale resta quattro. I giocatori umani
hanno priorità: un bot viene rimosso quando serve un posto e i posti liberi
vengono ripopolati fino al numero richiesto. I test aggiunti sono descritti
in `TEST_RESULTS.md`.
Il posto liberato rimane riservato durante l'ingresso: la pattuglia non viene
ricreata tra PreLogin e lo spawn del giocatore. Una connessione incompleta
libera la prenotazione dopo 30 s.

Entrambe le modalità hanno superato 28 controlli complessivi con un client
remoto locale; regressione gameplay (19) e ingressi (4 + 6) superate.
Il menu e l'avvio diretto con Metal sono stati verificati. Nella sessione
grafica il bot elimina il giocatore e Spazio lo riporta con nuovo HUD e arma.
Il collaudo della qualità delle animazioni del bot resta aperto.

## Controlli e menu — collaudo pratico

Verificati nella finestra Metal tramite input reali: WASD, salto, accovacciamento
con C, raffica con click sinistro, ricarica con R, Tab e apertura/ripresa del menu.
La navigazione Tab/Spazio avvia Bersaglio fermo dal menu. Il flag opzionale
`ERInputAudit` conserva dati di posizione, postura e munizioni per distinguere
queste prove dalle sonde di rete. Non è attivo nelle sessioni normali.

Aggiunto C come alternativa a Ctrl senza rimuovere gli altri binding.
Corretto lo sfondo del menu in partita: un brush pieno scuro rende leggibili
testi e pulsanti sulle geometrie chiare. Il crouch reale passa da capsula
di semialtezza 88 a 40 cm e abbassa la camera di 48 cm. Dettagli e limiti
in `TEST_RESULTS.md`.

Il prossimo intervento visibile è il feedback del fucile: animazioni e audio
di sparo/ricarica, mantenendo la posa corretta e le regole server già collaudate.


## Movimento e laboratorio — richiesta del 13 ottobre 2026

Questa richiesta precede il lavoro su audio/animazioni del fucile e aggiorna
le regole di movimento della specifica importata.

- Capsula in piedi alta **185 cm**, raggio 34 cm; occhi a 170 cm dal piede.
  Accovacciato: capsula 125 cm, occhi a 112,5 cm; camera e mira server allineate.
- Corsa leggera predefinita **10 km/h = 277,78 cm/s**; Shift sinistro o destro
  premuto: **5 km/h = 138,89 cm/s**. Bot mobili alla stessa velocità di corsa.
- Accelerazione 1000 cm/s², frenata 850 cm/s² e attrito direzionale 3:
  circa 0,28 s per raggiungere la corsa e 0,33 s per fermarsi da velocità piena.
- Salto: impulso 460 cm/s, gravità 1,8 volte quella standard (1764 cm/s²),
  quota teorica 60 cm e durata teorica 0,52 s. Nessun aumento di quota
  tenendo premuto Spazio; il tasto tenuto richiede invece la presa del bordo.
- Gradino automatico fino a 30 cm. Tra 30 e 80 cm, avanzando verso l'ostacolo,
  salita guidata di circa 0,42–0,54 s, arma visibile e utilizzabile.
- Oltre 80 cm: Spazio tenuto + avanti. Spalle assunte a 150 cm sopra i piedi
  correnti, anche durante il salto; quindi il salto amplia il bordo raggiungibile.
  Salita di circa 0,91–1,05 s, fucile nascosto in prima persona e fuoco sospeso.
  Per provare i blocchi 160–200 cm partire vicino al bordo, saltare e tenere
  Spazio mentre si avanza. Il blocco da 200 cm non si raggiunge da fermi.

`UERMovement` conserva gli intenti nella predizione standard di Unreal e li
trasmette con i movimenti compressi. La salita verifica parete, superficie
calpestabile e spazio per l'intera capsula, quindi usa spostamenti con sweep:
prima sale verticalmente e poi passa sopra il bordo. Collisioni sopraggiunte
interrompono la salita. È una prima implementazione procedurale; posa mani,
animazione di arrampicata e qualità sotto latenza artificiale restano da rifinire.

`L_MovementLab`: venti cubi da **200×200 cm**, altezze **10,20,…,200 cm**, in
quattro file di cinque con spazio per avvicinarsi. Ogni blocco ha cartelli
con quota. Colori blu fino a 30 cm, grigio fino a 80 cm, arancione oltre 80 cm.
Accesso dal menu **Laboratorio movimento** o `scripts/play-movement.command`.
Il laboratorio non aggiunge bot: in solitaria il TDM resta in attesa, ma il
movimento è attivo. Il deposito e le geometrie preesistenti sono conservati.

Collaudo completato: 57 controlli movimento, 19 gameplay LAN, 28 bot superati;
rendering Metal e cartelli verificati. Sessione finale aperta nel laboratorio.
Shift sulla tastiera fisica resta da verificare: il chord emulato non trasmette
il modificatore a Unreal. Dettagli delle prove in TEST_RESULTS.md.


## Rifinitura visuale del movimento — 13 ottobre 2026

Su richiesta successiva dell'utente: durante l'arrampicata alta nascondere anche
la mano sinistra, dare un leggero effetto al passo e aumentare il mouse.

Il ramo `upperarm_l` del solo corpo FPS viene nascosto insieme al fucile e
ripristinato alla fine della salita; questo rimuove anche avambraccio e mano,
senza staccare il fucile dal socket destro. La posa del template è conservata.

Aggiunta oscillazione locale della visuale in `CalcCamera`: ampiezza obiettivo
massima 0,7 cm verticale e 0,25 cm laterale, cadenza legata alla distanza
percorsa (circa due passi/s alla corsa). Ampiezza proporzionale alla velocità,
metà da accovacciati; raccordo morbido all'arresto e disattivazione in aria,
durante le salite guidate e alla morte. Il corpo FPS segue visivamente il
passo rispetto alla camera renderizzata; capsula, velocità, salto e punto
occhi nominale usato dal server non vengono spostati dall'effetto cosmetico.

Mouse: scalar dell'azione IA_Look da 0,12 a **0,18 (+50%)** su entrambi gli
assi. `scripts/tune_presentation.py` aggiorna solo lo scalar, conservando
le altre mappature e gli eventuali modificatori non scalari. Anche lo script
di setup iniziale usa il nuovo valore, evitando regressioni su rigenerazione.


Ritocco mouse successivo, approvato il 13 ottobre: scalar **0,16** su entrambi
gli assi, circa **11,1% in meno** rispetto a 0,18. Questo è il valore corrente;
resta più sensibile del valore iniziale 0,12.

L’utente ha richiesto di annullare l’ultima riduzione: scalar mouse corrente
nuovamente **0,18** su entrambi gli assi. Il passaggio a 0,16 è storico.

## Feedback dello sparo — 13 ottobre 2026

Richiesta successiva: procedere con lo sparo. Aggiunti animazione FPS,
rinculo cosmetico della camera, lampo alla volata e audio per ogni colpo
autorizzato dal server, attraverso l'esistente `MulticastShot` inaffidabile.
Nessun effetto viene prodotto da richieste respinte o dal caricatore vuoto.

`A_ER_Rifle_Fire` deriva dalla posa hip-fire di progetto: ciclo di 100 ms,
arretramento 1,4 cm e rialzo 0,18 cm del root visuale, ritorno alla posa iniziale.
Entrambe le mani e il fucile sul socket `HandGrip_R` si muovono insieme.
La copia delle pose disabilita il retargeting, evitando di applicarlo due volte.
L'animazione non estrae root motion e riguarda solo il corpo FPS.
La camera riceve un impulso visivo di 0,35°, limitato a 0,65°, con recupero
rapido; `ControlRotation`, camera fisica, mira server e proiettili restano invariati.
Morte e arrampicata alta nascondono il lampo e azzerano il rinculo.

Il lampo dura 35 ms ed è una piccola mesh Sphere con materiale additivo unlit
`M_ER_MuzzleFlash`, agganciata al socket `Muzzle` dei fucili FPS/world.
Il suono provvisorio riusa `FirstPersonTemplateWeaponFire02`, già incluso
nel template nella cartella GrenadeLauncher: audio 2D per il tiratore,
audio spazializzato alla volata world per gli altri, raggio pieno 3 m e
attenuazione nei successivi 37 m. Non è un nuovo campione rifle acquisito.
I riferimenti art sono nei default di `BP_ERCharacter`, assegnati da
`scripts/setup_shot_feedback.py`; geometrie e asset originali non vengono riscritti.

Rimangono per i prossimi interventi animazione/audio di ricarica, effetti di
impatto e animazione world dello sparo. Il lampo e l'audio world sono già attivi.
Esiti e limiti delle prove sono registrati in `TEST_RESULTS.md`.

## Corona munizioni e dispersione reale — 13 ottobre 2026

L'utente ha chiesto un indicatore simile al vecchio Respawn & Rage, attorno
al mirino, e ha scelto esplicitamente **dispersione reale**. Il riferimento
è `drawclip` in `src/game/hud.cpp` del progetto originale: corona munizioni
con glifi di dimensione fissa e raggio variabile. Il vecchio gioco è solo
consultato; non sono stati modificati o copiati i suoi asset.

L'HUD Unreal disegna una corona di trenta tacche: una illuminata per colpo
rimasto nel caricatore, posti vuoti attenuati, piccolo conteggio sotto la
corona e avviso arancione con il 20% o meno dei colpi. In ricarica la corona
resta color ambra e mostra R; le tacche si riempiono soltanto al trasferimento
autorizzato delle munizioni. I numeri di caricatore/riserva in basso restano.
Tacche con contorno scuro per contrasto; dimensioni scalate con il viewport.
Morte/menu rimuovono il mirino; il nuovo Pawn parte con il nuovo caricatore.

Prima taratura del fucile, tutti gli angoli sono semiangoli del cono:

- Da fermo in piedi: **0,18°**. Accovacciato: fattore **0,6**, quindi **0,108°**.
- Movimento: fino a **+0,55°** alla corsa di 10 km/h; a 5 km/h +0,275°.
  Il fattore crouch agisce anche sulla componente di movimento.
- In aria: **+0,9°**.
- Ogni colpo autorizzato aggiunge **0,12°** di dispersione da raffica,
  fino a **0,75°**. Recupero dopo 0,15 s senza colpi, a **1,2°/s**.

Il server campiona la direzione nel cono attorno alla normale mira dalla
volata e usa quella direzione per il proiettile effettivo. La verifica della
parete tra camera e volata rimane precedente allo spawn. Nessuna direzione
o seed di sparo è scelto dal client. Danno, RPM, munizioni e ricarica invariati.
Il cono usa velocità e postura reali del CharacterMovement; l'accumulo e
l'istante dell'ultimo colpo sono replicati solo al proprietario. Il recupero
si calcola col tempo sincronizzato del GameState, senza replica a ogni frame.

La corona legge `GetSpreadDegrees`, lo stesso valore usato dallo sparo.
Il raggio segue il cono proiettato tramite FOV più un margine fisso di 26 px
alla risoluzione 720p, necessario a separare le tacche; raccordo breve.
È un indicatore relativo della precisione, non il contorno geometrico esatto
dell'area dei colpi sullo schermo. Non riproduce la taratura di armi del vecchio
gioco. I parametri sono editabili in `DA_Rifle_A`.

Nuova sonda opt-in `ERAccuracyProbe`, avviata soltanto da
`scripts/test-network.py --accuracy`; diagnostica HUD opt-in `ERAccuracyAudit`.

Rifinitura richiesta successivamente: sostituite le tacche con pallini tondi
senza bordo, diametro 3 px a 720p (scalato col viewport). Accesi: bianco puro,
opacità 70%; spenti: nero, opacità 10%, anche durante ricarica e con poche
munizioni. Una fascia esterna con alpha sfumato ammorbidisce i bordi sul Canvas.
Numero/R sotto la corona conserva i colori di stato precedenti. Raggio e
calcolo della dispersione rimangono gli stessi. Attivato TSR nel renderer e il
supporto High DPI in game mode per Mac: prima `r.AntiAliasingMethod=0`
disattivava completamente l'antialiasing e il rendering Retina era disabilitato.

Impatti dei proiettili: rimossa la sfera rossa di debug. Il proiettile
autorizzato dal server replica posizione, normale e rotazione al momento
dell'impatto; ogni client applica `M_ER_BulletHole` alla parete. Il materiale
procedurale scurisce il centro, scheggia il bordo e aggiunge crepe corte;
scompare dopo 20 secondi. Gli impatti sui giocatori restano senza decal. Il
materiale si rigenera con `scripts/setup_bullet_impact.py` e non dipende da
immagini importate.

## Dimensione finestra e frame rate — 13 ottobre 2026

Menu principale e menu Esc includono un selettore 50%, 60%, 70%, 80%, 90%
e Schermo intero. La percentuale si applica a entrambe le dimensioni del
monitor che contiene il centro della finestra, nelle coordinate DPI del motore.
Schermo intero usa WindowedFullscreen alla risoluzione del monitor. Le scelte
applicano immediatamente UGameUserSettings, ignorando gli override iniziali del
launcher, e salvano le impostazioni. Il launcher conserva la sua misura iniziale.
Il testo del selettore legge modo e dimensioni effettive del viewport. Il menu
può scorrere verticalmente nelle finestre piccole. Un overlay locale mostra FPS
e millisecondi per frame con una media smussata, anche nel menu e dopo travel.
Il limite locale di 60 FPS e la risoluzione dinamica restano attivi: il contatore
mostra il risultato con queste impostazioni, non un benchmark senza limite.

## Rendering adattivo Retina — 13 ottobre 2026

L'utente ha misurato circa 45 FPS al 50% dello schermo e 10–15 FPS sopra l'80%,
preferendo la fluidità alla risoluzione. Il rendering ora separa dimensione della
finestra e costo della scena: il controller locale aggiorna la percentuale
secondaria quando cambia il viewport, limitando scena e uscita TSR a 1920×1080
(o meno se la finestra è più piccola), prima dell'upscale spaziale finale.
L'HUD conserva la risoluzione del viewport. La risoluzione primaria dinamica può
scendere fino al 50% di questa uscita; budget 13,8 ms, obiettivo circa 72 FPS con
headroom del motore. Cronologia TSR al 100% invece del precedente 200%, qualità
AA media nei launcher, VSync disabilitato e cap 90 FPS. Nessuna garanzia di minimo
FPS nelle scene più pesanti o quando il limite è CPU/GPU non scalabile.
L'overlay aggiunge la risoluzione 3D approssimata e “auto” se DynamicRes è attivo;
`-ERPerformanceAudit` registra FPS/ms, dimensioni e scala secondaria una volta al
secondo. Il precedente cap 60 è sostituito da 90 anche nei launcher bot/movimento.

Nota Mac: la presentazione Metal forza la sincronizzazione nelle finestre;
un monitor a 60 Hz può quindi mantenere il frame rate a circa 60 nonostante
VSync disabilitato e cap 90. La prova fullscreen nativa è registrata in
TEST_RESULTS.md, senza dichiarare un minimo garantito sopra 60.

## Reset dell'adattamento — 13 ottobre 2026

Il controllo rimane quello GPU di Unreal: una sola euristica modifica la scala
primaria. Per i cali persistenti la scala minima passa al 40% dell'uscita 3D
(al massimo 768×432 con uscita 1920×1080), con periodo minimo di modifica di
4 frame e intervento dopo 2 frame GPU consecutivi oltre budget. La qualità
risale più lentamente (`IncreaseAmortizationBlendFactor=0.05`) per limitare le
oscillazioni. Il menu principale e Esc includono “Reimposta risoluzione
automatica”: ResetHistory azzera la cronologia GPU e riavvia l'adattamento dalla
qualità iniziale, mantenendo finestra, fullscreen e partita. Un calo dovuto alla
CPU o alla sincronizzazione del monitor non si risolve abbassando i pixel; il
controllo GPU evita di interpretare automaticamente ogni attesa a 60 Hz come
carico grafico. Nessun secondo controllo basato sul contatore FPS smussato.

## Slider sensibilità mouse — 13 ottobre 2026

Menu principale e menu Esc includono “Sensibilità mouse” e slider 0–100%, con
default al 50%. Il modificatore IA_Look resta 0,18; il controller moltiplica
entrambi gli assi per (0,16/0,18) * 4^(2*slider-1), portando il nuovo default
a 0,16 (circa -11%) e gli estremi a 0,04 e 0,64. La curva logaritmica offre
variazioni proporzionali uniformi. Nessuna modifica ai Blueprint di input.
La preferenza vive nel GameInstance tra i travel; viene letta dalla sezione
EclipseRecoil.Input del GameUserSettings.ini locale e salvata alla fine del
drag/cattura tastiera e alla chiusura del gioco. Il valore UI è la posizione
dello slider, non una percentuale del vecchio moltiplicatore. Il menu blocca
la mira mentre si regolano i controlli. I valori caricati sono validati e
limitati a 0–1; dati non finiti ricadono sul default 0,5.

## Prima prova mercenario — 13 ottobre 2026

`BP_ERTrainingTarget` deriva da BP_ERCharacter e sostituisce solo la mesh del
corpo visibile in terza persona. Il GameMode sceglie questa classe soltanto
quando bStationaryBots e bTrainingTarget sono entrambi attivi; il bot Patrol
e il personaggio locale mantengono le classi precedenti. La scelta vale anche
al respawn. Capsula, danno, arma e regole di partita sono ereditati.

Si usa l'outfit completo dello starter pack per questa prova. Le 87 ossa del
mercenario corrispondono alla gerarchia condivisa del Manny; lo script verifica
anche le ossa necessarie per locomozione e fucile. Una copia dello skeleton
dichiara la compatibilità con Manny, mentre la copia della mesh riceve i socket
HandGrip_R/L tramite CopyTrainingGripSockets, helper editor limitato alla
cartella propria Mercenary. I riferimenti artistici restano nei default del
Blueprint. Scala 0,9201 per adattare l'altezza alla capsula da 185 cm, offset
Z -92,5 cm e yaw -90°. Nessun salvataggio degli asset originali Fab.

Prova con due bersagli: menu e launcher `play-bots.command target` richiedono
ERBots=2. Con almeno due bersagli richiesti il server bilancia i soli bersagli
tra Omega e Alpha, garantendo uno per squadra nella prova locale; la prova
ERBots=1 e i bot Patrol conservano il bilanciamento precedente. Limite totale
di quattro partecipanti invariato. Il colore del corpo segue la squadra
replicata: array di materiali nei default di BP_ERTrainingTarget, applicato
quando il PlayerState diventa disponibile o cambia squadra. Alpha blu, Omega
arancio su vestito, pantaloni e capelli; guanti/stivali scuri, pelle invariata.
Le normali regole di fuoco amico continuano a valere sul bersaglio alleato.

## Mercenari anche per i bot mobili — 13 ottobre 2026

La selezione di BP_ERTrainingTarget ora vale per ogni AERBotController,
indipendentemente dal modo Target/Patrol; il nome dell'asset resta invariato.
La classe è solo una variante di presentazione: l'immobilità dipende dal
controller, quindi il bot Patrol conserva movimento, percezione e combattimento.
Stesse livree Alpha/Omega selezionate dal PlayerState replicato, stesso ABP del
fucile e stessi socket. La scelta vale anche al respawn e al nuovo round.
Personaggio umano, bilanciamento dei bot Patrol e geometria restano invariati.

## Risposta mouse progressiva — 13 ottobre 2026

Richiesta: combinare rotazioni rapide e correzioni fini. Aggiunta una curva
basata sulla velocità del mouse, con slider Accelerazione mouse 0–100% nei
menu principale/Esc, default 50%. Sensibilità esistente conservata. A 0% il
guadagno è 1; al default passa da 0,75 per movimenti lenti a 1,8 per quelli
rapidi. Con sensibilità al 50% equivale a scala 0,12–0,288 rispetto alla
precedente 0,16. Transizione smoothstep tra 180 e 1800 counts/s stimati.

ERMouseResponse misura il modulo del delta IA_Look prima della sensibilità
utente, compensando il scalar asset 0,18 e usando il tempo reale del frame
FApp. Applica lo stesso guadagno ai due assi per conservare le diagonali; non
moltiplica nuovamente lo spostamento per DeltaTime. Nessuna cronologia della
mira, smoothing o inerzia aggiunti: il frame successivo lento riceve subito il
guadagno lento. Soglie dipendenti dai counts del dispositivo, quindi da tarare
con il mouse dell'utente; non sono una misura universale in cm/s. Enhanced
Input UE 5.8 legge KeyState.RawValue: la vecchia opzione di smoothing legacy
in DefaultInput.ini non è usata per applicare questa curva.

MouseAcceleration persiste insieme alla sensibilità in EclipseRecoil.Input
del GameUserSettings locale; clamp 0–1 e fallback 0,5 per dati non finiti.
Menu escluso dall'accelerazione. ERInputAudit ora registra ER_MOUSE_LOOK
(delta, tempo, guadagno e risultato) a frequenza limitata. Questa curva modifica
il normale input di ControlRotation, senza bypass della mira/combat server.

## Mercenario nella vista FPS — 13 ottobre 2026

FirstPersonBody di BP_ERCharacter e BP_ERTrainingTarget ora usa la mesh propria
del mercenario a scala 1. Solo braccia, mani e guanti sono renderizzati (quattro
slot su tutti i LOD); testa nascosta e nessuna ombra/collisione FPS come prima.
Materiali FPS separati da quelli world, selezionati dalla squadra replicata;
guanti Alpha blu e Omega arancio. Personaggio umano world ancora Manny, bot
world mercenario: la richiesta riguarda la nuova vista in prima persona.

La posa mannequin applicata direttamente spostava la presa sinistra di circa
9 cm. Generata una nuova posa IK sulle proporzioni reali, senza allungare i
segmenti delle braccia; animazione assegnata allo skeleton proprio tramite
helper editor SetMercenaryAnimationSkeleton limitato alla cartella Mercenary,
evitando un secondo retarget delle trasformazioni già valutate. I grip sono
alzati di 15 cm nello spazio mesh per stare entro la portata del rig, compensati
dall'offset del componente Z -167 invece di -152: posizione del fucile rispetto
alla camera conservata. Yaw -90 e attachment a HandGrip_R invariati.

Sparo FPS di 100 ms derivato dalla nuova posa, recoil root 1,4 cm e recupero
esatto della posa finale. Animazioni template e mannequin precedenti conservate.
Nascondimento del braccio sinistro durante high climb, bob, recoil camera,
capsula, mira server e regole del combattimento conservati.

## Rifinitura FPS dopo confronto utente — 13 ottobre 2026

Lo screenshot utente evidenzia una mano sproporzionata lungo il fucile. Nel
rig mercenario la traslazione dal polso all'index_01 è circa 10,54 cm, contro
6,45 nella posa Manny: avere i socket corretti non basta per una buona resa.
Le sole ossa hand_l/r nella posa FPS ora hanno scala 0,72, ereditata dalle dita;
il solver include lo spostamento del socket scalato prima di risolvere l'IK.
Ripreso l'orientamento relativo delle impugnature dalla posa ADS del mannequin,
anziché da quella già valutata sulle proporzioni del mercenario. Gomito
sinistro orientato verso il polo (35,60,135), più offset verticale 15 cm,
per riportare l'avambraccio nell'inquadratura.

FirstPersonRifle usa absolute_scale e scala 1: la riduzione della mano non
riduce l'arma. Le verifiche delle impugnature usano ora un riferimento rigido
senza scala, coerente con quello del fucile. Posizione arma/camera e scala
world del mercenario invariate; pose e materiali originali conservati.

La prova con mani ridotte mostrava ancora un avambraccio troppo voluminoso.
Aggiunta scala trasversale 0,6 sulle upperarm FPS (asse longitudinale X a 1),
ereditata dagli avambracci; le mani compensano questa scala per restare a 0,72.
Il solver conserva lunghezze, posizioni delle impugnature e dimensione arma.
Queste proporzioni appartengono alle sole animazioni FPS generate, non alla
mesh condivisa o alle animazioni world dei bot.

## Ripristino FPS per confronto — 13 ottobre 2026

L'utente richiede la vista precedente per produrre uno screenshot: avambraccio
sinistro dal centro inferiore dello schermo, mano sotto il fucile. Ripristinati
Manny Simple, A_ER_Rifle_HipPose e A_ER_Rifle_Fire, scala 1, offset (-10,0,-152),
yaw -90, materiali originali e tutte le sezioni FPS. Ripristino mediante
`scripts/restore_mannequin_first_person.py` su entrambi i Blueprint. Le palette
FPS e il filtro sezioni del mercenario sono disattivati; mesh world dei bot e
colori Alpha/Omega conservati. Asset FPS del mercenario mantenuti per il lavoro
successivo al confronto. Non rigenerare i default mercenario FPS finché
l'utente sta preparando il riferimento.

## Mercenario FPS dal riferimento approvato — 13 ottobre 2026

Riferimento fornito nello screenshot 16:39:27, con richiesta successiva di
tenere il bicipite fuori e mostrare meno avambraccio. La diagnostica opt-in
ERFPSAudit misura in gioco le ossa e il fucile nel riferimento camera.
Confronto tra asset caricati e runtime: rifle Manny (13,8795;11,6230;-24,0359)
cm; versione mercenario precedente circa 7 cm più bassa. I controlli precedenti
misuravano solo la distanza tra prese, non l'inquadratura assoluta dopo il
caricamento/retargeting, quindi non certificavano la posizione effettiva arma.

Offset FPS corretto a (-11,2263;1,4908;-159,9167), yaw -90, scala componente 1.
Polo del gomito sinistro (0,30,125), più offset di posa Z 15: avambraccio verso
il centro inferiore. Sono visibili solo LowerArm, Hand e Gloves (slot 2/4/11);
UpperArm escluso, mantenendo le ossa per IK e arrampicata. Scale FPS mano 0,72
e braccio trasversale 0,6 conservate. Arma a scala assoluta 1, palette di
squadra, animazione di sparo e mesh world invariati.

## Feedback dei danni e corpi a terra — 13 ottobre 2026

I proiettili ora applicano PointDamage con la direzione di viaggio catturata
prima dello stop. TakeDamage conserva la direzione per la morte e invia al
solo proprietario il feedback soltanto se la salute è effettivamente diminuita:
nessun feedback per protezione spawn o danno rifiutato. Impulso camera con
modulo massimo 1,5, offset massimo 1,2 cm, pitch/roll direzionali e decadimento
rapido a zero; nessuna modifica a capsula, ControlRotation o direzione di tiro.
Il suono di impatto mono viene spazializzato circa 140 cm verso la provenienza
del colpo. Suono provvisorio proprio di 160 ms, non una voce/animazione di dolore.

ApplyDeathVisuals resta idempotente e il server crea BP_ERCorpse separato dal
pawn morto. Mercenario completo e palette della squadra, ragdoll PA_MER_ME
originale, impulso lieve nella direzione del colpo e gravità. Contatti solo
WorldStatic; nessuna collisione con capsule o proiettili. Dopo cinque secondi
si azzerano le velocità, si addormenta la fisica e si interrompe il tick del
componente, mantenendo la posa a terra. Animazione fermata durante il ragdoll,
LOD 1 forzato per il corpo. Corpse indipendente dal pawn distrutto al respawn:
vita 90 secondi, massimo dodici, pulizia al nuovo round. Il corpo viene creato
e replicato dal server, ma la caduta fisica è cosmetica e simulata localmente
su ciascun peer: pose finali possono differire. La vista del morto scende di
55 cm, senza muovere la capsula o impedire la richiesta di respawn.

### Arma vicino alle pareti

Braccia, fucile e fiammata FPS usano il rendering FirstPerson di Unreal con
FirstPersonScale 0.2: la profondità renderizzata si comprime verso la camera,
senza cambiare dimensioni proiettate, posa o socket. Piano vicino a 1 cm per
conservare le parti della mano vicine alla camera. Impostazioni applicate in
BeginPlay anche ai Blueprint esistenti. Arma in terza persona, collisioni e
origine dei proiettili restano in coordinate fisiche originali. Nessun nuovo
asset. Il parametro di sviluppo -ERViewmodelWallTest posiziona il giocatore
a 36 cm dalla parete più vicina e misura il margine della volata renderizzata;
salva una schermata tramite Shot. Non attivato nelle partite normali.

### Arsenale Fab Low Poly (13 ottobre)

Importati i due ZIP forniti dall'utente in SourceAssets/LowPolyWeapons; sorgenti
FBX/TGA e hash degli archivi conservati nel progetto. Asset runtime in
Content/EclipseRecoil/Weapons/LowPoly: nessuna dipendenza dalla cartella Download.
Sedici mesh scheletriche originali, scheletri separati, palette T_Gun condivisa
e materiale proprio M_LowPolyGun. Nessuna animazione fornita dal pacchetto.

Sette principali selezionabili dal menu: AK74, M4, Uzi, Shotgun M4, M249, M107,
RPG7. Scelta salvata localmente, validata dal server e applicata alla prossima
nascita; nessun rifornimento o cambio di principale gratuito da vivi. M1911
di base, due HE RGD5, una fumogena e una Stun. Slot 1..5 e rotella selezionano
l'inventario replicato; cambio cancella raffica e ricarica senza trasferire o
ricreare munizioni. Niente pickup o raccolta dai corpi. Bot conservano AK74.

| Modello | Danno base | Intervallo | Munizioni | Comportamento |
|---|---:|---:|---|---|
| AK74 | 36 | 100 ms | 30+90 | Automatico |
| M4 | 33 | 90 ms | 30+90 | Automatico, ottica Optic_010 |
| Uzi | 26 | 70 ms | 30+60 | Automatico |
| Shotgun M4 | 20 per pallino | 350 ms | 7+32 | Sei proiettili per cartuccia |
| M249 | 32 | 80 ms | 100+200 | Automatico |
| M107 | 115 | 1455 ms | 5+10 | Semiautomatico, ottica Optic_02_Black |
| RPG7 | 140 massimo | 1200 ms | 1+4 | Razzo 2200 cm/s, esplosione raggio 350 cm |
| M1911 | 53 | 225 ms | 7+21 | Semiautomatico |
| HE RGD5 | 140 massimo | 800 ms | 2 | Lancio 1100 cm/s, miccia 2,5 s |
| Fumogena | 0 | 800 ms | 1 | Miccia 2,5 s; volume visivo 12 s |
| Stun | 0 | 800 ms | 1 | Miccia 2,5 s; flash visivo fino a circa 2,6 s |

Ruoli e ballistica base ricavati da config/csgopen/arsenal.cfg e tdm.cfg
del progetto originale, senza modificarlo. Usati nomi effettivi dei nuovi
modelli. M4, dispersione, ricariche ed esplosivi sono scelte di questo port;
nessun danno differenziato per ossa. Il danno esplosivo decade con la distanza
e usa un controllo WorldStatic tra esplosione e occhio del bersaglio; include
friendly fire e autolesione secondo salute/protezione/punteggio esistenti.
Feedback del danno e impulso del corpo usano la direzione dall’esplosione
al bersaglio, anche se la granata è rimbalzata dietro al giocatore.

Tasto destro: zoom 60° per le principali convenzionali, 75° per M1911 e 20° per M107; arma FPS mantiene
FOV 90° e profondità 0,2 per evitare tagli contro i muri. Lo zoom è presentazione
locale, senza cambiare ControlRotation o dispersione server. Palette e pose
proprie del mercenario restano; grip transform individuale orienta i modelli
+X sull'attacco HandGrip_R +Y. Posa idle/fire individuale con mano sinistra
adattata a ogni impugnatura; braccio sinistro nascosto per M1911 e granate.
Armi corte hanno un offset FPS proprio per restare visibili; il materiale
override ereditato dal fucile template viene rimosso dopo il cambio mesh,
così ogni nuova arma usa la palette importata. Munizioni HUD e indicatore leggono l'arma corrente, margine maggiore
per i cento pallini della M249.

Gli effetti sono una prima versione: razzo con sfera emissiva,
Stun con flash locale attenuato quando si guarda
lontano. Fumo non blocca la visibilità dei bot; nessuna animazione dedicata
di caricatore/otturatore/lancio o audio specifico del pacchetto. Le ottiche
non usano un render target; accessori laser/altre ottiche importati ma non
montati per default. Il bilanciamento e queste rifiniture restano iterabili.

scripts/import_low_poly_weapons.py importa solo asset assenti tramite Unreal;
scripts/setup_low_poly_arsenal.py genera dati/pose proprie e aggiorna soltanto
i default dei Blueprint. -ERArsenalProbe è un controllo esplicito a due peer;
-ERArsenalGallery salva le viste delle undici armi, mai attivo normalmente.

Nota UE 5.8: Materials del SkeletalMesh è una proprietà legacy transitoria;
l'assegnazione persistente della palette usa AssignWeaponMaterial, un helper
editor limitato al percorso LowPoly che chiama SetMaterials/PostEditChange.
La lettura in un secondo processo conferma M_LowPolyGun salvato sulle mesh.
Pistola e granate usano il template Pistol come base, mano FPS scala 0,5,
mentre le armi lunghe conservano 0,72. Animazioni ereditate del vecchio modello
d'arma vengono azzerate; pose del corpo e mesh importate restano separate.

### Rotella meno sensibile

Il primo impulso cambia uno slot immediatamente. Gli impulsi successivi dello
stesso gesto vengono ignorati e prolungano una finestra di quiete di 250 ms,
condivisa tra avanti e indietro. Nessun cambio viene accodato: anche l'inerzia
del trackpad produce un solo passo finché gli impulsi non cessano. Clock
monotono reale, indipendente dal frame rate e dal tempo della partita. Tasti
1..5 immediati e verifica server degli slot invariati. Nessun asset modificato.

### Esplosione HE con schegge animate

L'archivio Bomb Crack Ground contiene una frantumazione del suolo, senza
fiammata. Versione derivata senza pavimento: 128 schegge rappresentative con
animazione originale, ridotte a una mesh da circa 2.900 triangoli. La granata
HE mostra un lampo arancione di 120 ms e i frammenti per circa 2,02 secondi,
con dissolvenza finale di 400 ms. Nessuna geometria della mappa viene distrutta.

La detonazione rimane autorizzata dal server: danno, copertura e feedback dei
colpi usano il percorso esistente. Il multicast crea su ogni peer un attore
cosmetico locale, distinto dal proiettile che scompare dopo 300 ms. Schegge
senza collisione, senza ombre e senza replica dei singoli frammenti; massimo
otto effetti contemporanei e distruzione al termine. Scala 0,75 e velocità
1,5 nei default BP_ERGrenadeExplosion; nessuna modifica a fumo, stun o RPG.
Nessun audio aggiunto da questo pacchetto, che non contiene suoni.

Sorgenti e derivati in SourceAssets/BombCrackGround, runtime in Effects/Grenade.
`-ERGrenadeGallery` è un test grafico esplicito: una detonazione e cinque
fotogrammi tramite Unreal, mai attivo nelle partite normali.
Con `-ERGrenadeNoShots` campiona i tempi dei frame senza il costo di cattura.
Componenti del Blueprint configurati anche nei default; precache PSO del
motore richiesto al caricamento del primo loadout, prima della detonazione.

### Fumogena Niagara / Bullet Interactive Smoke

La sfera traslucida è sostituita dall'effetto fornito dall'utente: simulazione
Niagara Fluids con nube locale su ogni peer. La detonazione genera sul server
un ERSmokeCloud replicato; posizione e tempo iniziale sono autorevoli, durata
12 secondi, massimo quattro nubi e pulizia a fine round. GPU e densità visuale
non vengono replicate cella per cella. Nessuna collisione o danno della nube.

Versione originale UE 5.3 migrata con Unreal in 5.8.3; asset demo e mappe non
importati. Sistema proprio e Blueprint in Effects/Smoke, originale in
InteractiveSmoke, archivio sorgente in SourceAssets/InteractiveSmoke. Griglia
massima 40 celle (originale 45), dimensioni 6x6x4 m. Render target gestito per
istanza, senza condivisione fra nubi. Ultimi due secondi attenuano AppearFogParam.

I colpi autorizzati inviano al feedback il segmento nella dispersione realmente
campionata, fino al primo ostacolo della trace cosmetica. La nube riceve sfere
lungo il tratto interno, valide per 300 ms; il buffer è limitato a 128 punti.
I personaggi vivi nella nube contribuiscono alle posizioni di interazione.
Queste operazioni alimentano User.ActorPosSize come nel pacchetto originale;
non cambiano collisioni, traiettorie, danni o mira. La visibilità dei bot usa
ancora le trace geometriche e non considera la densità GPU del fumo.

Il profilo sg.ShadowQuality=1 disattiva r.VolumetricFog: per rendere anche la
parte volumetrica MD_Volume viene abilitato quel pass durante le nubi, con un
ExponentialHeightFog transitorio di densità quasi nulla se manca un supporto
attivo. Al termine viene ripristinata l'impostazione precedente e rimosso il
supporto proprio. Nessuna modifica ai livelli. -ERSmokeGallery verifica crescita,
colpo e termine; -ERGrenadeNoShots elimina la cattura PNG nelle misure dei frame.
Un'istanza locale senza densità prepara per 800 ms le pipeline della simulazione
al caricamento del primo loadout; non è replicata e non è una detonazione.

### Personaggi delle squadre — 13 ottobre, Twinblast / Drongo

Alpha usa ora Twinblast ActionHero, con volto umano e abbigliamento trattato in
verde oliva/antracite; Omega usa Drongo GDC, con tessuti marroni/scuri e metallo
opaco. Copie proprie in Characters/Teams, separate dai pacchetti Paragon.
La prima prova ShadowOps era troppo robotica per la richiesta e non è la
variante attiva. Armi Paragon incorporate nascoste su tutti i LOD; in Drongo
anche bazooka, boomerang, tubi della maschera e decorazioni d'osso. Gli asset
originali e le animazioni originali restano disponibili.

FERTeamAppearance raccoglie mesh, classe animazione, posa del cadavere, scala,
materiali e parti nascoste nei default Blueprint. Il team del PlayerState,
autorizzato dal server e replicato, seleziona lo stesso aspetto per umani e bot
su ogni peer. I cadaveri usano la stessa configurazione della squadra prima
della simulazione locale. Capsula, danni, scoring, respawn e limiti dei cadaveri
seguono il percorso esistente. La scelta non dipende dal materiale del marker.

IK Rig/FBIK e IK Retargeter propri adattano ABP_TP_Rifle, locomozione e posa ADS
ai rig Paragon. L'API batch UE 5.8 non restituisce i Blueprint duplicati: il
salvataggio esplicito della directory di output evita riferimenti a classi
presenti solo in memoria. Le ossa da nascondere sono filtrate sul rig della
mesh concreta, non sullo skeleton condiviso con tutte le skin Paragon.

I materiali derivati conservano texture e normali. Gli output Material
Attributes vengono separati prima di applicare desaturazione/tinta, emissivo
zero, metallic 0,05 e roughness 0,82. Il pin iniziale di Desaturation è senza
nome; i collegamenti vengono verificati. Pelle, occhi e capelli restano con i
materiali originali. Questo è un adattamento del look, non una ricostruzione
anatomica: le protesi/braccia di Twinblast e la stilizzazione di Drongo restano
nella geometria. Rimozione completa richiederebbe nuove mesh modellate.

Le braccia FPS continuano a usare il mercenario e le undici pose già adattate
all'arsenale. Guanti Alpha oliva e Omega marrone, senza alterare i grip o
l'inquadratura. Non sono braccia FPS retargettate dai due eroi.
`scripts/setup_team_characters.py` genera/configura gli asset; la verifica dei
riferimenti persistenti è in `scripts/verify_team_characters.py`.
`-ERTeamGallery` crea solo per la prova una coppia di modelli nel deposito,
salva Saved/Testing/Team-Characters.png e chiude il processo. Non modifica mappe.

### Correzione dei cadaveri Paragon — 13 ottobre, dopo lo screenshot utente

Riprodotti accessori visibili e deformazioni dopo la morte. Gli asset originali
scelti non erano ragdoll anatomici: TwinBlast_Physics contiene otto volumi
(mani/armi, piedi, root e testa); Drongo_Extents sei (torso, avambracci, piedi e
bazooka). Sostituiti sulle sole mesh proprie da PA_ER_Alpha_Ragdoll e
PA_ER_Omega_Ragdoll: 16 corpi (bacino, due parti del torso, testa, arti e mani/
piedi) e 15 articolazioni, traslazione bloccata e rotazioni limitate. Volumi
semplici orientati sul rig concreto; nessun corpo fisico sulle armi/accessori.
Collisione interna disabilitata, contatto soltanto con WorldStatic come prima.
Il profilo DefaultProfile delle articolazioni viene salvato esplicitamente:
Unreal scrive quello nel DefaultInstance durante la serializzazione.

ERCorpseMeshComponent applica nuovamente la riduzione a scala zero delle ossa
nascoste dopo il blend fisico e prima della pubblicazione del pose buffer. Le
flag di visibilità restavano corrette, ma il blend ricostruiva trasformazioni
con scala normale: la verifica precedente delle sole flag non lo rilevava.
Il cadavere usa LOD0 prima della prima valutazione; la posa ADS resta ferma,
con valutazione delle ossa attiva anche fuori campo fino allo stop a 5 secondi.
Questo mantiene dita, abiti e altri discendenti senza corpo fisico allineati.
Durata 90 secondi, limite 12, replica del team, respawn e reset invariati.

setup_team_ragdolls.py ricostruisce soltanto gli asset fisici; la generazione
completa setup_team_characters.py usa lo stesso helper ristretto a Teams.

### 14 ottobre — personaggi selezionabili

Cinque scelte per team nei default AlphaSkins/OmegaSkins di player, bersaglio e
cadavere; catalogo UI nel controller. Due preferenze persistenti locali, passate
nel travel e validate dal server (indici 0..4). ActiveSkin cambia alla nascita;
le modifiche dal menu Esc sono pendenti fino al respawn. Il cadavere replica
il suo indice iniziale e mantiene il vecchio aspetto. I bot usano indici diversi.
La palette è dominante e non esclusiva: cappotti/uniformi blu navy e freddi per
Alpha, stoffa arancio/ruggine per Omega; pantaloni, cinghie, scarponi e attrezzatura
neutri. Pelle e capelli rimangono naturali. Guanti FPS freddi/caldi, rig invariato.

Alpha: Ufficiale (ActionHero), Assalto (ShadowOps), Pattuglia (base + casco),
Ricognitore (Wasteland + zaino), Supporto (ShadowOps + casco/zaino). Omega:
Predone (base), Sciacallo (Scavenger senza decorazione orsetto), Incursore
(base + casco), Contrabbandiere (Scavenger + zaino), Miliziano (base + casco/zaino).
Non sono dieci corpi scolpiti da zero: le differenze derivano da abbigliamento
originale e accessori modellati nel progetto. AlienInvader, Electro, Halloween
e SummerTime non entrano nelle scelte per mantenere lo stile richiesto.

Casco e zaino sono mesh statiche proprie, create tramite ERSkinGear.cpp e Unreal;
nessuna collisione. Seguono testa/dorso anche nel ragdoll e la visibilità del
corpo. Modulo editor MeshDescription/StaticMeshDescription solo per generazione.
Gli originali Paragon e le animazioni retargettate esistenti restano conservati.
setup_team_skins.py gestisce anche materiali istanziati, conservandone le
texture e sostituendo il parent con una copia opaca priva di emissivo.
HasValidRagdollPose controlla i corpi/articolazioni, il profilo serializzato,
le scale realmente pubblicate delle parti nascoste e le lunghezze degli arti
con la scala dei parent, incluse le scale delle animazioni retargettate.
-ERCorpseGallery cattura i due cadaveri a circa 2, 7 e 12 secondi dalla creazione,
prima e dopo lo stop della simulazione, senza cambiare le mappe.

Il fermo del tick avviene nella prima FinalizeBoneTransform successiva alla
richiesta di stop, dopo la pubblicazione della posa fisica completa. Il controllo
delle lunghezze usa GetBoneTransform per entrambi gli estremi: i pacchetti
Paragon contengono socket foot_l/foot_r con lo stesso nome delle ossa, ma offset
circa 9–13 cm. GetSocketLocation non misura quindi la lunghezza della tibia.
Il tentativo di riscrivere tutte le trasformazioni fisiche è stato rimosso;
si conserva il blend del motore e si ripristinano soltanto le parti nascoste.

Rimossi anche i gruppi canister_pos e grenade_pos di Drongo in vivo/cadavere:
la gallery mostrava un piccolo contenitore separato vicino ai piedi. Nessuna
modifica alle granate, munizioni o slot dell'arsenale Respawn & Rage.


### 14 ottobre — avambracci della skin nella visuale FPS

Il tentativo di mostrare i materiali arms sulla mesh intera del personaggio,
usando il Blueprint animazione world e offset/scale del componente, è rimosso.
FirstPersonMesh nei default della skin seleziona una copia contenente soltanto
avambracci e mani. Il filtro geometrico considera le influenze delle ossa
lowerarm e discendenti, escludendo armi/accessori e le parti superiori.
ERFirstPersonForearms.cpp ricostruisce queste copie attraverso MeshDescription
in Unreal, mantenendo UV e indici dei materiali della skin e adattando il bind
allo scheletro del vecchio mercenario FPS. Gli asset world/Paragon restano intatti.

Il componente conserva posizione (-11,2263; 1,4908; -159,9167), yaw -90 e scala 1;
pose statiche/sparo dell'arsenale, offset per arma e socket HandGrip restano quelli
esistenti. Non vengono generate nuove animazioni per ciascuna skin. Gli asset
sono in Teams/{Alpha,Omega}/FirstPerson, referenziati da player e bersagli.
setup_skin_first_person_forearms.py genera le dieci mesh; setup_team_skins.py
conserva i riferimenti già presenti quando rigenera il catalogo.

L'avvio diretto della mappa poteva assegnare Inventory prima del BeginPlay del
pawn: quest'ultimo sovrascriveva Weapon con il fallback Rifle A, perdendo il
modello/posa dell'arma scelta. BeginPlay ora conserva Weapon se già assegnata.
-ERFPSForearmsGallery cattura il vecchio riferimento e le dieci varianti,
confrontando nel frame camera i grip, le ossa di avambracci/mani e il fucile.

Le influenze delle ossa twist Paragon degli avambracci vengono riportate al
segmento lowerarm della posa FPS, evitando l'applicazione di helper non
autorizzati dalle pose esistenti. Le copie conservano anche i bounds del vecchio
mercenario: i bounds della sola geometria in reference pose erano troppo stretti
e potevano escludere il viewmodel animato dal rendering. Non cambia la geometria
visibile né la collisione: si conserva soltanto il volume di culling.


## Bersagli fermi: tutte le varianti (14 ottobre 2026)

Il pulsante «BERSAGLI FERMI · 5 ALPHA / 5 OMEGA» apre dieci bersagli:
un esemplare per ciascun indice skin 0–4 di ogni team. Due file nelle zone
di squadra, distanziate di 180 cm, conservano posizione e skin al respawn.
I nomi sono Alpha 1–5 e Omega 1–5. I bersagli non camminano né sparano.
Questa modalità non consuma i quattro posti umani LAN; ingresso di un
client non rimuove i bersagli. I bot mobili restano limitati a tre e
liberano posti per i giocatori. Mappa e asset delle skin conservati.


## Audit repository e manutenzione (14 ottobre 2026)

Richiesta di valutare pulizia e separazione codice/asset. Creati clean-project.py
(anteprima predefinita, quattro profili, protezione file tracciati/processi/link),
audit-assets.py (registro Unreal in sola lettura) ed export-code.py (copia
locale senza asset o storia Git). L'analisi separa runtime, sorgenti per
ricostruire le skin e candidati da revisionare; mercenario ancora necessario
per rig/pose/post-process/fallback FPS. Raccomandati codice pubblico, Content
privato con percorsi stabili, archivi sorgenti privati versionati/hash su Drive.
Nessun asset eliminato, nessuna modifica indice/remote/storia/pubblicazione.
Dettagli e fonti in REPOSITORY_CLEANUP.md, ASSET_LICENSES.md e sintesi JSON.


## Mercenario scollegato (14 ottobre 2026)

Rig fitted SKEL_ER_FirstPerson e 24 pose spostati tramite copie Unreal in
Characters/FirstPerson; preview su avambracci Alpha, nessun collegamento
compatibile verso il vendor. Nessuna geometria/materiale del mercenario
copiata nel nuovo percorso. I fallback di player, target e corpse usano
Twinblast/Drongo; array materiali legacy svuotati. Il rig FPS salvato era
già senza post-process, che proveniva soltanto dalle vecchie dipendenze.
Pipeline corrente e audit aggiornati; vecchi script in scripts/legacy.

La migrazione confronta 88 campioni pre/post: zero errore sui due grip.
Verifica salvata 10 skin/11 armi/550 campioni, rete e Metal passano con le
due cartelle Mercenary assenti. 272 package non hanno referencer esterni;
ora possono essere eliminati attraverso Unreal. remove_legacy_mercenary.py
resta in anteprima senza -ERDeleteLegacyMercenary. Nessuna cancellazione
e nessuna modifica al remoto/indice. Le piccole copie rig/pose conservano
la provenienza dei dati, non diventano automaticamente asset pubblicabili.


## Pulizia eseguita (14 ottobre 2026)

Rimossi i 272 package Mercenary e 3399 package Paragon senza usi runtime,
necessità di ricostruzione o referencer esterni. Prima della rimozione
archivi indipendenti verificati SHA-256; copie pre-migrazione e fixture
spostate in _backups fuori dalla cartella progetto. Demo legacy UE non
compatibili causavano crash di ForceDeleteObjects: helper Editor valida
l'intero batch e rimuove package non caricati tramite Unreal, aggiornando
il registro. Asset vivi e mappe EclipseRecoil conservati.

Puliti log/screenshot/cache locali; build mantenuta funzionante. Cache LFS
4927 oggetti / 5,43 GiB verificata e spostata in _cache/lfs; solo config
locale lfs.storage aggiornata, niente prune/history rewrite/pubblicazione.
Progetto circa 4,1 GiB, Content 2,7 GiB. Verifiche salvate, rete, dieci
bersagli e gallery Metal world/FPS passano dopo la rimozione effettiva.


## Separazione codice/asset · 14 ottobre 2026

La scelta successiva dell’utente è un repository pubblico `respawn-and-rage/core`
e tre submodule privati (personaggi, armi/effetti, mappe/Blueprint comuni).
Il vecchio archivio privato originale resta privato. I percorsi Unreal sono invariati;
Content/SourceAssets sono collegamenti generati e ignorati. La struttura
attuale e i comandi sono in [ASSET_SUBMODULES.md](ASSET_SUBMODULES.md), che
sostituisce la precedente proposta di un solo submodule. Nessuna licenza
open-source del codice è stata scelta implicitamente.


## Sprint e posizione prona — 14 ottobre 2026

- Doppio W entro 300 ms, seconda pressione tenuta: sprint **18 km/h**, massimo
  **5 s**, poi recupero pari al **doppio della durata effettiva dello sprint**
  (aggiornamento 16 ottobre: 2 s di sprint → 4 s di recupero, 5 s → 10 s); tenere W non riavvia lo sprint. Stato e scadenze autorizzati
  dal server, intenti nelle quattro flag custom dei saved move Unreal.
- C/Ctrl premuto conserva l'accovacciamento; doppio tocco attiva la posizione
  prona persistente. Una nuova pressione risale verso accovacciato, il rilascio
  verso in piedi. I gesti si azzerano al menu e al cambio pawn.
- Accovacciarsi/rialzarsi: **0,4 s**, sdraiarsi/rialzarsi: circa **1,1 s**;
  capsula, camera e mira server cambiano progressivamente mantenendo i piedi.
  Movimento limitato a 45 cm/s durante le transizioni; da prono 55 cm/s.
  La rialzata attende spazio per la capsula finale. Salto e presa del bordo
  richiedono postura in piedi. Lo sprint termina con postura bassa, Shift,
  rilascio W, caduta o arrampicata.
- Prono: capsula verticale alta **68 cm**, occhi a **45 cm** dal piede.
  Collisione rimane la capsula del CharacterMovement; non è un volume anatomico
  lungo tutto il corpo. Un volume orizzontale supplementare di query copre
  i colpi/visibilità oltre la capsula (testa e gambe), senza ostacolare gli altri
  giocatori. La posa procedurale raddrizza le gambe, solleva la testa
  e conserva l'orientamento del fucile tramite un pivot comune delle braccia.
  Non introduce asset binari né animazioni mocap di strisciamento.
- Dispersione segue l'altezza effettiva e la transizione prona; la posizione
  completa usa il 65% del moltiplicatore accovacciato. HUD mostra sprint e recupero.
  La verifica grafica opzionale `ERPostureGallery` usa i personaggi Alpha/Omega.


## Hitscan ed effetti dei colpi — 14 ottobre 2026

Le armi con payload Bullet (fucili, pistola e fucile a pompa) usano ora
LineTraceSingleByChannel sul server. Il canale Projectile conserva i blocchi
delle capsule e delle superfici: Visibility ignora le capsule in piedi e non
è adatto al danno. Il server campiona la dispersione esistente, verifica
l'ostacolo tra occhio e volata logica, risolve ogni pallino e applica
ApplyPointDamage prima di terminare TryFire. Nessun AERProjectile per Bullet.
HitscanRange predefinito 60000 cm; munizioni, bloom, protezione e cooldown
restano autorizzati dal server. Rocket/HE/Smoke/Stun conservano movimento e fuse.

MulticastShot mantiene recoil, audio e muzzle flash già esistenti. La scelta
del tracciante arriva dal server: TracerEveryNShots=3, 1 per ogni sparo, 0
per disattivarlo. NS_ER_Tracer usa uno sprite Niagara orientato lungo il
segmento, senza particelle che rappresentino proiettili in volo; durata 55 ms,
larghezza 0,12 cm. Bounds espliciti comprendono tutto il segmento.
L'origine visiva è la volata dell'arma del peer; per l'owner si applica
la trasformazione FirstPerson della camera, rispettando la scala 0,2.
L'estremo finale è l'impatto reale o il limite di portata. Gli effetti vengono preriscaldati invisibilmente al caricamento del pawn/arma.
L'effetto non
calcola danno e non modifica la mira. Il feedback del fumo riceve la
trace autorizzata anche negli spari senza tracciante.

Ogni impatto sulle superfici multicast un burst Niagara orientato secondo
la normale (400 ms, simulazione locale e materiale caldo proprio) e una delle quattro varianti della PNG fornita
dall'utente. La PNG RGBA originale resta intatta in SourceAssets/BulletImpacts
nel submodule privato weapons. Un atlas 2x2 e quattro material instance
selezionano le varianti; indice e rotazione arrivano dal server. Nessuna
decal sui pawn. Dimensioni del foro 10x10 cm, profondità 1,5 cm, offset
0,15 cm; durata 20 s con fade negli ultimi 5 s, limite 128 decal e 64 burst Niagara simultanei per processo.
I default artistici sono salvati in BP_ERWeapon_Rifle/BP_ERCharacter.
setup_hitscan_effects.py genera gli asset propri sotto Effects/Shots;
setup_bullet_impact.py resta disponibile come fallback procedurale storico.

-ERHitscanGallery con una partita avviata (per esempio ERBots=2,
ERBotMode=Target) genera una parete temporanea, mostra le quattro varianti
e cattura Saved/Testing/Hitscan-Effects.png. Non salva modifiche alla mappa.
Le prove rete verificano danno immediato e assenza di attori Bullet; accuracy
controlla le direzioni delle trace e arsenal i sei pallini con una cartuccia.
Gli esiti effettivi sono registrati in TEST_RESULTS.md.


## Effetti discreti e bossoli — 14 ottobre 2026, revisione dopo screenshot

Rimosso il burst direzionale luminoso: l'impatto è un unico sprite Niagara
morbido da 1,6x1,6 cm, durata 35 ms, emissione sotto l'unità e opacità massima
0,12. Rimangono le quattro decal PNG. Il tracciante ogni tre colpi è molto
più tenue: larghezza 0,045 cm, colore (0,6; 0,45; 0,25), alpha 0,15, durata
ordinaria 55 ms. Muzzle flash ridotto a 1,2x2,5x1,2 cm e 18 ms, con materiale
M_ER_MuzzleFlashSoft proprio, senza cambiare i primitive FirstPerson.

Ogni secondo evento cosmetico di sparo Bullet espelle un AERSpentCasing
locale vicino all'otturatore, verso destra e leggermente in alto. È un
semplice parallelepipedo nero opaco (Cube Epic), 3x0,9x0,9 cm nel mondo,
ridotto alla scala 0,2 nella visuale FPS per seguirne la profondità compressa.
Gravità e CCD, contatto soltanto con WorldStatic: ignora pawn, visibilità,
proiettili e altri bossoli. Nessuna replica, danno o attore tick. Vita 3 s,
limite 32 per peer; anche gli eventi remoti generano la propria copia locale.
Razzi e granate non espellono bossoli.

Classe e materiali sono nei default Blueprint, generati attraverso Unreal
da setup_hitscan_effects.py; la logica di danno hitscan resta invariata.
La gallery disabilita gli input e isola la cadenza da eventuali clic iniziali
di focus. Verifica fisica/collisioni, discesa fino al pavimento e rimozione
prima/dopo il readback delle immagini. Esiti effettivi in TEST_RESULTS.md.


### Traccianti più leggibili — ulteriore feedback del 14 ottobre

Ripristinati i soli traccianti al livello precedente: larghezza 0,12 cm,
emissione dorata (4; 2,2; 0,65) e alpha 0,7. Durata 55 ms e cadenza uno
ogni tre spari. Bagliore d'impatto discreto, muzzle flash morbido e bossoli
continuano a usare la revisione precedente. Aggiornato il generatore e
modificato via Unreal soltanto M_ER_Tracer, senza rigenerare gli altri asset.


### Origine dalla punta durante il movimento — 14 ottobre, ulteriore revisione

I traccianti di sparo usano ora un attore cosmetico locale AERAttachedTracer
che aggiorna il segmento in TG_PostUpdateWork, dopo animazione e camera.
La punta visiva viene trasformata con il POV della camera del frame corrente,
poi spostata di 0,75 cm verso il punto colpito (offset limitato vicino ai muri).
L'estremo finale resta quello autorizzato dal server; l'origine segue invece
la punta attuale per tutti i frame visibili. Nessuna collisione/replica/danno.

Niagara ForceSolo con prerequisito sull'attore aggiorna i parametri dopo
il nuovo segmento, evitando la latenza della simulazione batched del frame
precedente. Durata ordinaria circa 20 ms, almeno un frame renderizzato:
a 60 FPS una o due immagini, senza lasciare una riga statica dietro al
giocatore. Camera/spread/hitscan non cambiano. Nei processi senza rendering
non vengono creati attori Niagara di questo tipo.

MuzzleOffset delle sette armi Bullet deriva dal centro degli ultimi 4 mm
di vertici reali del modello +X, includendo Y/Z invece di assumere zero.
fix_weapon_muzzles.py salva solo i data asset; il generatore dell'arsenale usa
lo stesso helper. Mesh, skeleton e trasformazioni dei grip sono preservati.

Rimossa la sfumatura longitudinale dal materiale della scia: nascondeva i
primi centimetri o metri in proporzione alla lunghezza del segmento, facendo
apparire l'origine lontana dalla canna. Resta la sfumatura trasversale.

-ERTracerMotion aggiunge una raffica in movimento laterale alla gallery,
con controllo della cadenza. -ERTracerAudit registra velocità, età del POV,
origine/endpoint e distanza dalla punta. Il campione della gallery può
durare 350 ms per il readback; i colpi ordinari mantengono 20 ms.


## Distruzione procedurale — 14 ottobre 2026

Richiesta attuale: importare PROCEDURAL_DESTRUCTION_SPEC.md e iniziare
la prima implementazione in **core**, preservando il gioco originale.
La specifica è conservata integralmente in docs/; il suo testo operativo non
sostituisce l'ambito concordato. Il modulo nativo resta `fdm` per compatibilità con gli asset / UE 5.8.3.

Prima fase: laboratorio standalone L_DestructionLab con interno 5×5×3 m,
pavimento indistruttibile e 504 blocchi logici, HP/materiali configurabili.
Core in ERBlockWorld/ERBlockRoom; ISM per chunk/materiale e mapping bidirezionale
ricostruito solo sul gruppo interessato. Arma/controlli esistenti riusati,
comandi console per danno, distruzione e rigenerazione; menu/HUD del laboratorio.
La partita si avvia con un solo giocatore esclusivamente in questa mappa standalone.

Replicazione e distruzione LAN rinviate: le mutazioni richiedono authority e
NM_Standalone; nessun RPC per danni arbitrari. Non implementati penetrazione,
esplosioni ai blocchi, crolli, stanze procedurali multiple o late join.
Preparazione del repository già presente: .uproject, build Editor, ignore cache
e separazione/LFS asset privati; nessun upgrade o nuovo progetto richiesto.
Vedere PROCEDURAL_DESTRUCTION.md e gli esiti reali in TEST_RESULTS.md.


## Milestone 2 — penetrazione ed esplosioni ai blocchi, 14 ottobre 2026

L'utente ha chiesto di proseguire dopo il primo laboratorio. Implementata la
seconda fase nel medesimo core/UE 5.8.3, sempre standalone.

ERBlockCombat calcola l'uscita dal volume orientato/scalato e prosegue il
colpo con energia residua, senza ignorare interi ISM. La resistenza, il danno
ai blocchi e alle esplosioni sono parametri di DA_ER_BlockMaterials; BlockShot
nel data asset arma espone energia, limite attraversamenti e angolo minimo.
Il danno a un bersaglio dietro usa l'energia rimasta. Il cartongesso può
restare solido dopo un colpo che lo attraversa; la breccia richiede HP esauriti.

Le esplosioni cercano nei bucket spaziali logici, attenuano per distanza dal
volume e tracciano una superficie esposta per il riparo. Tutti i candidati di
tutte le stanze sono valutati prima di applicare danni: un muro di cemento
abbattuto non lascia passare retroattivamente la stessa esplosione. Eventi
aggregati per ID e ricostruzione una sola volta di ogni gruppo interessato.
Integrati HE e razzi fisici del gioco; nessun effetto di smoke/stun sui blocchi.

Tre bersagli propri (cubi Epic e testo HP) dietro cartongesso/mattoni/cemento,
comandi ERBlockShot e ERBlockExplosion, indicazioni HUD. Tre decal circolari
procedurali proprie e il piccolo Niagara d'impatto già esistente; esplosione
cosmetica già presente, senza nuove simulazioni Chaos o suoni dedicati. Tutti
i nuovi binari appartengono al mount privato maps. Migrazione dei soli campi
nuovi una volta tramite metadata ERBlockCombatVersion=2; HP/materiali/art
esistenti preservati. Nessuna mappa originale rigenerata.

Le armi in mappe normali/LAN mantengono il primo impatto; l'ostruzione della
canna resta conclusiva anche nel laboratorio. Per i giocatori, i controlli
di riparo HE avvengono prima della rimozione dei blocchi. Nessun RPC nuovo.
Limiti: riparo a singolo campione, nessuna onda che aggira gli ostacoli,
penetrazione solo hitscan, massimo 16 attraversamenti, nessuna replica blocchi,
crollo o layout multistanza. Esiti reali in TEST_RESULTS.md.


## 14 ottobre 2026 — texture e Milestone 3

Richiesta: importare lo ZIP fornito e procedere con il piano procedurale.
Originali verificati (84 SHA256), ZIP/manifest/crediti nel submodule privato maps;
nuovo mount SourceAssets/ProceduralTextures. Importer Unreal preserva gli asset
esistenti, seleziona 41 mappe separate colore/DX/rough/AO/metal e crea dieci
superfici a proiezione world-space continua. Vetro traslucido parametrico separato.
Le superfici del catalogo cambiano; HP, energia, occlusione e FX rimangono quelli M2.

Generatore limitato a un piano: 6–10 stanze in due file con dimensioni/materiali/
finestre da FRandomStream, corridoio centrale e rami dedicati. Pareti ritagliate
alle aperture, ritorni agli angoli senza sovrapposizioni, pavimenti/soffitti ISM.
Un unico AERProceduralMap estende AERBlockRoom e carica lo storage validato;
nessun sistema di distruzione parallelo. L_ProceduralLab riusa GameMode/personaggio/
armi/input. Seed iniziale da proprietà/travel/flag e ERMapGenerate seed stanze;
rigenerazione ricolloca il giocatore, respawn calcolato dalla stessa configurazione.

Finestre vuote, vetro e materiali legno/pietra/metallo pronti solo come libreria
visiva. Nessuna replica, crollo, packaging o pubblicazione in questa fase.
Topologia centrale intenzionalmente semplice: il seed varia la geometria e
i materiali; packing libero e varianti di grafo restano estensioni future.
Validazione effettiva e misure in TEST_RESULTS.md.

Basamento asfaltato indistruttibile sotto le lastre interne (superficie a -20cm),
con bordo esterno percorribile dopo una breccia. Le superfici si toccano senza
sovrapporre volumi. PlayerStart mobili nel solo laboratorio procedurale: i
PlayerStart statici salvati impedivano la prima ricollocazione runtime. Luci
di riempimento calibrate a 3 e migrazione una tantum per preservare modifiche future.


## 14 ottobre 2026 — Milestone 4 stabilità/ottimizzazione

Profiler con tempi CPU per storage/grouping, clear/remove/add ISM e mapping;
query spaziale con bucket/esaminati/candidati; esplosione distinta in query,
copertura e mutazione. ERBlockStats espone tempi, memoria tracciata, RSS del
processo, corpi ISM ed effetti. Scope per Unreal Insights e runner CSV ripetibile.
Baseline misurata prima di cambiare l'algoritmo: 54 cicli di piano da dieci stanze,
seed42/7919 e chunk200/400/800. I batch da256 costavano circa44ms a chunk400.

Aggiornamento selettivo tramite handle nativi persistenti FPrimitiveInstanceId:
si rimuovono solo le istanze morte, si ricavano gli indici superstiti dall'engine,
nessuna ipotesi sugli swap e nessuna ricreazione dei corpi superstiti. Generazione
in batch, lookup hash chunk/materiale e componente/gruppo, conteggio chunk cached,
eliminata la copia dei record nel layout. Full rebuild resta solo generazione/
recupero di handle invalidati esternamente, su ISM runtime (mai HISM/Editor).

Batch256 sceso a1,821ms median a chunk400 (baseline43,901ms); single p95
1,211→0,032ms; memoria esclusiva tracciata5,676→3,779MiB. Chunk400 confermato:
chunk200 passa da93 a420 gruppi e genera più lentamente; chunk800 aumenta
il mapping di una rimozione singola. Nessun cambio ai blocchi50x20x25 o alle
regole danno/collisione/replica. Scope M4 standalone; crolli e rete successivi.

Test stress: 18 cicli automatici con rimozione del 90% e trace su tutti i
superstiti, più 54 cicli per run nativo/Metal con rimozione completa e GC.
Raffica reale di 30 colpi, limiti 0/24/hard64 per decal/Niagara e 8 burst,
culling per distanza; rigenerazione ed EndPlay ripuliscono effetti posseduti.
Nessun asset binario aggiunto/modificato per M4. Misure, metodologia e limiti
in DESTRUCTION_BENCHMARK.json; risultati effettivi in TEST_RESULTS.md.


## Milestone 5 — crolli controllati (14 ottobre 2026)

Implementato `FERBlockSupportGraph`: adiacenza geometrica tra facce, ancoraggi
alla base locale Z=0 o espliciti/indistruttibili, flood delle parti sostenute e
componenti isolate. Connessioni laterali sostengono architravi; spigoli/vertici
non trasmettono supporto. Il grafo attraversa chunk/materiali e copre lo stesso
actor della stanza o del piano procedurale. È un modello di connettività per
un piano, senza calcolo dei carichi; soffitti/pavimenti restano statici.

Rimozioni schedulano un timer da 50 ms con massimo un callback per frame; il
lavoro prosegue a passi, coalescendo danni intervenuti senza ricominciare a ogni
colpo. Default 4096 unità/1,5 ms per flood, 128 distacchi e quattro detriti per
callback. `Detached` resta nello storage con ID/HP, ma esce da ISM, collisioni,
query e danno; contatori separati dai blocchi distrutti direttamente.

`AERBlockDebris` è temporaneo: riutilizza mesh/materiale, simula solo un campione
con Chaos (32 actor globali default, hard64; spawn4/hard8 per callback), collide
solo con WorldStatic e non modifica player/proiettili/copertura. Simulazione
spenta a due secondi, scadenza a otto/hard20. Nessun actor/tick dinamico permanente
per ogni blocco. Regenerate/EndPlay eliminano detriti posseduti e timer.
I blocchi eccedenti il campione sono comunque rimossi: nessun resto sospeso.

Il laboratorio espone `ERBlockCollapseDemo`, che isola nove blocchi nella parete
sud, e statistiche di distacco/supporto/detriti in HUD e `ERBlockStats`. La
configurazione Collapse è esposta sull'actor. Restano authority/NM_Standalone;
replica, late join, più piani e ulteriori materiali appartengono alla M6.

Nuovi test `StructuralSupport` e `CollapseAndIdentity`; suite totale otto casi.
Il probe `--collapse` usa la partita salvata con fisica reale, include vita e
pulizia, cap globali tra due stanze e collasso completo di dieci stanze; la
variante `--render` usa Metal e salva immagini. Esiti/misure effettivi in
TEST_RESULTS.md e receipt COLLAPSE_BENCHMARK.json. Il receipt M4 resta storico.

## Milestone 6 — replica e ampliamento mappe (15 ottobre 2026)

Implementata authority anche nelle partite LAN dei laboratori. Colpi autorizzati,
perforazioni, esplosioni e supporto strutturale usano la stessa pipeline del
server; nessun RPC client per imporre BlockID/danno. Le mutazioni debug sono
riservate all'authority. Le mappe ordinarie e le relative armi mantengono la
pipeline precedente.

Replica: descriptor protocol6 con epoch/seed/config/layout fingerprint e
manifest versioni, più actor senza tick da128 record con HP/stato assoluti.
Questo mantiene ogni canale piccolo anche per late join dopo rimozione completa
(137 pagine per17.427 blocchi nel test a tre piani). Il client ricostruisce dal
seed, confronta geometria/IDs/profili/mesh collision GUID e applica la transazione
solo quando tutte le versioni corrispondono. Epoch precedenti ignorate, snapshot
invalidi rifiutati prima della mutazione. Il caricamento iniziale sospende movimento
locale/server fino all'ack dell'epoch sul controller posseduto; i teleport del
server usano la correzione nativa del movimento. I delta successivi conservano
la precedente collisione coerente fino al commit.

Nuovi piani1..3: stesso algoritmo per piano con seed offset, wall ID globali,
slab condiviso tra pavimento superiore/soffitto inferiore, ultimo soffitto opzionale,
scale statiche larghe150cm (alzata<=20/pedata30) e ritorno in corsia separata.
La base superiore è ancorata alla slab; nessun modello dei carichi. Limite50k
record e branch>=250 per l'edificio. Più actor di mondo indipendenti/streaming,
packing arbitrario, NavMesh dinamica e crollo dei solai restano esclusi.

Sette profili: legno60HP, pietra200, metallo400 e vetro8 oltre ai tre originali.
M6 assegna i materiali opachi per stanza, i vetri sottili alle finestre se richiesti.
La migrazione additiva setup_procedural_multiplayer.py ha aggiunto quattro righe
riusando wood_planks/stone_wall_02/rusty_metal_04/M_ER_Glass; nessun nuovo download.
Rerun verificato con added0/filled0 e SHA del catalogo uasset invariato. HP/art
preesistenti restano; bootstrap/import ora tollerano sette profili senza modificarli.

Menu **OSPITA EDIFICIO PROCEDURALE LAN** (due piani, sei stanze/piano, nuovi materiali,
vetro), join dalla casella IP, opzioni MapSeed/MapRooms/MapFloors/MapMaterials/MapGlass.
Console authority `ERMapGenerate 42 6 3`; terzo argomento omesso conserva i piani.
Snapshot di Detached replica la rimozione, non i detriti: questi restano locali,
limitati/query-free e non vengono riprodotti per late join. Impatti piccoli sono
multicast non affidabili con epoch/budget/culling. Statistiche includono epoch,
versione, pagine/readiness e buffer posseduti, esclusi heap actor/canali e GPU.

Undici automation test e probe LAN con host listen, client iniziale, client tardivo
post-crollo e ulteriore client tardivo post-rimozione completa. Verifica hash di
stato completo, collisioni campionate e mapping, fuoco client reale, attraversamento
brecce, salita/discesa di due piani, tre epoch e delta delle esplosioni. Varianti
loss3%/lag40ms e Metal. Esiti reali e limiti in TEST_RESULTS.md.

## Launcher unico — 15 ottobre 2026

Avvio tramite `scripts/play.command`; modalità, laboratori e dimensioni si scelgono
dal menu. Rimossi i launcher play separati. La finestra iniziale usa il 70%
di larghezza e altezza dello schermo: i punti AppKit vengono convertiti in pixel
con backingScaleFactor per Unreal high-DPI, evitando il precedente 35% su Retina.
La misura è passata all’avvio, prima di mostrare il menu; nessun resize al menu.

## Menu e sessioni — 15 ottobre 2026

Menu a tre colonne con scorrimento indipendente: partita, personaggio/armi,
impostazioni video e mouse. Locale/server/client condividono un unico avvio;
client sceglie nome, squadra e aspetto, mentre mappa e bot restano del server.
Locale/server consentono 0–10 bot Alpha e 0–10 Omega, fermi, solo movimento
o movimento/sparo. Budget bot esplicito separato dai quattro slot umani;
le vecchie opzioni ERBots dei collaudi mantengono il loro budget storico.
Il launcher forza MULTIHOME=0.0.0.0. La partita parte con un solo umano anche
su listen server; non attende più entrambe le squadre. Nei tre laboratori le
ricariche rinnovano la riserva lato autorità, mantenendo durata e capacità.
La scelta di squadra è validata dal server (solo Alpha/Omega).


## Template da mappa community — 15 ottobre 2026

Analizzato l'archivio Workshop526448928 con il parser Source già esistente,
letto senza cambiare il progetto csgopen: contiene de_killhouse.bsp, BSP21,
1584 brush,95 prop statici e20 spawn. Misure/aree materiali/prop/coordinate e
hash di provenienza salvati privatamente in SourceAssets/MapTemplates.
Adattamento proprio41x79m: bunker in compensato, tramezzi sfalsati, casse,
coperture basse, container cavi fissi e torre centrale con scala a20cm.

Nuovo UERMapTemplate configurabile come data asset, selezionato da LayoutConfig;
il generatore precedente resta invariato quando non è assegnato. Volumi di
assemblaggio con offset di gruppo deterministici limitati al reticolo50cm,
nessun file esterno letto a runtime. Venti spawn originali XY/orientamento
(conteggio20: dieci per squadra), trasformazione Source(X,-Y,Z)*2,54cm e
altezza100cm per la capsula. Parti distruttibili Wood/Concrete nel catalogo
proprio; involucro/container/piattaforma in ISM fissi con materiali PBR esistenti.

Mappa separata L_Killhouse e DA_Killhouse sotto Art/MapTemplates, salvati da
Unreal nel submodule privato maps. Menu locale/LAN include Killhouse procedurale;
MapSeed e console ERMapGenerate cambiano il seed. RoomCount ignorato per i
template, un solo piano. Descriptor replica il riferimento al template;
fingerprint esteso alle geometrie fisse, mesh/collision GUID, revisioni e spawn.
Late join/danni/epoch riusano il protocollo esistente, nessun RPC mutazione client.

FactoryEnvironmentCollect era in download: su richiesta dell'utente si procede
con i sostituti, lasciando FixedMesh per la sostituzione artistica successiva.
Nessuna mesh/texture Valve copiata, nessuna licenza community presunta verificata.
Dettagli, limiti dell'adattamento e procedura per altri template in MAP_TEMPLATES.md.


## Killhouse bilanciata e Factory — 15 ottobre 2026

La precisazione dell'utente e la schermata della mappa originale aggiornano il
layout: ingresso delle basi dal retro, finestre verso il campo, scala interna,
foro nel solaio e tetto praticabile con parapetti. Basi speculari per rotazione
180°, incluse scale, finestre, copertura e spawn. Il vincolo di bilanciamento
prevale sulla conservazione esatta dei vecchi spawn Omega: Alpha resta nel
riferimento originale, Omega deriva dalla stessa trasformazione del layout.

HalfTurn è il default dei nuovi template. Ogni gruppo viene campionato una volta
e duplicato tramite rotazione, incluse le celle parziali dei blocchi. Nessuna
randomizzazione indipendente delle due squadre. I pannelli esterni usano un
reticolo centrato per evitare piccole differenze di arrotondamento fra i lati.
I laboratori precedenti rimangono fixture di collaudo.

Arena più piena con pareti basse e ritorni a L, casse, container, serbatoi e
barili, tutti accoppiati. Torre con accesso da entrambe le estremità. Selezione
Factory caricata con Unreal dal progetto UE5.6 fornito: migrazione di dipendenze
in scratch isolato, rename/save via Unreal nel namespace privato
EclipseRecoil/Art/FactoryEnvironment e successiva migrazione in maps.
Nessun pacchetto sorgente Factory salvato. Il catalogo distruttibile e le sue
superfici/proprietà restano identici; solo le parti fisse ricevono mesh/materiali
Factory. Le mesh visuali non hanno query collision; volumi box di progetto
mantengono scale, copertura e piattaforme coerenti fra server e client.

Asset upgrade revision2 con hash del manifest, validazione nativa di103 seed distinti e
conservazione degli edit successivi. Tutte le modifiche binarie via Unreal.


## Killhouse con composizioni variabili — 15 ottobre 2026

La nuova richiesta estende la variabilità oltre gli offset: arena36..48x70..90m,
basi10..14x8,5..10,5m con posizione/altezza variabili e caratteristiche conservate.
Quattro-sei moduli per metà (otto-dodici complessivi), selezionati fra stanze
attraversabili con due porte e finestre laterali, muri a L/U fissi o distruttibili,
pareti sfalsate, container, serbatoi/barili e cataste di legno. Varia anche la
piattaforma centrale; accesso bilaterale conservato. Tutte le scelte sono
accoppiate per rotazione180°, incluse celle dei blocchi, materiali e spawn.

Il difetto della schermata derivava dal fitting di piattaforme/griglie Factory
complesse nei volumi di solaio/gradino: la collisione box copriva vuoti visibili
e dettagli stirati. Tetti, pianerottoli e gradini diventano box solidi visibili
con lo stesso volume di collisione e superficie Metal esistente. WalkableSurface
impedisce di sostituirli con mesh dettagliate aperte. Parapetti lungo i bordi e
protezione laterale del vano scala; solo il vano previsto resta aperto.

Ricetta Variation nel data asset, con palette Factory già importata; nessun nuovo
asset vendor. Layout.ArenaSize guida involucro, pavimento, digest e pattuglia.
Spazi di circolazione riservati prima degli ostacoli, celle separate e validazione
di sovrapposizioni, limiti, spawn e segmenti completi di percorso. Le basi e le
scale espongono metadati per test fisici indipendenti da coordinate fisse.
Nuovo seed per ogni avvio Killhouse dal menu; console/travel restano riproducibili.
Migrazione esplicita revision3 attraverso Unreal, candidato staccato validato
su516 seed prima del salvataggio; hash di mappa/catalogo conservati e marker
separato per preservare gli edit successivi. Riferimento revision2 conservato
come costruzione authored quando Variation è disabilitata, con fix superfici.


## Arrampicata sui parapetti Killhouse — 15 ottobre 2026

Il probe verticale dell'arrampicata cercava a raggio+55cm davanti al personaggio:
superava completamente i parapetti spessi20cm e non ne trovava la sommità. Ora
parte2cm oltre la faccia colpita, campiona a intervalli limitati il piano continuo
e trova il bordo con tre raffinamenti. Sui piani stretti sceglie un appoggio
interno; sui blocchi ampi conserva la precedente distanza di arrivo. Restano
limite150cm, requisito Spazio oltre80cm, capsula intera libera, movimento swept
e replica attraverso saved moves. Altezza del salto e geometria/materiali delle
basi invariati. Nuovo test reale host/client sui quattro lati di entrambe le basi
tramite il normale binding Spazio e input di avanzamento.


## Bot solo movimento dimostrativi — 15 ottobre 2026

Estesa la modalità esistente Solo movimento: salti ordinari periodici,
accovacciamento in cammino, pausa da proni, strisciamento e ritorno in piedi.
Ciclo22 secondi sfalsato per controller; usa Jump/Crouch e gli intenti del normale
UERMovement, senza teletrasporti o animazioni fittizie. In postura eretta i bot
provano anche l'arrampicata; dopo una traversata sostano brevemente sull'appoggio.
Una ricerca locale ogni2 secondi usa al massimo12 direzioni entro7m e sceglie
ostacoli30..150cm con percorso della capsula libero e suolo campionato all'approccio.
I bot non usano altri personaggi come obiettivi della ricerca. Le posture non
interrompono una traversata in corso; una risalita bloccata da soffitto torna a
muoversi lentamente invece di attendere per sempre.

Il comportamento è server-authoritative, riusa replica/posture/skin esistenti e
azzera gli intenti al respawn. Solo movimento continua a non sparare; Fermi e
Movimento e sparo mantengono il loro ramo precedente. Nessuna mappa o risorsa
binaria nuova: per osservare altezze diverse si può usare il Laboratorio movimento
esistente. Nuovo probe con due bot veri: si limita a collocare il test davanti ai
blocchi80/140cm, poi osserva le decisioni normali e il modello dal client remoto.


## Dust Mirror urban/desert — 15 ottobre 2026

La richiesta usa Workshop3117367319 come riferimento per un template, con
priorità all'incrocio su due livelli e alle porte strette. La mappa CS2 annidata
è completa (561 file,15 spawn per squadra), a differenza dei precedenti
placeholder. Geometria esportata per analisi locale con Source2Viewer20.1;
nessuna mesh/texture CS2 entra negli asset Unreal.

Nuovo DA_DustMirror/L_DustMirror, selezionabile nel menu locale/LAN e con seed
nuovo per sessione. Adattamento simmetrico60x80m: passaggio sopra/sotto, catwalk
a320cm, luce inferiore290cm, scale20cm e quattro varchi110cm. Geometria critica
fissa; offset procedurali delle coperture accoppiati per rotazione180°.
Non è la ricostruzione completa di DustII e non varia ancora la composizione
come Killhouse. Catalogo distruttibile e altre mappe preservati.

Muri/ante/pavimento da MOUT_Arabian, selezione27 pacchetti con migrazione Unreal
attraverso scratch isolato e namespace privato Art/ArabianEnvironment. Nessuna
parte industriale Factory. Primo controllo fisico ha rilevato un parapetto
che chiudeva l'arrivo delle scale: revisione2 apre le due bocche delle scale,
con controllo fisico e test del personaggio dedicati.


## Dust town revision3 — vicoli, cortili ed edifici

La piantina allegata dall'utente sostituisce l'arena precedente: schema
macro asimmetrico con T a sud, CT a nord, cortili B/A a ovest/est, Mid, tunnel B,
lungo e Short_A sopra CT_A. Il vincolo HalfTurn resta a Killhouse. La ricetta
Town nel data asset campiona proporzioni dell'area, deformazioni condivise degli
assi, larghezze delle vie, altezze e sagome degli isolati. Gli edifici sono il
complemento dell'unione di strade/cortili; i seed cambiano quindi la disposizione
urbana intera, mantenendo i collegamenti del riferimento.

Passaggio su due livelli invariato nella funzione: Short_A a320cm, luce CT290cm,
scale20cm alle estremità. Tre portali ad arco con ante curve di legno naturale
socchiuse20..30gradi e varchi110..130cm, come la seconda foto. Mesh di progetto
importate tramite Unreal; volumi delle ante ruotati, collisione dell'arco a
segmenti, decorazioni senza collisione. Palette/materiali salvati privatamente;
catalogo distruttibile e Killhouse preservati. Native metadati di strade, scale,
porte e incrocio guidano test fisici e probe, senza le vecchie coordinate fisse.


## Dust town revisione4 — dimensioni ridotte e ante opposte

Richiesta successiva: area66..75x73..84m, circa13% in meno per lato, mantenendo
pianta indicativa e quote dell'incrocio. Le due ante ora ruotano rispettivamente
verso interno/esterno: proiezione frontale sovrapposta24cm, nessuna visuale
rettilinea a quota occhi, passaggio diagonale110..130cm. Percorsi di strade e
pattuglia inseriscono lo stesso breve zigzag, percorso anche dal probe reale.
La migrazione setup_dust_town_compact.py preserva palette e materiali esistenti;
validazione515 seed prima del salvataggio. Collisioni ruotate controllate nel
riferimento locale di ogni volume; punti di pattuglia Town con raggio10cm per
evitare tagli delle curve, valore standard90cm nelle altre mappe.

## Phase in Omega e Belica in Alpha — 15 ottobre 2026

Richiesta corrente: aggiungere cinque varianti per ciascun nuovo personaggio,
con lo stesso trattamento militare/opaco e palette dominante di squadra, parti
neutre e pelle/capelli naturali. I cinque indici Twinblast/Drongo esistenti restano
0–4; Belica Alpha e Phase Omega occupano 5–9, per dieci scelte per squadra.
La scelta rimane locale persistente, validata dal server e applicata al respawn;
i cadaveri conservano l'indice della vita precedente.

`setup_phase_belica.py` crea scheletri e locomozione rifle adattati separatamente,
ragdoll anatomici e avambracci sul rig FPS esistente. Preserva texture, normal,
opacity e override dei materiali originali, elimina l'emissione e usa superfici
opache. Belica usa fanteria, PolarStrike, Biohazard, HeavyArmor e supporto con
zaino; Phase usa base, Kitty e Pandapack ripulite, casco da campo e zaino.
Giocattoli/zaini decorativi, gioielli pendenti, maschera Kitty, armi originali
e ologrammi vengono nascosti. Le mesh Paragon restano riconoscibili: non sono
stati modellati pantaloni o corpi militari sostitutivi. Il marker
ERPhaseBelicaVersion protegge i ritocchi successivi da una nuova importazione.

Gli originali completi sono montati nel submodule privato characters agli
stessi percorsi /Game/ParagonPhase e /Game/ParagonLtBelica. `assets.json` include
i due nuovi mount; nessun pacchetto binario entra nel repository pubblico.
Gallerie a pagine da cinque e galleria FPS coprono il catalogo esteso. Il test
bersagli verifica venti modelli replicati; `test-network.py --phase-belica`
esercita selezione, morte, conservazione del cadavere e nuova skin al respawn.
Gli esiti effettivamente eseguiti sono registrati in TEST_RESULTS.md.

Rifinitura Phase: capelli lunghi nascosti mantenendo la sezione corta naturale;
maniche in tessuto opaco e casco adattato al cranio. La migrazione separata
`refine_phase_field_appearance.py` conserva i ritocchi successivi tramite marker.

Correzione Omega 9: il presunto residuo sulla testa era la calotta del casco
con facce esterne scartate dal rendering, che lasciava visibili retro e bordo.
`fix_phase_engineer_helmet.py` genera una copia dedicata con winding esterno,
materiale opaco a due facce e bordo rialzato sulla fronte. Aggiornati soltanto
gli accessori PhaseEngineer nei tre cataloghi Blueprint; casco condiviso e altre
skin restano invariati. Marker separato per conservare i ritocchi successivi.
La galleria supporta `-ERSkinDetail=8 -ERSkinPage=1 -ERSkinDetailYaw=0` (indice
zero-based) per dettagli della testa; yaw90/180 danno lato e retro, senza etichette.

Correzione ragdoll rilevata nell’ispezione Metal: le pose Phase richiedevano un
controllo esplicito della copia visibile e limiti articolari meno ampi.
Il flag Blueprint `CopyPhysicsPoseForRendering` delle sole skin Phase abilita
un componente visibile senza collisioni che riceve la posa fisica dopo il passo
fisico. I trasformi locali vengono aggiornati in un unico batch; dopo cinque
secondi la fisica si addormenta e la copia visibile viene congelata. Il componente
fisico conserva sedici corpi/quindici giunti, WorldStatic soltanto, limite dodici
cadaveri e durata novanta secondi. Anche materiale, ossa nascoste e accessori
mantengono la selezione originale. Il costo non è stato misurato come benchmark.
La galleria verifica posa, assestamento e concordanza della copia visibile.

Profilo Phase campale: braccia 45°, gambe 35°, ginocchia 25°, caviglie 15°,
polsi 20° e torsione delle gambe 15°. I parametri del generatore restano 65°
per i personaggi precedenti; `fit_phase_ragdolls.py` applica il nuovo profilo
alle sole copie Phase una volta, poi conserva i ritocchi dell’artista.
Corpo e vestiti Phase sono opachi; capelli, occhi e ciglia mantengono le loro
maschere naturali. Restano le geometrie originali di gonne, stivali e giacche.


## Personaggi approvati: nomi e pulizia — 15 ottobre 2026

L'utente approva le venti skin e autorizza la rimozione degli originali e degli
asset non necessari al loro uso attuale. Questa richiesta sostituisce la
precedente riserva integrale Phase/Belica. Nomi omogenei `personaggio · ruolo`
per Twinblast, Drongo, Belica e Phase, definiti in character_catalog.py e salvati
nei tre cataloghi; gli indici di selezione restano stabili.

Separati i riferimenti di ricostruzione dalle dipendenze effettive: anteprime
skeleton/animazioni/BlendSpace sulle mesh possedute, materiali predefiniti delle
mesh allineati alle skin attive e materiale neutro per le sezioni nascoste.
Svuotati i collegamenti agli asset delle ombre a capsule soltanto dopo aver
verificato che entrambi i relativi flag siano disattivati nei tre Blueprint.
Conservati geometria, materiali visibili, dati delle pose retarget, rig FPS,
profili fisici e impugnature. Migrazioni separate con marker per i ritocchi futuri.

Rimozione Unreal in due lotti: 3.136 + 74 package, 2.329.822.505 byte (2,170 GiB).
Una copia di prova senza il primo lotto carica tutte le skin e sette mappe;
verifica fresca finale anche dopo il secondo lotto. Trentaquattro package sono
conservati per referencer esterni, oltre al rig/pose FPS protetti; il piano
finale non contiene altri candidati eliminabili. Archivio di ripristino privato
esterno, verificato SHA-256, contenente rimossi e versioni precedenti dei file
modificati; la copia completa ridondante e il progetto di prova sono rimossi.
I vecchi generatori richiedono il ripristino degli input prima di ricostruire.

Il primo test bersagli aveva una corsa temporale: l'host usciva a 20s, meno di 5s
dopo il join del client a freddo. La sola finestra del probe esplicito passa a 35s;
nessuna attesa aggiunta alle partite normali. La successiva esecuzione LAN passa
su host e client. Esiti e limiti in TEST_RESULTS.md e CHARACTER_CLEANUP_RESULT.json.


## Armi Sketchfab — richiesta del 15 ottobre 2026

Importato l'elenco `weapons.md`, conservato integralmente in
`references/weapons.it.md` e tradotto in `WEAPONS_REFERENCE_EN.md`. La richiesta
aggiorna il precedente arsenale Fab: dieci slot disponibili, tasti1..8/F/G e
fucile d'assalto iniziale. Scelta esplicita successiva: statistiche AK-47 e SCAR;
la riga SCAR disponibile è scar20. La geometria rimane quella dei modelli linkati.

Originali, metadati/licenze, ricevute SHA-256 e derivati Blender sono nel mount
privato weapons; mesh, materiali, animazioni e dati in Weapons/Sketchfab. Le
mani mantengono il rig e le skin esistenti, con adattamento IK delle traiettorie
originali. Aggiunti armatura AP100, attenuazione di distanza, coltello breve e
Milkor con proiettile fisico ricavato dal modello. `ERLegacyArsenal=1` conserva
la fixture precedente per le regressioni. Dettagli, valori non presenti nel
foglio e limiti del retarget sono espliciti in `WEAPON_IMPORT.md`.

Correzioni di presentazione del 15 ottobre: presa AR15 ricentrata e mano di
supporto in terza persona adattata alla singola arma con IK dopo locomozione e
prone. I bot fermi mostrano tutte le dieci armi in ordine casuale, conservando
lo slot al respawn. Avambracci FPS più spessi sulle skin attuali. Gli idle
meccanici sono pose fisse; Milkor senza ottica e con UV consolidati per materiale.
Il retarget usa punti del palmo completi, scala coerente con l'arma e raggiunge
i bersagli spostando la spalla nascosta invece di separare mano e caricatore.

Secondo ritocco su nuove immagini: i vecchi HandGrip non coincidevano con la
superficie visibile dei palmi. Aggiunti socket ER_Palm specifici alle quaranta
mesh possedute, ricavati dai vertici della mano e dalle nocche, senza cambiare
geometria o socket fisici. Cinque fucili usano contatti campionati sui triangoli
deformati dell'idle e orientano il palmo contro l'astina; calcio arretrato di12cm.
La pistola FPS ha un attacco dedicato al palmo e gomito abbassato verso il centro,
con ricarica riferita al nuovo perno. Statistiche sniper esplicitamente verificate
contro AWP: danno115, intervallo1,46s, ArmorRatio1,95 e RangeModifier0,99.

L'utente rifiuta poi il braccio della pistola troppo verticale e il nuovo angolo.
Ripristinata la posa precedente di braccio/polso e la rotazione dell'arma dal
vecchio HandGrip, mantenendo il contatto misurato del palmo. Offset FPS avvicinato
di6cm alla camera per mostrare meno avambraccio; rigenerate le relative clip.

Ulteriore ritocco richiesto: pistola inclinata di più, in basso a destra, con
punta riferita all'AR15. Posa rilassata conservata; inclinazione camera
(-3 pitch,-10 yaw,-7 roll), punta a40cm contro74,8cm dell'AR15 e leggermente
sotto/destra nella proiezione. Impugnatura affondata di3,5mm nel palmo; tre
falangi del pollice adattate al lato opposto del calcio conservando le lunghezze.
La posa del pollice segue anche sparo e ricarica. Dati in pistol-presentation.json.

Il confronto nel gioco ha rilevato la scala assoluta1 dell'arma, distinta dalla
scala0,5 della mano. Corretti contatto, geometria del pollice e retarget della
sola pistola con le dimensioni reali e il nuovo riferimento inclinato; il test
grafico verifica ora anche i contatti e la proiezione effettivi durante il gioco.

L'utente approva la presa e chiede un lieve avanzamento e la canna allineata al
mirino centrale. Profondità della punta40->44cm; eliminati pitch/yaw aggiuntivi,
conservando il roll laterale-7°. Asse della canna parallelo alla linea centrale
della camera, senza modificare mira o dispersione del server. Thumb fit e clip
rigenerati nello stesso riferimento, mantenendo il contatto nel palmo.

Il successivo screenshot di riferimento del viewer definisce l'inquadratura
desiderata: impugnatura verticale, pistola compatta in basso a destra e poco
avambraccio. Rimosso il roll laterale, profondità punta46cm e centro della
canna proiettato circa a(58,2%,64,25%) in16:9, con mirino anteriore più alto.
Questo riferimento sostituisce il precedente vincolo sotto/destra della punta
AR15. Per riprodurre anche la prospettiva della canna, la rotazione converge
dal centro della volata al raggio centrale della camera a150cm, senza roll.
Presa, pollice e convergenza restano verificati geometricamente. Mira, origine
autoritativa dei colpi e dispersione restano quelli del server.
Contatto di palmo/pollice alzato di5cm sul calcio, per avvolgere l'impugnatura
come nel riferimento; rigenerate idle, sparo e ricarica con il nuovo attacco.


## Respawn & Rage — 15 ottobre 2026

Nome definitivo: **Respawn & Rage**. Sottotitolo: **Serious guns. Questionable players**.
Il progetto attivo, i tre submoduli e le cache LFS sono stati trasferiti in
`/Users/agea/git/respawn-and-rage`: codice in `core`, cache in `_cache/lfs-*`.
Il prototipo precedente rimane separato, come richiesto dall'utente.
GitHub usa `respawn-and-rage/core` e i tre repository privati `assets-characters`,
`assets-weapons`, `assets-maps`; `.gitmodules` conserva URL relativi per SSH/HTTPS.

Progetto `RespawnAndRage.uproject`, target `RespawnAndRage` e `RespawnAndRageEditor`;
compilazione con `scripts/build-editor.command`, avvio con `scripts/play.command`.
Nome e sottotitolo sono nel README, nei metadati Unreal e nel menu Slate.
Il modulo nativo `fdm`, `/Script/fdm`, `/Game/EclipseRecoil` e le chiavi delle
preferenze restano identificatori interni per preservare classi serializzate,
Blueprint, mappe modificate e impostazioni. Non sono il marchio del gioco.
I JSON di verifica storici mantengono i valori/percorsi originali come ricevute;
non sono configurazione dei repository correnti. Nessuna modifica al gameplay.


## 16 ottobre 2026 — Cattura personaggi

`scripts/capture-character-set.py` avvia la galleria opt-in
`-ERTeamGallery -ERCharacterShots`: tre skin per ciascuno dei quattro modelli,
arma casuale per variante, viste frontale/laterale/posteriore. Camera, luci e
fondale sono transitori; output in `Saved/Screenshots/CharacterSet`.


## 16 ottobre 2026 — Asset del marchio

Originali forniti dall’utente copiati senza modifiche in `Branding/Originals`.
`scripts/prepare-branding.py` genera logo Slate 1024×414, icona 1024²,
splash gioco/Editor 1599×900 e icone native ICNS/ICO. Il launcher mostra
la splash; il menu usa il logo con proporzioni preservate. Su Mac il processo
locale `-game` aggiorna anche l’icona Dock senza modificare UnrealEditor.app.
Inventario e percorsi in `Branding/README.md`; nessun accesso runtime a Downloads.


## 16 ottobre 2026 — Sfondo menu

La richiesta successiva sostituisce la splash separata: il launcher torna
a usare `-NoSplash`. La stessa illustrazione, ridotta a 2560×1441, è lo
sfondo della prima schermata interattiva e del menu richiamato durante
la partita. Riempie il viewport conservando le proporzioni e ritagliando
i bordi eccedenti. I tre pannelli hanno fondo nero al 76% e testo chiaro.
Originali, logo e icona rimangono in `Branding`; il file dello sfondo è
`Branding/background.jpg`, rigenerabile con `prepare-branding.py`.


## 16 ottobre 2026 — Logo menu e dimensione iniziale

Rimosso il logo aggiuntivo dal pannello centrale: resta quello presente
nell’illustrazione di sfondo. All’avvio del menu principale il gioco applica
una sola volta il 70% del monitor attivo, anche quando le dimensioni iniziali
del launcher ricadono nel fallback o nelle preferenze salvate. Il launcher
passa `-ERInitialWindowPercent=70`; le dimensioni esplicite di cattura/probe
senza questo flag sono rispettate. Il guard nella GameInstance preserva
le successive scelte utente durante i cambi mappa e il ritorno al menu.


## Protezione alla nascita: grigio opaco — 16 ottobre 2026

La revisione sostituisce la trasparenza al50%: corpo, accessori della skin e
braccia FPS usano LevelColorationLitMaterial già incluso in Unreal, con una sola
istanza dinamica transitoria grigia(.32,.32,.32) per personaggio. Nessuna
texture, mesh o copia di materiali aggiunta su disco. Armi e materiali originali
restano invariati; la resa protetta è uniforme e rinuncia alle texture della skin.

Lo stato legge la scadenza server esistente, senza nuove proprietà replicate.
Gli originali sono conservati per componente/slot e ripristinati su scadenza,
attacco, morte e prima di cambiare skin. setup_spawn_protection.py salva i default
BP_ERCharacter/BP_ERTrainingTarget ed elimina le vecchie varianti traslucide in
Characters/Protection tramite Unreal, controllando prima i riferimenti esterni.
Verifiche LAN owner/proxy/respawn/attacco e confronti Metal Alpha/Omega.


## Rimozione indicatori sopra la testa — 16 ottobre 2026

TeamMarker è nascosto in BeginPlay per tutti i giocatori e bot, compresi
i respawn e i Blueprint già salvati. Disattivate anche le ombre del marker
e rimossi la creazione e l'aggiornamento della sua istanza materiale dinamica.
Il componente resta per compatibilità con gli asset esistenti.


## Granate e fumogena — 16 ottobre 2026

La richiesta del 16 ottobre sostituisce l'effetto HE a schegge di pavimento e
la simulazione Niagara della fumogena descritti nelle sezioni storiche sopra.
HE e proiettile Milkor condividono P_ER_CompactHE, copia propria del burst
Epic Content Examples con sprite subUV, fumo e piccoli detriti cosmetici.
Il riferimento Niagara Fluids dello screenshot guida l'aspetto, ma la
simulazione 3D della demo non viene eseguita per ogni detonazione.
Sorgente acquisita in Documents/Unreal Projects/ContentExamples, lasciata
intatta; 12 package migrati e rinominati attraverso Unreal nel mount privato
Effects/Grenade/Epic. Il precedente FBX e l'archivio restano conservati, senza
collegamenti attivi all'animazione dei frammenti.

La fumogena usa un volume traslucido con densità propria, semisfera di circa
6×6×2,8 m, crescita di 1,1 s e dissolvenza negli ultimi 2 s dei 12 s totali.
Una griglia 16×16×8 verifica celle interamente libere e sweep fra vicini:
il riempimento connesso passa nelle aperture sufficientemente ampie, senza
attraversare pareti sottili. Il bordo è conservativo a risoluzione circa
38×38×35 cm: le fessure più piccole possono restare chiuse. Il materiale
integra 48 campioni lungo la vista e termina alla profondità della scena;
funziona anche dalla camera dentro la nube. Ogni nube possiede la propria
texture di densità. Revisioni ed epoche dei blocchi invalidano il contenimento
con controllo ogni 750 ms. Nessuna modifica globale a volumetric fog o ai livelli.
Fino a quattro segmenti di sparo autorizzati aprono brevi tagli cosmetici;
restano invariati danni, collisioni e vista dei bot. Nessuna interazione
procedurale coi corpi dei giocatori nella nuova versione semplice.

HE, smoke, stun e LaunchedHE usano lo stesso percorso fisico: gravità, attrito,
rotazione su più assi, variazione contenuta dei contatti e assestamento di lato.
Solo il server campiona le variazioni e replica posizione/rotazione. La miccia
non è riavviata dai rimbalzi e il lifetime non può troncarla; il Milkor mantiene
la velocità di lancio del suo profilo, ma ora detona solo a fine miccia.
Danni e copertura restano server-side sul percorso condiviso dei blocchi.

Setup attuale: scripts/setup_grenade_effects.py dopo la migrazione privata
(scripts/migrate_epic_grenade.py). I vecchi setup_grenade_explosion.py e
setup_interactive_smoke.py documentano le integrazioni precedenti.
Verifica dedicata: scripts/test-grenades.py, anche --render, con due processi
reali, rimbalzi prima della miccia, trasformazioni remote, parete chiusa,
finestra aperta, forma sopra il suolo e cleanup. Esiti in TEST_RESULTS.md.


## Danno HE/Milkor e cook — 16 ottobre, richiesta successiva

HE e Milkor ora usano 140 danni fino a 5m e una coda quadratica fino a zero
a 15m. Con 100AP, la riduzione massima è 20%:112HP nel nucleo completamente
esposto, circa 63HP a 7,5m e 28HP a 10m. Distanza 3D dal centro capsula; tre
trace WorldStatic verso testa, torso e parte inferiore pesano l'esposizione.
Muro pieno blocca il danno, copertura parziale lo riduce. Le coperture sono
valutate prima di mutare i blocchi; questi mantengono il raggio strutturale
3,5m distinto dalle schegge a 15m. Nessuna modifica all'armatura dei proiettili.

Click sulle granate a mano arma un proiettile reale e consuma una munizione.
Miccia 2,5s sul server, senza tempo cook fornito dal client. Finché tenuta,
la granata resta nascosta dietro/sopra la testa; mano/braccio FPS e modello
scompaiono anche tramite previsione locale dell'input. Il rilascio lancia
lo stesso attore con tempo residuo, da un'origine arretrata/alta controllata
con sweep, velocità rivolta leggermente verso l'alto e inerzia del portatore.
L'attore conserva rimbalzi/rotazione precedenti. Se non si rilascia esplode
in mano; morte e salita interrotta lasciano cadere l'ordigno già vivo. Nessun
rimborso o reset; cambio arma/ricarica non annullano la miccia. Il recupero
mostra la granata successiva soltanto se rimangono munizioni. Cook applicato
anche a smoke/stun manuali; Milkor resta click normale e miccia all'uscita.

Script di migrazione: setup_grenade_combat.py. Test dedicato esteso:
test-grenades.py --combat --render, input mouse reali su host e client,
scadenza originale, morte portatore, danni/coperture su entrambi i payload.


### Ultima taratura granate — 16 ottobre

Richiesta successiva: miccia **3,5s**, nucleo letale **2m**, danno decrescente
fino a **6m** per HE e Milkor. Picco 140 e riduzione armatura massima 20%
invariati: 112HP nel nucleo scoperto con 100AP, circa 63HP a 3m, 28HP a 4m e
1,75HP a 5,5m. A 6m il danno è zero. Stesso decadimento quadratico e verifiche
di copertura; il cook usa sempre la scadenza dal click. Miccia aggiornata
anche alle fumogene e stun manuali; durata del fumo invariata. Danno
strutturale distinto 3,5m e tutti gli effetti restano quelli precedenti.
Rigenerazione profili e migrazione esistente aggiornate; test di danno
adattati alle nuove distanze, con entrambi i payload e armatura piena.

### Fumogena irregolare, raggio 3m — 16 ottobre

Volume massimo 6x6x3m, raggio 3m. Il bordo usa lobi morbidi asimmetrici
con fase casuale server replicata, oltre al rumore mobile della densità.
Le rientranze variano tra granate e rimangono dentro il limite di 3m.
Restano griglia conservativa, passaggi connessi, pareti/finestre, durata12s,
quattro volumi e clipping contro la scena. setup_smoke_shape.py rigenera
soltanto materiale e Blueprint del fumo senza cambiare gli asset HE.

### Lancio ridotto ed esplosione sobria — 16 ottobre

Milkor1800cm/s (−50%); granate manuali880cm/s (−20%). Miccia3,5s, cook,
rimbalzi e danno2/6m invariati. L'effetto HE/Milkor disabilita gli emettitori
di fuoco colorati: bagliore neutro65ms, luce breve e tre piccoli sbuffi
con durata0,55..0,95s. Solo entro60cm da una superficie statica con normale
verso l'alto genera3..6 schegge piatte casuali, con materiale del pavimento.
Le schegge sono cosmetiche, gravità/rotazione e arresto contro geometria
statica; spariscono entro3s. Nessuna lastra intera animata né schegge in aria.

### Fumo4m, contorni mobili e interazioni — 16 ottobre

Milkor1530cm/s (−15% rispetto1800); granate manuali880cm/s. Dotazione
provvisoria10 per tipo manuale HE/smoke/stun; tamburo Milkor6 e riserva12.
Fumo8x8x4m, raggio massimo4m, crescita2,2s, dissolvenza4s, durata totale12s.
I lobi asimmetrici evolvono con l'età sincronizzata della nube; griglia sempre
16x16x8, ora celle50cm, conservando coperture e aperture sufficientemente ampie.
Colpi autorizzati: scie12cm/.3s già presenti; granate/proiettili fisici
replicati: scia25cm/.6s (10Hz massimo); esplosioni: apertura150cm/1s.
Quattro aperture contemporanee per nube, chiusura automatica e nessun cambio
al moto, ai danni o alla mira. Le interazioni sono visive, non fluidodinamiche.

### Origine sulla spalla e cook visibile — 16 ottobre

Milkor1750cm/s (vicino ai precedenti1800). Il lancio manuale parte da
(-14 avanti,24 destra,-4 alto)cm rispetto agli occhi, con sweep anti-parete.
Il braccio FPS resta nascosto durante cook; il modello terza persona mostra
la granata anche quando è l'ultima, con posa procedurale destra piegata,
mano sopra la spalla e transizione morbida. Usa lo stato cook replicato,
rig esistenti e socket della presa; nessun cambio a fisica/danni o miccia.
HE/Milkor aprono nel fumo un raggio300cm per2,5s, invece150cm/1s, con
richiusura graduale; le scie piccole non espellono le grandi aperture dal
budget4. Esplosioni accanto alla nube interagiscono anche se il centro non
è dentro il suo box, usando il raggio esteso della query cosmetica.

### Fumo del colore della squadra — 16 ottobre

Le fumogene assumono i colori HUD: Alpha azzurro(.2,.65,1), Omega
rosso-arancio(1,.25,.1), in spazio lineare. Il projectile cattura sul server
la squadra all'armamento; il colore non dipende dalla sopravvivenza del
lanciatore o da successivi cambi squadra. La nube riceve il colore prima
del BeginPlay, lo replica e aggiorna il materiale con RepNotify anche se
arriva dopo la creazione della MID. Nessun cambio al fumo cosmetico HE,
densità, durata12s, raggio4m, contorni, coperture o aperture temporanee.

### Colori più scuri e crescita a sbuffi — 16 ottobre

Tinte squadra al40% dei precedenti RGB lineari: Alpha(.08,.26,.4),
Omega(.4,.1,.04). La densità/trasparenza non cambia. La crescita applica
ritardi morbidi diversi secondo la direzione, fino al22% del tempo totale
(circa0,48s); le zone avanzano in modo meno uniforme, arrivando comunque
alla crescita completa a2,2s. Lobi mobili leggermente più marcati e limite4m
invariato. Preservati squadra replicata,12s totali/fade4s, pareti/finestre
e aperture di colpi/granate/esplosioni. Migrazione smoke-only.

### Fumo denso, dispersione duratura e granate colpibili — 16 ottobre

Densità ottica circa+35% (.035 invece.026). I fori dei colpi restano2s e si
richiudono lentamente (raggio con radice del tempo residuo); scie delle granate
1,5s. Le esplosioni HE/Milkor disperdono400cm, fino alla fine della nube, con
solo5% di recupero del raggio. Sempre quattro aperture massime, priorità alle
esplosioni; colori/raggio4m/crescita2,2s/durata12s/fade4s e contenimento invariati.
Le granate armate hanno sfera di query dedicata (6cm in volo,12cm in mano),
che blocca i trace delle armi e non cambia il movimento. Danno puntuale da
proiettile verificato sul server provoca la detonazione del payload, anche
durante il cook; corpo a corpo e danno radiale non creano catene automatiche.
Stessa pipeline del fuse per danni, effetti, pulizia e cook; nessun rimborso.

La maschera esplosiva libera completamente il nucleo fino all'80% del raggio,
poi sfuma nel bordo; le scie piccole mantengono il nucleo al30%. Questo rende
la dispersione HE nettamente visibile anche nel nuovo fumo più denso.

### Reazione a catena tra granate — 16 ottobre

HE/Milkor e razzi con danno positivo possono innescare granate già armate nel
proprio raggio (HE6m). Ogni passaggio controlla la linea WorldStatic verso la
granata prima di danni ai personaggi o rimozione dei blocchi; una copertura
intatta al momento dell'esplosione protegge l'ordigno. I bersagli esposti
sono accodati al tick successivo, con guardie pending/detonated, senza ricorsione
o duplicati. Vale anche in mano, usando la normale chiusura del cook. Smoke e
stun attivati in anticipo conservano il loro payload e non propagano una
ulteriore onda esplosiva. Nessun cambio al danno da proiettile o alle micce.

### Freesound explosion audio — 16 October 2026

HE and Milkor now use qubodup's Freesound162363 public HQ preview, converted
to mono44.1kHz PCM16 (0.601s), retained with credit/provenance in the private
weapons repository. setup_grenade_audio.py imports PCM with FORCE_INLINE and
binds sound/attenuation/concurrency to BP_ERGrenadeExplosion. The existing
authoritative detonation multicast produces one local sound per peer; dedicated
servers skip it. Volume.65, original pitch, spatial inner radius5m plus75m
falloff, maximum8 simultaneous sounds (stop farthest then oldest). The smoke
and stun payloads retain their own behavior. No additional sound library imported.
`test-grenades.py --render --audio` enables the host's real audio device and
checks active playback plus remote asset binding.

## XM1014: pallini, zone e rosata — 16 ottobre 2026

Valori richiesti dall'utente, già nel CSV locale:6 pallini da20, ArmorRatio1.6
(80% attraversa corazza), RangeModifier.70 ogni1270cm. Profili delle altre armi
invariati. Un click consuma una cartuccia; la modalità automatica esistente resta.

Il nuovo flag bUseBodyHitZones attiva sul solo XM1014 danno per osso: testa/collo
x4, pelvis/spine_01 stomaco x1.25, torace/braccia x1, gambe/piedi x.75. La
corazza non protegge le gambe; Health.bHelmet separato e replicato protegge la
testa quando c'è AP. Defaulttrue conserva il kit iniziale; non dedotto dalla skin
e nessuna nuova scelta inventario aggiunta. AP si consuma separatamente a ogni
pallino e, se esaurito, non può assorbire il danno dei successivi. Sei pallini
al petto fanno120 grezzi o96 con AP sufficiente a distanza zero; lo stomaco
corazzato può invece ricevere120. A distanza positiva si applica il decadimento.

ERShotTrace riusa i corpi anatomici dei physics asset già esistenti: le mesh
autoritative sono query-only, ignorano i normali canali e aggiornano le pose
anche fuori vista. Il mondo viene tracciato prima e limita le query scheletriche;
nessun cambio a collisione locomozione o balistica delle altre armi. Anche la
continuazione nei blocchi usa l'anatomia, mantenendo l'attenuazione energetica.
A contatto con una capsula, i pallini partono dall'occhio per non saltare le
hitbox o assegnare automaticamente petto. Non è simulazione di frammentazione
fisica o mesh poligonale: usa le hitbox anatomiche dei physics asset attuali.

Rosata: sei posizioni radiali stabili, raggio alternato65/85% del cono3.2gradi,
più jitter su disco fino12% del cono. Il centro della rosata ha imprecisione
condivisa .25gradi più penalità movimento/postura/bloom esistenti. Il server
estrae il jitter dal suo stream; nessun pallino centrale garantito. Nessun asset
aggiuntivo di hitbox, animazione o mesh; setup_shotgun.py salva solo DA_Shotgun.

## English interface and generic weapon names — 16 October 2026

Translated all owned menu/HUD strings, setup controls, pause/help, scoreboard,
match outcomes, reload/protection/respawn prompts, map synchronization and
connection/travel/admission errors into English. Default player is Recruit and
stationary bot labels use Target. GameInstance selects en for engine-supplied
messages without saving global language preferences.20 skin labels translated
in character/training/corpse defaults, preserving indices and source identities.

Requested mapping:2 Pistol,3 SMG,4 Shotgun,5 Assault Rifle,6 Sniper Rifle,
7 Marksman Rifle,8 Grenade Launcher. Knife/HE Grenade/Smoke Grenade and legacy
weapon labels are also generic English. Existing internal Carbine key remains
slot6,Sniper key remains slot7; no reorder, stats or input changes. HUD and menus
read the saved DisplayName. presentation_labels.py centralizes weapon metadata,
character_catalog.py centralizes skin labels; setup_english_presentation.py saves
only owned weapon data and Blueprint labels. Future import/setup scripts reuse
these names. Asset filenames, source model/stat references and license credits
retain original identities. No map/mesh/material regeneration for this change.

## Rifle scopes — 16 October 2026

Right mouse now cycles on press/release rather than hold. Only active Assault,
Sniper (serialized Carbine/SCAR-20) and Marksman (serialized Sniper/AWP) opt in.
Assault toggles iron-sight framing at75deg; shift the entire fitted FPS rig to
centre the muzzle; hide the cropped forearms in this gun-only view, preserving
both grip transforms and all server shot origins. Its
spread is unchanged. Optical rifles cycle90/40/10/90, scale mouse sensitivity
by FOV/90 and hide FPS arms/gun behind a circular black mask with fine black
crosslines. Normal centre crosshair, ammo ring and centre count are hidden for
optical rifles even without zoom; corner ammo/HP remain available.

Weapon-owned reliable RPCs validate active slot, alive/match/reload/traversal;
level replicates, owner receives synchronized settle start. Settling adds real
server cone spread over.16s Sniper/.25s Marksman and briefly softens crosslines.
Stationary, settled and recovered optical shots have zero base spread; motion,
airborne state and burst bloom still apply. Hip cones3deg Sniper/6deg Marksman
are server sampled. Sniper bloom.45deg/shot recovers6deg/s after.08s; Marksman
.8deg/shot recovers2deg/s after.2s. Sniper keeps scope after firing; Marksman
exits. Reload, death/holster/cancel and menu clear zoom; no automatic rezoom.
Shared tuning is in scope_profiles.py, applied narrowly by setup_scopes.py and
reused by future full arsenal imports. No model/animation/texture regeneration.
Opt-in test-scopes.py drives actual remote key presses and server ballistics;
--render captures all states through Metal. Test outcomes are recorded separately.

### Assault iron-sight centre revision — 16 October 2026

The newer user reference supersedes the low/distant gun-only framing above.
Centre the actual rear aperture at camera(14,0,0)cm, moving the entire fitted
FPS rig. The preserved AR15 geometry places its aperture centre at weapon-local
(6.07019,0,18.71037)cm; measure this point rather than centring the muzzle below
the view. Keep arms hidden in ADS and restore the ordinary hip frame on exit.
No FOV, sensitivity, spread, server origins or optical-rifle behavior changes.
The Metal scope probe now projects this anchor through the actual first-person
camera transform and requires less than one pixel error from viewport centre.

## Grenade range, launcher aim and structural blast — 16 October 2026

The user clarified range means throwing distance, not explosion radius. Hand
HE/smoke/stun now multiply the horizontal release velocity by1.2, retaining the
880cm/s reference speed's vertical component and inherited player velocity.
Launcher retains1750cm/s unscoped; right click toggles centred iron-style aiming
at75FOV, with horizontal launch velocity multiplied1.5 while aimed. Gravity and
3.5s fuse are unchanged. This gives20%/50% more unimpeded horizontal travel for
the same flight time; terrain, walls, rebounds and cooking still bound distance.
Fuse-projectile speed cap10000cm/s avoids different vertical fall caps distorting
that comparison. No client velocity/position authority is introduced.

ScopeMode1 now reads weapon-owned IronSightOffset/EyeDistance instead of the
Assault-only constant. Existing Assault anchor/default14cm is preserved. Launcher
anchor(18.493,.034,14.8316)cm comes from the retained model's upper front aiming
feature, eye distance30cm. Whole FPS rig moves together; cropped arms hide in ADS,
normal hip framing returns on exit. Launcher keeps its ordinary spread/cooldown.

Fuse projectiles (including launched HE) now sample bounciness.18..25 instead of
.28..38, with friction.4 instead of.32. Server contact variation/tumble remains.
HE and launched HE supply3x damage only to ApplyWorldExplosion, preserving the
3.5m structural envelope, material resistance and intact-cover snapshot. Peak
player damage140,2/6m falloff, armor, chain radius and rocket damage stay intact.
setup_grenade_combat.py and setup_scopes.py update existing saved weapon profiles;
future Sketchfab imports reuse the shared aiming/range settings. No new art.

## Continuous iron aim and posture-dependent optical focus — 16 October 2026

Assault and launcher now raise/lower the complete fitted FPS rig over.2s with
smoothstep easing, including90-to75 FOV and mouse sensitivity. Reversing the
right-click toggle reverses from the current blend. Reset on weapon changes;
server shots/launch bonus retain the authoritative logical scope state.
Launcher aim anchor sits2.5cm below the camera centre with3deg upward rig pitch;
Assault remains centred. Both palm contacts remain attached to the same rig.

Optical focus now remains blurred while standing movement exceeds5cm/s, or
while airborne. Marksman/AWP has full moving blur and7.5deg running penalty;
Sniper/SCAR has graded blur and4deg. The server records recent scoped motion;
after actual stopping, standing focus recovers over.25/.16s. Opposite movement
inputs use existing physical braking; no input-key shortcut selects a precise shot.
Grounded crouch intent/low posture reduces focus to.025s and ignores the motion
recovery timestamp. Grounded crouch-walking Marksman has zero movement/burst
spread once focused; jumping never receives this exemption. Sniper crouch-walk
has a small.08deg full-speed motion term, scoped burst/recoil gain x.2 and bloom
recovery x4 with at most.025s delay. Its standing burst gains.5deg per shot,
up to3deg, waits.3s and recovers6deg/s; walking adds further burst growth.
Scope crossline blur reads the same focus and burst state. Camera recoil stays
cosmetic, reduced in amplitude and recovered faster while scoped crouching.

These are requested gameplay rules, not a claim of exact Counter-Strike parity.
Shared scope tuning/setup updates the existing four data assets only.

### Launcher arc and player contacts — 16 October

Launcher projectiles now start six degrees above the aim direction, scoped or
unscoped; the existing scoped horizontal range bonus remains 1.5. This changes
server launch velocity without changing camera/control rotation. All fused
grenades have a soft player contact response (25% tangential speed, 5% normal
rebound). Only launcher HE inflicts25% of base damage on player contact:
35 raw with the140 damage profile, 28 against sufficient armor. Each projectile
can damage a given player once on contact; the original fuse and blast remain.

### Scoped launcher projectile origin — 16 October

Launcher shots during iron aim now originate at the raised FPS muzzle rather
than the fixed hipfire offset. Server uses the rig in camera-local space and
authoritative eye/aim, with first-person FOV correction and full projectile
depth. Origin blends during raising/lowering. The existing obstruction sweep
and blocked-muzzle fallback still prevent firing through nearby geometry.
Six-degree elevation, scoped range bonus, contact damage and fuse are retained.

## Audio granate — 17 ottobre 2026
Su richiesta: grenade1.wav per HE (intero, 2,361s), smoke_1.wav per fumogena
(6s, dissolvenza lineare 4,5..6s). Originali preservati in SourceAssets privati
SonnissGDC2024; runtime mono PCM16 44,1kHz. Release locale alla nascita della
nuvola replicata, offset per arrivo tardivo e stop alla rimozione.

## F1 e rimozione sfera HE — 17 ottobre 2026
Sostituita RGD-5 con F1 dal medesimo Grenades.fbx; texture F1 del pacchetto.
Migrazione setup_f1_grenade.py aggiorna solo mesh e idle modello, mantenendo
pose mani e parametri HE. Rimossa la mesh sferica Flash; luce del bagliore,
fumo, detriti e suono Sonniss mantenuti.
