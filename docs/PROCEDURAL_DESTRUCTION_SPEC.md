# Respawn & Rage — Mappe procedurali a blocchi e distruzione dinamica

> **Destinatario:** OpenAI Codex (agente di sviluppo)
> **Progetto:** Respawn & Rage, porting/evoluzione verso Unreal Engine 5
> **Documento:** specifica tecnica e piano di implementazione, versione iniziale
> **Priorità:** realizzare **un prototipo funzionante e misurabile**, non una simulazione fisica completa.

## 0. Istruzioni operative per Codex

Agisci come sviluppatore Unreal Engine 5 C++ nel repository su cui stai lavorando.

1. **Ispeziona prima il repository**: struttura, linguaggi, motore attuale, eventuale `.uproject`, moduli C++, asset, script di build, test, licenze e istruzioni già presenti (`AGENTS.md` inclusi). Non presumere che il vecchio Respawn & Rage sia già un progetto Unreal.
2. Individua la versione di Unreal Engine effettivamente installata/richiesta dal progetto e **adatta API, Build.cs e plugin a quella versione**. Non effettuare upgrade arbitrari del motore.
3. Se esiste già un progetto Unreal, integrati con esso preservando il gameplay funzionante. Se il repository contiene solo il gioco originale in un altro motore, realizza il prototipo Unreal **in una cartella/progetto separato**, secondo le convenzioni esistenti; non riscrivere o cancellare il codice originale.
4. **Implementa codice ed esempi eseguibili**, non limitarti a descrivere la soluzione. Procedi per incrementi piccoli e verificabili; mantieni il progetto compilabile quando è disponibile la toolchain.
5. Se non hai Unreal Editor o toolchain disponibili, implementa quanto verificabile staticamente, prepara le istruzioni locali e segnala esplicitamente quali verifiche non hai eseguito. Non dichiarare test o build completati se non lo sono.
6. Usa asset elementari di Unreal (cubi, materiali semplici, placeholder) per il primo prototipo. Non rendere il lavoro dipendente da asset Fab, Marketplace o plugin commerciali.
7. Al termine di ogni milestone, documenta: file creati/modificati, come avviare il prototipo, funzionalità verificabili, test eseguiti, problemi aperti e prossima milestone.
8. **Comincia dalla Milestone 1** di questo documento. Implementa le milestone successive solo se la precedente è verificata e i tempi/strumenti lo consentono. In caso contrario lascia il lavoro in uno stato coerente, indicando ciò che manca.

**Obiettivo del progetto:** un FPS con mappe generate proceduralmente, inizialmente costituite da stanze rettangolari, corridoi, porte, finestre, pareti e (successivamente) scale. Quasi tutte le strutture, tranne i pavimenti e altri elementi dichiarati portanti, devono poter essere danneggiate o distrutte a seconda del materiale. La logica di distruzione deve essere progettata fin dall'inizio per un futuro multiplayer server-authoritative.

---

## 1. Decisione architetturale fondamentale

**Il mondo è composto da blocchi distruttibili LOGICI, non da un Actor Unreal per ogni mattone.**

- Organizza la mappa in **due livelli**:
  - **Layout:** grafo delle stanze, corridoi, porte, finestre ed eventualmente piani/scale.
  - **Geometria:** blocchi modulari che riempiono le pareti del layout, lasciando vuote le aperture.
- Usa un database/griglia di blocchi lato gameplay, con ID stabili e stato del danno, **indipendente** dalla rappresentazione grafica.
- Visualizza i blocchi ripetuti con `UInstancedStaticMeshComponent` (ISM), o `UHierarchicalInstancedStaticMeshComponent` (HISM) solo dopo confronto misurato sul caso d'uso. Non generare migliaia di Actor separati.
- Mantieni la **collisione reale** coerente con i blocchi presenti. Quando un blocco viene rimosso, non deve più fermare giocatori, proiettili o visibilità.
- Usa **Chaos solo per i detriti e i pezzi in movimento**, non per simulare permanentemente tutti i blocchi della mappa.
- **Non usare Geometry Scripting o booleane runtime come prerequisito per l'MVP**: la distruzione consiste principalmente nella rimozione di blocchi interi. I fori piccoli possono essere effetti visivi e meccaniche di penetrazione, senza vera modifica della mesh.
- La fonte di verità è la **struttura logica dei blocchi**. Rendering, collisione, FX e rete sono proiezioni di tale stato.

