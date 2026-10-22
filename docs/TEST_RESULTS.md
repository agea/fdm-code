## 22 October 2026 — destructible Middle Eastern architecture, revision6

- Editor arm64 Development build: PASS. Thirteen derived LOD0/UV/normal assets
  baked through Unreal; revision5 rollback verified,516 candidate layouts pass.
- `scripts/test-destruction.py`: all16 native tests PASS with cell protocol9,
  including512 layouts, six physical worlds and initially supported structures.
  New cases cover source facade HP, a standing capsule through a new wall hole,
  a real roof fall opening, lower/lateral support loss, collision mappings,
  restoration, identical compressed masks, bounded decode rejection and a
  physical target inside a house after bullet penetration.
- `test-dust-template.py --alleys --architecture --render --network --seed42`:
  PASS. Real weapon on weakened source cells, standard HE damage, real pawn
  entry through the controlled facade cut, standing on/falling through the
  roof, support collapse and seed7919 restoration. Host plus two local clients
  agree before damage, after damage (including late join) and after rebuild.
  Digests1/6/7:3022322659 /266656279 /2434774792. Both remote movement bases
  resolve to the new stable skin component names; no unresolved corrections,
  rejected map states or oversized network bunches in the passing run.
- Standard `test-dust-template.py --alleys --render --network --seed42`: PASS.
  Real arch passage, bridge over/under route, both roof staircases, house entry,
  up/down terrain grades, all four tower ladders/decks, full inter-base route,
  lower passage/both ramps in both directions, HE road breach and actual fall,
  wood weapon damage/collapse and initial/late peer rebuilds all pass.
  Digests1/6/7:3022322659 /461304668 /2434774792; no unresolved bases.
- Full seven-step art pipeline rerun: PASS; all53 package/protected hashes
  unchanged, inventory15 original meshes,5 materials,14 textures,13 structures.
  Original archives and source-file hashes also pass verification.
- Five Metal screenshots inspected: matching facade and roof before/after,
  plus collapsed upper building. Large openings/support cuts are controlled
  geometry fixtures; the firearm fixture weakens a local region beforehand.
  No claim that one unchanged HE creates a full doorway, physical two-PC LAN,
  competitive balance or arbitrary fully demolished-map performance.

Evidence: `Saved/Testing/alleys-architecture-*-summary.log`,
`alleys-architecture-validation-report.json`,
`alleys-network-metal-architecture-report.json`, `alleys-architecture-assets.log`
and `MiddleEasternDestruction-*.png`.

## 22 October 2026 — macOS startup focus / mouse capture

- Editor arm64 Development build: PASS.
- `scripts/test-lobby.py --focus`: Metal foreground launch with a fresh isolated
  profile PASS, process exit 0. Initial fullscreen 5120x2880 settles with visible
  menu cursor and no viewport capture; the name field retains text focus.
  GameOnly captures without a click; explicitly releasing Slate pointer capture
  is recovered automatically. Returning to the menu and resizing to 3072x1728
  restores the visible, uncaptured cursor. All eight probe checks PASS.
- `scripts/test-lobby.py --settings`: PASS across two graphical launches, both
  exit 0. Fullscreen default and saved window mode/resolution, input, sound,
  profile and server capacity survive restart.
- `git diff --check` and Python syntax check: PASS.

Evidence: `Saved/Testing/Lobby/focus.log`. The capture loss is injected through
Slate; this does not establish reproduction of the intermittent itch-downloaded
app failure or physical mouse/camera testing in a packaged build. No new itch
package was built or uploaded during this fix.

## 22 October 2026 — team graffiti / recon case

- Editor arm64 Development build: PASS. Unreal import/save: PASS, zero errors
  and warnings in the final asset import commandlet. Original PNG alpha retained.
- `test-recon.py --automation`: Flight, FogAndSight, SmokeOcclusion PASS (3/3).
  Engine startup also logs pre-existing generic automation condition messages;
  all three requested test results and process exit are successful.
- `test-team-graffiti.py --map CentralBuilding`: Metal PASS; 5 Alpha / 8 Omega.
- `test-team-graffiti.py --map MiddleEasternAlleys`: Metal PASS; 26 / 30.
- `test-team-graffiti.py --map Depot`: Metal PASS; 6 / 3.
  Actual wall and closed/open/closing/closed case captures inspected. Corrected
  decal UV orientation after the first capture; materials preserve transparency.
  Counts are seed-specific, not a coverage percentage or balance claim.
- `test-recon.py --render`: PASS, actual listen host, enemy, ally and late client.
  Existing sight/smoke, authorized drone hit, five losses, actual 30-second
  replacement, debris and round reset checks pass. New checks verify saved source
  keys/parts, team lid emblem, 1.2s opening lead-in, and opening/reclosing on all
  three remote clients, including late join. The initial client sample timing
  was corrected to observe the open and closed portions of the cycle.
- `test-industrial-map.py`: industrial native automation PASS (layout/collision/
  evolution and finishes). `test-network.py --accuracy`: actual host/client PASS.
- `git diff --check` and Python syntax checks: PASS.

Reports and named captures: `Saved/Testing/graffiti-*-report.json`,
`graffiti-*-GraffitiWall.png`, `graffiti-*-GraffitiCase[0-3].png`,
`recon-metal-report.json`. These are local checks on one Mac; no separate-device
or subjective gameplay/balance claim.

# Verifiche locali — 22 ottobre 2026


## 22 ottobre 2026 — coperture basse, sottopassi e pavimentazione forabile

Build Editor Mac Development arm64 UE5.8.3 PASS. Migrazione esplicita revision5
con backup revision4 verificato,516 candidati e salvataggio della sola ricetta.
Target14 coperture centrali e18 nei distretti (minimi verificati8/6), a65–110cm,
in coppie ruotate180°; varchi, percorsi, scale, pioli, rampe e ingressi riservati.
Sottopasso a -3.6m con curve e due rampe, seed base-base oppure ovest-est.
Due sezioni di laterizio10cm hanno veri fori nel terreno dietro le celle.
Il resto del pavimento e della struttura è permanente. I2–4 varchi sono quelli
in superficie; il sottopasso aggiunge una connessione inferiore.

PASS `test-destruction.py`,15 test nativi,512 layout e sei mondi fisici.
Verificate entrambe le orientazioni, coperture accoppiate, appoggio e spazio per
la capsule sull'intero percorso inferiore/rampe, tetti e torri. Il profilo
strutturale dell'asset reale `DA_Grenade_RGD5` apre un foro attraversabile nella
pavimentazione in tutti i sei mondi, con appoggio sottostante a -360cm.
HP, trasmissione, armi e catalogo condiviso invariati. Corretti il verso dei
triangoli delle rampe e le facce dei muri di contenimento; nessun piano nascosto
resta nei fori. Il conteggio degli archi campiona lo spessore della cinta;
le nuove coperture sono comunque escluse dalle bocche degli archi.
Regressioni celle, Central Building e più piani passate. Riepilogo:
`Saved/Testing/alleys-passages-native-summary.log`.

PASS idempotenza delle migrazioni3/4/5 e inventario15 mesh/5 materiali/14 texture:
36 package propri e quattro asset protetti identici; originali verificati e
quattro seed estremi aggiuntivi validati sulla build finale. Report:
`Saved/Testing/alleys-passages-idempotence-report.json`.

PASS quattro viste Metal1536×1536, PNG distinti ispezionati. Seed42/73 hanno
sottopasso base-base;511/7919 ovest-est. Tutti e quattro mostrano32 coperture
nuove,14 centrali. Viste Entry/Lower/Intact/Breached1280×720 ispezionate:
rampa e curva visibili, foro reale aperto con il profilo HE salvato.
Report `alleys-passages-aerial-report.json`, immagini `MiddleEasternPassages-*`
e archivio `Saved/Testing/MiddleEasternPassages-Maps-and-Tunnels.zip`.



PASS `test-dust-template.py --alleys --seed 511` (exit0): percorso superiore
Alpha–Omega, sottopasso ovest-est completo nei due versi, entrambe le rampe,
foro con il profilo HE reale e caduta del personaggio fino al piano inferiore.
Passati anche case, tetti, torri, rampe ordinarie, arma/coperture e rigenerazione.
Report `Saved/Testing/alleys-passages-crosswise-report.json`; riepilogo
`alleys-passages-crosswise-summary.log`.

PASS `test-dust-template.py --alleys --render --network --seed 42` (exit0):
intero sottopasso tra le basi nei due versi, foro nella pavimentazione con
il profilo HE salvato e caduta reale del personaggio sul piano a -3.6m.
Passate anche le funzionalità precedenti. Host Metal e due client locali
concordano3866611771 intatto,2052419806 danneggiato e1790143882 rigenerato.
I client controllano fisicamente il foro nello stato6 e la strada ripristinata
nello stato7; snapshot iniziale/tardivo e basi di movimento passati, zero
correzioni scartate per basi irrisolte o oggetti non supportati. Report:
`Saved/Testing/alleys-passages-network-report.json`; riepilogo
`alleys-passages-network-summary.log`. Compilazione Python e diff-check PASS.
Queste prove locali verificano geometria, movimento, danno e ricostruzione;
non misurano bilanciamento competitivo o reti/dispositivi differenti.

## 22 ottobre 2026 — centro aperto e zero/una/due cinte

Build Editor Mac Development arm64 UE5.8.3 PASS. `setup_alley_core.py` salva
la sola ricetta revision4 dopo516 seed validati, con rollback revision3
verificato. Conteggio cinte0–2 indipendente dalle2–4 direttrici principali.
Senza cinta le direttrici si collegano liberamente al centro; due cinte
lasciano23.6–29.6m tra le facce, senza case o torri nella fascia riservata.
Ponti accoppiati e piccole coperture restano nel centro. Parità geometrica a180°.

PASS `test-destruction.py`,15 test nativi:512 layout, tutte le nove combinazioni
cinte/accessi, parità degli isolati, determinismo e percorsi completi; sei mondi
fisici0,42,73,511,7919,INT32_MAX. Sweep della capsule contano i varchi su ogni
cinta presente; nelle varianti0/2 il centro deve avere oltre75% di larghezza
attraversabile, senza una barriera aggiuntiva. Confermati ponti, tetti, scale,
pioli/pianerottoli, ingressi nelle case e terreno. Passate le regressioni celle,
Central Building e più piani. `Saved/Testing/alleys-core-native-summary.log`.

PASS idempotenza delle migrazioni revision3/revision4:36 package della mappa
e quattro asset protetti invariati; inventario15 mesh/5 materiali/14 texture,
originali con hash corretti. Altri quattro seed estremi rivalidati. Report:
`Saved/Testing/alleys-core-idempotence-report.json`; log `alleys-core-assets.log`.

PASS quattro viste Metal ortografiche1536×1536, PNG distinti ispezionati:
42 una cinta/2 accessi,73 due cinte/3 accessi/29.6m centrali,511 nessuna
cinta/3 direttrici/18m centrali,7919 due cinte/4 accessi/27.6m centrali.
Gli accessi indicano il conteggio per ciascuna cinta, quando presente.
Immagini `Saved/Testing/MiddleEasternCore-Aerial-<seed>.png`; report con
metadati/hash `alleys-core-aerial-report.json`; ZIP `MiddleEasternCore-4maps.zip`.


PASS `test-dust-template.py --alleys --render --network --seed 511` (exit0):
centro senza cinte, ponte/sottopasso, due scale ai tetti, ingresso/uscita casa,
quattro rampe in salita/discesa, quattro torri e percorso Alpha–Omega.
Fucile, distruzione/collasso, ingresso tardivo e rigenerazione7919 con due
cinte passati. Host Metal e due client locali condividono47045505 intatto,
3981942898 danneggiato e260779632 rigenerato. Entrambe le basi di movimento
risolte; zero correzioni scartate/oggetti di rete non supportati. Report:
`Saved/Testing/alleys-core-network-report.json`; riepilogo
`alleys-core-network-summary.log`. Nessuna misura di bilanciamento competitivo
o collegamento tra dispositivi/reti differenti.

PASS anche `test-dust-template.py --alleys --seed 73` (exit0): personaggio
reale attraverso tutti e sei gli archi delle due cinte, ponte/sottopasso, due
tetti, casa, rampe, quattro torri, percorso Alpha–Omega, danno e rigenerazione.
Report `Saved/Testing/alleys-core-twinwalls-report.json`; riepilogo
`alleys-core-twinwalls-summary.log`. Compilazione Python e diff-check PASS.

## 22 ottobre 2026 — distretti variabili, piazze e torri

Build Editor Mac Development arm64 UE5.8.3 riuscita. Migrazione esplicita
revision3 salvata attraverso Unreal dopo516 seed validati, con backup della
ricetta revision2. La nuova ricetta genera il grafo dei vicoli:2–4 varchi
inter-base reali, quattro impostazioni di tortuosità, due/quattro piazze e
due/quattro torri con scale a pioli del gioco. Spazi, lotti, spawn, quote,
scale ai tetti e torri hanno controparti ruotate180°.

PASS finale `test-destruction.py`,15 test nativi:512 layout con tutte le
varianti di accessi/tortuosità/piazze/torri, parità delle opportunità di salita
e degli isolati, determinismo e rifiuto atomico; sei mondi fisici0,42,73,511,
7919,INT32_MAX. I varchi sono contati tramite sweep della capsule lungo la
cinta centrale; percorsi completi, scale, pioli, maniglie e pianerottoli delle
torri verificati fisicamente. Rampe percorse da sweep con contatti ammessi
solo sul terreno realmente calpestabile (normale Z≥.72). Le quote di percorso
interpolano gli stessi triangoli della collisione. I controlli hanno rilevato
e corretto raccordi troppo bruschi presso gli spawn, senza cambiare le regole
di movimento. Passate le regressioni Central Building, celle e più piani.
Log: `Saved/Testing/destruction-automation.log`.

PASS finale `test-dust-template.py --alleys --render --network` (exit0):
attraversati tutti gli archi del seed42, sottopasso e ponte, due scale fino
all'appoggio sulla mesh dei tetti, ingresso/uscita da una casa, quattro
attraversamenti delle rampe e tutte e quattro le torri con salita reale sulle
scale a pioli e appoggio sui pianerottoli. Percorso completo Alpha–Omega,
fucile, distruzione/collasso e rigenerazione con seed7919 passati. Host Metal
e due client locali condividono995061080 intatto,1253835183 danneggiato e
2654399937 dopo rigenerazione. Snapshot iniziale/tardivo e basi di movimento
passati, zero correzioni scartate per basi irrisolte/oggetti non supportati.
Il driver attende ora la conferma dello snapshot danneggiato del client tardivo
prima del cambio seed: elimina una gara con i tempi di caricamento del test,
senza cambiare la rete di produzione. Report:
`Saved/Testing/alleys-network-metal-report.json`; riepilogo
`Saved/Testing/alleys-district-network-summary.log`.

PASS inventario15 mesh/5 materiali/14 texture e idempotenza:36 package della
mappa e quattro risorse protette invariati. La migrazione revision3 modifica
soltanto la ricetta; mesh, materiali, sorgenti e livelli preservati. Report:
`Saved/Testing/alleys-district-idempotence-report.json`.
Quattro seed estremi aggiuntivi (-17,7919,INT32_MAX,INT32_MIN) rivalidati
sulla build definitiva, oltre ai512 del test nativo.

PASS quattro viste Metal ortografiche1536×1536, senza HUD né overflow ombre:
seed42 (2 accessi, winding2,4 piazze/4 torri),73 (3,0,4/4),511 (3,1,4/4),
7919 (4,1,2/2). PNG distinti ispezionati; nuove immagini separate dalle
catture revision2 in `MiddleEasternDistrict-Aerial-<seed>.png`. Report con
seed/dimensioni/digest/hash: `Saved/Testing/alleys-district-aerial-report.json`;
archivio `Saved/Testing/MiddleEasternDistrict-4maps.zip`.
Galleria Metal completa passata; viste Interior/Ramp ispezionate e vista
Rooftop della torre ricatturata con `--alleys --tower` dopo aver corretto una
camera occlusa dalla cinta. Il probe verifica con un raggio la vista libera
della torre; immagine `MiddleEasternDistrict-Rooftop.png` ispezionata, scala
a pioli e pianerottolo chiaramente visibili. Compilazione Python e
`git diff --check` passati. Queste verifiche locali non misurano il
bilanciamento competitivo né una partita fra dispositivi o reti differenti.

## 22 ottobre 2026 — quattro mappe, catture dall'alto

PASS `test-dust-template.py --alleys --plans`, build Editor Mac riuscita ed
exit0 della galleria Metal. Generati seed42,73,511,7919; salvati e ispezionati
quattro screenshot verticali ortografici1536×1536, senza HUD, con inquadratura
completa. Dimensioni rispettive:67×75m,71×73.5m,71×81m,67.5×82.5m. Il passaggio
finale usa4096 pagine ombre solo per queste catture, senza overflow; i PNG hanno
hash distinti e dimensioni verificate. Nessuna modifica alle regole di gioco.
Report: `Saved/Testing/alleys-local-metal-gallery-plans-report.json`.
Immagini `Saved/Testing/MiddleEasternAlleys-Aerial-<seed>.png`; archivio
`Saved/Testing/MiddleEasternAlleys-Aerial-4maps.zip` con i quattro PNG.
Compilazione sintattica Python e `git diff --check` passati.

## 22 ottobre 2026 — case accessibili, tetti e terreno graduato

Build Editor Mac Development arm64 UE5.8.3 riuscita. Migrazione esplicita
revision2 eseguita attraverso Unreal con backup verificato: dodici case con
collisione sulle superfici originali, due materiali con facce interne visibili
e ricetta aggiornata; livello, altre mappe e catalogo condiviso preservati.
Validazione prima del salvataggio:516 seed. Nessun volume pieno chiude più le
aperture delle dodici case. Scale esterne a tornante riservate negli isolati,
terreno reale a quota0/+120/+180cm e rampe; perimetro alzato a14m per contenere
anche i percorsi sui tetti.

PASS finale `test-destruction.py`:15 test nativi. I512 layout verificano almeno
due accessi ai tetti e due percorsi sul terreno; sei mondi fisici verificano
passaggi nelle case, spazio per la capsule sulle scale, appoggio finale sulla
mesh originale del tetto, collisione del terreno entro3cm e dislivelli oltre1m.
Passate le regressioni Central Building, celle, materiali e generatore a più
piani. Log: `Saved/Testing/destruction-automation.log` e
`Saved/Testing/alleys-traversal-native-summary.log`.

