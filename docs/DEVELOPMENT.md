# Eclipse Recoil — sviluppo Unreal

## Ambito concordato

Richiesta del 12 ottobre 2026: partire dal progetto in `/Users/agea/git/fdm`,
realizzare una versione alternativa in Unreal e riuscire a eseguirla localmente.
La guida originale è conservata in `ECLIPSE_RECOIL_UNREAL_BASE.md`.
Queste decisioni aggiornano la guida senza cancellarne i criteri di accettazione.

- **B10–B12 rinviati:** niente packaging, distribuzione, CI, pubblicazione o ZIP.
- La compilazione del modulo Editor serve esclusivamente a eseguire il C++ locale.
- Progetto mantenuto: `fdm/fdm.uproject`; modulo `fdm`, titolo Eclipse Recoil.
- Unreal fissato a **5.8.3**. First Person Blueprint con variante Arena Shooter
  già creata dall'utente; aggiunto un modulo C++ senza sostituire il progetto.
- Blueprint del template mantenuti come contenuti di riferimento. Le loro regole,
  bot, pickup e armi aggiuntive non entrano nel GameMode Eclipse Recoil.
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
fucile automatico 600 RPM; 30+90; ricarica manuale 1,5 s; proiettile swept
30.000 cm/s, gravità zero; primo impatto conclusivo. Server autorevole.
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

L'utente ha chiesto un indicatore simile al vecchio Eclipse Recoil, attorno
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
modifica alle granate, munizioni o slot dell'arsenale FDM.


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

La scelta successiva dell’utente è un repository pubblico `agea/fdm-code`
e tre submodule privati (personaggi, armi/effetti, mappe/Blueprint comuni).
Il vecchio `agea/fdm` resta privato. I percorsi Unreal sono invariati;
Content/SourceAssets sono collegamenti generati e ignorati. La struttura
attuale e i comandi sono in [ASSET_SUBMODULES.md](ASSET_SUBMODULES.md), che
sostituisce la precedente proposta di un solo submodule. Nessuna licenza
open-source del codice è stata scelta implicitamente.