### Schema ad alto livello

```text
MapSeed + MapConfig
       |
       v
ProceduralLayoutGenerator
       |
       v
Rooms / Corridors / Openings [no overlap, connected]
       |
       v
BlockWorldBuilder
       |
       v
BlockWorld (stable BlockID, Material, Health, State)
       |
       +------> ChunkRenderer (ISM per tipo / materiale)
       |
       +------> BlockDamageSystem <--- Hitscan / Projectiles / Explosions
       |                 |
       |                 +----> Penetration + Damage + BlockRemoval
       |
       +------> StructuralSupport (fase successiva)
       |
       +------> Replication (fase successiva, server authority)
       |
       +------> Debug / Benchmark
```

---

## 2. Scala e coordinate iniziali

Unreal usa centimetri. Rendi queste dimensioni configurabili; valori suggeriti per il prototipo:

| Parametro | Valore iniziale | Note |
|---|---:|---|
| Blocco parete | **50 × 25 × 20 cm** | larghezza × altezza × spessore |
| Altezza stanza | **300 cm** | 12 corsi di blocchi |
| Lato della cella di layout | **100 cm** | indipendente dalla risoluzione dei blocchi |
| Porta standard | **100 × 200 cm** | aperture allineate alla griglia dei blocchi |
| Finestra standard | **100 × 100 cm** | davanzale iniziale a 100 cm |
| Spessore parete | **20 cm** | orientabile sugli assi X/Y |
| Dimensione spaziale di un chunk | **400 × 400 × 400 cm** | valore da misurare, non vincolo assoluto |
| Prima stanza di test | **500 × 500 × 300 cm** | un pavimento e quattro pareti |

**Attenzione:** la griglia delle stanze (1 m) e la griglia dei blocchi (50 × 25 cm) hanno risoluzioni diverse. Prevedi trasformazioni esatte tra coordinate locali delle pareti, coordinate del mondo e ID dei blocchi. Le pareti sui due assi hanno orientamento diverso; evita errori di offset che creano fessure o blocchi sovrapposti agli angoli.

Esempio: una parete di 5 m per 3 m contiene 10 colonne × 12 corsi = **120 blocchi**, esclusi porte e finestre.

Per la fase iniziale **il pavimento è indistruttibile** e può restare una Static Mesh normale; anche eventuali fondamenta/ancoraggi saranno esplicitamente indistruttibili. Le pareti di cemento sono *molto resistenti*, non necessariamente indistruttibili per definizione.

---

## 3. Modello dati consigliato

I nomi sotto sono indicativi: adattali allo stile dei moduli Unreal esistenti.

### Enumerazioni

```cpp
enum class EBlockMaterialType : uint8
{
    Drywall,
    Wood,
    Brick,
    Concrete,
    Stone,
    Glass
};

enum class EBlockState : uint8
{
    Intact,
    Damaged,
    Destroyed
};
```

### Strutture e responsabilità

- `FBlockId`: identificatore stabile di un blocco nella mappa (per esempio combinazione di `WallId` + coordinate locali `Column/Row`, oppure un indice stabile nella struttura generata). **Non usare `InstanceIndex` come BlockID**.
- `FBlockRecord`: ID, tipo di materiale, trasformazione/dimensioni, HP/stato, `bIndestructible`, chunk proprietario, eventuali flag di supporto strutturale.
- `FBlockMaterialProperties`: resistenza a danno e penetrazione, HP, coefficiente delle esplosioni, categorie FX, eventuali limiti sulla perforazione. Esponi come `UDataAsset`, `UPrimaryDataAsset` o `UDataTable`, a seconda delle convenzioni del progetto.
- `FMapGenerationConfig`: seed, dimensioni, numero stanze, larghezza minima corridoi, parametri porte/finestre, distribuzione materiali.
- `FRoomDescriptor`, `FWallDescriptor`, `FOpeningDescriptor`: rappresentazione del layout PRIMA della conversione in blocchi.
- `FBlockDamageEvent`: BlockID, posizione, direzione, tipo danno, energia/danno, instigatore se presente.

### Classi/servizi suggeriti