PASS finale `test-dust-template.py --alleys --render --network` (exit0):
personaggio reale attraverso le porte, sotto/sopra Short A, due scale esterne
con appoggio effettivo sulla mesh del tetto, ingresso/uscita da una casa e
quattro attraversamenti delle rampe nelle due direzioni. Tiro di fucile,
distruzione/collasso e rigenerazione con seed7919 passati. Il fixture di tiro
segue ora l'altezza effettiva del terreno. Host Metal e due client locali sulla
stessa build, iniziale e tardivo, condividono i digest1793307698 intatto,
3164166188 dopo danno e1210327691 dopo rigenerazione. Entrambi risolvono la
base del ponte, senza correzioni scartate né oggetti di rete non supportati.
Report: `Saved/Testing/alleys-network-metal-report.json`; riepilogo
`Saved/Testing/alleys-traversal-network-final-summary.log`.

PASS `verify_middle_eastern_assets.py`: inventario15 mesh/5 materiali/14 texture,
marker di collisione revision2 sulle dodici case e materiali interni bifacciali.
PASS seconda migrazione: exit0, tutti i36 package del template e quattro
risorse protette SHA-256 invariati; report
`Saved/Testing/alleys-traversal-idempotence-report.json`.
PASS galleria Metal con dieci viste, comprese Rooftop, Interior e Ramp,
ispezionate visivamente. Le viste Interior guardano dall'apertura verso il
vicolo; nessun nuovo arredamento o stanza è aggiunto ai modelli sorgente.
Compilazione sintattica Python e `git diff --check` passati.
Questi controlli locali non stabiliscono il bilanciamento competitivo né
verificano Internet o una partita fra due dispositivi fisici.

## 22 ottobre 2026 — Middle Eastern Alleys

Build Editor Mac Development arm64 UE5.8.3 riuscita. Nuovo livello e DataAsset
salvati attraverso Unreal; validazione candidata di516 seed passata prima del
salvataggio. Il nuovo template si aggiunge al catalogo menu/rotazione e al cook.

PASS `test-destruction.py`:15 test nativi, compresi i nuovi
`MiddleEasternAlleysLayout` (512 seed, variabilità degli isolati, percorsi
completi,20 spawn, determinismo e rifiuto atomico) e
`MiddleEasternAlleysCollision` (sei mondi fisici:0,42,73,511,7919,INT32_MAX;
capsule alle porte/spawn/scale, piano del ponte a320cm e sottopasso libero).
Passate anche le regressioni Central Building, celle, balistica, rete,
materiali e generatore a più piani. Log: `Saved/Testing/destruction-automation.log`.

PASS `verify_middle_eastern_assets.py`:15 StaticMesh (dodici case e tre mesh
proprie arco/ante),5 materiali e14 texture; dodici campioni PBR collegati al
blend delle tre sabbie, uso ISM, dipendenze private e hash delle sorgenti.
I quattro archivi/metadati Sketchfab e i crediti CC BY4.0 sono conservati nel
privato maps. SHA-256 invariati per Depot, Central Building, il suo DataAsset
e il catalogo condiviso della distruzione. Un secondo setup è terminato exit0
senza cambiare nessuno dei36 nuovi package (34 art + template + livello).
Report: `Saved/Testing/alleys-assets-report.json` e
`Saved/Testing/alleys-idempotence-report.json`.

PASS finale `test-dust-template.py --alleys --render --network`: host Metal e
due client locali, ingresso iniziale/tardivo, attraversamento reale delle tre
porte diagonali, sottopasso, entrambe le scale e Short A; tiro reale di fucile,
distruzione/collasso, ricostruzione con seed7919 ed epoch2. Digest condivisi:
1237050361 intatto,4198946245 dopo danno,1848351699 dopo rigenerazione. Entrambi
i client risolvono il movimento relativo al ponte; nessuna correzione scartata
per base irrisolta o oggetto di rete non supportato. Report:
`Saved/Testing/alleys-network-metal-report.json`. Il primo passaggio aveva
rilevato un fixture storico non aggiornato: `Primary=0` selezionava il coltello;
il driver ora usa `Primary=4` per il nuovo template e la prova di tiro passa.

Galleria Metal con camere ferme dopo compilazione asset, HUD sostituito soltanto
nel probe e sette viste in `Saved/Testing/MiddleEasternAlleys-*.png`: due piante
dei seed42/7919, corte A, Long, ponte, sottopasso e porta. Il pool ombre maggiore
della galleria evita overflow nella vista completa; le impostazioni di gioco
restano invariate. Compilazione sintattica Python e `git diff --check` passati.
Queste prove non misurano bilanciamento competitivo, prestazioni di una partita
completa, collegamenti fra due Mac fisici o Internet. A/B sono corti di scontro
del deathmatch esistente, senza nuova modalità bomba/cattura.


## 22 ottobre 2026 — EOS senza account Epic

Build Editor Mac Development arm64 UE5.8.3 riuscita con l'override locale del
plugin OnlineSubsystemEOS. Il motore installato resta invariato; la copia
privata generata è ignorata da Git. Verificati generazione isolata/idempotenza
(sorgente generato identico e timestamp preservato) e assenza di credenziali
nella generazione. Compilazione sintattica Python degli script modificati e
`git diff --check` passati. Config/EOS.local.ini resta ignorato e mode0600.

