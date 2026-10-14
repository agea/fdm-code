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
non dimostrano la causa del rallentamento fullscreen di FDM.
Limite: CUA ha selezionato il vecchio browser Editor e, dopo la sua chiusura,
ha restituito `noWindowsAvailable`; il nuovo riavvio è verificato dai log e
dal processo attivo, senza nuova verifica visiva del menu o della fluidità.

## Menu dimensione finestra — 13 ottobre 2026

Compilazione Editor **Succeeded** (`window-menu-build.log`). Avvio Metal tramite
Launch Services fuori dal sandbox riuscito (`window-menu-play.log`), seguito da
partita L_Depot con bersaglio fermo. Durante l'interazione della sessione, il log
ha registrato applicazioni dal menu al 60% (3072×1728) e 80% (4096×2304), entrambe
in finestra sul monitor 5120×2880; il processo è rimasto attivo. Il controllo CUA
ha identificato la finestra Eclipse Recoil ma la cattura successiva ha restituito
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
superato. Test Unreal `FDM.Input.MouseResponse` **Success**
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

Progetto sorgente 5.3 migrato con AssetTools di Unreal in FDM 5.8.3.
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

New checkout: `/Users/agea/git/fdm/fdm-code`. The original private repository
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
`agea/fdm-code` is public; `fdm-assets-characters`, `fdm-assets-weapons`,
`fdm-assets-maps` are private. Each private main branch matches its pinned
gitlink, including all LFS objects. The old `agea/fdm` remains private.

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