```text
AProceduralMapActor or UProceduralMapSubsystem
UMapLayoutGenerator          -> layout deterministico, senza dipendenza dal rendering
UBlockWorldComponent         -> storage/lookup/query dei blocchi
UBlockChunkComponent         -> gestione rendering + collisioni per chunk
UBlockDamageComponent        -> danni, perforazioni, distruzione
UBlockStructuralComponent    -> fase avanzata, blocchi sospesi/crolli
UBlockWorldDebugComponent    -> seed, ID, statistiche e visualizzazione diagnostica
```

Non creare classi vuote solo per rispettare esattamente i nomi. Preferisci **poche classi con responsabilità chiare e codice testabile**.

### Identificatori e indici ISM

Le operazioni di rimozione possono modificare gli indici delle istanze di un ISM/HISM; la semantica varia in base all'API/versione e alle impostazioni. Serve un mapping coerente:

```text
BlockID -> (ChunkID, RenderGroupID, CurrentInstanceIndex)
(ChunkID, RenderGroupID, CurrentInstanceIndex) -> BlockID
```

Dopo una rimozione, **ricostruisci/aggiorna il mapping coinvolto** oppure usa API con ID di istanza stabili disponibili nella versione effettiva del motore, se adatte. Non dare per scontato che l'indice rimanga invariato. Raggruppa le rimozioni per chunk/render group quando possibile. Verifica anche il comportamento di hit result sulle istanze (ad es. indice dell'istanza nel risultato del trace) nella versione in uso.

---

## 4. Materiali, proiettili e distruzione

### 4.1 Profili dei materiali

Parti con tre materiali obbligatori: **cartongesso, mattoni, cemento**. Aggiungi legno/vetro/pietra quando il core funziona.

Valori iniziali *di gameplay*, non misurazioni fisiche reali:

| Materiale | HP blocco (esempio) | Penetrazione | Esplosioni | Comportamento atteso |
|---|---:|---|---|---|
| Cartongesso | 30 | facile | molto vulnerabile | fori/rottura rapida |
| Legno | 65 | dipende dallo spessore | vulnerabile | schegge |
| Vetro | 10 | molto facile | molto vulnerabile | rottura rapida |
| Mattoni | 140 | difficile | medio | danni cumulativi |
| Cemento | 300 | molto difficile | resistente | scheggiature/breccia costosa |
| Pietra | 1000 | quasi nulla | molto resistente | principalmente effetti superficiali |

I valori devono essere **interamente configurabili**, non sparsi in condizioni `if/else` nel codice.

### 4.2 Colpi singoli

Prevedi hitscan iniziale, integrabile col sistema di armi già presente. Pipeline:

1. Trace dal punto di sparo alla distanza massima, con filtri/canali di collisione appropriati.
2. Se si colpisce un blocco, ricava `BlockID` dal chunk e dalla specifica istanza.
3. Calcola danno e possibilità di penetrazione in base a arma/energia, materiale, **spessore effettivamente attraversato** e angolo di impatto (con limiti per evitare instabilità agli angoli radenti).
4. Applica il danno al blocco. Se `HP <= 0`, impostalo a `Destroyed` e rimuovilo da rendering/collisione.
5. Se il colpo penetra prima della distruzione, simula il proiettile **oltre il blocco** con energia residua, anche se la collisione di quel blocco resta presente. Calcola correttamente un punto di uscita dal volume e un nuovo punto di partenza; non basta rilanciare il trace di pochi millimetri e colpire infinite volte lo stesso blocco.
6. Imposta limiti al numero di attraversamenti e alla distanza/energia residua per evitare cicli e costi non limitati.
7. Aggiungi decal/particelle/suoni placeholder in base al materiale. Un foro di proiettile non deve necessariamente distruggere un blocco da 50 cm.

La **penetrazione balistica** e la **breccia fisica** sono meccaniche differenti: un proiettile può attraversare cartongesso senza aprire automaticamente un passaggio di mezzo metro.

### 4.3 Esplosioni

Una granata genera un evento con origine, raggio, danno/impulso. Il sistema deve:

- Individuare i blocchi candidati tramite query spaziale (evitare scansioni complete della mappa se possibile).
- Applicare danno con attenuazione per distanza e proprietà del materiale.
- Considerare una forma minima di occlusione/riparo, anche semplificata (non distruggere pareti protette dietro cemento come se fossero esposte direttamente).
- Rimuovere **più blocchi insieme** nel caso di un'esplosione e aggiornare gli ISM/collisioni per batch.
- Generare solo un numero limitato di detriti/FX vicini alla camera; detriti puramente visivi non devono bloccare i percorsi del gameplay.