PASS `test-online-service.py`, due avvii reali consecutivi senza intervento
umano (`-unattended`): EOS Connect autenticato con Product User ID e nessun
Epic Account ID, identità persistente fra lascia/annulla/riospita e fra processi,
Device ID riutilizzato al secondo avvio. Il primo collaudo ha esercitato anche
la creazione iniziale; la ripetizione sulla versione finale riusa correttamente
la credenziale esistente senza cancellarla. Rinnovo Connect realmente riuscito
tramite lo stesso handler invocato alla scadenza (richiamato esplicitamente dal
probe di sviluppo, senza aspettare un'ora). Nessun browser o login Epic.

Verificati sessione pubblica, ricerca EOS con coerenza eventuale, nome host preso
dal nickname di gioco senza profilo Epic, endpoint P2P risolto, registrazione
dell'host e driver EOS reale senza passthrough. Passati distruzione, annullamento
durante creazione senza viaggio, riapertura, READY e viaggio a Depot mantenendo
P2P, ritorno al menu e Exit Game con sessione attiva. Entrambi i processi sono
usciti con codice0, `ER_EOS_PROBE_COMPLETE result=PASS`,
`ER_ONLINE_QUIT cleaned=1` e `ER_ONLINE_SHUTDOWN instance_released`.
Log: `Saved/Testing/OnlineService/probe.log` e `probe-relaunch.log`;
riepilogo `/private/tmp/rr-eos-guest-test.log`, build
`/private/tmp/rr-eos-guest-build.log`.

PASS regressione `test-lobby.py`: tre processi locali, discovery UDP e trasporto
IP, votazione/READY, viaggio e conservazione di team, skin, arma e scorte.
Log riepilogativo `/private/tmp/rr-eos-guest-lan.log`.

Collaudo limitato a questo Mac come richiesto: non verificati JoinSession fra
due identità/dispositivi distinti, partita fra due PC o NAT/relay fra reti diverse.
Gli helper di preparazione Mac/Windows/Linux ora generano l'estensione; nessun
nuovo pacchetto, build Windows/Linux o caricamento esterno eseguito in questa fase.

## 21 ottobre 2026 — colori pastello e icona corsa

Su richiesta successiva, barre e simboli usano rosso, verde e azzurro più scuri
e desaturati, con palette sRGB condivisa. Il fulmine stamina è sostituito da
una figura inclinata in corsa, con testa circolare e braccia/gambe piegate.
Build Editor Mac Development arm64 riuscita; galleria locale Metal1280×720
terminata con exit0 e `ER_RECON_GALLERY_COMPLETE`. Ispezionata la cattura
`Saved/Testing/Vitals/pastel-runner.png`: tinte attenuate coerenti fra barre
e icone, figura della corsa leggibile e layout preservato. Log:
`Saved/Testing/vitals-pastel-build.log` e `vitals-pastel-render.log`.
Modifica solo visiva; nessun asset binario o regola di movimento modificati.

## 21 ottobre 2026 — tre barre vitali e minimappa decentrata

Build Editor Mac Development arm64 UE5.8.3 riuscita. Le tre barre Canvas
condividono dimensioni, sfondo e scala; salute rossa con croce, armatura verde
con scudo e stamina azzurra con fulmine, senza etichette o valori numerici.
Nessun asset binario modificato. `git diff --check` passato.

PASS `test-movement.py --posture-only --render` (exit0): input reali remoti
W/E, doppioW, crouch/prone, consumo10s/recupero20s, ripartenza parziale e replica
owner. Quattro catture Metal1280×720 aggiornate in `Saved/Testing/Stamina/`.
Ispezionata `Phase45.png`: barre nell'ordine richiesto, salute/armatura piene,
stamina parziale azzurra, icone leggibili e nessun testo vitale. Marker owner
sotto il centro della minimappa, con margine dal bordo; terreno allineato.
Log riepilogativo: `Saved/Testing/vitals-movement-render.log`.

PASS `test-recon.py --render` (exit0), host Metal e tre client locali:
marker nemico effettivamente disegnato, sight/fog condivisi, smoke, late join,
scadenza contatti e ciclo droni. Ispezionata `Recon02_Minimap.png` con yaw180°:
owner ancora in basso con tick verso l'alto, alleato e terreno coerenti,
clipping sul cerchio e riferimentoN ruotato in basso. La cattura stamina
verifica invece yaw0°. Ricevuta `Saved/Testing/recon-metal-report.json`;
log riepilogativo `Saved/Testing/vitals-recon-render.log`.
Verifica locale tramite modulo Editor; nessun nuovo cook/package eseguito.

## 21 ottobre 2026 — aim punch reale e feedback rosso del danno

Implementata la specifica dell'utente in [AIM_PUNCH.md](AIM_PUNCH.md). Il rinculo
worksheet attuale era già gameplay: resta indipendente. I danni Bullet accettati,
positivi e non letali aggiungono pitch/yaw all'unica mira effettiva, prima della
normale dispersione. Tuning iniziale .40° + .012°/HP, testa1.5, armatura mitigante.6,
yaw.35, limiti2.5°/1.5°, emivita.20s. Rimossi i doppi pitch cosmetici dei proiettili;
conservati traslazione/roll e risposta precedente degli altri danni. Nessuna modifica
alle statistiche delle armi, collisioni, capsule o ControlRotation.

PASS build Editor Mac Development arm64 UE5.8.3 (133.38s) e test nativo
`EclipseRecoil.Weapons.AimPunch`: 20HP=.64°/.224°, testa/armatura, pellet senza
ripetere il termine base, duplicati/riordino/wrap, accumulo/limiti, recupero numerico
identico a30/60/120Hz e assenza di deriva. Comando: `test-aim-punch.py --automation`.

PASS tre esecuzioni di `test-aim-punch.py`: default, `--render` e `--lag 100`.
43 controlli autorità per ciascuna: HP effettivi, testa/armatura reali, protezione,
danni non Bullet, accumulo/cap, input e posizione invariati, controsterzo, pellet,
bot, colpo fisico host verso vittima remota, sparo remoto durante il punch,
recupero, morte, nuova pawn e cambio possessore. Undici casi di sparo coprono
le sei armi Bullet, i livelli ottici di Sniper/Marksman e il launcher; nessuna
penalità aggiuntiva alla dispersione. La camera owner coincide con il suo accessor
effettivo in pitch/yaw entro .001°, senza duplicazione; verificati anche FOV ottico
e recupero. Questo controllo locale non implica sincronizzazione perfetta tra peer.

Misure con timestamp FPlatformTime condiviso dallo stesso Mac. Ogni esecuzione
ha sette osservazioni del recupero degli snapshot alla ricezione (incluse
transizioni fra pellet e morte); non sono un campionamento continuo dello stato
corrente del server. Il confronto fra mira autorevole prima della dispersione e
POV owner usa il frame più vicino, a distanza massima50ms.

| Esecuzione | Ritardo reale di ricezione, ms | Errore recupero snapshot mediano/max, ° | Confronti camera |
| --- | ---: | ---: | ---: |
| Nessun ritardo aggiunto, NullRHI | 6.0–17.4 | 0.012 / 0.038 | 31 |
| Nessun ritardo aggiunto, Metal/catture | 16.3–185.0 | 0.042 / 0.412 | 2 |
| 100 ms in uscita per peer, NullRHI | 128.7–251.5 | 0.022 / 0.043 | 31 |

Nei due test NullRHI i29 confronti dopo la notifica hanno scarto massimo della
sola componente punch .049° senza ritardo aggiunto e .060° con100ms per peer.
Lo scarto massimo del pitch completo è rispettivamente .361° e1.936°: comprende
il rinculo da fuoco già esistente, la sua replica e la separazione fra campioni.
La prova Metal, con caricamenti/readback pesanti, presenta un outlier di .412°
nel recupero di uno snapshot; non viene dichiarata equivalenza perfetta. Il nuovo
clock usa l'ultimo campione server, metà RTT stimata e un limite temporale monotono,
senza cambiare i clock medi preesistenti. Misurati anche due spari prima della
notifica in ciascun test NullRHI: il server usa già il punch mentre l'owner ancora
non conosce quel danno. Questo intervallo di rete non viene nascosto o eliminato.

PASS catture Metal1280x720 ispezionate: `AimPunch/owner-hit-5.png`,
`owner-hit-7.png` (ottica) e `owner-recovered.png`. Arco rosso a due linee nella
direzione del colpo, fade.65s e flash rosso sfumato ai bordi.45s; centro libero e
scomparsa completa. La piattaforma del probe è transiente e replicata; handshake
owner evita di saltare intere fasi durante il caricamento dei modelli.

PASS regressioni `test-network.py --shot-feedback` (incluse morte/respawn e audio
owner danno), `test-network.py --accuracy`, `test-network.py --recoil`,
`test-arsenal.py`, `test-grenades.py` e `test-movement.py` completo, inclusi InputKey,
posture e stamina. Nessun asset binario o submodule modificato in questa revisione.
Log grezzi in Saved/Testing e ricevute `aim-punch-final-*.log`; riepilogo preservato
in [AIM_PUNCH_RESULTS.json](AIM_PUNCH_RESULTS.json). Le prime iterazioni del fixture
hanno evidenziato cadute dall'area sospesa e caricamenti iniziali: corretti il
pavimento replicato e la sincronizzazione del test, poi ripetute le prove.
Non eseguiti LAN su due PC fisici, nuovo cook/package o playtest soggettivo.

## 21 ottobre2026 — passi, varianti di sparo e smoke -10dB

Implementazione e pipeline: [AUDIO_PASS.md](AUDIO_PASS.md). Importazione Unreal
UE5.8.3 e build Editor Mac Development arm64 riuscite. Asset nuovi soltanto nel
privato armi: 69 SoundWave mono PCM48kHz (28 spari/41 passi), 13 cue a durata
finita, 13 palette, mappa superfici e cinque impostazioni attenuazione/concurrency.
Aggiornati sette binding armi attuali, il componente nel BP_ERCharacter privato
mappe e il salvataggio BP smoke. Sonniss originali/runtime, audio HE, LowPoly
storico e mappe esistenti risultano invariati nel diff dei submodule.

PASS i cinque test EclipseRecoil.Audio: Cadence (30/60/144Hz, muro senza
avanzamento, salti/teleport/landing), NoRepeat (anche cambio ciclo), SavedAssets
(armi distinte, grafi finiti, component binding, smoke gain), SurfaceQueries
(PhysicalMaterial e veri trace celle wood/metal/concrete), NoDeviceCleanup
(100 selezioni senza accumulo di componenti in un mondo senza audio).
Log: Saved/Testing/audio-automation.log.

PASS test-grenades.py --render --audio: fuse/rimbalzi/containment/interazioni/
cleanup, audio HE e release smoke realmente avviati; guadagno smoke .20554803
sia host sia remoto, equivalente a .65 * 10^(-10/20), senza cambiamenti al WAV.
Nuovo controllo numerico nel runner. Ispezionata la cattura Metal OpenWindow;
cloud/fade/temporizzazioni originali. Log e JSON grenades-render-audio-* sotto
Saved/Testing; nessuna valutazione soggettiva in cuffia dichiarata.

PASS test-network.py --shot-feedback: danni/ammo, morte/respawn, materiali,
feedback locale/remoto, camera e fixture LAN preesistente. PASS il successivo
run completo test-movement.py su host/client, incluse soglie10..200cm, crouch/
prone, sprint/stamina, dispersione e input reali W/E/C/Ctrl. Due run precedenti
avevano segnalato, rispettivamente, soglia50cm e doppio C; assenti
nel run finale PASS. Conservati i log concorrente e isolato iniziali sotto
Saved/Testing/audio-movement-*. Nessuna modifica al codice di movimento o input.

PASS test-audio.py --network --render nella versione finale: 54 controlli,
445 eventi/altrettante voci per host e 445 per client, 20+ colpi per arma e
prospettiva, raffiche SMG/Assault4s, nessuna ripetizione immediata nelle sei
palette multiple, nessun audio su vuoto/cooldown, uno sparo per sei pallini,
passi owner2D e bot3D. Concurrency 24/owner e96/global per gli spari,
StopOldest/25ms per conservare gli attacchi, passi4/48 con priorità inferiore,
budget reale mixer128. Verificato autostart=0 su ogni voce di sparo: la
registrazione del componente non avvia il fallback prima della selezione. Dataset e cue verificati dopo rigenerazione UE.
Risultati macchina: [AUDIO_PASS_RESULTS.json](AUDIO_PASS_RESULTS.json).

PASS anche test-network.py --recoil sull’ultima build: tutte le sei armi/
modalità, colpi rifiutati, raffica remota autorevole, camera owner/input e
recupero. La decisione audio usa il dato dello sparo, mentre la condizione
visiva/recoil rimane esattamente quella precedente.

La prima prova audio ha esposto una dipendenza reale dalla replica tardiva di
Weapon->Data: ora l'identità dell'arma accettata accompagna il multicast
esistente; nessun indice sample replicato. Il test aspetta inoltre un ACK del
modello effettivo sul peer dopo frame di presentazione stabili, evitando di
contare eventi storici mentre il client carica. La prova del passo owner usa
un pavimento temporaneo libero, anziché attraversare ostacoli del Depot.
Durante il controllo è stato sostituito WaveParam (durata infinita in UE) con
Switch locale e fallback Random, disabilitando la virtualizzazione dei one-shot.

Le sessioni sono due processi sullo stesso Mac tramite loopback, non una prova
su due PC fisici o Internet. Audio mixer reale attivo nelle prove grafiche;
nessun ascolto soggettivo o confronto umano A/B in cuffia dichiarato. Nessun
nuovo cook/package/piattaforma pubblicata da questa sessione. Launcher e light
hard hanno un solo sample; snow/dirt fallback documentati. Tabasco non importato
per conflitto di licenza. Conservati originali, crediti e hash nel privato.

## 20 ottobre2026 — rimozione texture inutilizzate, successiva richiesta

Audit Unreal: nessun referencer esterno per quattro set Sketchfab non selezionati
e tre vecchi set CC0 pavimento/controsoffitto. Rimossi tramite Editor37 package
(7 materiali/30 texture) e90 file sorgente; crediti del menu e inventari privati
aggiornati. Archivio di ripristino esterno verificato membro per membro SHA256.
Confronto hash PASS su781 file conservati, inclusi Depot, catalogo condiviso,
ricetta industriale e finiture attive. Restano12 materiali/51 mappe Sketchfab da10
sorgenti, sette set CC0 e vetro proprio. Tutti gli originali/hash dei set rimasti
e le attribuzioni incluse/escluse sono stati ricontrollati.

PASS la reimportazione (solo12 materiali conservati), la verifica asset/ricetta
salvata su516 seed e la galleria reale Metal regola3/seed42 (quattro viste,
nessun errore shader; ispezionata la vista dall'alto). Risultati e percorsi dei
log sono in [TEXTURE_CLEANUP_RESULTS.json](TEXTURE_CLEANUP_RESULTS.json).
Le verifiche di importazione iniziali qui sotto descrivono l'inventario prima
della pulizia. Nessuna modifica al codice di gioco o alle palette attive.

## 20 ottobre2026 — texture Sketchfab RR e tre suoli esterni

Editor UE5.8.3 Mac Development arm64: build finale riuscita,14.29s.
Quattordici ZIP originali conservati (1,089,613,135byte), sedici set materiali e68
mappe runtime fino a2048px. Verificati i14 URL sorgente nei crediti del menu,
crediti/metadati per ogni autore e hash invariati del Depot e del catalogo danni.
La conversione16bit è controllata anche sul caso Concrete2K: albedo originale
12189..51706, copia preparata47..201; nessun clipping a bianco uniforme.

PASS i quattro test `CentralBuildingCollision`, `CentralBuildingEvolution`,
`CentralBuildingFinishes` e `CentralBuildingLayout`. Layout valido su516 seed;
nuovo test su32 seed: tutte le tre varianti di terreno campionate, superfici
deterministiche, volumi/identità/trasformazioni e resistenza immutati, palette
vuote compatibili e riferimenti nulli/eccessivi rifiutati.
Log: `Saved/Testing/sketchfab-textures-native.log` e `industrial-automation.log`.

PASS importazione e verifica degli asset salvati tramite Unreal. Ricetta7,
palette2 cementi/4 murature/1 legno/3 terreni, acciaio e piastre Sketchfab,
quattro vernici container Factory e PCGEx `DA_CentralBuilding_42_SketchfabTextures`.
Log: `sketchfab-textures-import.log`, `sketchfab-textures-verify.log`;
ricevuta privata in `SourceAssets/SketchfabTextures/import-receipt.json`.

PASS LAN con rendering Metal del server, seed16/rule3, client iniziale e tardivo:
fase1 digest1295029505, fase2/3 digest851614393, nuovo round/epoch2 digest3514103708.
Verificate le superfici dei chunk effettivi, il materiale del terreno, distruzione,
supporto, brecce, ripristino ed epoch; client coerenti nelle fasi attese.
Log/rapporto: `sketchfab-textures-network.log`, `industrial-network-metal-report.json`.

La rifinitura successiva modifica esclusivamente gli shader dei tre terreni:
seconda campionatura ruotata/miscelata e tinta dell'erba più contenuta. Percorsi
dei materiali, palette, fisica e codice LAN restano identici. La verifica visiva
Metal finale di questa rifinitura è conservata separatamente per asfalto seed16,
erba seed1 e neve seed42 in `Saved/Testing/SketchfabTextures/{asphalt,grass,snow}`.
I risultati aggregati e i limiti della sequenza sono in `SKETCHFAB_TEXTURES_RESULTS.json`.


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
# Archetipi industriali — 19 ottobre 2026

Esito finale **PASS**, UE5.8.3 / PCGEx0.79.2. Ricevuta riproducibile:
`INDUSTRIAL_MAP_RESULTS.json`; log e catture in `Saved/Testing`.

- `scripts/build-editor.command -MaxParallelActions=3`: compilazione finale riuscita.
- `scripts/test-destruction.py`: **11/11** test automatici superati. Include
  celle/calibrazione/mesh/collisioni, danni, transazioni di rete, generazione
  multi-piano e i nuovi CentralBuildingLayout/Collision.
- **516 seed**, incluse estremità int32: 516 combinazioni di dimensioni e
  **150 composizioni di coperture**; determinismo, simmetria delle celle e
  delle collisioni fisse, dieci spawn per squadra e rifiuto atomico degli input
  invalidi. Collisioni reali per tutte le nove combinazioni attraversamento/piani.
- `test-industrial-map.py --render --gallery --seed 4`: passaggio fisico delle
  rampe, colpo di fucile autorizzato, danni/crollo, cambio round reale e nuovi
  spawn dei due bersagli fermi. Gallery Metal dei tre attraversamenti ispezionata.
- `test-industrial-map.py --render --seed 10`: **sei rampe effettivamente percorse**,
  edificio a tre piani, sparo/distruzione, nuovo round e respawn dei bersagli;
  **PASS**. Ispezionata la cattura `Industrial-Ground-rule3-seed10.png`:
  rete ritagliata e trasparente, pareti/piani, illuminazione e materiali presenti.
  Nessuna compilazione materiale fallita nel log Metal finale.
- `test-industrial-map.py --network`: host + client iniziale + ingresso tardivo
  dopo sparo e crollo; entrambi i client hanno lo stesso digest dello stato
  stabile e del nuovo seed/epoch dopo il round. **PASS**, anche con i due bersagli
  fermi che ricevono posizioni del nuovo layout.
- Protocollo8: nuovo test deterministico consegna l'intestazione di uno shard
  prima della fast array. La collisione resta chiusa fino a conteggio/digest del
  payload completi. Compilazione e test verificano anche il rifiuto di protocolli
  incompatibili. Gli eseguibili precedenti richiedono aggiornamento su ogni peer.
- Regressioni eseguite: `test-network.py`, `test-network.py --accuracy`,
  `test-arsenal.py`: **tutte PASS** sul Deposito.
- `verify_industrial_maps.py`: **PASS**, 516 seed sul template salvato, grafo PCGEx
  con loader e Path Properties collegati, maschera wire dal canale R e due lati.
  Restano solo L_MainMenu, L_Depot e L_CentralBuilding nel namespace delle mappe.
- Rimossi tramite Unreal **46 package** vecchi dopo verifica dei referencer e
  archivio locale SHA-256 di **70 file**. Conservati identici L_Depot e
  DA_ER_BlockMaterials; nessun package dei personaggi/armi o del progetto Factory
  originale modificato. Backup in `Saved/MapRefactorBackup/2026-10-19`.

Limiti: collisione degli oggetti industriali approssimata con volumi autoriali;
il grafo PCGEx è uno snapshot di authoring del seed42. La geometria di gioco e
la distruzione usano il generatore nativo validato. Questi collaudi non sono un
benchmark FPS o una verifica di packaging/distribuzione.

## Affinamenti rete, preset e muro — 19 ottobre 2026

- Rete indistruttibile con geometria visibile/collisione coincidenti: montanti
  da 4cm e fili diagonali da 0,4cm, aperture a rombo. Test reali delle query da
  entrambi i lati: impatti sul telaio e sulle due famiglie di fili, passaggio
  nei vuoti, permeabilità parziale su 256 raggi per layout. Verificate anche le
  query per oggetto WorldStatic usate dalle esplosioni. Collisione cotta in parti
  sotto 60.000 triangoli; nessun limite di collisione superato nel collaudo finale.
- Container confermati come copertura indistruttibile. M_BaseMaterial e
  M_WaterContainer della copia migrata abilitano l'instancing; i container ora
  mostrano le texture industriali su Metal. Progetto Factory sorgente, Deposito
  e catalogo dei materiali distruttibili conservati.
- Selettore attraversamento rimosso dal menu e dalla ricetta: durante il gioco
  il preset viene sempre estratto dal seed. MapCrossing funziona soltanto con
  il probe non-shipping; il test override resta nel descrittore replicato.
- Muro da 300cm con due passerelle a 320cm e scale fisse su entrambi i lati.
  Suite completa: **11/11 PASS**, 516 seed e nove combinazioni piani/regola;
  il ledge query reale rifiuta lo scavalcamento diretto da terra.
- Metal, seed10/regola2: **10 rampe percorse**, inclusi **4 attraversamenti
  completi** del muro con salita e discesa. Sparo autorizzato, distruzione e
  cambio round reale PASS. Regola3/seed10 e gallery dei tre preset PASS.
- LAN, regola3/seed10: host + client iniziale + ingresso tardivo PASS; entrambi
  verificano filo/apertura e percorsi dopo distruzione e nuovo round. Digest
  finale 1311821174, epoch2. Ricevute in `Saved/Testing/industrial-refinements-*`.
- Verifica degli asset e regressioni Deposito LAN, precisione e arsenale:
  **tutte PASS**. Esiti in `industrial-refinements-regressions.json`.

## Percorsi distruttibili e varianti verticali — 19 ottobre 2026

- Ricetta industriale revisione2: container cavi in acciaio rinforzato,
  muro di mattoni da 3m e accessi in legno distruttibili. Fabbricati secondari
  e torrette da uno a tre piani; pareti in mattoni/calcestruzzo, piani e scale
  in legno. Coperture rettangolari a sette angoli obliqui; coppie a 180 gradi.
- **12/12 test nativi PASS**, 516 seed e **177 composizioni** di coperture.
  Collisioni reali per nove combinazioni regola/piani; le griglie ruotate
  mantengono celle, collisioni e sostegni nel riferimento locale.
- `CentralBuildingEvolution`: acciaio Strength=8 (800HP per cella nel preset
  corrente), danni da fucile senza penetrazione dell'involucro integro, varco
  attraversabile nel container, perdita della passerella dopo il taglio dei
  sostegni e nuovo attraversamento a terra dopo la breccia nel muro. Il ponte
  opposto rimane utilizzabile. I tagli di collaudo sono operazioni controllate
  sulle celle; il probe conserva anche uno sparo di fucile autorizzato reale.
- Sostegno iniziale senza ancoraggi permanenti aggiunti: zero celle scollegate
  nei seed42, 10, 16 e 7919, compresa la torre a tre piani. Il test aggiuntivo
  è in `industrial-evolution-tower-support.log`. Corrette le casse cave: le
  pareti devono terminare sotto il tetto per avere contatti tra facce reali.
- Metal, regola2/seed10: **12 rampe**, **4 attraversamenti completi** del muro,
  brecce e cedimento dell'accesso, nuovo round e gallery dei tre preset **PASS**.
  Log e ricevuta conservati come `industrial-evolution-wall10-metal-*`.
- `setup_industrial_evolution.py`: revisione2 salvata e nuovo snapshot
  `DA_CentralBuilding_42_Evolution`; grafo PCGEx aggiornato, pin CellVolumes
  presente. Verifica asset PASS; SHA256 del Deposito e del catalogo invariati.
  I due modelli di scale a pioli sono presenti nel pack sorgente, ma non è
  implementata una meccanica di salita verticale: le torrette usano scale.
- Metal + LAN, regola2/seed16: torre e centrale entrambi a tre piani,
  **16 rampe** e quattro attraversamenti del muro percorsi. Host, client iniziale
  e tardivo verificano identiche brecce nel container/muro e perdita dell'accesso;
  ripristino su nuovo seed/epoch PASS. Digest finale 2822008614, epoch2.
- LAN, regola3/seed14: centrale ed edifici laterali a tre piani, **12 rampe**
  percorse, container aperto, ingresso tardivo e round successivo **PASS**.
  Ricevute `industrial-evolution-wall16-metal-network-report.json` e
  `industrial-evolution-bank14-network-report.json` in Saved/Testing.
- Regressioni Deposito rete, precisione e arsenale: **tutte PASS** sulla build
  aggiornata. `industrial-evolution-integration-results.json` riporta cinque
  scenari conclusi con exit0. Nessun nuovo packaging o benchmark FPS eseguito.

## Cadute, scale a pioli e isteresi del rendering — 19 ottobre 2026

Richiesta: danno da caduta, scale con mani libere o pistola e ridimensionamento
meno sensibile alle oscillazioni brevi. Build Editor UE 5.8.3 riuscita.

- `EclipseRecoil.Player`: **2/2 PASS**. Danno nullo per salti/cadute brevi,
  progressione 3–11m, input invalidi; isteresi a 30/60/144Hz, 59FPS tollerati,
  cali transitori, attesa3s, convergenza, stabilizzazione2s e blocco4s.
- Distruzione: **12/12 PASS**, con **516 seed**. Collisioni e pioli reali,
  capsula libera sull'intera scala, supporto iniziale senza distacchi e rimozione
  di un piolo che interrompe il percorso. Lamiera del catalogo sempre2mm;
  le guide scatolate sono composte da pannelli disgiunti in contatto reale.
- `test-player-mechanics.py --render --fps 30`: **PASS**, host Metal e client,
  lag60ms ±15ms. Velocità osservate144 e180cm/s; pistola ammessa, fucile
  rifiutato, X ripone l'arma, discesa dal tetto, Spazio stacca e pioli rimossi
  fanno cadere. Caduta6m:37,68HP a30FPS, armatura100 invariata; morte da caduta
  ricevuta dal proxy. Confermato anche con la build finale senza renderer.
- Il limite energetico all'istante dell'impatto evita l'eccesso di danno dovuto
  al sottopasso finale di fisica. Salute/protezione/morte restano autorevoli.
- Metal: `ER_RENDER_POLICY` ha letto tempi GPU reali e mantenuto la qualità
  ferma sotto carico nella fascia tollerata. Viewport e cap secondario1080p
  sono preservati; i ritardi e la risposta attiva sono verificati nei test nativi.
- `test-movement.py --base-only`: **PASS**; dopo la correzione del soffitto
  temporaneo, `--posture-only`: **PASS** nella build finale. La collisione viene
  pubblicata prima della richiesta di alzarsi su entrambi i peer. Sono usati
  geometria temporanea nel Deposito, input reali per sprint e test separati
  dei tasti C/Ctrl per la postura. Include soglie30/80/150cm, salto+salita200cm,
  postura del proprietario/proxy, recupero e tasti W/C/Ctrl. Nessun laboratorio
  salvato viene ricreato. `test-network.py --accuracy`: **PASS**.
- `test-industrial-map.py --network --render --rule 2 --seed 16`: **PASS**,
  16 rampe fisiche, host Metal, client iniziale e ingresso tardivo; distruzione,
  rigenerazione del round ed epoch2 con digest finale1151053692.
- `setup_industrial_ladders.py` e `verify_industrial_maps.py`: **PASS**.
  Ricetta revision3 e snapshot `DA_CentralBuilding_42_Ladders`; Depot e catalogo
  materiali mantengono gli SHA256 precedenti. Scene Metal ispezionate.
- Python compile e diff whitespace (repository principale e asset privati): PASS.

Ricevuta: `docs/PLAYER_MECHANICS_RESULTS.json`. Log in `Saved/Testing/player-mechanics-*`.


## Armi sulle scale automatiche — 19 ottobre 2026

Questa revisione sostituisce la precedente regola X/sole mani libere o pistola.
Pistola, coltello e granate a mano rimangono impugnati; le altre armi vengono
riposte automaticamente. L'inventario conserva l'arma selezionata e permette
cambi sulla scala: gli oggetti compatibili vengono impugnati subito, le altre
armi rimangono riposte fino all'uscita. La transizione sul tetto mantiene la
stessa regola. Nessun comando X. Munizioni e cooldown rimangono per arma;
sparo, ricarica e mirino dell'arma riposta sono bloccati dal server.

- Build Editor UE5.8.3: PASS.
- `test-player-mechanics.py --fps 30`: PASS, 15 controlli server e4 client,
  con latenza60ms ±15ms. Fucile riposto automaticamente, cambio sequenziale
  coltello/granata/pistola/lanciagranate sulla scala, mantenimento dell'ultima
  selezione, blocco sparo/ricarica/mirino, passaggio sul tetto, discesa,
  distacco e distruzione dei pioli. Velocità144/180cm/s confermate dal client.
  Le cadute brevi/6m/letali e la morte replicata restano corrette.
- `test-network.py --accuracy`: PASS, sparo, munizioni, dispersione e recupero
  replicati senza regressioni.
- `EclipseRecoil.Player`: 2/2 PASS (cadute e isteresi risoluzione), rieseguiti
  con la build aggiornata.
- Diff whitespace e compilazione Python: PASS.

Log di questa revisione: `Saved/Testing/ladder-equipment-*`.
Gli esiti precedenti relativi a geometria, seed e rendering restano storici;
questa revisione non modifica gli asset delle mappe o il governor grafico.


## Combattimento e precisione sulle scale — 19 ottobre 2026

Pistola, coltello e granate a mano sono utilizzabili sui pioli. Aggiunto alla
pistola un malus massimo0,6° al semiangolo del cono, proporzionale alla velocità
verticale assoluta, con tolleranza5cm/s: identico in salita/discesa e nullo da
fermi. Il dato LadderMovingSpreadDegrees è configurabile e il calcolo condiviso
GetSpreadDegrees alimenta sia i colpi autorevoli sia il reticolo del client.

- Build Editor UE5.8.3: PASS.
- `test-player-mechanics.py --fps 30`: PASS, 23 controlli server e4 client,
  latenza60ms ±15ms. Uso reale di coltello/pistola e armamento/rilascio della
  granata sulla scala; cono normale da fermo, maggiore durante salita/discesa,
  LastShotSpreadDegrees uguale al cono mostrato e previsione del client.
  Confermati anche selezione/ripristino, tetto, pioli distrutti e cadute.
  Il proiettile della granata viene rimosso dopo il rilascio solo nella fixture
  di prova, per non interferire con le successive prove di movimento.
- `test-network.py --accuracy`: PASS. Nessuna regressione negli altri stati
  di movimento, nei colpi, nelle munizioni o nel recupero del bloom.
- Diff whitespace e compilazione Python: PASS.

Log: `Saved/Testing/ladder-combat-*`.


## Generatore con priorità alle scale a pioli — 19 ottobre 2026

Ricetta industriale revision4: torrette e strutture compatte usano scale a pioli
tra ogni piano, con botole160cm alternate e sbarchi rivolti verso la stanza.
Il corridoio tra le porte resta libero. Gli edifici secondari più ampi
(almeno800×1150cm, escluso il tipo torretta) conservano rampe; il centrale
mantiene le sue due rampe e aggiunge una coppia di scale a pioli esterne.
Restano gli accessi esterni degli edifici secondari. Guide e pioli sono acciaio
calibrato2mm distruttibile; geometria e metadati rispettano la simmetria.

- Build Editor UE5.8.3: PASS.
- `test-destruction.py`: 12/12 PASS, 516 seed, 516 sagome e177 composizioni.
  Verificati accesso per piano, pairing, collisione della capsula sull'intera
  scala, pavimento degli sbarchi, corridoi liberi, supporto iniziale e distruzione.
- `setup_industrial_ladders.py` e `verify_industrial_maps.py`: PASS.
  Ricetta salvata revision4 e snapshot attivo DA_CentralBuilding_42_PreferredLadders.
  Depot e catalogo materiali mantengono gli SHA256 precedenti.
- `test-industrial-map.py --network --render --rule 2 --seed 16`: PASS.
  Il personaggio ha percorso realmente 10 rampe e tutte le 10 scale
  a pioli della variante a tre piani, compresi accessi interni ed esterni.
  Confermati distruzione/crolli, client iniziale, ingresso dopo la distruzione
  e rigenerazione del round (epoch2, digest finale170260109).
  Pianta e vista a terra Metal ispezionate.
- Compilazione Python e diff whitespace, anche asset privati: PASS.

Ricevuta: `docs/INDUSTRIAL_MAP_RESULTS.json`. Log e immagini permanenti di questa
verifica in `Saved/Testing/preferred-ladders-*`.


## Finiture Factory grigie e container colorati — 19 ottobre 2026

Ricetta revision5: acciaio grigio poco usurato su scale a pioli, strutture,
piani metallici e reti. Container verniciati blu, rosso scuro, verde e ocra.
Il colore viene scelto indipendentemente per ogni copia simmetrica, con un hash
visivo deterministico separato dal generatore geometrico. Dimensioni, collisione,
materiale fisico, rinforzo8, supporto e percorsi rimangono quelli precedenti.
Le superfici per pannello entrano nel digest del layout e vengono applicate
anche ai frammenti cosmetici risolvendo il pannello di origine dal payload esistente.
Il catalogo condiviso e Depot conservano gli SHA256 precedenti.

I materiali privati adattano BC/N/ORM Factory: ritaglio grigio delle travi per
l'acciaio e sola zona corrugata del container per la vernice, senza cerniere/loghi
ripetuti. La lamiera ripete su600×300cm, l'acciaio su150×150cm, con UV speculari.
Gli originali Factory restano invariati. Collegamenti UV e tipi di campionamento
sono stati verificati con i sorgenti dell'API Unreal e su Metal.

- Build Editor: PASS.
- `test-destruction.py`: 12/12 PASS, 516 seed, 516 sagome e177 composizioni.
  Colori indipendenti possibili su geometrie simmetriche e scelta ripetibile;
  collisione, pioli, resistenza, supporto e distruzione senza regressioni.
- `setup_factory_finishes.py` e `verify_industrial_maps.py`: PASS, ricetta5 e
  snapshot attivo DA_CentralBuilding_42_FactoryFinishes; palette verificata.
- `test-industrial-map.py --network --render --rule 2 --seed 16`: PASS.
  Verificati materiali effettivi dei chunk su host/client, tutte le10 rampe e10
  scale a pioli, distruzione/crolli, ingresso tardivo e nuovo round. Nessun
  fallback degli shader Factory. Digest finale epoch2: 3776669601.
- Dopo il test LAN è stato rifinito soltanto il rettangolo/scala UV del container:
  `test-industrial-map.py --finish-gallery --rule 2 --seed 16`: PASS su Metal.
  Pianta, struttura e container integro ispezionati; lamiera pulita, senza parti
  delle porte ripetute. Palette, identità dei materiali e fisica invariate.
- Compilazione Python e whitespace (anche asset privati): PASS.

Ricevuta: `docs/FACTORY_FINISHES_RESULTS.json`; log/immagini
`Saved/Testing/factory-finishes-*`. La galleria rapida è un probe esplicito e
non sostituisce la prova LAN o le prove di distruzione.
## Coperture intermedie industriali — 19 ottobre 2026

Ricetta revision6 e snapshot PCG attivo `DA_CentralBuilding_42_DenseCover`.
Più fasce di ripari distruttibili: pareti a L/U, divisori in mattoni/cemento,
casse di legno e container rinforzati. OpenYard aggiunge schermature sui fianchi
centrali. Ingressi, nascondigli, percorsi e accessi agli edifici/scale riservati.

- Build Editor arm64 Development: PASS. Suite distruzione: **12/12 PASS**.
- **516 seed**, 516 geometrie distinte e 515 composizioni di coperture; minimo
  osservato di 26 punti di copertura in OpenYard e 18 nelle altre varianti,
  contando entrambi i lati della mappa.
- Collisioni fisiche su tutte le tre regole e uno/due/tre piani: capsule nei
  nascondigli, ingressi liberi, raggi all'altezza reale degli occhi e schermi
  dei fianchi OpenYard. Breccia nella copertura: nascondiglio esposto.
- OpenYard seed42, Metal LAN: **PASS**. 38 ripari verificati (28 in piedi,
  10 accovacciati), palette Factory, accessi verticali reali, distruzione,
  replica al client iniziale e tardivo, nuovo seed e round ripristinato.
- Ricetta salvata e verificata su 516 seed; PCG esporta `CoverPockets`.
  SHA256 del Deposito e catalogo condiviso invariati. Immagini Plan/Ground
  controllate; da terra le pareti formano ripari e spezzano le visuali.

Ricevuta: `docs/INTERMEDIATE_COVER_RESULTS.json`; prove e immagini in
`Saved/Testing/intermediate-cover-*`. Nessun packaging o pubblicazione.

## Indicatori colpi/dispersione in scope — 19 ottobre 2026

L'anello e il contatore restano visibili con tutte le armi, anche nelle ottiche;
Assault Rifle e Grenade Launcher conservano la croce centrale durante la mira.
Il raggio continua a leggere dispersione reale, FOV corrente e stato replicato.

- Editor Mac arm64 Development: **PASS**, 25,91s, con i warning di deprecazione
  esistenti in altri file (`Saved/Testing/scoped-reticle-build.log`).
- `test-scopes.py --render`: **PASS**, due peer reali, 26 stati/input replicati,
  FOV, allineamento mire, transizioni e catture Metal aggiornate. Anche i controlli
  server di precisione, colpi e lancio in mira sono passati.
- Ispezionate `Scope_101`, `104`, `109`, `115`, `120` e `121`: anello e numero
  nelle ottiche, croce centrale con Assault/launcher, munizioni dopo lo sparo
  aggiornate a 19 per Sniper e a 5 per launcher.
- Primo avvio interrotto dalla chiusura della finestra prima dell'ingresso del
  client; non contato come prova riuscita. Ripetizione completa finale passata.
- `git diff --check`: **PASS**. Modifica limitata alla presentazione HUD;
  nessuna rigenerazione degli asset o modifica alla balistica.

Log: `Saved/Testing/scoped-reticle-scopes.log`, `scopes-render-{host,client}.log`.
Schermate: `Saved/Testing/Scopes/Scope_100..125.png` e `Transition_*.png`.

## Dispersione Sniper/Marksman — 19 ottobre 2026

- Editor Mac arm64 Development: **PASS**, 32,97s (`optical-accuracy-build.log`),
  warning di deprecazione preesistenti in altri file.
- `setup_optical_accuracy.py`: **PASS**, salvati tramite Unreal soltanto
  DA_Carbine (Sniper) e DA_Sniper (Marksman). Coni base senza mirino5/8 gradi;
  nuovi coefficienti di movimento in mira accovacciati/sdraiati .8/1.2.
- `test-scopes.py --render`: **PASS**, 26 stati reali del client, FOV,
  allineamento mire, transizioni, precisione e comportamento dei colpi.
  Il probe server verifica entrambe le armi in posture controllate: crouch
  a100cm/s .288/.432 gradi, crawl a55cm/s .103/.154 gradi, ritorno a zero
  con ottica assestata da fermi. Senza mirino il movimento aumenta ancora il
  cono in entrambe le posture. Client fresco: dispersione base5/8 confermata.
- Catture Metal `Scope_103.png` e `Scope_107.png` ispezionate: anelli senza
  mirino più ampi e contatori ancora visibili.
- `test-network.py --accuracy`: **PASS**, compresa replica munizioni/bloom e
  recupero sul clock server dell'owner.
- `test-reference-weapons.py`: **PASS**, tutti i dieci slot remoti e controlli
  esistenti di munizioni, danno, armatura, pellet, copertura e launcher.
- `test-network.py`: **PASS**, danno/copertura, munizioni, squadre, protezione,
  morte/corpi, respawn e reset del round.
- `test-arsenal.py`: **PASS**, fixture precedente, cambio slot/rotella,
  ricariche, ordigni ed effetti replicati.

Log: `Saved/Testing/optical-accuracy-{build,assets,scopes,accuracy,reference,network,arsenal}.log`,
`scopes-render-{host,client}.log`. Sintassi Python e whitespace verificati.

## Dispersione aggiuntiva in sprint — 19 ottobre 2026

Penalità comune basata sulla velocità orizzontale reale: zero fino a10km/h,
+0,75° a14km/h e +1,5° a18km/h, poi limitata a1,5°. Viene aggiunta dopo il
limite ordinario del cono, così anche Marksman già a10° peggiora in sprint.
Server e HUD condividono il calcolo, anche in mira; segue accelerazione/frenata.

- Editor Mac arm64 Development: **PASS**, 29,22s (`sprint-accuracy-build.log`).
- `test-network.py --accuracy`: **PASS**. Tutte le sette armi da tiro attive,
  comprese Assault/launcher in mira e le due ottiche: incremento progressivo,
  limite oltre18km/h e rimozione del surplus tornando alla corsa normale.
  Passati anche i controlli esistenti su colpi, bloom e replica all'owner.
- `test-movement.py --posture-only`: **PASS**, due peer con input W reali.
  Owner: cono .730° alla corsa normale contro2.230° durante lo sprint.
  Sul server il colpo realmente sparato in sprint legge lo stesso cono ampliato.
  Sprint/recupero, interruzioni, crouch/prone e input C/Ctrl restano verificati.
- `test-reference-weapons.py`: **PASS**, dieci slot remoti e combattimento del
  catalogo corrente.
- `test-arsenal.py`: **PASS**, fixture precedente, cambio armi/rotella,
  ricariche, ordigni ed effetti replicati.

Log: `Saved/Testing/sprint-accuracy-{build,accuracy,movement,reference,arsenal}.log` e relativi
`accuracy-{host,client}.log`, `movement-{host,client}.log`.
Nessun nuovo asset o campo replicato; sintassi Python e whitespace verificati.


## Minimap and recon drones — 19 October 2026

Editor module compiled successfully with UE5.8.3 on Mac arm64.
`test-recon.py --automation`: both FogAndSight and Flight passed. The engine
prints its existing pre-session automation self-test diagnostics at startup;
the two requested tests report Success and the process exits0.

`test-recon.py --render`: PASS,18 server assertions plus actual opposing owner,
teammate and late-join snapshots. Listen host rendered with Metal at1280×720;
remotes used nullrhi,40ms packet lag, and2% incoming loss on the late joiner.
Physical cover blocked both feeds, removal exposed enemies, a teammate shared
sight, turning both observers away removed the enemy without erasing explored
terrain, and drone360 sight added the contact. A normal remote rifle RPC and
authorized bullet destroyed the opposing drone. Next launch was scheduled for
server time56.55 and actually occurred at56.76,30.21 seconds after loss. It
remained absent at the early check. Four further server point hits exhausted
stock; only those subsequent replacement deadlines were advanced by the probe.
The fifth loss scheduled no replacement. Reset restored five units and empty
fog. The late owner received existing epoch1,512 fog bytes and remaining4;
joining did not reset team history/stock. Screenshot Recon02_Minimap.png
inspected: circle/top-right placement, team colors, heading and world mapping.

Regressions after integration: test-network.py --shot-feedback PASS (normal
combat, owner/proxy shot feedback, corpses and protection); --accuracy PASS
(server spread/bloom and owner clock). test-industrial-map.py --network --render
PASS, seed42/fenced crossing, host Metal plus initial/late remotes. Existing
stairs/ladder traversal, rifle and container/cover breaches, collapse and actual
round regeneration passed. Initial/state-changed/late-join/fresh-epoch digests
agreed. Recon logged one reset/capture per geometry epoch (1 then2), both
bases present; viewed Ground/Plan captures confirm static overhead rendering,
fog after reset and correct live markers. No new material shader failures.

Original models/credits and both archives retained privately. The first art
preview exposed missing mirrored drone arms because the source blend hides
authoring objects; the bake now unhides evaluated objects before applying
source modifiers (5186 derived vertices). Case preview confirms the supplied
PBR textures and closed lid/lock. Final import and drone preview outcomes are
recorded with the private import receipt and Saved/Testing/recon-assets.log /
recon-gallery.log. The final reimport used the full Editor with
-ExecutePythonScript after the commandlet encountered a PCGEx startup-load
failure; full Editor import passed, with widths 85cm/70cm and heights 27.06cm/27.40cm.
Final Recon102_Drone.png was inspected on Metal: all four mirrored arms and
motors are present. Final gallery and both recon automation tests exited0;
asset-mount verification passed for all28 mounts with no unresolved LFS pointers.

Evidence: Saved/Testing/recon-metal-report.json, recon-automation.log,
recon-build.log, recon-regression-{network,accuracy,industrial}.log and
industrial-network-metal-report.json. No cook, packaging, commit or publication.
Limits:64×64 fog, one static roof-inclusive image for all storeys; later
destruction is not recaptured. Smoke still does not interrupt reconnaissance.
These tests do not certify isolated-game FPS or all hardware/network conditions.


## Enemy contacts, smoke occlusion and drone fragments — 19 October 2026

The previous30m player cutoff could suppress an otherwise exposed enemy. Drone
sight also used3D distance, shrinking its ground coverage with altitude. Player
sight now spans the map; the drone uses a42m horizontal footprint. Enemy marks
are larger team-colored diamonds with white/dark outlines, and near-rim marks
are clamped inside the circle instead of being dropped. Camera origins match
authoritative shots. Fog ray work is capped at2048 traces/team/update; contact
visibility is evaluated independently every250ms.

Final native compilation and Unreal Python import: PASS. Five actual S9 model
fragments were saved with Blueprint defaults and original attribution.
Automation: FogAndSight, Flight and SmokeOcclusion all PASS. Smoke tests cover
ground/overhead rays, misses, initial growth, fade, expiry and blast pockets.
The first automation launch was closed before tests ran; the repeat completed
all three. Existing engine startup self-test diagnostics remain in logs.

Metal LAN recon: PASS,31 server assertions and8 sighting assertions on each
opposing/teammate remote, plus a late join. The exposed target was40m away.
The drone spot used36m horizontal separation plus25m altitude (over42m in3D).
Actual HUD drawing was asserted and the outlined enemy marker inspected in
Recon02_Minimap.png. A real mature smoke cloud suppressed player, teammate and
drone observations and the drawn enemy marker; blast dispersal restored them,
a fresh cloud hid them again, and cloud removal restored them. Both remote
feeds matched every expected sighting phase. Lag40ms; late peer incoming loss2%.
Already-known terrain persists. The shader's texture noise uses its mean in
the authoritative density calculation; no pixel-exact GPU readback is claimed.

A real client rifle hit generated exactly one breakup event and five local
source fragments. Metal verified actual downward movement under gravity;
Recon10_DebrisAir.png was visually inspected. Existing30s replacement, stock
exhaustion and late-join checks passed; round reset cleared the fragments.
The initial fixture's inactive drones were moved out in XY as well as Z to
match the corrected horizontal optics. The import now sets fragment references
before Blueprint compilation and verifies the saved defaults.

Regressions: test-grenades.py PASS, including remote tumble, both fuse payloads,
team smoke colors, hemisphere/window containment and material checks;
test-network.py --shot-feedback PASS, including combat, owner/proxy feedback,
corpses and protection. Final rim placement adjustment was compiled before
these regressions. No packaging, publication or commit was performed.
Full labels, source hashes and log/capture paths: RECON_FIXES_RESULTS.json.
This section supersedes the earlier note that smoke did not interrupt recon.

## Minimap orientation by team — 19 October 2026

Editor module compilation: PASS. The HUD uses the replicated axis between the
two spawn groups, keeping the viewer's spawn below the opposing spawn. The
common world-space capture/fog grid is unchanged; all displayed image vertices,
fog cells, contacts, direction ticks and north reference share one rotation.

Two actual Metal standalone sessions on Central Building Industrial, revision6,
seed42, OpenYard: PASS. Alpha's HUD reports79.3 degrees and Omega259.3, matching
the actual offset spawn centers (-850,-4480)/(850,4480) and differing by180.
Both generated bird captures completed, both intact-map gallery probes passed,
and both Ground screenshots were visually inspected: the owner's team markers
are below, opposing contacts above, with aligned terrain/fog and compass.
The expected angles were checked against the spawn positions, not an assumed
world-Y heading. This validation covers both local team views on this seed;
the earlier combat/recon LAN receipt remains historical.

Evidence: Saved/Testing/minimap-team-orientation-build.log,
minimap-orientation-{Alpha,Omega}.log, minimap-team-orientation-report.json and
Minimap-{Alpha,Omega}-{Plan,Container,Ground}.png. No asset reimport was needed.

## Minimap follows player heading — 19 October 2026

The latest request supersedes the fixed spawn orientation above. The HUD reads
the local controller's yaw each rendered frame. All existing projection paths
share that rotation, keeping the owner's direction tick up. The spawn-axis
calculation and replicated heading field were removed. The overview remains
centered on the map and the static capture/world fog grid is retained.

Final Editor module build: PASS. Actual Metal standalone on Industrial seed42,
Omega: PASS. Temporary engine CSV frame commands changed ControlRotation to
0/135/270 degrees during one session and captured each view. The HUD audit
observed all three headings, and the intact-map gallery completed successfully.
All three screenshots were visually inspected: terrain, explored-cell edges,
contacts and north rotate together while the owner's white tick stays up.
The bird image was captured once; turning did not recapture it. Normal local
mouse input also produced intermediate headings in the audit. No test-only
runtime feature or additional asset was introduced.

Evidence: Saved/Testing/minimap-player-orientation-build.log,
minimap-player-orientation.log, minimap-player-orientation-report.json and
Minimap-PlayerYaw-{0,135,270}.png. This is a local rendering check; the previous
combat/recon LAN results remain historical.
## Menu music — 19 October 2026

- Mac arm64 Development Editor build: succeeded (26.90 s; existing unrelated
  engine deprecation warnings).
- `setup_menu_music.py`: PASS. Original WAV preserved byte-for-byte,
  SHA-256 `d488ef5f8371743fd39126ecdc86b80eab330952eec5a93dac732a79e0b35603`;
  saved stereo SoundWave duration 149.97351 s, looping, Bink Audio quality80,
  prime-on-load, controller default bound at volume0.35. Derived-data encoder
  reported 5,333 KiB compressed from 25,835 KiB PCM (20.64%). This is the
  local encoder result, not a packaged-build size measurement.
- Actual `-game -nullrhi` processes with audio enabled: menu→Depot PASS
  (one start in main menu, stop on travel); Depot→menu PASS (no music start
  during Depot, start after main-menu world begins). Logs and receipt:
  `Saved/Testing/menu-music-{menu-to-game,game-to-menu}.log` and
  `Saved/Testing/menu-music-results.json`.
- Scope of verification: runtime audio component lifecycle and saved import
  settings. No listening-quality or full 150-second loop-boundary audition.
  Headless run reported CoreAudio device/sample-rate and Mac text-input window
  warnings; no claim of verified physical speaker output.

### Music options and Esc menu — 19 October 2026

- Supersedes the initial Esc-menu silence behavior: the music slider applies
  immediately to an open menu, zero stops/destroys the component, and the
  in-game menu uses half the front-end gain. Closing the menu stops playback.
- Mac arm64 Development Editor build: succeeded (29.96 s).
- Two fresh audio-enabled `-game -nullrhi` starts read saved preferences:
  MusicVolume0.5 produced logged gain0.175 in L_MainMenu; MusicVolume0
  loaded the same world without starting music. Both exited normally;
  test preferences were restored afterward. Logs:
  `Saved/Testing/menu-volume-saved-{0.5,0}.log`.
- Rendered menu startup logged gain0.350. Native UI automation could not bind
  the game window; slider dragging and Esc-key interaction were not verified
  through that interface. The shared ShowMenu/UpdateMenuMusic path and Slate
  callbacks were reviewed; no claim of an audible playback assessment.


### Irregular structural explosion breaches — 19 October 2026

- UE5.8.3 / macOS arm64 Development Editor: final build PASS
  (`scripts/build-editor.command`,21.13s; `/tmp/respawn-fracture-build.log`).
- `python3 scripts/test-destruction.py`: PASS,12/12 automation tests. Includes
  the extended DamagePreset contour checks and actual cell collision/brick
  passage tests. Initial symmetric-noise tuning narrowed a thin-brick breach;
  corrected by preserving the old breach and adding bounded outward fractures.
  Final run: `Saved/Testing/destruction-automation.log`.
- `python3 scripts/test-industrial-map.py --network --render`: PASS. Metal
  listen host + initial remote + late remote, seed42/OpenYard. The probe now
  applies a real structural explosion after the authorized rifle shot; damage,
  removals and cell validation PASS. Both remotes match destroyed-state
  digest4196437526 and regenerated-state digest2556534412. Report:
  `Saved/Testing/industrial-network-metal-report.json`.
- Metal ground capture inspected; the standard gallery framing does not
  provide a close-up of the blast edge, so it is not standalone visual evidence
  of contour quality. Contour variation is covered by the equal-radius
  removal-threshold test; physical breaching and replica parity run in Unreal.
- Historical `test-cell-network.py --render` cannot load L_DestructionLab
  (removed from the current project); replaced here by the current industrial
  LAN probe above. No legacy lab assets recreated.
- `git diff --check`: PASS. Other in-progress menu/music changes preserved.

### Standalone distribution preparation — 19 October 2026

- Mac Development universal compilation PASS, executable contains arm64 and
  x86_64. Version `26.10.19.dd192e70`, UE5.8.3 / Xcode27.1. Isolated receipt:
  `/private/tmp/rr-macos-builds/20261019-201705/receipt.json`.
- Initial packaged startup exposed missing CentralBuilding: UAT's successful
  default cook included only the menu dependency closure. Added explicit
  MainMenu/Depot/CentralBuilding MapsToCook and recooked all three maps with
  the existing linked binaries; 2223 packages cooked, packaging PASS.
- Actual packaged Development executable, Metal offscreen host plus initial
  and late LAN clients: PASS. Real stair/ladder traversal, rifle damage,
  structural explosion, collapse and regeneration. Both remotes match destroyed
  digest4196437526 and regenerated digest2556534412. Receipt:
  `Saved/Testing/industrial-network-packaged-metal-report.json`. Runtime tested
  on Apple Silicon; the Intel slice was inspected, not executed on an Intel Mac.
- Local Editor rebuilt after generated-file cleanup: PASS (22.83s). Restored
  the installed plugin's generated Editor headers from the isolated build only
  after checking that every plugin source file was byte-identical. A fresh
  headless Editor game process loaded MainMenu and exited cleanly.
- Linux snapshot fixture verifies source-only Marketplace dependency inclusion,
  unchanged original descriptor and gzip-only stdout streaming: PASS. Four
  signing/notarization helper tests with mocked Apple tools: PASS. These tests
  do not claim an actual Apple notarization.
- Actual Developer ID identity created with the user's explicit confirmation:
  Dr Wolf srl, team8V893A78SP. Private key remains in local Keychain.
  Notarization and public distribution are still pending at this checkpoint.
- Final app signed with Developer ID, secure timestamps and hardened runtime;
  deep strict codesign verification PASS. Signed app MainMenu and Depot starts
  with real offscreen Metal both PASS (expected stamp and clean exit). Logs:
  `/private/tmp/rr-release-20261019/{L_MainMenu,L_Depot}-signed-metal.log`.
- Notarization checkpoint: Xcode interface unavailable because the Mac is
  locked; CLI upload fails `No Accounts` / missing `Xcode-Token`. The signed
  app and an independent APFS-cloned xcarchive are retained. This is not an
  Accepted notary submission or a verified Gatekeeper distribution.
- Corrected Linux input uploaded directly to private S3 (6194953371 bytes),
  task `7e45bc1a951e41a69a6bf5f1aef86602`, 16vCPU/64GiB, in progress.
  Output prefix `outputs/1792436291`; earlier incomplete inputs were not
  published. No claim of a completed Linux build at this checkpoint.

### Menu, saved setup and LAN lobby — 20 October 2026

- UE5.8.3 Editor arm64 module compilation: PASS. Final build log:
  `Saved/Testing/lobby-build-final.log`. Existing engine/PCG deprecation warnings
  remain; no new font-constructor deprecation.
- Native `EclipseRecoil.Menu` automation: 2/2 PASS (IPv4 validation/name
  sanitization; strict-majority quorum, absent/invalid votes, ties and plurality).
  Log: `Saved/Testing/menu-automation.log`.
- `scripts/test-lobby.py --timeout`: PASS. Three actual Unreal processes on
  one Mac, broadcast discovery using the LAN IPv4 address, human quorum 2/3,
  Depot seamless travel, replicated team/skin/weapon/supplies and actual
  owner ammunition. Host and beacon UDP sockets verified by lsof as wildcard
  listeners. Separate process confirms the real 30-second no-server timeout.
- `scripts/test-lobby.py --settings`: PASS. Two real graphical launches using
  an isolated user directory and config saving enabled. First launch fullscreen
  5120x2880; saved 60% window 3072x1728 restored on the next launch. Sound/music,
  mouse sensitivity/acceleration and name/team also restored. The development
  launcher no longer supplies resolution/window-mode overrides. Logs:
  `Saved/Testing/Lobby/settings-{write,read}.log`.
- `scripts/test-lobby.py --gallery`: PASS. Eight 1280x720 Metal captures in
  `Saved/Testing/Lobby/user-gallery/Saved/Screenshots/Menu`. Visually inspected
  initial menu (only five large white entries on the right, no dark panel),
  character selection with real 3D skin preview, equipment and complete lobby
  READY control. Disabled Internet/privacy options remain visible. No claim
  of testing all skins or of physical multi-machine LAN testing in this run.
- `scripts/test-arsenal.py`: PASS, including remote weapon changes, ammo,
  respawn, HE/smoke/stun, smoke cuts and replicated effects. Output:
  `Saved/Testing/menu-arsenal-regression.txt`.
- Source-only Linux build-input fixture: PASS, including the font/license;
  Mac input now copies the same font directory. `zsh -n scripts/play.command`,
  Python syntax checks and Mac Info.plist validation: PASS. No new standalone
  package, notarization or publication performed for these menu changes.
- Issues found and corrected before final checks: BSD broadcast address setup
  cleared the destination port; manual camera exposure hid the preview;
  weapon BeginPlay overwrote supplies set before world BeginPlay; a legacy
  remote player's default weapon must use the server world's arsenal flag,
  rather than looking for that flag in the client's connection URL.
- Final `scripts/test-network.py`: PASS. Combat, manual respawn, ammo/reload,
  team scoring, protection, corpse replication and round restart all passed
  after correcting the legacy default. `Saved/Testing/menu-network-regression.txt`.
- `scripts/test-admission.py`: PASS. Four humans, balanced 2/2 automatic
  assignment, fifth player rejected, late join does not reset the match,
  disconnect releases the slot; normal hosting creates no bots. Output:
  `Saved/Testing/menu-admission-regression.txt`.
- Final `scripts/test-lobby.py --industrial`: PASS. No MULTIHOME supplied by
  the test launcher: Start Server selected IPv4 wildcard binding itself,
  verified for both game and discovery sockets. READY 2/3 selected Central
  Building; server generated 2318926 cells / 14 shards, epoch 1. Both remotes
  reported `ER_BLOCK_NET_PLAYER_READY epoch=1` before the host accepted the
  run; every peer then passed actual geometry-ready, travel and supply checks.
  Output: `Saved/Testing/menu-industrial-lobby.txt`.
- Final `git diff --check`: PASS. Existing unrelated distribution changes in
  the working tree preserved; only font inclusion added to build-input helpers.

### Configurable human capacity and expanded spawns — 20 October 2026

- Editor arm64 compilation PASS (`Saved/Testing/capacity-build-final.log`).
  Server configuration defaults to 16 humans including the listen host, accepts
  positive typed capacities (spinner 1–64) and remains authoritative through
  admission, beacon availability, replicated lobby status and seamless travel.
- `scripts/test-capacity.py`: PASS with six actual Unreal peers on one Mac.
  All six selected Alpha. Depot created four additional starts around its two
  authored Alpha starts; all six pawns had supporting floor and non-overlapping
  capsules. Six-player lobby required four READY; three waited. Configured 6
  survived travel and matched GameSession. Full server stopped advertising;
  seventh client was rejected despite injected MaxPlayers=999 URL options.
  Disconnect released a slot, discovery resumed and replacement client 99 spawned.
  Report: `Saved/Testing/capacity-depot-report.txt`, full logs `CapacityDepot/`.
- `scripts/test-lobby.py --settings`: PASS, including the new server capacity 23
  surviving two graphical launches with isolated settings. Existing fullscreen,
  display, audio, mouse and profile checks remain PASS.
  Report: `Saved/Testing/capacity-settings-report.txt`.
- Native `EclipseRecoil.Menu` tests 2/2 PASS. Added positive/minimum capacity,
  typed 128 above the spinner range and 16-player quorum boundary checks.
  Log: `Saved/Testing/capacity-automation.log`.
- `scripts/test-session.py --map L_Depot`: PASS for stationary and movement-only
  bots, explicit per-team counts, clear spawns, real motion/no firing and remote
  admission/team selection. Report: `Saved/Testing/capacity-bots-report.txt`.
- Existing four-player admission test now explicitly requests ERMaxPlayers=4;
  the number is a fixture, not the production default. No saved maps or private
  art assets changed; procedural and overflow starts are runtime-only actors.
- `scripts/test-capacity.py --industrial`: PASS with six humans on Alpha,
  all ten industrial starts per team available, separated/supported pawns and
  every remote reporting its generated map ready before the client spawn check.
  Full-server rejection, beacon suppression and discovery/replacement after a
  disconnect also passed. `Saved/Testing/capacity-industrial-report.txt`.
- Metal menu gallery: PASS; inspected the numeric 16 field in Start Server and
  the replicated 1/16 lobby header. READY remained fully visible. Captures:
  `Saved/Testing/Lobby/user-gallery/Saved/Screenshots/Menu/{server,lobby}.png`.
- `scripts/test-network.py`: PASS after the final spawn changes, including
  manual respawn, initial/new-life ammo, hitscan, reload, team scoring, corpse
  replication/protection and round restart. `Saved/Testing/capacity-network-report.txt`.
- Python syntax checks and final `git diff --check`: PASS. Tests use local
  processes; no claim of sixteen-machine testing or large-server performance.

### Teko menu font and button feedback — 20 October 2026

- Inspected the owner-supplied Teko ZIP and ran its download script unchanged.
  Google Fonts original: 292108 bytes; generated a static weight-600 instance
  with fonttools 4.67.0, preserving copyright and the full SIL OFL 1.1 notice.
  Runtime font has no `fvar` table and contains printable ASCII glyphs.
- Editor arm64 build PASS (`Saved/Testing/teko-menu-build.log`). Native Slate
  logs confirm successful FreeType loading of `Teko-SemiBold.ttf`, rather than
  fallback rendering (`Saved/Testing/teko-interaction.log`).
- Metal menu gallery PASS (`Saved/Testing/teko-gallery-report.txt`), eight
  captures at 1280x720. Inspected home, Setup, server and lobby. Five large
  white shadowed home entries retain the artwork without a dark panel.
  Wrapped map vote labels and reduced vote/READY padding after spotting clipping.
- Native UI navigation: Return opened Play Offline, Esc returned home and
  Tab/Return activated Exit Game in an isolated profile. Main button styles
  implement gold hover, orange press, a small pressed offset and hand cursor.
  macOS automated coordinate clicks did not reliably reach Slate; hover and
  click activation are therefore not recorded as a successful physical mouse
  test. Diagnostic log: `Saved/Testing/teko-input-debug.log`.
- Linux build-input archive fixture PASS with the Teko font and license paths
  (`python3 -m unittest discover -s scripts/tests -p test_archive_linux_project.py`).
  Runtime dependencies include the static TTF, credits and full license; build
  input scripts retain the font sources and regeneration instructions as well.
- Final rebuild PASS (`Saved/Testing/teko-menu-build-final.log`); final gallery
  PASS (`Saved/Testing/teko-gallery-final-report.txt`). Inspected the refreshed
  lobby capture: both map vote labels and the entire READY button fit at 720p.
  Final archive fixture and `git diff --check` PASS.

### Dedicated Credits page — 20 October 2026

- Editor arm64 builds PASS (`Saved/Testing/credits-build.log` and
  `Saved/Testing/credits-build-final.log`). Added Credits before Exit Game in
  the main/pause menu and a shortcut from Setup. Back retains its source page.
- `scripts/test-lobby.py --gallery` PASS (`Saved/Testing/credits-gallery-report.txt`).
  Inspected actual Metal `home.png` and `credits.png` at 1280x720: all six home
  entries fit, the Credits title/Back sit above the scrollable content, Teko
  headings and body text wrap inside the panel and the font credits load.
  This checks rendering; it does not claim a physical mouse interaction test.
- Existing Linux archive fixture PASS after adding `Branding/Credits.txt`
  inclusion. Runtime staging and isolated Mac/Linux inputs carry the same file.
  Final `git diff --check` PASS. No gameplay, map or private asset edits.

### Windows packaging helper — 20 October 2026

- PowerShell 7.6.7 on macOS: script parsing and
  `scripts/tests/test_package_windows.ps1` PASS. Checks cover missing executables,
  nonzero native exit codes, successful stderr, combined log streams, spaces in
  arguments, aggregated preflight errors, Launcher manifest/environment engine
  discovery, disk-space rejection, and stopping before UAT after an Editor
  failure. Mocked UAT failure propagates; Shipping applies to Game while Editor
  stays Development.
- Running the real helper with `-Check` on macOS correctly rejects the host
  with exit code 1. No Unreal build or cook was launched.
- Public-tree fixture tests PASS (7 tests), including allowing textual `.ps1`
  scripts while still rejecting embedded binary blobs. Source-export fixture
  includes the new PowerShell helper while excluding assets and symlinks.
- Native Windows PowerShell 5.1 execution, actual MSVC/SDK discovery, Windows
  junction preparation, UE compilation/cooking and packaged game launch remain
  unverified; no Windows build success or distribution is claimed.

### Windows Butler upload helper — 20 October 2026

- The requested Windows helper now publishes successful packages to
  `aaggeeaa/respawn-and-rage:windows-x86_64` by default, using the packaged menu
  version; `-NoUpload` retains local-only builds.
- PowerShell 7.6.7 helper regression checks PASS with mocked Butler. Cases cover
  missing credentials, failed read-only auth preflight, explicit identity paths
  with spaces, environment credentials absent from command arguments, selecting
  the staged launcher instead of the nested runtime executable, correct target
  and version, upload success/failure receipts, retained files after failure,
  invalid/mismatched versions and missing cooked containers.
- Main-script integration PASS on macOS with a temporary Windows-host test
  environment and a local fake Butler executable: `-UploadOnly -Check` made no
  push and left the build receipt unchanged; a simulated exit-37 push failure
  returned 1 and saved `upload_failed`; a subsequent retry returned 0, saved
  `uploaded`, removed the stale error and retained the original package. These
  tests used no network, credentials, Unreal installation or compiler.
- `git diff --check` PASS. No actual Butler upload or native Windows build was
  performed; Windows execution and the downloadable game remain unverified.

### Windows-hosted Windows/Linux packaging — 20 October 2026

- `package-windows.ps1` now selects Both (default), Windows or Linux. Preflight
  checks Linux target support plus v26/clang20.1.8 metadata, executable tools,
  libc++ headers/libraries and a C++20 compile/link producing ELF64 x86_64.
  The smoke command and toolchain selection were exercised with mocked tools;
  the real Windows cross-compiler was not run on this Mac.
- PowerShell helper regression suite PASS: native stdout capture, one shared
  Windows Editor command, distinct Win64/Linux UAT arguments, Linux SDK/support
  rejection, wrong compiler/missing library/link failures, correct SDK environment,
  Linux ELF architecture rejection, shell line endings and correct per-platform
  Butler channels/permission flag, in addition to earlier Windows/auth tests.
- Desktop workspace Python fixtures PASS (2 tests): isolated source copies,
  shared Content, original commit/version stamper, Marketplace source-only
  plugins with Installed=false, preserved engine descriptors, and rejection of
  existing/nested workspaces. Windows junction creation still needs native testing.
- Dual-platform main-script upload integration PASS on macOS with a local fake
  Butler: check-only made no pushes or receipt changes; a Windows upload failure
  returned 1 while Linux still uploaded; retry pushed only Windows and cleared
  its stale error; old Windows receipts without platform metadata still worked.
  No network, real credential or real upload was used.
- No native Windows Editor/Game build, Windows-to-Linux cross-build, or target
  game startup is claimed. Both targets require validation on the Windows host
  and runtime testing on their respective operating systems.

### One-command Mac release and Python 3.9 on Windows — 20 October 2026

- Added `release-macos.py`: aggregated dependency/space/credential preflight,
  isolated package, Developer ID signing, notarization, stapling, Gatekeeper and
  Butler upload. Completed packages can retry notarization or upload separately.
  The submission ZIP uses the release workspace disk; logs/receipts remain
  outside the app upload directory. Single-architecture channels are distinct
  from `mac-universal`; uploads use the packaged version.
- Full Python fixture suite PASS: 42 tests on Python 3.14.6, including nine new
  release workflow tests and six notarization tests. Covered build failure,
  rejected/failed notarization, failed ticket validation, no-upload with full
  notarization, upload failure/retry route without rebuilding/signing, version
  mismatch, missing credentials, ambiguous identity selection, check-only with
  no workspace mutation and submission ZIP placement on the build disk.
- Python 3.9.6 PASS: six asset-mount tests, two desktop workspace tests and the
  Linux snapshot archive fixture (both file and stdout streaming). Junction
  detection now falls back to the exact Windows mount-point reparse tag on
  Python 3.9–3.11, retaining directory/conflicting-mount protections. Streaming
  gzip uses a separate writer for compatibility with Python 3.9's tarfile API.
  PowerShell 7.6.7 helper regression suite PASS after lowering the Windows
  minimum to Python 3.9. Native Windows junctions/builds still need Windows testing.
- Real Mac **read-only preflight**: UE 5.8.3, Xcode 27.1/macOS SDK, bundled .NET,
  plugin source, branding, 28 asset mounts, closed Editor and a valid Developer
  ID identity were available. Release correctly stopped before build: Metal
  Toolchain missing, Butler absent from PATH, `RespawnAndRage` notarytool Keychain
  profile missing, and only about 13 GiB free against the 30 GiB universal-release
  threshold. The credential check was repeated outside the restricted sandbox
  to distinguish Keychain access restrictions from the actual missing profile.
- No new Unreal build, signing operation, notarization submission or real itch
  upload was performed. Runtime execution of this release remains unverified.

### Windows preflight corrections from the user's log — 20 October 2026

- Fixed native executable resolution when `Get-Command` returns multiple Python
  installations: invoke only the first PATH match, retaining stderr handling and
  exit-code checks. The Python 3.9 minimum and junction fallback remain supported.
- Replaced the obsolete `UEPrereqSetup_x64.exe` check with both `vc_redist` files
  actually staged by UE 5.8.3 `WinPlatform.Automation.cs`. This packaging check is
  required only when Windows is selected; engine discovery now prints its path
  before validating individual components.
- MSVC discovery now covers VS 2022 and VS 2026, reads the engine's minimum and
  banned compiler versions, and reports incomplete tools/ATL with paths. Valid
  14.44.35211 and 14.50.35723 fixtures pass; banned 14.44.35210/14.50.35722,
  below-minimum compilers and missing ATL fail. Product-version fields mirror
  UBT's compiler validation, with the same folder-version fallback.
- Fixed the plugin null-Path failure: `-match` no longer overwrites a `$matches`
  variable containing plugin descriptors. Marketplace source/version checks pass
  with the actual regex branch exercised, and missing source or a wrong PCGEx
  version produces the intended diagnostic.
- PowerShell 7.6.7 regression suite PASS on macOS, including the above cases and
  existing package/Linux/Butler tests. `git diff --check` PASS. Windows compilation
  and native Windows PowerShell 5.1 still require execution on the user's host.

### Windows missing mounts and native stderr diagnostics — 20 October 2026

- The subsequent user screenshot reports MSVC, Windows redistributables, SDK,
  Linux target support and plugins passing. Remaining diagnostics are the v26
  cross-toolchain lookup and missing Content mounts; this is a preflight stop,
  not a compiler error. Mount-only repair and separate Linux/Windows routes are
  documented without overwriting assets or weakening the compiler check.
- Native stderr objects are normalized to strings before display, capture and
  logging, retaining streaming output and nonzero-exit failure handling. This
  avoids the extra PowerShell ErrorRecord rendering visible around Python's
  missing-mount message. Regression cases cover mixed stdout/stderr with exit 0
  and captured missing-mount stderr with exit 1.
- PowerShell 7.6.7 regression suite PASS on macOS; six asset-mount tests PASS on
  Python 3.9.6. `git diff --check` PASS. No native Windows build or real upload
  performed; PowerShell 5.1 formatting still needs confirmation on Windows.

### Automatic asset junctions in Windows builds — 20 October 2026

- Normal builds now invoke `setup-assets.py` without a flag after tool checks
  pass, creating missing junctions and validating existing ones without fetching
  submodules. `-InitAssets` still opts into pinned submodule/LFS downloads;
  `-Check` and failed tool preflights use only `--check` and make no mounts.
- PowerShell 7.6.7 helper suite PASS: automatic mount invocation, paths with
  spaces, read-only checks, explicit initialization, deferred setup after tool
  failures and propagation of asset-setup failures. Six Python asset-mount tests
  PASS on Python 3.9.6, including idempotence and preservation of conflicting
  data/links. `git diff --check` PASS. Native Windows junction/build execution
  remains to be confirmed on the Windows host.

### Existing Mac release configuration discovery — 20 October 2026

- `release-macos.py` now discovers the installed Butler executable and existing
  login in standard itch locations and the optional sibling builder's `.work`.
  Explicit paths take precedence; no key contents enter script output/receipts.
  Real Butler 15.32.0 read-only status PASS for both current game channels.
- Metal was installed (Xcode component 27A266a) but its mapping/mount was stale.
  Refreshing the installed component made `xcrun --sdk macosx metal -v` work.
  The script now attempts that refresh before reporting a missing compiler;
  it does not download components or disable the compiler check.
- Default notarization now selects a working notarytool profile or the existing
  Xcode account. Only public account-list metadata is inspected; private Xcode
  tokens/passwords are never extracted. Xcode export failures preserve the
  original packaged app; ticket/signature/Gatekeeper/version verification is
  required before replacing it or allowing a Butler upload.
- **Real Apple integration PASS** with a tiny universal release probe: Developer
  ID signing, Xcode upload, processing/poll, notarized export, stapling, strict
  codesign and Gatekeeper (`source=Notarized Developer ID`). Receipt:
  `/private/tmp/rr-xcode-notarization-probe-20261020/RespawnAndRageReleaseProbe-notarization.json`.
  This was an Apple notarization test, not a new game build or an itch upload.
- Full Python fixture suite PASS (51 tests). New cases cover auto-discovery and
  explicit-path precedence, Metal refresh/rejection, Xcode account fallback,
  empty account lists, pending notarization, accepted export and preservation
  after upload/ticket/version failures. `git diff --check` PASS.
- Real default Mac preflight now passes tool/authentication checks; it stops
  solely on disk space (about 19.7 GiB free versus 30 GiB required for universal).
  A SHA-256-verified APFS deduplication of 20 cooked container files in existing
  package copies preserved every app/archive, but recovered no meaningful space
  because their data was already shared. Receipt:
  `Saved/AssetAudit/macos-release-apfs-dedup-20261020.json`. No disk-space guard
  was weakened and no new game build started with insufficient space.


### Windows Editor compiler failures from user log — 20 October 2026

- The supplied `editor.log` contains 43 MSVC errors: 24 C4458 inherited-member
  shadowing diagnostics, 18 C4459 diagnostics caused by the anonymous-namespace
  smoke-grid `Count`, and one C4805 mixed bool/int bitmask diagnostic. The final
  `OtherCompilationError`/exit 6 is the summary; C4996 deprecations are warnings.
- Renamed only local variables/ordinary parameter names and the private smoke
  grid count; preserved reflected properties, asset paths, function types and
  gameplay. Cell layout flags now explicitly use uint32 before bit operations.
  No warning suppression or engine/plugin edits were introduced.
- PASS: all nine affected translation units checked with Apple Clang C++20 and
  the installed UE 5.8.3 Editor compilation response files (`-fsyntax-only`,
  existing warning-as-error flags). Source copies, logs and command receipts are
  in `/private/tmp/rr-windows-compile-validation-20261020`; the existing Mac build
  snapshot and its outputs were read only.
- PASS: the actual cell-flags expression extracted from ERCellWorld.cpp retains
  all 16 expected flag values under clang-cl C++20 `/W4 /WX /Zs`.
- Native MSVC/Win64 compilation, linking, cooking and packaging were not run on
  this Mac. The Windows host must rerun the build with the corrected sources;
  the failed build workspace contains its previous source copy.


## HUD Teko e minimappa locale — 20 ottobre 2026

- `scripts/build-editor.command`: **Succeeded** (Unreal 5.8.3, Mac arm64
  Development Editor). Ultima compilazione 10.20s; log
  `Saved/Testing/hud-minimap-build.log`.
- `python3 scripts/test-recon.py --automation`: **3/3 Success**
  (Flight, FogAndSight, SmokeOcclusion), prima degli ultimi soli ritocchi di
  presentazione. Output `hud-minimap-automation.log` / `recon-automation.log`.
- `python3 scripts/test-recon.py --render`: **PASS** con listen host Metal,
  avversario, alleato e late join; 31 asserzioni host e 48 conferme client
  (22/22/4). Inclusi contatto realmente disegnato a 40m sul bordo, visibilità,
  fumo, ricambio drone e reset. Report `Saved/Testing/recon-metal-report.json`.
  Questa esecuzione precede il ponte UFont e il filtraggio mipmap finali.
- La prima cattura ha rilevato testi Canvas assenti con il solo composite font
  Slate: corretto usando un UFont runtime transiente, conservato dall'HUD, con
  lo stesso Teko SemiBold. Nessun nuovo asset salvato.
- `python3 scripts/test-industrial-map.py --finish-gallery --offscreen`:
  **PASS**, ripetuto dopo la correzione del font e dopo gli ultimi ritocchi
  a pannello punteggio/mipmap. Rendering reale Metal 1280x720 su Central Building
  seed42/crossing3; catture Plan, Container e Ground. Ispezionata visivamente
  `Saved/Testing/Industrial-Ground-rule3-seed42.png`: testi presenti/leggibili,
  pannello punteggio con margini, numero centrale assente, puntini con ombra,
  minimappa circolare morbida e ritaglio del terreno attorno al giocatore.
  Report `Saved/Testing/industrial-game-metal-report.json`, output
  `hud-minimap-industrial-render.log`.
- `git diff --check`: superato. Nessuna modifica al gameplay, ai pacchetti
  salvati o alla politica di risoluzione della scena principale; nessun nuovo
  cook/pacchetto distribuito per questa modifica. Le catture verificano 1280x720,
  non certificano tutte le risoluzioni o stati possibili della scoreboard.


## HUD minimale e margini Retina — 20 ottobre 2026

- `scripts/build-editor.command`: **Succeeded**, ultima compilazione9.52s;
  `Saved/Testing/hud-minimal-build.log`.
- La prova Metal nativa con `-fullscreen -ForceRes -ResX=1920 -ResY=1080`
  ha effettivamente usato3456x2234 (Retina). Il probe industriale è passato,
  ma le catture fullscreen erano nere: non usate come evidenza visiva.
- Ripetuto `test-industrial-map.py --finish-gallery --offscreen` con
  ResX3456/ResY2234/ForceRes: **PASS** dopo gli ultimi cambi. Controllata la
  cattura Ground reale Metal3456x2234: punteggi colorati/target neutro senza
  pannello o nomi squadra, timer, HP/AP e sprint con margine inferiore, nessun
  suggerimento TAB/ESC. Individuato e corretto anche il testo recon che prima
  usciva a destra; ripetuta/ispezionata la cattura con etichette entro il viewport.
- Evidenza: `Saved/Testing/HUDMinimal/Retina-3456x2234.png` e
  `retina-report.json`; log `hud-minimal-retina-render.log`. Le catture del
  probe escludono l'overlay Slate: formato FPS/risoluzione e font11 invece di22
  verificati nel codice compilato, senza attribuire loro una verifica visiva.
- `git diff --check`: superato. Nessun cambiamento alle scorciatoie,
  al calcolo della risoluzione effettiva o alle regole della partita.


### Dimensione testo arma/munizioni — 20 ottobre 2026

Scala1.4→1.1 (circa21%) e ancoraggio riadattato alla fascia HP/AP.
`scripts/build-editor.command`: **Succeeded**,11.80s; log
`Saved/Testing/hud-ammo-size-build.log`. `git diff --check`: superato.
Modifica solo tipografica; nessuna nuova cattura o prova gameplay per questo ritocco.


### Ancoraggio inferiore e allineamento a destra — 20 ottobre 2026

- `scripts/build-editor.command`: **Succeeded**,28.02s; log
  `Saved/Testing/hud-edge-alignment-build.log`.
- Rendering Metal industriale `--finish-gallery --offscreen` a3456x2234:
  **PASS**; cattura Ground ispezionata in
  `Saved/Testing/HUDEdgeAlignment/Retina-3456x2234.png`, report adiacente.
- Misurati i pixel delle lettere rispetto all'area visibile, escluse le bande
  nere: margine sinistro HP84px, destro del riepilogo arma83px. HP termina121px
  sopra il margine inferiore visibile, contro80px nella cattura precedente:
  riga alzata di41px. Testo arma a108px dal bordo inferiore, con larghezza
  misurata dal font; sprint impilato sopra HP/AP. Nessun testo tagliato.
- `git diff --check`: superato. Nessuna modifica a dimensioni font, munizioni,
  comandi o gameplay. La verifica visiva usa la stessa risoluzione del
  fullscreen Retina tramite rendering senza finestra.


### Contatore prestazioni senza sfondo — 20 ottobre 2026

SBorder sostituito da SBox senza sfondo, mantenendo testo/ombra/padding.
`scripts/build-editor.command`: **Succeeded**,11.85s; log
`Saved/Testing/fps-no-background-build.log`. `git diff --check`: superato.
Nessuna nuova prova grafica per questa modifica del solo contenitore Slate.


### Versione menu entro il viewport — 20 ottobre 2026

- `scripts/build-editor.command`: **Succeeded**,11.82s;
  `Saved/Testing/menu-version-inset-build.log`.
- Avvio di L_MainMenu con `-ERMenuGallery -fullscreen -RenderOffscreen
  -ForceRes -ResX=3456 -ResY=2234` e UserDir isolata: catture Slate reali
  di home e Credits a3456x2234, ispezionate entrambe. Versione completa
  visibile con margine inferiore e destro; font/dimensione conservati.
  Catture in `Saved/Testing/MenuVersionInset/{home,credits}.png`, report e log
  nella stessa cartella. La galleria è stata fermata dopo queste catture,
  senza attendere il passaggio automatico al server LAN.
- Le catture includono anche l'overlay prestazioni: FPS/dimensioni piccoli
  e senza pannello di sfondo, coerenti con le modifiche precedenti.
- `git diff --check`: superato. Nessun asset o stato di gioco modificato.


### Recupero notarizzazione Mac — 20 ottobre 2026

- La release `release-20261020-211424-c4061bf9` ha completato il package
  universal (x86_64/arm64), versione `26.10.20.85cfceab`, e la verifica della
  firma Developer ID. Il primo invio Xcode è fallito con `No Accounts` e
  `missing Xcode-Token`; non era un errore del compilatore Unreal.
- Il preflight ora distingue la configurazione dell'account Xcode dalla sua
  autenticazione, verificabile durante l'upload. Gli errori di autenticazione
  durante upload/export indicano come rinnovare l'accesso e usare `--resume`;
  il receipt di notarizzazione conserva anche la diagnostica dell'errore.
- Suite offline Mac: **28 test PASS** con Python 3.14.6. Casi aggiunti:
  account configurato senza promessa di autenticazione, token mancante durante
  upload/export senza polling o verifiche successive, errore upload generico,
  errore persistito nel receipt e recupero con `--resume --no-upload` senza
  ricompilazione. Le fixture non inviano dati ad Apple o itch.io.
- Recupero reale **PASS** con `release-macos.py --resume` sulla stessa directory
  e `--no-upload`: Xcode ha autenticato e completato l'invio, esportato l'app
  notarizzata, preservato la versione e superato firma strict, stapling/validate
  e Gatekeeper (`accepted`, `source=Notarized Developer ID`). Receipt finale
  `builds/macos/release-20261020-211424-c4061bf9/release.json`: `ready`.
  Nessuna ricompilazione o pubblicazione itch.io in questo tentativo.
- `git diff --check`: **PASS**.

### Prestazioni e crash della build itch — 21 ottobre 2026

- Esaminati i due report macOS delle08:07 e08:19: stesso stack
  `UAnimSequence::EvaluateCurveData_Lockless`, esecuzione nativa arm64.
  Riprodotto nell'app scaricata26.10.20.85cfceab con Depot e3 bot per team:
  exit **-11**, assertion `!bForceUseRawData || CanEvaluateRawAnimationData()`
  in `AnimSequence.cpp:3249`. Log `Saved/Testing/packaged-pistol-crash.log`.
  La causa è `SetDisableRetargeting` nel percorso della pistola: la build
  cooked non contiene i dati raw richiesti da quel flag di debug.
- Confermati gruppi salvati Epic3 nell'app distribuita, contro
  AA2/GI0/riflessioni0/ombre1 nel vecchio launcher. Prima misura Metal offscreen
  sulla stessa CentralBuilding seed42, viewport5120x2880: tratti stabili
  circa48–49FPS a768x432 con Epic,63–67FPS a1267x713 con il profilo del launcher.
  Non è una misura isolata finale: compilazione concorrente e hitch successivi
  sono presenti nei log `packaged-{epic,play-profile}.log`.
- Modulo Editor aggiornato: **Succeeded**,37.14s;
  `Saved/Testing/cooked-fix-editor-build.log`.
- Migrazione Unreal di51 animazioni e due sole modalità di traslazione
  (`pelvis`, `upperarm_r`): confronto di ogni frame **PASS**, massimo scarto
  delle posizioni locali0.00001921cm. Backup ZIP privato verificato prima
  delle modifiche; prima verifica compressa su20 skin **PASS**, scarto massimo
  delle mani0.01321cm. Log `bake-cooked-first-person.log`.
- Test nativi `FallDamage`, `ResolutionHysteresis`, `ReferenceBallistics`:
  **3/3 Success** in `cooked-fix-automation.log`. I messaggi `Condition failed`
  precedenti all'avvio della coda provengono dai self-test del motore.
- `test-reference-weapons.py`: **PASS**, due processi reali, tutti e10 gli
  slot osservati sul peer remoto, ricarica/colpi/armatura verificati.
  `Saved/Testing/cooked-fix-reference-test.log`.
- L'audit storico degli asset non è globalmente verde: confronto con i package
  originali ripristinati in una fixture isolata conferma scarti preesistenti
  nelle ricariche SMG/Carbine/Sniper/Launcher e attese obsolete su ArmorRatio
  HE/Launcher. Il confronto delle pose effettive prima/dopo resta distinto
  dalle attese source/raw di quell'audit. Log `cooked-fix-{weapon-assets,original-runtime-audit}.log`.
- Compressione precisa delle tre clip della pistola con il profilo privato
  `BC_ER_PistolPrecise`: quaternion completo, massimo errore delle posizioni
  delle mani0.00002418cm sulle20 skin; osso di orientamento della canna entro
  0.001gradi. Il massimo0.004802gradi nell'intero campione comprende le dita,
  non altera il controllo stretto della canna. Stessa verifica **PASS** nella
  copia isolata effettivamente cotta; `cooked-fix-snapshot-verification.log`.
- Regressione Metal finale **PASS**, due peer con renderer reale: dieci slot,
  sette ricariche e cattura aggiuntiva Pistol_Alpha; shader e tutti i controlli
  gameplay/impugnatura **PASS**. Screenshot della pistola ispezionato.
  Il tentativo intermedio con peer nullo non produceva le catture richieste;
  la prova finale usa Metal su entrambi. Log `reference-weapons-render-{host,client}.log`.
- Package universal locale26.10.21.85cfceab: compilazione Editor833.52s,
  gioco1014.67s, cook/stage/archive **BUILD SUCCESSFUL**,117.29s.
  `lipo`: x86_64/arm64. Smoke del pacchetto **PASS**, giocatore esplicitamente
  con pistola (`Primary=1`) e sei bot,20s, nessuna assertion, exit0. La prima
  versione del test non garantiva la pistola fra i loadout casuali dei bot;
  ripetuta sullo stesso pacchetto scegliendola esplicitamente per il giocatore.
  Receipt in `builds/macos/fix-runtime-20261021/{receipt.json,runtime-smoke/null.json}`.
- Prova Metal/LAN anche con due istanze del pacchetto: probe gameplay completo
  **PASS**, tutti i10 slot remoti e inquadratura/canna della pistola corretti.
  Le catture Slate non sono prodotte dal Mac packaged in RenderOffscreen;
  non conteggiate come riuscite in questa prova. Log `reference-weapons-packaged-render-{host,client}.log`.
- Confronto finale isolato, compilatore fermo e una sola app alla volta:
  CentralBuilding seed42, Metal offscreen,45s per app, ultimi20 campioni a1Hz.
  Viewport effettivo identico2880x2160 (macOS ha limitato la richiesta5120x2880).
  App itch originale: mediana **47FPS**,46..48, scena576x432.
  App corretta: mediana **65FPS**,59..66, scena1190x892. Entrambe exit0.
  Ricevuta e log in `builds/macos/fix-runtime-20261021/performance/`.
  Il dato riguarda questa scena e questa macchina; non certifica altri seed,
  combattimenti o l'esecuzione Intel. Nessun upload itch in questa correzione.

### Pubblicazione Mac e pulizia locale — 21 ottobre 2026

- Versione universal `26.10.21.85cfceab` firmata Developer ID Application
  Dr Wolf srl (`8V893A78SP`), notarizzata da Apple e verificata con firma
  strict, ticket stapled e Gatekeeper: `accepted`, `Notarized Developer ID`.
- Invio Apple `6000ddd9-2869-40e8-9b12-9422cf07b837`: upload completato;
  un timeout HTTP -1001 nella lettura dello stato è stato recuperato con
  `-exportNotarizedApp` sullo stesso archivio, senza nuovo invio o build.
  Hash dei dati cooked e versione preservati nell'esportazione.
- Smoke Metal dell'app notarizzata: **PASS**, pistola e sei bot,20s,
  nessun fatal, exit0, mediana64FPS nella scena di prova Depot.
- Upload itch `mac-universal` completato: nuova build **2175732**, upload
  **19859089**. All'ultimo controllo il server stava ancora elaborando la
  nuova build; la verifica della copia scaricata è stata lasciata all'utente
  su sua richiesta, interrompendo attesa/download automatici.
- Su richiesta rimossi tutti e quattro i run locali in `builds/macos`,
  inclusi snapshot, intermedi, archivi Xcode, app finali e log di quei run.
  Cartella vuota; recuperati circa **39.14GiB fisici** (cloni APFS).

### Metal SM5 compatibility release — 21 October 2026

- Universal Development build `26.10.21.0f726326`, source commit
  `0f7263261d65dadbb5122571cb213195e953933b`, UE 5.8.3: compilation and
  packaging PASS; 2286 content packages cooked. Mac targets now include both
  Metal SM6 and SM5, with Global and game shader libraries generated for each.
- Packaged Depot checks before notarization: NullRHI, forced SM5 and forced
  SM6 all PASS, each with the pistol and six bots, clean exit and no fatal.
  Rendered checks ran on Apple M2 Pro / macOS 27.0.1. Logs explicitly confirm
  `shaderplatform="METAL_SM5"` and `shaderplatform="METAL_SM6"` respectively.
  M1 and Intel hardware were not tested; M1 verification is deferred.
- Developer ID Application Dr Wolf srl (`8V893A78SP`), Xcode notarized export,
  ticket stapling/validation, strict signature and Gatekeeper checks all PASS:
  `accepted-and-gatekeeper-verified`, `Notarized Developer ID`.
- itch.io `mac-universal`: upload complete, build **2176283**, upload
  **19859089**. Butler confirmed server processing complete and the current
  downloadable version `26.10.21.0f726326`.
- Upload retry initially counted the diagnostic `.xcarchive` app as a second
  delivery app. Retained that archive under the release's `diagnostics/` and
  uploaded the unchanged notarized app. The helper now excludes diagnostic
  Xcode archives while still rejecting multiple actual delivery apps; release
  workflow tests **15/15 PASS**, notarization workflow tests **14/14 PASS**.
- App, build/notarization/upload receipts and logs retained in
  `/private/tmp/rr-sm5-releases/release-20261021-111304-93c27528/`.


## Barra stamina e sprint 10/20 secondi — 21 ottobre 2026

- Editor Mac arm64 Development: **PASS**, build finale 28,93 s.
  Warning di deprecazione preesistenti; nessun errore di compilazione.
- `python3 scripts/test-movement.py --render`: **PASS**, 79 controlli server,
  quattro controlli owner/proxy e quattro catture Metal aggiornate.
  Verificati corsa, cammino, salto, ostacoli 10–200 cm, limiti di scavalcamento,
  clearance, crouch/prone, camera/occhio server e input reali W/C/Ctrl.
  Sprint remoto a 18 km/h esaurito dopo 10 s; ricarica in 20 s senza riavvio
  automatico tenendo W. Interruzioni con recupero doppio del consumo effettivo.
  Verificati consumo del 40% dopo 4 s, breve recupero parziale, nuova partenza
  senza reset a 10 s e ricarica completa dopo una seconda interruzione.
- `python3 scripts/test-network.py --accuracy`: **PASS**, compresi stato owner,
  dispersione reale e recupero bloom. Cono owner corsa .730°, sprint 2.230°.
- `python3 scripts/test-network.py`: **PASS**, combattimento, protezione,
  morte/respawn, ragdoll e replica dei personaggi conservati.
- Ispezionate le catture Metal 1280×720: barra ambra durante sprint,
  azzurra con riempimento parziale durante riposo; etichetta, HP/AP e margini
  inferiori leggibili. Nessuna verifica grafica di altre risoluzioni in questa prova.

Log finali: `Saved/Testing/stamina-build.log`, `stamina-movement-final.log`,
`stamina-accuracy-final.log`, `stamina-network-final.log`, con log host/client
nelle consuete posizioni. Catture: `Saved/Testing/Stamina/Phase44.png`–`Phase47.png`.
Nessun cook, packaging, pubblicazione o modifica di asset binari.


## Sprint con E tenuto premuto — 21 ottobre 2026

- Editor Mac arm64 Development: **PASS**; build iniziale 31,44 s,
  ricompilazione della sola fixture 10,17 s.
- `python3 scripts/test-movement.py --posture-only`: **PASS** su host e client
  reali. Verificati E premuto durante W, rilascio E mantenendo W, E prima
  della ripartenza con W oltre la finestra del doppio tocco, arresto/ripresa
  con E ancora premuto, rilascio E durante doppio W e rilascio di entrambi.
  Tre riscontri owner confermano lo sprint predetto; il server valida tutti
  gli input. Restano superati stamina 10/20 s, consumo/ricarica parziale,
  interruzioni, doppio W, crouch/prone e relativa replica.
- Prima esecuzione: nuovi casi E falliti perché la fixture teletrasportava il
  client ancora prono alla quota da personaggio in piedi, facendo ignorare C
  durante la caduta. Corretto solo il test usando la mezza altezza effettiva
  della capsula e aspettando il contatto col suolo prima di rialzarsi.
- `python3 -m py_compile scripts/test-movement.py` e `git diff --check`: **PASS**.
  Prova headless: nessuna nuova verifica grafica del testo dei controlli.

Log: `Saved/Testing/sprint-e-build.log`, `sprint-e-movement.log`,
`movement-host.log`, `movement-client.log`; primo tentativo conservato in
`sprint-e-movement-initial.log`. Nessun asset binario o packaging modificato.


## Gameplay notifications and killer camera — 21 October 2026

- UE5.8.3 Mac Development Editor target: final compile **PASS**. New Json module
  dependency and explicit NonUFS catalog entry appear in the build receipt.
- Owner-supplied catalog retained byte for byte: **72 events / 720 variants**,
  SHA-256 `b1582b77ee393805601ac62aaf4f06f43cd299405bdc625d458adf3ae14ab004`.
  Native dispatch connects40 events; the32 deferred events remain intact.
- `python3 scripts/test-gameplay-messages.py`: **PASS**,
  `EclipseRecoil.Messages.CatalogAndDelivery`. Checks complete event count,
  shuffle uniqueness/boundary, missing context, independent audience variants,
  bonus priorities, mandatory confirmations, final-kill preservation, actual
  lead changes through ties, and projectile-controller retention after destroying
  the originating pawn. The transient unit world explicitly invokes the normal
  PostInitializeComponents hook; actual owner RPC delivery is tested below.
- `python3 scripts/test-network.py --render`: **PASS**, actual listen server and
  remote client rendered on Metal at1280x720. Both private kill confirmations,
  remote teamkill confirmation, actual killer reference/aim and normal camera
  after respawn passed. Existing damage, scores, corpse, protection, reload,
  ammo, manual respawn and round-result/restart checks also passed.
  Captures inspected: `Saved/Testing/Messages-KillHUD.png` and
  `Saved/Testing/Messages-DeathCamera.png`. New text is peripheral, readable and
  clear of the reticle; the raised camera keeps the corpse out of its lens.
- `python3 scripts/test-grenades.py --combat`: **PASS**, gameplay probe and all
  remote held/release/material checks. No changes to damage/fuse/cover tuning.

The first graphical death-camera capture exposed the corpse blocking the lens;
corrected with a raised/rear position and a12cm static-only sphere sweep. Probe
readiness now waits for Metal startup and for death properties/confirmation to
arrive on their separate actor channels; respawn validation runs on the new pawn,
not on an earlier phase notification. Final executed checks above pass.

This verifies local Editor gameplay, not packaged platforms or performance.
Not every phrase or every supported recognition was independently triggered in
the graphical probe; unsupported correlations/modes remain deferred as listed
in `GAMEPLAY_MESSAGES.md`. No packaging, publication or asset-binary edits.

- `python3 scripts/test-network.py --accuracy`: **PASS**, actual listen/client
  server spread/burst/ammo rejection and owner data/ammo-bloom/server-clock
  recovery checks. Log: `Saved/Testing/accuracy-{host,client}.log`.


## Close killer-camera framing — 21 October 2026

- Mac UE5.8.3 Development Editor: **PASS**. Smooth local approach plus90→60deg
  zoom, capsule-centred framing, per-step12cm static sweeps and lower respawn HUD.
- Actual Metal1280x720 camera check: **PASS**, projected capsule coverage0.65,
  FOV60.0deg, camera distance440.0cm. Capture inspected: head and feet visible,
  killer centred, respawn text below the figure.
- Final `python3 scripts/test-network.py --render --render-size 1920 810`:
  **PASS**, complete host/client regression. The close-view check reports0.65
  coverage,60.0deg and585.8cm, with full figure visible in the wide capture.
  Private notifications, teamkill confirmation, respawn reset and the existing
  damage/ammo/protection/corpse/scoring/round checks also passed.
- The initial full graphical run's camera checks passed but its friendly burst
  raced the autonomous pawn's test teleport. The probe now waits700ms for that
  correction before firing; the final complete run passes. Production combat
  and teleport rules are unchanged.
- `git diff --check` and Python compilation of the updated test runner: **PASS**.

Logs: `/tmp/rr-killcam-build.log`, `/tmp/rr-killcam-render.txt`,
`/tmp/rr-killcam-wide.txt`, `Saved/Testing/network-{host,client}.log`.
The wide capture includes the local menu overlay beside the figure; framing
was also inspected without it in the1280x720 capture. No cooked build,
publication or binary asset changes were performed for this refinement.


## Gameplay message pre-push allowlist — 21 October 2026

The original public-history check rejected only the owner-supplied message JSON
and its README in commit a1898cce. Added those two exact Config paths to the
text allowlist, preserving binary/LFS/symlink and all other path restrictions.

- `python3 -m unittest discover -s scripts/tests -p test_public_tree.py`:
  **PASS**,14 tests, including the real catalog/README and negative cases for
  binary/LFS/symlink replacements and unapproved neighbouring Config paths.
- `python3 scripts/verify-public-tree.py`: **PASS**,47 commits /14221 approved
  entries; private assets remain gitlinks only. Output retained in
  `/tmp/rr-public-tree-messages.log`.
- Python compilation and `git diff --check`: **PASS**.

No history rewriting, commit or push performed by this fix.


## Random bot firearms and headshot diagnosis — 21 October 2026

- Mac UE5.8.3 Development Editor build: **PASS**.
- `python3 scripts/test-bot-loadouts.py`: **PASS**, actual listen/client peers
  for Target, Movement and Patrol. Six bots per mode, each observed at initial
  spawn, after a real death/manual respawn delay, and after round reset:54
  server selections and matching client observations. Every group uses six
  distinct Bullet slots1..6 (mask126), fresh magazine capacity and new pawn
  identities. The dedicated fixture pauses AI decisions to isolate loadouts.
- `python3 scripts/test-target-skins.py`: **PASS**, all20 team skin variants,
  firearm-only mask126, stationary behavior and support-hand contacts, remotely
  replicated. The updated request supersedes the earlier all-ten-weapon gallery.
- `python3 scripts/test-reference-weapons.py`: **PASS**, current saved data
  explicitly reports anatomical=1 only for slot3 Shotgun; all other slots0.
  Existing six physical anatomical pellet hits, body-zone multipliers,
  helmet/armor depletion and cover checks pass. A separate authoritative lethal
  head point-damage event also verifies a nonempty private Headshot catalog
  toast for the killer after higher-priority recognition has expired.
  This confirms message dispatch; it does not enable head zones on other guns.
- `python3 scripts/test-bots.py`: **PASS**, both legacy Patrol and Target modes,
  including actual firing, reload, damage, death, protection, respawn, match
  reset and remote team/model/health replication.
- `python3 scripts/test-recon.py`: **PASS**, actual owner/teammate/enemy/late-join
  visibility, stock/replacement, packet delay/loss fixture and round reset after
  reserving bot corridors during case placement.
- Python compilation and `git diff --check`: **PASS**.

The first legacy route-clearance test found BP_ReconBase_C_0/C_1 on authored
Depot segments0/5. Diagnostics identified actual case collisions, not weapon
selection. Runtime case placement now keeps120cm clear of vertically intersecting
patrol segments. The unchanged standing-capsule sweep and complete legacy probe
then pass. Maps, vendor assets, collision rules and weapon balance were preserved.

Logs: `/tmp/rr-bot-firearms-build.log`, `/tmp/rr-bot-firearms-test.log`,
`/tmp/rr-body-zones-test.log`, `/tmp/rr-bot-firearms-skins.log`,
`/tmp/rr-bot-firearms-legacy.log`, `/tmp/rr-bot-firearms-recon.log` and
`Saved/Testing/bot-loadouts-{target,movement,patrol}-{host,client}.log`.
Headless checks verify gameplay/replication; no new graphical or packaged-build
claim, binary asset edits, commit or publication for this increment.

## All-firearm hit zones and cover HP absorption — 21 October 2026

Latest request supersedes the shotgun-only scope above. Source: the supplied
CS2 XLSX, March18,2026 update; complete private archive matches the original
byte for byte, SHA-256
`77389fd80fe6ffc55b62345c20dfaa1e99d774e631973fe4e8f848e31fdb7fde`.

- Read-only extraction: **PASS**, six Raw Values profiles, explicit Analysis
  stomach/leg formulas and24 Damage @Range cached results at2.54/12.7/25.4/76.2m.
  Deagle head3.9; other five4. Source cell references retained in public JSON.
- RespawnAndRageEditor Mac Development, UE5.8.3: **PASS**, final build43.80s.
  Existing unrelated deprecated API warnings remain.
- Unreal Python migration: **PASS**, saved only six owned firearm data assets
  with anatomical tracing and workbook damage/armor/range/head multipliers.
  Original presentation assets and per-weapon scopes/accuracy retained.
- `EclipseRecoil.Weapons.ReferenceBallistics`: **PASS**, range/armor/fractional
  math, Deagle3.9, grenade armor and deterministic bounded shotgun pattern.
- `python3 scripts/test-cell-destruction.py`: **PASS**, all four native cell
  suites, real Chaos ray/collision and terminal damage. A physical target takes
  exactly40 from100 through60HP wood,85 when the wood was reduced to15HP,
  nothing after exact exhaustion or a surviving cell. Two20HP drywall boards
  share one budget:60 leaves20;36 leaves the rear board at4HP. Brick100 absorbs
  all36 in its first layer. Existing blast/calibration/network storage checks pass.
- `python3 scripts/test-reference-weapons.py`: **PASS**, two real Depot peers,
  ten remote weapon slots;60 zone/armor combinations at12.7m plus six head hits
  without helmets. Real AK anatomical hit uses the source range/armor formula;
  its single ray matches the sampled direction. Six real shotgun pellets consume
  one shell. Real AK headshot kills a100HP/100AP helmeted target and delivers the
  private Headshot message. Ordinary confirmation remains on the shared path.
- `python3 scripts/test-network.py --accuracy`: **PASS**, actual server-sampled
  single-bullet cones, posture/sprint/burst recovery and remote owner replication.
- `EclipseRecoil.Destruction.CentralBuildingEvolution`: **PASS**, reinforced
  steel stops the bullet, damages locally and preserves support/breach behavior.
- `python3 scripts/test-scopes.py`: **PASS**, all26 remote scope states,
  zoom/FOV, unscope behavior and physical launcher trajectories.
- Python compilation, source/archive hash equality, public/private diff checks:
  **PASS**. No commit, push, cook or packaged/graphical validation in this change.

Initial physical tests were corrected without weakening assertions: the wood
fixture needed the room's world transform (it previously shot the original40HP
room); AK now aims at an actual torso bone; the six-pellet contact fixture is80cm
away so all six bounded rays physically intersect the body. The older
`test-cell-destruction.py --game` cannot run because L_DestructionLab is absent
from this checkout. This is an unavailable historical fixture, not a passing
test. Current validation uses native physical cell tests and the real Depot LAN
weapon/accuracy/scope probes. No retired map was recreated.

Logs: `/tmp/rr-zone-cover-build.log`, `/tmp/rr-weapon-zones-assets.log`,
`/tmp/rr-zone-cover-ballistics.log`, `/tmp/rr-zone-cover-cells.log`,
`/tmp/rr-zone-cover-reference.log`, `/tmp/rr-zone-cover-accuracy.log`,
`/tmp/rr-zone-cover-central.log`, `/tmp/rr-zone-cover-scopes.log`.
Historical unavailable game fixture: `/tmp/rr-zone-cover-game.log` and
`Saved/Testing/cells-game.log`. Detailed successful peer logs remain in
`Saved/Testing/reference-weapons-{host,client}.log`, `accuracy-{host,client}.log`
and `scopes-{host,client}.log`; cell automation in `cells-automation.log`.

## Worksheet dispersion and inaccuracy — 21 October 2026

User request: align all six firearm accuracy profiles with the supplied CS2
workbook, including motion, posture, firing, recovery and sights. This supersedes
the prior custom scope/rosette/sprint accuracy expectations. Full source archive
and SHA-256 are unchanged. Numeric extraction now preserves hip/Alt fields,
source cells, source firing intervals and independently cached comparison values.
Implementation and the worksheet's unspecified intermediate-motion/prone
adaptations are documented in WEAPON_ACCURACY.md.

- RespawnAndRageEditor Mac Development UE5.8.3: **PASS**, final build10.54s.
- Unreal Python numeric migration: **PASS**, all15 hip and15 scoped properties
  checked on each of six existing owned data assets, plus source firing cycles.
  No meshes, animations, physics assets or sight meshes regenerated. Backups in
  `/tmp/rr-weapon-accuracy-backup` preserve the preceding zone-damage increment.
- `Automation RunTests EclipseRecoil.Weapons`: **PASS**, ReferenceBallistics and
  new WorksheetAccuracy. Saved assets match every extracted coefficient; computed
  results match108 cached standing/crouched burst samples,36 tapping samples,
  36 Simple posture/movement/ladder/apex/landing results, and six Analysis full
  angles. Optical Analysis angles explicitly reference the scoped profile.
  4096 deterministic, varying spread/inaccuracy samples stay inside the HUD's
  enclosing half-angle. Existing damage/armor/range/grenade math remains valid.
- `python3 scripts/test-network.py --accuracy`: **PASS**, two real Depot peers.
  144 runtime combinations of six firearms, available optical modes, standing/
  crouch/prone and actual speeds0/138.889/277.778/500cm/s plus source maximum and
  beyond maximum. Physical crouch/walk/run/jump/landing, actual sampled shots,
  remote RPC burst, owner snapshots and server-clock recovery pass. Crouch changes
  the recovery slope continuously; standing cannot resurrect recovered error.
  Empty shots add no firing impulse.
- `python3 scripts/test-scopes.py`: **PASS**, all26 owner input/replicated states,
  source hip and nonzero scoped minima, source-scaled crouch/prone motion, actual
  scoped shot, unscope/reload/holster behavior and existing launcher trajectories.
- `python3 scripts/test-reference-weapons.py`: **PASS**, all ten remote slots,
  source damage/armor/range/zone checks, actual AK and six-pellet shotgun hits,
  one shell per trigger, real armored headshot and private Headshot notification.
- `python3 scripts/test-movement.py --posture-only`: **PASS**, real host/client
  sprint/posture/stamina and InputKey binding cases, including wider actual
  sprint shots and the same cone on the owner HUD.
- `python3 scripts/test-player-mechanics.py`: **PASS**, actual ladder equipment,
  stationary/moving pistol accuracy and shots, predicted climb/stow/roof behavior
  and replicated fall damage under60ms packet lag plus15ms variance.
- `python3 scripts/test-grenades.py --combat`: **PASS**, gameplay and remote
  hand/release checks. Its intentional zero-spread detonation fixture explicitly
  disables worksheet accuracy on its transient data clone; production firearms
  retain their worksheet mode.
- Python compilation, source archive hash and public/private diff checks:
  **PASS**. No commit/push, cook, packaged build or new graphical validation.

Initial assertions were corrected to compare the right source/physical state:
Analysis AO uses optical Alt standing accuracy for SCAR/AWP, while Simple uses
hip values. Grounded accuracy calibration now explicitly starts MOVE_Walking
instead of inadvertently measuring a spawn fall. The scope test retains and
calculates any real landing residual rather than assuming it is already zero.
Source values or tolerances were not changed to mask those differences.

Logs: `/tmp/rr-worksheet-accuracy-build.log`, `/tmp/rr-worksheet-accuracy-assets.log`,
`/tmp/rr-worksheet-accuracy-native.log`, `/tmp/rr-worksheet-accuracy-lan.log`,
`/tmp/rr-worksheet-accuracy-scopes.log`, `/tmp/rr-worksheet-accuracy-weapons.log`,
`/tmp/rr-worksheet-accuracy-movement.log`, `/tmp/rr-worksheet-accuracy-traversal.log`,
`/tmp/rr-worksheet-accuracy-grenades.log`. Detailed per-peer logs remain in
Saved/Testing under accuracy, scopes, reference-weapons, movement,
player-mechanics and grenade-combat.

## Worksheet recoil — 21 October 2026

- UE5.8.3 Editor Development build: **PASS** (37.85s full incremental compile;
  latest fixture build10.65s).
- Narrow `setup_weapon_recoil.py` migration: **PASS**, six READY markers,
  exit0. Only the six owned firearm data assets were saved.
- `EclipseRecoil.Weapons`: **PASS** — ReferenceBallistics, WorksheetAccuracy,
  WorksheetRecoil. All36 hip/Alt raw recoil values, pattern class and stable
  project seeds match the extraction;6144 bounded samples distinguish fixed
  and Random behavior; analytic response agrees at30/60/144Hz and recovers.
- `test-network.py --recoil`: **PASS**,134 server/owner assertions across ten
  hip/optical cases. Actual shot impulses, one per shotgun shell; first/next
  bullet timing and server aim; remaining velocity accumulation; cooldown,
  empty and reload rejection; shared residual on holster; mouse compensation
  and vertical limit; remote RPC burst, owner camera/input and clock recovery.
- `test-scopes.py`: **PASS**, all26 owner states and authority scope checks,
  including real AWP scoped-shot/automatic-unscope behavior.
- `test-reference-weapons.py`: **PASS**, all ten remote slots, anatomical
  AK/body/range/armor checks, six real shotgun pellet hits, physical head-centre
  trace with source .4016° stationary cone and lethal/private headshot message.
- `test-network.py --accuracy`: **PASS**,144 runtime posture/speed/profile
  cases, real sampled shot rays, physical jump/landing, stance-continuous
  recovery, remote burst and all three owner replication/clock markers.
- Python compilation, original XLSX SHA256, public/private diff checks and
  current working text paths against the public asset policy: **PASS**. The
  original XLSX remains private; no commit/push, cook or packaged/Metal run.

The workbook contains no spray seeds, per-shot coordinates or recoil decay
settings. Exact extracted parameters are used with the explicitly documented
project impulse/return model; this is not a claim of identical CS2 spray paths.
See WEAPON_RECOIL.md.

The initial LAN fixture exhausted AWP's five-round magazine across its three
mode cases; each case now starts with its own normal magazine. No production
ammo rule changed. The final headshot fixture also exposes its physical head
centre and requires production stationary accuracy: teleporting a suspended
shooter had restored falling and correctly selected the AK's approximately14°
airborne cone. The fixture now fixes movement state after teleport and aims at
the real head collision centre with mouse-style recoil compensation; no
production cone, anatomy, damage or headshot/message condition was relaxed.

Logs: `/tmp/rr-worksheet-recoil-build.log`, `/tmp/rr-worksheet-recoil-assets.log`,
`/tmp/rr-worksheet-recoil-native.log`, `/tmp/rr-worksheet-recoil-lan.log`,
`/tmp/rr-worksheet-recoil-scopes.log`, `/tmp/rr-worksheet-recoil-weapons.log`,
`/tmp/rr-worksheet-recoil-accuracy.log`. Per-peer logs: Saved/Testing/recoil,
scopes, reference-weapons and accuracy.


## 21 October 2026 — EOS Internet source integration and LAN compatibility

- UE 5.8.3 Mac arm64 Editor module: **PASS**, including new EROnline subsystem,
  EOS-derived driver, plugin dependencies and menu changes.
- `scripts/test-online-unconfigured.py`: **PASS**, five real-runtime checks:
  online host/search return actionable missing-config status without travel,
  cancel restores idle, and subsequent listen hosting uses the EOS-derived
  driver's IP passthrough with no EOS dependency.
- `scripts/test-lobby.py`: **PASS**, real host/two remotes. UDP game/discovery
  sockets bind to any-address; both clients discover/join, one READY of three
  does not start, two READY start Depot, and teams/skins/equipment/supplies plus
  the explicit test bot survive seamless travel.
- `scripts/test-network.py`: **PASS**, direct-IP gameplay regression using the
  new driver: hitscan/RPC damage, protection, ammo/reload, death/respawn, score,
  teamkill, round restart, remote messages, cosmetics/corpses and owner feedback.
- `scripts/test-lobby.py --gallery`: **PASS**, real Metal captures. Internet/LAN
  selector and search page inspected; host page refined to remove obsolete
  coming-later placeholders and collapse empty status text.
- Portal read-only checks: existing untrusted Peer2Peer policy includes Sessions;
  EAS application is Published, Permissions Configured, Linked clients Configured
  and the RespawnAndRage client is selected. No external settings were changed.
- **Not verified:** populated EOS configuration, SDK login/create/find/join,
  service error/cancellation paths, P2P NAT/relay traversal and two-computer WAN
  gameplay. Automatic approval review rejected saving copied credentials to a
  document with an unspecified persistent destination. No client secret was
  saved; approval is needed for the exact ignored Config/EOS.local.ini path.
  No new package or distribution was produced.

Logs: `/private/tmp/rr-eos-build.log`, `/private/tmp/rr-eos-unconfigured.log`,
`/private/tmp/rr-eos-lobby.log`, `/private/tmp/rr-eos-network.log`,
`/private/tmp/rr-eos-gallery.log`; per-peer runtime logs under Saved/Testing.
Menu images: Saved/Testing/Lobby/user-gallery/Saved/Screenshots/Menu.

## 21 October 2026 — authorized real EOS testing on one Mac

The user explicitly approved the exact private EOS.local.ini path and asked to
proceed with EOS testing, then selected testing on this Mac for now. The
credential is saved with mode0600 and Git ignores it. A content scan of all
Git-eligible tracked/untracked files found no copy of the secret. Temporary
non-secret TextEdit content was discarded, and clipboard content was replaced
with the non-secret client ID. No external client/policy credentials were reset.

The first real-service run passed login, session creation/search, P2P listen,
READY travel, cancellation and cleanup but exited with SIGBUS in OSS module
shutdown after EOSShared had already released its platform. The test runner
correctly rejected this run despite the probe's PASS marker. EROnline now
releases its owned world-specific EOS instance during GameInstance subsystem
teardown, after net drivers shut down and while EOSShared is still loaded.
The test subsystem declares its dependency and drops interface references first.

The subsequent real-service run **passed with process exit0**: SDK Epic/Connect
login (including persistent login on repeat), public session capacity2,
registration of the human host, cloud discovery with game/version filters,
resolution to an EOS P2P endpoint, actual EOS P2P listen driver, leave/destroy,
cancellation of an in-flight create without travel, rehost, READY travel to
Depot with P2P preserved, and final session cleanup. SDK shutdown reported
EOS_Success, with ER_ONLINE_SHUTDOWN instance_released and no SIGBUS.

With the populated private configuration, `scripts/test-lobby.py` **passed**
again using a real LAN host/two remotes, discovery, READY and Depot travel.
The host reported ER_TRANSPORT LAN/IP and did not perform an EOS login.

The Exit Game path additionally awaits session destruction (bounded at five
seconds) before requesting exit, and stops LAN discovery while quitting.
The extended `scripts/test-online-service.py` requires its clean-exit marker as
well as probe PASS and process exit0; final extended-run outcome is below.

This is a one-account/one-Mac test. Remote JoinSession/client transport,
multiple accounts, two-computer gameplay and cross-network NAT/relay traversal
remain unverified. No new package, upload or publication was performed.

Logs: `/private/tmp/rr-eos-service-build.log`,
`/private/tmp/rr-eos-service-test.log`,
`/private/tmp/rr-eos-configured-lan.log` and
`Saved/Testing/OnlineService/probe.log`.

Final extended real-service run: **PASS**, 21 October2026, process exit0.
`ER_EOS_PROBE_COMPLETE result=PASS`, `ER_ONLINE_QUIT cleaned=1` and
`ER_ONLINE_SHUTDOWN instance_released` were all present. The probe recreated a
live public EOS server after the leave/cancel/rehost/READY checks, requested exit
through UERGameInstance::QuitGame (the actual menu path), waited for successful
session destruction and exited without the earlier shutdown crash. The final
build succeeded, `git diff --check` passed, and a second credential-content scan
confirmed mode0600, Git ignore/untracked status and no secret in Git-eligible
files. EOS_Shutdown returned EOS_Success. No live test server was left running.


## 21 October 2026 — proportional viewport HUD

- Editor arm64 Development build: **PASS** (`scripts/build-editor.command`).
  Shared ERHUDLayout applies independent percentage edge coordinates and an
  uncapped uniform scale to Canvas HUD and Slate FPS. Existing unrelated UE
  deprecation warnings remain in the build log.
- `python3 scripts/test-hud.py`: **PASS**, native
  EclipseRecoil.Player.HUDViewportLayout, process exit0. Checks proportional
  sizing at0.5/1/1.5/2/3/4x and fit/insets for4:3,ultrawide,portrait.
  The editor logs the same14 pre-test `Condition failed` lines already present
  in earlier recon/audio/aim-punch automation logs; the requested test succeeds.
- `python3 scripts/test-hud.py --render`: **PASS**, process exit0,14 fresh
  Metal screenshots from seven real sizes of the same live viewport:
  640×360,1280×720,1920×1080,3840×2160,2560×1080,1024×768,720×1280.
  Each size includes normal HUD and scoreboard. All pairs inspected via
  Saved/Testing/HUD/review.png, with individual full-size inspections too.
  Disc radii57.6/115.2/172.8/345.6px at360p/720p/1080p/4K;
  bottom insets20/40/60/120px at16:9. Circles stay round and corner elements
  stay inside the viewport in all tested aspect ratios. The4K viewport retains
  its1920×1080 scene cap while HUD uses3840×2160, confirming independence from
  3D resolution. The first render run correctly failed because the native window
  capped larger requests at the monitor's1920×1080; the runner now starts with
  -ForceRes and a4K offscreen window to exercise actual requested dimensions.
- `python3 scripts/test-network.py --accuracy`: **PASS**, process exit0,
  server spread/recovery/rejected-shot checks and three owner replication checks.
  The reticle retains its real FOV/cone projection after normalized radius storage.

Logs: /private/tmp/rr-hud-build.log, /private/tmp/rr-hud-accuracy.log,
Saved/Testing/HUD/automation.log and render.log. Captures and visual review:
Saved/Testing/HUD/. No saved assets, package, upload or publication.

Final recon regression: `python3 scripts/test-recon.py` **PASS**, process exit0,
actual listen host/enemy/ally/late client. Verified server contact visibility,
smoke occlusion/dispersal, late-join fog, authorized drone hits,30-second
replacement, five-loss stock exhaustion and round-reset cleanup. Log:
/private/tmp/rr-hud-recon.log; peer logs/report in Saved/Testing/recon-*.
Final `git diff --check`: **PASS**.


## 21 October 2026 — smaller, raised minimap and darker vitals

Minimap radius reduced15% (97.92 reference units), top inset90→64 units,
preserving percentage placement and isotropic scaling. Status text uses the
same right screen inset as the disc to keep longer messages inside the viewport.
Bars and matching icons now use sRGB health154/84/84, armor86/132/100,
stamina82/130/154, with darker empty tracks. No gameplay or asset changes.

- Editor arm64 Development build: **PASS**, final exit0; log
  /private/tmp/rr-hud-refinement-build.log.
- `python3 scripts/test-hud.py`: **PASS**, native HUDViewportLayout, exit0.
- `python3 scripts/test-hud.py --render`: **PASS**, final exit0,14 fresh HUD/
  scoreboard captures on640×360,1280×720,1920×1080,3840×2160,2560×1080,
  1024×768 and720×1280. Radius48.96/97.92/146.88/293.76px for16:9 sizes.
  Visual inspection includes small and720p images and the updated contact sheet
  Saved/Testing/HUD/review.png. No clipping with the raised minimap; colors stay
  distinguishable and the independent width/height edge proportions remain intact.
- `git diff --check`: **PASS**. No network retest required for these visual
  constants/text alignment; previous successful recon/accuracy results remain
  recorded above. No packaging, upload or publication.