L'MVP può usare una granata di test richiamabile da un tasto/comando debug, senza richiedere l'intero sistema di armi, inventario o animazioni.

### 4.4 Integrità strutturale: fase successiva

Non usare inizialmente rigid body per ogni blocco. Quando alcuni blocchi vengono distrutti:

1. Identifica l'area/chunk connessa alla distruzione e gli eventuali chunk confinanti.
2. Costruisci/aggiorna un grafo di adiacenza di blocchi **connessi fisicamente**, con regole di contatto definite (non considerare connessioni diagonali attraverso il vuoto).
3. Esegui BFS/DFS a partire da blocchi ancorati a pavimento/fondazioni/strutture portanti.
4. Rileva componenti rimaste senza connessioni di supporto.
5. Marca tali componenti per il crollo, rimuovendole dal mondo logico e creando, solo se necessario, un numero limitato di corpi simulati o gruppi di detriti con Chaos.
6. Metti un budget al numero di blocchi controllati e ai corpi Chaos attivi per frame.

Per l'MVP questa funzionalità può restare **disattivata**, ma l'architettura deve consentirne l'aggiunta. Il pavimento resta il riferimento indistruttibile per il supporto.

---

## 5. Generatore procedurale delle mappe

### 5.1 Layout prima della geometria

Usa `FRandomStream` (o PRNG equivalente esplicitamente seedato) e dati deterministici. Genera prima il layout, poi i blocchi. Non inserire casualità dipendente dal frame, dall'ordine di iterazione di mappe/hash o da funzioni globali non seedate.

Per una prima mappa giocabile:

- Genera **6–10 stanze rettangolari** di dimensioni configurabili.
- Evita sovrapposizioni illecite tra stanze; gestisci corridoi e pareti condivise **senza duplicazioni**.
- Connetti tutte le stanze con un algoritmo semplice (es. albero di connessione + eventuali collegamenti extra per percorsi alternativi).
- Apri porte coerenti sui collegamenti; opzionalmente finestre su alcune pareti non di passaggio.
- Definisci un punto di spawn sicuro in uno spazio libero e accessibile.
- Converti i segmenti di muro in una griglia di blocchi, omettendo quelli occupati da porte/finestre.
- Crea i pavimenti statici e indistruttibili.
- Non posizionare il giocatore dentro pareti o elementi solidi.

**Validazione automatica richiesta:** verifica che tutte le stanze siano raggiungibili dalle altre attraverso i passaggi previsti *prima* di iniziare la simulazione della distruzione. Testa seed ripetuti e casi limite con poche stanze o corridoi stretti.

### 5.2 Scale e più piani

Sono **fuori dall'MVP**. Predisponi descrittori di piano e collegamento verticale, ma implementali solo dopo che generazione e distruzione a un piano funzionano bene. Le scale inizialmente possono essere indistruttibili; verifica che la loro geometria e collisione siano percorribili.

### 5.3 Aspetto visivo

- Inizia con primitive molto semplici, materiale diverso per tipo di blocco.
- Fai sì che la griglia dei blocchi **non debba essere necessariamente visibile** a parete integra (texture/materiale continuo, mesh allineate, assenza di fessure).
- I primi test possono deliberatamente mostrare le giunzioni per facilitare il debug.
- Prevedi porte come aperture vuote prima di introdurre veri Actor porta apribili/distruttibili.

---

## 6. Chunk, collisioni e prestazioni

### 6.1 Rappresentazione

Ogni chunk gestisce un insieme di blocchi spazialmente vicini. Al suo interno raggruppa le istanze per tipo di mesh/materiale, in modo da ridurre componenti, draw call e aggiornamenti.

- Un Actor/Component per chunk, **non per blocco**.
- ISM iniziale: facile da ispezionare e sufficiente per l'MVP.
- Mesh con **collisione semplice** adeguata: una scatola per mattone va bene per i primi test.
- Occhio alle collisioni dell'ISM: le opzioni di collisione sono largamente condivise a livello di componente, mentre la presenza/assenza dell'istanza va sincronizzata con lo stato logico.
- Se un chunk viene ricostruito, conserva i BlockID; ricrea solo le corrispondenze grafiche.
- Mantieni separata la struttura usata per le query balistiche/di danno dal solo disegno a schermo, se necessario.

### 6.2 Query e aggiornamenti

- Query spaziale per individuare i blocchi entro raggio esplosivo.
- Damage events aggregati per BlockID nello stesso frame/tick, se necessario.
- Non rigenerare tutta la mappa dopo ogni colpo.
- Evita un aggiornamento per frame dei blocchi intatti: logica **event-driven**.
- Limita effetti particellari, suoni, decal e detriti con budget configurabili.

### 6.3 Metriche da registrare

Mostra almeno tramite log/debug HUD:

```text
Seed della mappa
Numero stanze / porte / finestre
Numero blocchi totali / intatti / distrutti
Numero chunk / ISM instances
Ultimo tempo di generazione (ms)
Ultimo tempo di distruzione in batch (ms)
Tempo di aggiornamento delle collisioni (se misurabile)
Numero corpi fisici attivi (quando implementato Chaos)
```

Esegui benchmark graduali: una parete da 120 blocchi, una stanza intera, una mappa di 6–10 stanze, una raffica di colpi e un'esplosione che distrugge decine di blocchi.

**Non fissare a priori una promessa di FPS**: raccogli misure su hardware reale e annota piattaforma, configurazione e condizioni del test. L'obiettivo progettuale è una mappa piccola giocabile su PC senza freeze percepibili quando si rompe una parete.

---

## 7. Multiplayer: requisiti di progetto, non prerequisito dell'MVP

La prima versione può funzionare in single-player/PIE, ma deve essere predisposta per una simulazione **server-authoritative**.

- Server/authority possiede lo stato logico: seed, configurazione, HP dei blocchi e distruzioni.
- Il client invia una richiesta di fuoco/azione; l'authority valida e applica danni/penetrazioni. **Non fidarsi di danni o BlockID arbitrari inviati dal client**.
- Per l'avvio della partita, replica o concorda seed e versione/hash della configurazione del layout. Asset/configurazioni differenti non devono portare a mappe incoerenti.
- Durante il match sincronizza **solo le modifiche** (`BlockID`, nuovo stato/HP o evento distruzione), non l'intera geometria ad ogni frame.
- Per client che entrano in ritardo invia uno **snapshot coerente** dei blocchi modificati, oltre a seed/configurazione, poi applica gli eventi successivi nell'ordine corretto.
- La distruzione deve produrre **stesse collisioni e stessi passaggi** su server e client. I detriti decorativi possono restare locali e non replicati.
- Evita RPC affidabili per ogni particella o frammento; valuta batching e, quando appropriato, strutture di replica differenziale come Fast Array, compatibili con la versione del motore.

**Test multiplayer futuro:** due client e un server; un client rompe un muro; entrambi vedono la breccia; il secondo può attraversarla e sparare attraverso di essa; un client entrato dopo vede lo stato aggiornato.

---

## 8. Milestone ordinate e criteri di accettazione

### Milestone 0 — Preparazione del repository (obbligatoria)

**Implementazione:**

- Identifica motore/versione e struttura del repository.
- Configura il progetto/prototipo Unreal C++ in maniera coerente con la struttura esistente.
- Imposta `.gitignore` per `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/` e altri artefatti generati; documenta Git LFS per asset binari versionati quando appropriato.
- Mantieni gli asset non redistribuibili fuori dal repository pubblico; verifica licenze degli eventuali asset Fab prima di includerli.
- Scrivi nel README come aprire il `.uproject` e compilare, indicando prerequisiti e versione Unreal.

**Accettazione:** progetto Unreal identificato o creato senza danneggiare il gioco preesistente; istruzioni di apertura/build presenti.

### Milestone 1 — Una stanza con pareti a blocchi (PRIMA IMPLEMENTAZIONE)

**Implementazione:**

- Crea una scena di test con **una stanza 5 × 5 × 3 m**, pavimento indistruttibile e quattro pareti a blocchi.
- Implementa `BlockWorld` e storage di BlockID stabili, HP, stato e materiale.
- Usa ISM raggruppati per materiale/chunk; niente Actor per singolo blocco.
- Configura almeno cartongesso, mattoni e cemento.
- Esponi un comando debug (input tastiera o console) per colpire il blocco sotto il mirino e un comando per distruggerlo.
- Alla distruzione, rimuovi l'istanza e la relativa collisione.
- Aggiungi HUD/log dei contatori e BlockID colpito.

**Accettazione (obbligatoria):**

- Il personaggio può muoversi nella stanza.
- Si può colpire un blocco specifico e distruggerlo.
- Il blocco sparisce sia visivamente sia dalla collisione.
- I blocchi adiacenti **non vengono rimossi per errore** dopo distruzioni ripetute in ordine casuale.
- L'intera parete può essere abbattuta senza crash.
- La mappa può essere rigenerata senza duplicazioni o riferimenti a componenti ormai eliminati.

### Milestone 2 — Danni, penetrazione, esplosioni

**Implementazione:**

- Collega hitscan/armi già esistenti o uno strumento di tiro minimale.
- Implementa HP/materiali ed energia residua dei proiettili dopo la penetrazione.
- Distingui danno visivo, penetrazione e rimozione del blocco.
- Implementa esplosione radiale con distruzione di più blocchi, occlusione semplice e batch updates.
- FX minimi per ogni materiale, anche placeholder.

**Accettazione:** cartongesso si distrugge più facilmente del mattone; il cemento resiste maggiormente; i colpi attraversano materiali penetrabili e possono colpire un bersaglio dietro; una granata apre una breccia; la collisione corrisponde allo stato finale.

### Milestone 3 — Generazione procedurale di un piano

**Implementazione:**

- Genera 6–10 stanze rettangolari e corridoi da un seed.
- Crea porte/finestre come aperture coerenti e non occupate da blocchi.
- Evita sovrapposizioni, pareti duplicate e spawn ostruiti.
- Supporta diversi seed configurabili da console o proprietà esposte.
- Integra le stesse regole di danno e distruzione della stanza di test.

**Accettazione:** stesso seed e config generano la stessa topologia e gli stessi BlockID; seed differenti producono layout diversi; tutte le stanze sono raggiungibili; ogni parete prevista è danneggiabile secondo materiale; niente disallineamenti evidenti sulle aperture.

### Milestone 4 — Stabilità e ottimizzazione

**Implementazione:**

- Profilazione delle operazioni ISM/collisioni e dell'uso memoria.
- Chunk update/batch, query spaziali, limiti agli FX.
- Test regressione su rimozione di molte istanze e rigenerazione ripetuta.
- Eventuali aggiustamenti di dimensione blocco/chunk sulla base dei risultati.

**Accettazione:** nessun crash, nessuna istanza fantasma, nessuna collisione residua dopo numerose distruzioni; benchmark riportati nel README con metodologia e limiti.

### Milestone 5 — Crolli controllati (successiva)

**Implementazione:** algoritmo di supporto strutturale, ricerca delle componenti non ancorate e conversione selettiva in detriti simulati tramite Chaos.

**Accettazione:** dopo la rimozione dei supporti i pezzi realmente isolati cadono; quelli ancorati restano al loro posto; la simulazione dei detriti è limitata e non congela il gioco.

### Milestone 6 — Multiplayer e ampliamento mappe (successiva)

**Implementazione:** server authority, replica seed e delta, late join, test a due client; quindi più piani, scale e ulteriori materiali.

**Accettazione:** stato e collisioni delle brecce identici per tutti i giocatori, incluso chi entra a partita iniziata; mappe multilivello con percorsi validi.

---

## 9. Test da implementare

Preferisci `AutomationSpec`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` o l'infrastruttura già usata dal repository. Dove le simulazioni richiedono PIE, documenta un test manuale ripetibile.

| Test | Verifica |
|---|---|
| Seed deterministico | stesso seed/config => stesse stanze, aperture e BlockID |
| Connettività | tutte le stanze generate sono raggiungibili |
| Identità blocco | dopo rimozioni casuali il trace individua sempre il blocco corretto |
| Danno materiale | uguale colpo: cartongesso < mattone < cemento per resistenza |
| Distruzione | HP fino a zero => stato distrutto e istanza rimossa |
| Collisione | trace e movimento non incontrano più un blocco rimosso |
| Penetrazione | bersaglio dietro materiale fragile colpibile se energia sufficiente |
| Limite tracce | un proiettile non produce loop infinito in pareti/angoli |
| Esplosione | colpisce più blocchi, senza danneggiare indiscriminatamente quelli schermati |
| Rigenerazione | ripetute generazioni non lasciano Actor/componenti duplicati |
| Stress | raffica e grandi rimozioni non causano crash o mapping corrotto |

**Test manuale essenziale Milestone 1:** mira un blocco al centro della parete, rimuovilo, verifica il foro; distruggi il blocco vicino e verifica che sia quello voluto; distruggi una colonna completa; controlla il trace attraverso il vuoto; rigenera e ripeti.

---

## 10. Vincoli, cose da NON fare e compromessi

### Da fare

- **C++ per il core** (layout, block world, danni, chunk/instance mapping). Blueprint per configurazione e test rapido.
- Mantieni un'interfaccia semplice per sostituire materiali, mesh e armi senza riscrivere la distruzione.
- Evita dipendenze inutili da plugin di terze parti.
- Mantieni il prototipo riproducibile da seed e compatibile con il futuro multiplayer.
- Usa log diagnostici mirati, controlli sugli indici e protezioni contro blocchi già distrutti.

### Da non fare nell'MVP

- Non creare un Actor e un rigid body permanentemente attivi per ogni blocco.
- Non simulare crolli strutturali di interi edifici in tempo reale.
- Non modellare fori esatti di ogni proiettile con operazioni booleane sulle mesh.
- Non introdurre subito scale, più piani, NavMesh dinamica complessa o edifici fotorealistici.
- Non convertire tutto il progetto originale al nuovo motore prima di dimostrare il nucleo della distruzione.
- Non includere asset Fab/Marketplace nel repository senza averne verificato licenza e modalità di distribuzione.
- Non dichiarare funzionalità multiplayer completate solo perché il codice è predisposto alla replica.

### Limiti consapevoli

- I blocchi da 50 × 25 cm sono una discretizzazione del gameplay, non mattoni fisicamente fedeli.
- Un proiettile può penetrare la parete senza produrre immediatamente un passaggio geometrico.
- Il realismo è **relativo e configurabile** (materiali più o meno resistenti), non una simulazione ingegneristica della balistica o delle strutture.
- Gli elementi grafici, i detriti Chaos e gli effetti possono essere più dettagliati della griglia logica purché non generino collisioni incoerenti.

---

## 11. Output richiesto a Codex

Alla fine del lavoro restituisci:

1. **Risultato realizzato**, con riferimento esplicito alla milestone raggiunta.
2. **Elenco dei file** nuovi/modificati e loro funzione.
3. **Istruzioni pratiche** per aprire il livello di test, rigenerare la mappa, sparare e innescare una granata/esplosione (per le milestone che le prevedono).
4. **Esito dei test** realmente eseguiti e output/log essenziali; se non è stato possibile compilare o usare l'Editor, indica esattamente perché.
5. **Problemi noti e scelte tecniche** (in particolare mapping tra BlockID e ISM, aggiornamento collisioni e limiti di prestazione).
6. **Prossimi passi** ordinati, senza affermare di aver implementato fasi ancora non svolte.

**Prima azione richiesta:** implementa la Milestone 0 (ricognizione) e poi la **Milestone 1: una stanza giocabile con blocchi distruttibili e collisioni realmente aggiornate**. Questo è il traguardo minimo da perseguire prima di passare a mappe procedurali complete.

---

## 12. Riferimenti tecnici ufficiali

Verifica le API sulla versione Unreal effettiva del progetto; i link sotto puntano alla documentazione ufficiale e potrebbero mostrare una versione più recente.

- [Instanced Static Mesh Component — Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-static-mesh-component-in-unreal-engine)
- [UInstancedStaticMeshComponent API — Epic](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UInstancedStaticMeshComponent)
- [Geometry Collections User Guide — Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/geometry-collections-user-guide)
- [Destruction Quick Start — Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/destruction-quick-start)
- [Chaos Fields User Guide — Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/chaos-fields-user-guide-in-unreal-engine)

**Nota sul repository:** riferimento comunicato dal committente: `https://github.com/respawn-and-rage/core`. La struttura reale non è stata verificata durante la stesura: Codex deve basarsi sul repository locale effettivamente disponibile, non assumere dettagli non confermati.
