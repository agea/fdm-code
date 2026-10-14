# Eclipse Recoil — Guida al gioco base in Unreal Engine

> **Aggiornamento dello sviluppo locale — 12 ottobre 2026:** questa è la specifica
> importata. Le decisioni correnti sono in [DEVELOPMENT.md](DEVELOPMENT.md), gli
> esiti reali in [TEST_RESULTS.md](TEST_RESULTS.md). Per richiesta dell'utente,
> packaging, distribuzione e CI (B10–B12) sono rinviati. Il progetto esistente
> `fdm.uproject` è stato mantenuto e convertito a Blueprint + modulo C++.
> Le istruzioni persistenti per lo sviluppo sono in `../AGENTS.md`.
> La prima visuale FPS è stata corretta e verificata nel deposito: attacco
> `HandGrip_R`, posa statica adattata delle braccia e assi Python espliciti.
> Movimento e ricarica animati restano rifiniture successive.
> L'utente ha successivamente approvato bot di collaudo opzionali e bersaglio
> fermo: il precedente divieto di aggiungere bot è aggiornato da questa scelta.
> Le partite LAN normali conservano zero bot; dettagli in `DEVELOPMENT.md`.
> Collaudo dei controlli avviato nella finestra Metal: aggiunto C come
> alternativa a Ctrl e corretto lo sfondo del menu in partita. Esiti e limiti
> del controllo tramite automazione sono riportati in `TEST_RESULTS.md`.

> Specifica operativa per realizzare il primo prototipo multiplayer giocabile del port.
> Lingua: italiano · Revisione del documento: 12 ottobre 2026.
> Stato della specifica importata: istruzioni e criteri di accettazione. Il prototipo locale è ora implementato; verifiche superate e casi ancora aperti sono documentati in `TEST_RESULTS.md`.

## 1. Obiettivo e uso del documento

Realizzare **Eclipse Recoil in Unreal Engine come piccolo FPS Team Deathmatch**, con una mappa, due squadre, un fucile e partite tra 2–4 persone. Il risultato deve essere un pacchetto autonomo: un amico lo estrae, lo avvia senza Unreal Editor e si collega all'host nella stessa rete locale.

Questa guida serve sia per lavorare personalmente nell'Editor sia come specifica da consegnare a Codex o a un altro sviluppatore. Le fasi richiedono codice C++ e collegamenti nell'Editor: scaricare un template o importare asset non realizza da solo il multiplayer.

**Scelta proposta:** nuovo progetto C++ dal template **First Person**, variante **None** se presente nella versione installata; Blueprint per modelli, animazioni, dati e interfaccia. Le regole di gioco e di rete rimangono in C++. Lyra può essere studiato in seguito, ma non è una dipendenza del prototipo. Epic documenta separatamente le varianti del template e l'implementazione multiplayer. [E1], [E2]

Procedere nell'ordine delle fasi. Ogni fase indica cosa produrre e quando considerarla completata. Prima verificare il comportamento con forme semplici e personaggi provvisori; poi inserire gli asset scelti.

### Cosa sappiamo e cosa va confermato

| Elemento | Stato |
|---|---|
| Progetto di partenza | Repository `agea/eclipse-recoil`, branch `develop`. |
| Revisione usata per ricavare le regole | `c5e6fad873e4bbddf189c1be4ba555b916bacad5`. |
| Unreal Engine | L'utente dichiara di averlo scaricato; versione e piattaforma di sviluppo da registrare. |
| Asset | L'utente ne ha aggiunti alla libreria Fab; nomi, licenze e compatibilità dei singoli pacchetti non sono ancora verificati. |
| Nuovo progetto | Nome proposto `EclipseRecoilUE`, titolo mostrato ai giocatori **Eclipse Recoil**. |
| Prima piattaforma | Quella compilabile con il computer e gli strumenti disponibili; Windows se si dispone di Windows e della relativa toolchain. |
| Verifiche Unreal | Da eseguire durante l'implementazione. Nessun esito positivo è presunto da questa guida. |

Le regole dell'originale riportate sotto derivano dalla configurazione e dal codice della revisione indicata. Alcuni passaggi introduttivi della documentazione storica descrivono preset precedenti: in caso di differenze, fare riferimento ai file effettivi e ai vincoli correnti del progetto. [R1], [R2], [R3]

## 2. Specifica del primo prototipo

### 2.1 Regole da conservare

| Aspetto | Comportamento richiesto |
|---|---|
| Modalità | Team Deathmatch, squadre **Alpha** e **Omega**. |
| Salute | 100 HP alla nascita; nessuna rigenerazione. |
| Friendly fire | Attivo, con lo stesso danno di un colpo contro un avversario. |
| Movimento | Movimento terrestre, salto singolo, accovacciamento. |
| Arma iniziale | Un fucile automatico `Rifle_A`, assegnato a ogni nascita. |
| Munizioni iniziali | 30 nel caricatore, 90 di riserva. |
| Tipo di sparo | Proiettile simulato, fermato al primo impatto valido. |
| Respawn | Attesa minima di **3 secondi** e richiesta con sparo o salto. Il solo trascorrere del tempo, senza richiesta, non fa rinascere. |
| Protezione alla nascita | **4 secondi**, interrotti dal primo sparo autorizzato. Muoversi non interrompe la protezione. |
| Equipaggiamento | Nessun pickup, nessuna arma lasciata a terra e nessuna raccolta dai cadaveri. |
| Bot e melee | Bot di collaudo opzionali approvati dall'utente; zero bot di default. Melee assente. |

Queste regole sono ricavate dal preset TDM, dall'arsenale e dalla gestione di stato del gioco originale. Il gioco completo interrompe la protezione anche con altre azioni, fra cui lo zoom: non essendoci zoom, granate o obiettivi nel prototipo, qui viene implementato lo sparo. [R1], [R2], [R4]

### 2.2 Valori del fucile

Le statistiche iniziali riprendono l'AK-103 dell'originale, mentre il modello può essere qualsiasi fucile di cui sia disponibile un asset adatto.

| Parametro | Valore iniziale | Provenienza |
|---|---:|---|
| Danno | 36 HP | Danno al torso derivato dall'originale. |
| Cadenza | 600 colpi/minuto | Intervallo minimo di 0,100 secondi. |
| Caricatore | 30 | Originale. |
| Riserva alla nascita | 90 | Originale. |
| Ricarica | 1,5 secondi | Valore ereditato dallo slot dell'arma nell'originale. |
| Moltiplicatore testa/arti | 1 per tutte le zone | Semplificazione del prototipo. |
| Velocità del proiettile | 30.000 cm/s come primo valore di prova | Proposta da tarare in Unreal. |
| Durata massima del proiettile | 2 secondi | Proposta da tarare sulla mappa. |
| Gravità del proiettile | Disabilitata | Semplificazione del prototipo. |
| Rimbalzo, penetrazione, splash | Disabilitati | Primo impatto conclusivo. |

Nell'originale il valore di danno 360 è moltiplicato per la scala globale 0,1, ottenendo 36 HP al torso; il moltiplicatore della testa è 4. La ricarica da 1.500 ms deriva dai valori dell'arma nello slot Zapper. Il danno uniforme scelto qui rinvia quindi la localizzazione dei colpi. [R1], [R2], [R5]

**Non copiare direttamente la velocità `10000` del vecchio motore:** le sue unità non sono una conversione verificata in centimetri Unreal. Anche velocità del personaggio, salto, capsula e gradini vanno adattati alla nuova scala. Il valore di prova sopra è una scelta del port.

### 2.3 Nuove scelte del prototipo

Queste decisioni rendono la prima implementazione finita e verificabile; non pretendono di riprodurre tutto il sistema originale.

- Massimo **4 giocatori** nella prima configurazione; collaudo minimo con 2, poi 3–4.
- Per semplificare questo primo MVP, accettare la richiesta di respawn soltanto dopo i 3 secondi. Gli input anticipati non vengono accodati. È una differenza intenzionale: nell'originale una richiesta anticipata valida può rimanere in attesa fino alla scadenza. [R6], [R8]
- **Listen server**: chi ospita gioca ed esegue anche il server.
- Collegamento per IP nella LAN; nessuna ricerca automatica di partite.
- Una mappa `L_Depot`, una mappa menu `L_MainMenu`.
- Assegnazione server alla squadra meno numerosa; in parità alternare Alpha/Omega.
- Partita avviata quando entrambe le squadre hanno almeno un giocatore.
- Vittoria a **50 eliminazioni di avversari** oppure dopo **10 minuti**. Parità allo scadere: pareggio.
- Uccisione di un avversario: `Kills +1` al responsabile, `TeamScore +1` alla sua squadra, `Deaths +1` alla vittima.
- Teamkill: `Deaths +1` alla vittima, nessun punto squadra e nessun incremento di `Kills`; registrare separatamente `TeamKills`.
- Morte ambientale o suicidio: solo `Deaths +1`, senza credito a una squadra.
- Nessun riequilibrio delle squadre a metà partita. Un nuovo ingresso usa il bilanciamento; una riconnessione conta come nuovo ingresso.
- A fine partita, risultato visibile per 10 secondi e poi nuova partita nella stessa mappa.

Il punteggio semplificato è una decisione nuova: il server originale contiene anche punti e penalità più articolati. [R6]

### 2.4 Funzioni rinviate

Arsenale completo, pistola secondaria, loadout, granate, ADS/zoom, danno differenziato per ossa, recoil e dispersione fedeli all'originale, danni da caduta, sprint separato, arrampicata avanzata, bot competitivi avanzati, conversione delle vecchie mappe, generazione procedurale durante la partita, dedicated server, matchmaking, Steam/EOS, account e compensazione avanzata della latenza.

I danni da caduta e alcune delle altre funzioni esistono nell'originale: rinviarli è una differenza dichiarata del primo port. Una zona fuori mappa può comunque causare una morte ambientale per evitare personaggi bloccati. [R1], [R3]

## 3. Fase 0 — Preparare progetto, strumenti e asset

### 3.1 Registrare l'ambiente

Creare `docs/ENVIRONMENT.md` e annotare:

- Versione esatta di Unreal installata, compresa la patch, e provenienza dell'installazione.
- Sistema operativo, CPU, RAM e GPU della macchina di sviluppo.
- Compilatore, IDE e SDK realmente usati.
- Prima piattaforma da distribuire e macchina prevista per il test.

Usare la stessa versione del motore sulle macchine di sviluppo e di build. Non aggiornare il motore durante una fase senza una ragione concreta e una copia recuperabile del progetto.

Per C++ servono gli strumenti compatibili con quella versione Unreal: su Windows la toolchain Visual Studio/Windows SDK; su macOS Xcode. Seguire la matrice della documentazione selezionando la versione effettiva, senza installare numeri di versione ricavati da una guida diversa. Per Linux esistono compilazione nativa e percorsi di cross compilation documentati; pianificarli dopo il primo pacchetto funzionante. [E11], [E12]

L'installazione del motore distribuita da Epic è sufficiente per questo client con listen server. Non rendere obbligatoria una compilazione del motore da sorgenti per iniziare.

### 3.2 Creare il progetto

1. Nel Project Browser scegliere **Games → First Person → C++**.
2. Se è disponibile una scelta di variante, scegliere **None**. Le etichette possono cambiare con la versione.
3. Salvare il progetto come `EclipseRecoilUE` in una cartella nuova.
4. Aprire il progetto, compilare il template senza modifiche e avviare una prova locale.
5. Conservare gli asset del template necessari al personaggio provvisorio.
6. Non aggiungere subito il contenuto di tutti i pacchetti Fab: iniziare con il minimo.

La variante Arena Shooter offre contenuti di esempio, ma non va trattata come prova che il TDM con le regole di questo documento sia già implementato. [E1]

### 3.3 Repository e file binari

Creare un **repository privato separato** per il progetto Unreal. Il repository originale rimane un riferimento per regole e comportamento. Il codice nuovo potrà essere pubblicato separatamente in seguito, dopo aver controllato quali contenuti siano ridistribuibili.

Versionare almeno `.uproject`, `Config/`, `Source/`, il contenuto necessario in `Content/`, gli eventuali plugin redistribuibili e la documentazione. Un esempio iniziale di `.gitignore` è:

```gitignore
# Output generati alla radice del progetto
/Binaries/
/DerivedDataCache/
/Intermediate/
/Saved/
/.vs/
/.idea/
/dist/
/BuildArtifacts/
/*.sln
/*.suo
/*.VC.db
/*.VC.opendb
.DS_Store
```

Non ignorare indiscriminatamente `Build/`: può contenere icone e risorse utili al packaging. Non applicare un'esclusione globale a tutti i `Binaries/` dei plugin: un plugin senza sorgenti potrebbe richiedere proprio quei binari.

Installare Git LFS prima del primo commit dei contenuti. Una base per `.gitattributes` è:

```gitattributes
*.uasset filter=lfs diff=lfs merge=lfs -text
*.umap filter=lfs diff=lfs merge=lfs -text
*.fbx filter=lfs diff=lfs merge=lfs -text
*.blend filter=lfs diff=lfs merge=lfs -text
*.wav filter=lfs diff=lfs merge=lfs -text
*.png filter=lfs diff=lfs merge=lfs -text
*.tga filter=lfs diff=lfs merge=lfs -text
*.exr filter=lfs diff=lfs merge=lfs -text
```

Aggiungere i formati effettivamente usati. LFS conserva nel repository dei puntatori, mentre i file binari sono nello storage LFS: verificare che un clone recuperi i file reali e controllare le quote del proprio piano. Non versionare l'installazione di Unreal o gli ZIP delle release. [G1]

### 3.4 Scegliere gli asset

Creare `docs/ASSETS.md` con una riga per ogni pacchetto importato:

| Campo | Cosa registrare |
|---|---|
| Nome e autore | Identità del pacchetto e del fornitore. |
| Pagina originale | URL Fab o del sito di provenienza. |
| Licenza | Nome esatto, eventuali condizioni particolari e data di acquisizione. |
| Compatibilità | Versioni Unreal dichiarate, plugin richiesti, sistema operativo se rilevante. |
| Contenuto usato | Cartella nel progetto, personaggi/armi/materiali effettivamente impiegati. |
| Attribuzione | Testo e posizione dei crediti richiesti. |
| Evidenza | Collegamento o copia conservata delle condizioni applicabili. |

**Gratuito non significa liberamente ridistribuibile come sorgente.** La Fab Standard License consente l'impiego degli asset nei progetti e la distribuzione dei progetti che li incorporano, oltre alla condivisione privata con collaboratori del progetto secondo le sue condizioni; vieta la redistribuzione degli asset come prodotti autonomi. Un repository privato non autorizza a condividerli indiscriminatamente. Verificare la licenza effettiva di ogni pacchetto, perché non tutti i contenuti hanno necessariamente lo stesso regime. [L1]

I contenuti soggetti all'Epic Content License Agreement hanno proprie condizioni per distribuzione integrata e condivisione dei sorgenti; le eventuali restrizioni UE-Only vanno rispettate. Per materiali CC0 il regime è diverso. Tenere questi casi distinti nell'inventario. [L2], [L3]

Per la prima fase bastano un personaggio provvisorio, un fucile e materiali semplici. Se un asset richiede un retargeting complesso, usare temporaneamente il mannequin. Un'arma con scheletro diverso non diventa compatibile con le animazioni del personaggio semplicemente importandola.

**Fase completata quando:** il progetto C++ originale del template compila, il repository è configurato e le dipendenze utilizzate sono identificate.

## 4. Fase 1 — Organizzare codice e contenuti

### 4.1 Cartelle proposte

| Percorso | Contenuto |
|---|---|
| `Source/EclipseRecoilUE/` | Classi C++, separate in sottocartelle di gameplay, armi e UI se utile. |
| `Content/EclipseRecoil/Core/` | Blueprint delle classi principali. |
| `Content/EclipseRecoil/Characters/` | Configurazione dei personaggi e animazioni proprie. |
| `Content/EclipseRecoil/Weapons/` | Blueprint arma, proiettile e dati. |
| `Content/EclipseRecoil/Input/` | Input Actions e Mapping Contexts. |
| `Content/EclipseRecoil/UI/` | Widget HUD, menu, scoreboard e risultato. |
| `Content/EclipseRecoil/Maps/` | `L_MainMenu` e `L_Depot`. |
| `Content/EclipseRecoil/Art/` | Materiali, icone e contenuti creati per il progetto. |
| Cartelle originali dei pacchetti | Asset importati dal fornitore, mantenendo i riferimenti validi. |
| `docs/` | Ambiente, inventario asset, decisioni e risultati delle verifiche. |

Spostare o rinominare gli asset dall'Editor, controllando i riferimenti; non riorganizzare i `.uasset` con il file manager. Per importare da un progetto di esempio, selezionare gli asset nel Content Browser, scegliere **Asset Actions → Migrate**, controllare le dipendenze elencate e indicare la cartella `Content` del nuovo progetto. Verificare poi i riferimenti nell'Editor di destinazione. [E17]

### 4.2 Responsabilità delle classi

Questa è l'architettura proposta per il port.

| Classe | Responsabilità |
|---|---|
| `UERGameInstance` | Nome locale, host/join, errori di rete e ritorno al menu. Nessun punteggio condiviso. |
| `AERGameMode : AGameMode` | Regole sul server: squadre, ingresso, spawn, morte, assegnazione punti, inizio/fine partita. |
| `AERGameState : AGameState` | Punteggi Alpha/Omega, scadenza partita, risultato condiviso. |
| `AERPlayerState` | Nome, squadra, uccisioni, morti e teamkill del singolo giocatore. |
| `AERPlayerController` | Input del proprietario, creazione della UI e richiesta di respawn anche senza Pawn vivo. |
| `AERCharacter : ACharacter` | Movimento, camera, mesh, riferimento all'arma e richieste di combattimento. |
| `UERHealthComponent` | Salute, stato morto e scadenza della protezione; modifiche autorevoli e notifiche locali. |
| `UERWeaponData : UDataAsset` | Parametri del fucile, riferimenti a mesh, suoni ed effetti. |
| `AERWeapon` | Munizioni, ricarica, cadenza e creazione dei proiettili sul server. |
| `AERProjectile` | Collisione, movimento del proiettile e singola applicazione del danno sul server. |
| `AERTeamPlayerStart : APlayerStart` | Identificatore della squadra per ogni punto di nascita. |

`GameMode` è disponibile solo sul server; `GameState` e `PlayerState` rendono disponibili ai client i dati che devono conoscere. Usare la coppia **AGameMode/AGameState** per il ciclo della partita. Il `PlayerController` remoto non è disponibile su ogni client: i dati dello scoreboard appartengono ai `PlayerState`. [E3], [E4]

Creare i Blueprint `BP_ERGameMode`, `BP_ERCharacter`, `BP_ERWeapon_Rifle` e `BP_ERProjectile`. Creare `DA_Rifle_A` da `UERWeaponData`. Impostare classi e riferimenti nell'Editor, senza percorsi di asset dispersi nel codice.

Nel modulo C++ includere le dipendenze effettivamente usate: quelle del template, `EnhancedInput` e `UMG`; aggiungere `Slate`/`SlateCore` se il codice li usa. Il primo collegamento per IP non richiede un'integrazione Steam o EOS.

### 4.3 Replica e dati locali

| Dato/azione | Chi decide | Chi deve riceverlo |
|---|---|---|
| Movimento | Percorso di rete di CharacterMovement, con verifica server | Giocatori interessati. |
| Danno, morte, punteggi | Server | Stato necessario a tutti i partecipanti. |
| Caricatore e riserva | Server | Proprietario dell'arma. |
| Stato ricarica/arma equipaggiata | Server | Proprietario e altri giocatori per le visuali. |
| Squadra e statistiche | Server | Tutti tramite PlayerState. |
| Input, menu e sensibilità mouse | Client proprietario | Locali. |
| Lampo, audio e impatto | Conferma del colpo dal server; effetti locali | Giocatori interessati, senza decidere il danno. |

Attivare esplicitamente la replica degli Actor e dei componenti che la richiedono; registrare le proprietà replicate in C++. Usare proprietà replicate per lo stato persistente e RPC per le richieste/eventi. Una RPC multicast non sostituisce lo stato che un giocatore entrato dopo deve ricevere. [E2], [E5]

## 5. Fase 2 — Costruire la mappa e il movimento

### 5.1 Prima mappa giocabile

Creare `L_Depot` con blocchi semplici. Layout proposto: deposito compatto con due estremità per le squadre, un'area centrale e due passaggi laterali. Deve consentire di raggiungere uno scontro senza una lunga camminata e offrire coperture leggibili.

1. Creare pavimento, confini, pareti e coperture con mesh semplici o CubeGrid.
2. Collocare almeno **due `AERTeamPlayerStart` per squadra**.
3. Eliminare le linee di tiro dirette tra le aree di nascita opposte.
4. Verificare le dimensioni con la capsula del personaggio: porte attraversabili, coperture utili, gradini non bloccanti.
5. Aggiungere collisioni semplici e controllare pavimenti, angoli e zone fuori mappa.
6. Aggiungere illuminazione sufficiente per distinguere giocatori e ostacoli.
7. Salvare il livello e provarlo con il personaggio prima di decorarlo.

CubeGrid consente di creare geometria a griglia nel Modeling Mode ed è sufficiente per una prima costruzione a blocchi. [E13]

Se un pacchetto già acquisito contiene una mappa dimostrativa, aprirlo prima in un progetto di prova, verificare versione e plugin, poi migrare solo ciò che serve. Una scena dimostrativa non garantisce dimensioni, spawn, collisioni o prestazioni adatte a un TDM.

Per generare facilmente una variante in seguito, si può scrivere uno script Editor che posizioni moduli secondo una griglia e salvi `L_Depot`. Conservare quel risultato come normale mappa del gioco. Non introdurre generazione diversa su ciascun client; Python per Unreal è uno strumento di automazione dell'Editor, non il linguaggio runtime del gameplay. [E14]

### 5.2 Input e movimento

Usare **Enhanced Input**. Creare `IMC_Gameplay` e queste azioni:

| Azione | Tipo | Controllo proposto |
|---|---|---|
| `IA_Move` | Axis2D | WASD. |
| `IA_Look` | Axis2D | Mouse. |
| `IA_Jump` | Bool | Spazio. |
| `IA_Crouch` | Bool | Ctrl oppure C tenuto premuto. |
| `IA_Fire` | Bool | Tasto sinistro del mouse. |
| `IA_Reload` | Bool | R. |
| `IA_Scoreboard` | Bool | Tab tenuto premuto. |
| `IA_Menu` | Bool | Esc. |

Aggiungere il mapping al sottosistema Enhanced Input del giocatore locale. `Started`, `Completed` e `Canceled` permettono di distinguere pressione e rilascio: non usare una richiesta affidabile di sparo a ogni frame. [E6]

Conservare `ACharacter` e `CharacterMovementComponent`. Collegare movimento e rotazione ai percorsi del template, il salto a `Jump`/`StopJumping` e il crouch a `Crouch`/`UnCrouch`, abilitando la capacità di accovacciarsi. Non costruire un secondo sistema di replica con `SetActorLocation` inviato dal client: CharacterMovement dispone già del proprio percorso di previsione e correzione. [E7]

Mantenere il movimento provvisorio del template, eliminando eventuali abilità fuori specifica, e registrare i valori finali. La calibrazione del feeling originale viene dopo la prima partita completa.

Il Controller deve poter intercettare sparo/salto anche quando il Pawn è morto o non esiste. Se il giocatore è vivo, inoltra l'azione al Character posseduto; se è morto, usa il percorso di richiesta respawn descritto nella fase 5. Non legare questa richiesta soltanto al Pawn da distruggere.

### 5.3 Visibilità del personaggio

Configurare due rappresentazioni:

- **Prima persona:** braccia/corpo e arma adatti alla propria camera.
- **World/terza persona:** corpo e arma visibili agli altri giocatori.

Il template documentato attualmente utilizza una rappresentazione in prima persona che non compare nelle altre camere: verificare esplicitamente la mesh world. Nelle versioni con First Person Rendering, usare correttamente `First Person` e `World Space Representation`; con il percorso tradizionale, configurare `Only Owner See`/`Owner No See` e l'ownership coerente. [E1], [E8]

Iniziare con un'animazione semplice di locomozione. Le visuali leggono velocità, crouch e stato dell'arma; il prototipo non richiede la replica manuale di ogni osso. Distinguere Alpha/Omega con colore e indicatore del team, applicati anche quando il relativo `PlayerState` diventa disponibile sul client.

**Fase completata quando:** due finestre PIE, una host e una client, vedono reciprocamente corpo, arma, movimento, salto e crouch. Entrambe percorrono la mappa senza cadere attraverso il pavimento.

## 6. Fase 3 — Squadre, ingresso e avvio partita

1. Definire `EERTeam` con `None`, `Alpha`, `Omega`.
2. Sul server assegnare la squadra prima di scegliere il punto di nascita. Usare il ciclo di login e `HandleStartingNewPlayer` in modo che lo spawn avvenga una sola volta.
3. Nel `ChoosePlayerStart` filtrare i punti per squadra e controllare che la capsula possa comparire senza sovrapposizioni bloccanti.
4. Se un punto è occupato, provarne un altro; se non esiste un punto valido, mantenere il giocatore in attesa e riprovare dal server.
5. Creare il Pawn tramite il percorso `RestartPlayer` del GameMode; evitare di aggiungere uno spawn manuale duplicato.
6. Fare iniziare il match solo quando è presente almeno un giocatore per squadra. Prima mostrare “In attesa di giocatori” e consentire il movimento, senza danni.
7. Quando parte il match, azzerare le statistiche della partita e impostare `MatchEndServerTime = ServerTime + 600`.
8. Per nuovi ingressi a partita iniziata, inviare lo stato corrente senza azzerare punteggi o timer.

Implementare `ReadyToStartMatch_Implementation` con il requisito di almeno un giocatore valido per squadra, sostituendo il criterio di avvio predefinito. In `HandleStartingNewPlayer` gestire esplicitamente anche lo spawn durante `WaitingToStart`, tramite `RestartPlayer`, per consentire il movimento prima del match. Il successivo avvio non deve creare un secondo Pawn per chi ne possiede già uno valido.

Usare il tempo sincronizzato del `GameState`, per esempio `GetServerWorldTimeSeconds`, per ricavare quanto manca da una scadenza replicata. La scelta autorevole del termine della partita rimane sul server. [E3]

Configurare la capacità di quattro giocatori nel percorso di ammissione del server; disabilitare un pulsante nel menu non è un limite di ingresso sufficiente. Il nome inserito nel menu viene inviato dal proprietario e validato dal server prima di finire nel `PlayerState`: usare un valore predefinito se vuoto e una lunghezza massima ragionevole.

**Fase completata quando:** i primi due giocatori sono in squadre opposte, un terzo viene assegnato coerentemente e chi entra dopo vede la partita già in corso.

## 7. Fase 4 — Fucile, proiettili e ricarica

### 7.1 Equipaggiamento

Creare `DA_Rifle_A` con i valori della sezione 2.2. Il server crea l'arma alla nascita del Pawn e ne imposta `Owner`/`Instigator` correttamente. La catena di ownership deve risalire al Character e al Controller del giocatore. [E4]

Usare un socket della mano per la mesh world e un attacco adatto alla camera per quella in prima persona. Se il modello è statico, può bastare una Static Mesh; animazioni del caricatore e parti mobili possono arrivare dopo. L'asset visivo non deve contenere le regole di danno.

Tenere `MagazineAmmo`, `ReserveAmmo`, `bReloading` e i tempi autorevoli sul server. Le munizioni possono essere replicate al solo proprietario; l'Actor world dell'arma deve comunque essere visibile agli altri. Non impostarlo interamente come rilevante solo per il proprietario.

### 7.2 Contratto delle richieste

| Richiesta | Dove arriva | Comportamento server |
|---|---|---|
| `ServerStartFire` | Character posseduto | Attiva l'intenzione di sparare e tenta il primo colpo rispettando la cadenza. |
| `ServerStopFire` | Character posseduto | Disattiva l'intenzione e ferma il timer di fuoco. |
| `ServerRequestReload` | Character posseduto | Avvia la ricarica solo se consentita. |
| `ServerRequestRespawn` | PlayerController | Verifica la richiesta quando il giocatore è morto. |

Start/stop e reload sono eventi discreti e possono essere RPC `Reliable`. Non inviare vittima, danno o munizioni scelti dal client. Le normali richieste non eseguibili — caricatore vuoto, ricarica già attiva, Pawn morto — vengono semplicemente rifiutate nell'implementazione. Una `_Validate` che restituisce false può disconnettere il client: non usarla come normale gestione del caricatore vuoto. [E5]

### 7.3 Algoritmo del fuoco

Con il tasto tenuto premuto, un timer **sul server** gestisce i colpi. Per ciascun tentativo:

1. Verificare match attivo, Pawn vivo, arma corrente, assenza di ricarica e almeno una munizione.
2. Verificare `Now >= NextAllowedShotTime`. Pressioni ripetute di Start non devono anticipare il colpo successivo.
3. Ricavare punto di vista e direzione di mira dal Character lato server. Usare la control rotation disponibile nel percorso di movimento; verificare anche la mira da fermo. Se serve un aggiornamento aggiuntivo, limitarne frequenza e valori, senza una RPC Reliable per frame.
4. Calcolare un punto di mira con una traccia della vista. Questa traccia serve a dirigere il proiettile: non applica danno istantaneo.
5. Calcolare l'origine fisica del colpo da un riferimento autorevole del Character/arma. Verificare che la canna non si trovi oltre o dentro una parete; controllare anche il percorso verso l'origine prevista.
6. Sottrarre una munizione, aggiornare il prossimo istante consentito e terminare subito la protezione del tiratore.
7. Creare il proiettile sul server, oppure risolvere un impatto immediato se l'ostacolo è davanti alla canna.
8. Notificare gli effetti del colpo. Fermare il ciclo se non può continuare; per questo MVP una nuova ricarica richiederà un nuovo comando di fuoco.

Non dipendere esclusivamente da un socket la cui posizione richiede un'animazione non aggiornata sul server. Un riferimento fisico dedicato e coerente con la posa è sufficiente per il prototipo; i socket possono gestire le visuali.

### 7.4 Proiettile e danno

`AERProjectile` usa una collisione semplice e `UProjectileMovementComponent`, con movimento swept, nessun rimbalzo, velocità configurabile e scadenza. Definire un profilo di collisione che blocchi mondo e capsule dei personaggi; ignorare il tiratore e la sua arma.

Impostare `bReplicates = true` e `SetReplicateMovement(true)` sul proiettile. La collisione è il Root Component e l'Updated Component del movimento. Per questo primo modello simulare movimento e collisioni sul server; le copie client seguono gli aggiornamenti replicati, senza una seconda simulazione che possa fermare o distruggere autonomamente il proiettile. Eventuali interpolazioni rimangono visuali.

Lo spawn e il danno avvengono sul server. Le copie client servono a mostrare il proiettile: non devono applicare danno o decidere autonomamente morti. Una guardia `bImpacted` impedisce elaborazioni multiple dello stesso impatto.

All'impatto, il server:

1. Marca il proiettile come già risolto e ne ferma il movimento.
2. Identifica il bersaglio e controlla che sia vivo e non protetto.
3. Applica 36 HP di danno, anche se il bersaglio è della stessa squadra.
4. Invia la notifica cosmetica dell'impatto e conclude la vita del proiettile.

Per proiettili rapidi la rappresentazione replicata può durare troppo poco per essere osservata bene: mantenere un effetto d'impatto e un tracer cosmetico separabili dall'Actor che decide il danno. Non affidare l'effetto soltanto alla collisione locale del client.

Il primo MVP non compensa retroattivamente il movimento dei bersagli per la latenza e non deve essere descritto come rete competitiva già ottimizzata. La guida multiplayer Epic offre un riferimento pratico per proiettili e salute replicata. [E2]

### 7.5 Ricarica

Quando arriva la richiesta, controllare vivo, match attivo, nessuna ricarica in corso, caricatore non pieno e riserva maggiore di zero. Fermare il fuoco e avviare un timer server da 1,5 secondi. L'animazione rappresenta questo stato; un Animation Notify client non decide quando arrivano le munizioni.

Alla fine del timer verificare nuovamente arma, Pawn e vita corrente, poi calcolare:

```text
necessari = 30 - MagazineAmmo
trasferiti = min(necessari, ReserveAmmo)
MagazineAmmo += trasferiti
ReserveAmmo -= trasferiti
bReloading = false
```

Non ricaricare automaticamente in questa versione. Morire, cambiare livello o distruggere l'arma cancella il timer. Un callback della vita precedente non deve modificare l'equipaggiamento appena creato al respawn.

**Fase completata quando:** host e client si danneggiano a vicenda; tre colpi da 36 uccidono un bersaglio non protetto da 100 HP; pareti, cadenza, caricatore vuoto e ricarica funzionano in entrambe le direzioni.

## 8. Fase 5 — Morte, respawn, protezione e risultato

### 8.1 Un solo evento di morte

Il server modifica gli HP, limitandoli fra 0 e 100. Alla prima transizione a zero:

1. Impostare immediatamente `bDead` per evitare una seconda elaborazione.
2. Disabilitare movimento e combattimento del Pawn; fermare fuoco e ricarica.
3. Registrare la causa e aggiornare statistiche e punteggio una sola volta.
4. Impostare sul percorso del Controller `RespawnAllowedAt = ServerTime + 3`.
5. Passare a una camera di morte semplice; disattivare le collisioni del corpo se intralciano la partita.
6. Distruggere o disattivare l'arma senza creare pickup.

Un `OnRep` aggiorna le visuali e la UI, non assegna punti. Nel C++ il listen server non riceve automaticamente la propria notifica RepNotify come un client: richiamare la stessa funzione di aggiornamento locale anche dopo la modifica autorevole, così l'HUD dell'host rimane corretto. [E2]

### 8.2 Respawn richiesto dal giocatore

Durante i primi tre secondi mostrare il conto alla rovescia. Dopo mostrare **“Premi sparo o salto per rientrare”**. Come semplificazione esplicita della sezione 2.3, un input anticipato viene ignorato: qui non si riproduce l'accodamento della richiesta presente nell'originale.

Il Controller riceve l'input e invia `ServerRequestRespawn`. Il server verifica:

- Match ancora attivo e Controller connesso.
- Giocatore effettivamente morto e privo di un nuovo Pawn vivo.
- Tempo minimo trascorso.
- Nessuna richiesta già in elaborazione.

Dopo aver individuato uno spawn valido, il server esegue `UnPossess` del vecchio Pawn e lo distrugge, oppure lo conserva esclusivamente come corpo visivo non posseduto. Solo quando il Controller non possiede più il Pawn morto chiama `RestartPlayer` per creare e possedere il nuovo personaggio. Conservare sul Controller lo stato di attesa e `RespawnAllowedAt`, replicati al proprietario: input e countdown devono funzionare anche senza il vecchio Pawn.

Il nuovo Pawn nasce con 100 HP e 30+90 munizioni; squadra e statistiche restano nel `PlayerState`.

**Consumare la pressione che ha richiesto il respawn:** il medesimo click non deve sparare con il nuovo Pawn e annullare subito la protezione. Richiedere rilascio e nuova pressione per la successiva azione di sparo o salto. La UI deve riagganciarsi al nuovo Pawn, evitando riferimenti persistenti al corpo morto.

Se il giocatore esce, cancellare le attese e non creare un Pawn per un Controller non più valido. [R4]

### 8.3 Protezione

Alla nascita impostare `ProtectionEndServerTime = ServerTime + 4`. La decisione di ignorare danni usa il tempo server e lo stato del bersaglio.

La protezione:

- È visibile nell'HUD e riconoscibile sul personaggio, per esempio con un indicatore temporaneo.
- Rimane attiva mentre si cammina, salta o ci si accovaccia.
- Termina prima di elaborare il primo colpo autorizzato del protetto, anche se il proiettile finirà contro una parete.
- Non viene rimossa da un tentativo di fuoco che il server rifiuta.
- Non viene rinnovata da una ricarica o da un aggiornamento grafico.

Sparo e scadenza della protezione sono regole server; il countdown client è solo una visualizzazione. [R1], [R4]

### 8.4 Fine partita

Quando una squadra raggiunge 50 punti o il tempo termina, il server conclude la partita una sola volta, registra vincitore/pareggio, blocca nuovo danno, fuoco e respawn e mostra i risultati.

Dopo 10 secondi avvia una nuova partita nella stessa mappa. Per il primo prototipo usare `ServerTravel` verso la mappa già inclusa nel pacchetto; scegliere inizialmente travel non seamless, così ricreazione della partita, login e assegnazione squadre hanno un percorso semplice da collaudare. Ripristinare il nome locale dopo il collegamento e azzerare le statistiche della nuova partita.

Un generico `OpenLevel` sull'host durante una sessione non è il sostituto di `ServerTravel` per far seguire la nuova mappa ai client. [E9]

**Fase completata quando:** la morte viene contata una volta, il respawn rispetta tempo e input, la protezione segue le regole e tutti arrivano alla nuova partita senza Pawn duplicati.

## 9. Fase 6 — Menu, HUD e connessione

### 9.1 Mappe e classi predefinite

Creare `L_MainMenu`, con un GameMode menu derivato da `GameModeBase` e senza Pawn combattente. Può utilizzare il Controller del progetto configurato per la UI.

In **Project Settings → Maps & Modes** configurare:

- Game Instance: `UERGameInstance` o il suo Blueprint.
- Game Default Map: `L_MainMenu`.
- Editor Startup Map: `L_Depot`, per comodità di sviluppo.
- GameMode del livello `L_Depot`: `BP_ERGameMode`, con Character, Controller, GameState e PlayerState del progetto.
- Override del livello `L_MainMenu`: GameMode menu.

### 9.2 Widget minimi

| Widget | Contenuto |
|---|---|
| `WBP_MainMenu` | Titolo, nome, Ospita, indirizzo IP/porta, Connetti, Esci, stato/errore. |
| `WBP_HUD` | HP, munizioni, squadra, punteggi, tempo, ricarica, protezione e respawn. |
| `WBP_Scoreboard` | Nome, squadra, uccisioni, morti e teamkill, leggibili tenendo Tab. |
| `WBP_PauseMenu` | Riprendi, ritorna al menu. Il match online continua. |
| `WBP_MatchResult` | Squadra vincitrice o pareggio, risultato, attesa nuova partita. |

Creare widget solo per il Controller locale. Cambiare input mode e visibilità del cursore fra menu e gameplay. All'apertura del menu annullare il fuoco; il rilascio perso mentre la UI cattura il mouse non deve lasciare una raffica attiva.

Aggiornare l'HUD dagli eventi di stato e dai riferimenti validi; derivare i countdown dalle scadenze server. Lo scoreboard legge i `PlayerState` disponibili nel `GameState`, senza cercare i Controller remoti. [E3], [E4]

### 9.3 Host e Join

Il menu deve funzionare anche in Shipping, senza console di sviluppo.

**Ospita:** aprire il livello noto `/Game/EclipseRecoil/Maps/L_Depot` con l'opzione `listen`. Se il caricamento fallisce, tornare a uno stato di menu utilizzabile.

**Connetti:** accettare per il primo MVP un indirizzo IPv4, con porta opzionale, per esempio `192.168.1.20:7777`. Validare formato e intervallo della porta, quindi chiamare `ClientTravel` dal Controller locale con l'indirizzo previsto. Gestire input vuoto, connessione rifiutata, server pieno e timeout con un messaggio comprensibile. [E9]

**Ritorna al menu:** il client chiude la propria connessione e apre `L_MainMenu`. Se esce l'host, gli altri devono ricevere la disconnessione e tornare al menu tramite il gestore degli errori del GameInstance.

Per la LAN usare l'IP locale dell'host e consentire sul suo firewall la porta scelta, normalmente UDP 7777 salvo configurazione diversa. Fuori dalla LAN servono anche raggiungibilità del server e una soluzione coerente per firewall/NAT: questo prototipo non implementa il collegamento automatico attraverso router o CGNAT. Distribuire lo ZIP e rendere raggiungibile una partita sono due attività distinte.

**Fase completata quando:** si può ospitare, collegarsi, giocare e tornare al menu usando soltanto pulsanti e controlli normali.

## 10. Fase 7 — Integrare gli asset scelti

Sostituire un elemento alla volta, ripetendo una breve prova multiplayer dopo ogni sostituzione.

1. **Personaggio:** verificare scheletro, proporzioni, animazioni e mesh world. Adattare la capsula solo se necessario e poi ricontrollare porte e coperture.
2. **Fucile:** assegnare mesh, socket e orientamento. Controllare sia la visuale locale sia quella dell'altro giocatore.
3. **Mappa:** sostituire i blocchi con moduli ambientali preservando percorsi, larghezze e aree di nascita.
4. **Materiali ed effetti:** distinguere le squadre, aggiungere lampo, impatto e suoni senza duplicarli sull'host.
5. **Licenze:** aggiornare inventario e crediti per i contenuti davvero inclusi.

Se il pacchetto contiene un livello completo, non sovrapporre automaticamente il suo GameMode e la sua logica alle classi del progetto. Controllare Level Blueprint, plugin, riferimenti ad altri livelli e contenuti non necessari prima del packaging.

Ottimizzare sulla macchina di riferimento: scegliere un obiettivo esplicito, per esempio 60 FPS a 1080p sul PC di prova, e registrare hardware e impostazioni. È un obiettivo proposto, non una prestazione garantita. Se la mappa è troppo pesante, ridurre prima gli elementi che il profilo individua come costosi; non disabilitare funzioni a caso.

**Fase completata quando:** il gioco continua a rispettare le stesse regole con gli asset finali del prototipo e il contenuto inserito ha una provenienza documentata.

## 11. Fase 8 — Produrre un pacchetto distribuibile

### 11.1 Prima build locale

1. Salvare tutte le mappe e gli asset modificati.
2. Includere esplicitamente nel cook `L_MainMenu` e `L_Depot`, oltre ai contenuti referenziati necessari.
3. Compilare e creare un pacchetto **Development** per la piattaforma scelta.
4. Avviarlo fuori dall'Editor e controllare menu, caricamento e connessione.
5. Eseguire il test con due processi separati e poi su due computer nella stessa LAN.
6. Creare il pacchetto **Shipping** e ripetere la prova tramite menu.
7. Verificare i prerequisiti runtime della piattaforma su una macchina senza ambiente di sviluppo.

Unreal esegue fasi di build, cook, stage e package per ottenere l'applicazione standalone. Il risultato non è il solo eseguibile: distribuire l'intera cartella prodotta, con le sue dipendenze e i contenuti cucinati. L'Editor non serve al destinatario. [E10]

Nomi proposti:

- `EclipseRecoil-0.1.0-Windows-x64.zip`
- `EclipseRecoil-0.1.0-Mac-arm64.zip`, solo dopo una build e un test reali di quella destinazione.
- `EclipseRecoil-0.1.0-Linux-x64.zip`, quando la relativa toolchain è stata predisposta.

Ogni piattaforma richiede il proprio pacchetto. Per macOS considerare anche firma e distribuzione secondo le regole della piattaforma prima di una release ampia. Non dichiarare una piattaforma supportata soltanto perché Unreal la supporta in generale.

### 11.2 GitHub Actions, dopo la prima build riuscita

Il percorso iniziale consigliato è un **runner self-hosted** sulla macchina che sa già compilare il progetto. GitHub Actions coordina il lavoro; il runner contiene motore, toolchain e spazio sufficiente. Un runner ospitato da GitHub è un'altra possibilità, ma richiede di predisporre il motore e valutare risorse, tempi e costi. [G2]

Preparare un workflow che:

1. Parta manualmente all'inizio, poi eventualmente da un tag di versione.
2. Recuperi sorgenti e file LFS reali del repository privato.
3. Verifichi che versione del motore e toolchain siano quelle registrate.
4. Esegua `RunUAT`/`BuildCookRun` con la configurazione già verificata localmente.
5. Raccolga log utili se la build fallisce.
6. Archivi soltanto l'output distribuibile in uno ZIP e lo carichi come artifact.

Usare un profilo del **Project Launcher** per ottenere una configurazione funzionante e la relativa chiamata UAT. Il punto di ingresso si trova in `Engine/Build/BatchFiles/RunUAT.bat` su Windows e `RunUAT.sh` negli ambienti corrispondenti. Percorsi, opzioni e toolchain del workflow vanno derivati da una build reale: questa guida non contiene un workflow dichiarato funzionante senza averlo eseguito. [E15]

Non caricare sul repository l'intero motore o distribuire l'Editor insieme al gioco. Per release e condivisione usare il pacchetto compilato, rispettando licenze Unreal e dei contenuti. [L1], [L2], [L4]

### 11.3 Condividere il prototipo

| Destinazione | Uso consigliato |
|---|---|
| Google Drive o altro spazio file | Primo ZIP per pochi tester; condividere la cartella completa compressa e brevi istruzioni. |
| itch.io | Pagina del gioco, versioni per piattaforma, download più ordinati; accesso ristretto se il test non è pubblico. |
| GitHub Releases | Download associati alle versioni; una release di repository privato richiede accesso al repository. |

Su itch.io è possibile caricare gli aggiornamenti con butler; inizialmente è sufficiente anche un caricamento manuale. Per un test privato scegliere controlli di accesso effettivi: una pagina semplicemente non elencata non equivale a una pagina riservata. [D1], [D2]

Per GitHub si può tenere il progetto privato e, se desiderato, usare un repository pubblico separato per distribuire soltanto i pacchetti del gioco. Gli archivi automatici “Source code” non sono l'applicazione confezionata. [G3]

Allegare una breve `README_PLAYER.txt`: requisiti verificati, avvio, controlli, istruzioni LAN, versione, crediti e problemi noti. Per continuare la convenzione del progetto originale, scrivere eventuali `README.md` e `AGENTS.md` del repository in inglese; questa guida operativa rimane in italiano. [R7]

**Fase completata quando:** un tester su un secondo computer senza Unreal Editor estrae il pacchetto, lo avvia e completa una partita LAN.

## 12. Piano di verifica e definizione di “gioco base finito”

Usare prima PIE con più giocatori, poi processi separati e infine due computer. Il test in una sola istanza Editor non basta a dimostrare il comportamento di una build distribuita. La documentazione Epic descrive le opzioni per eseguire le prove multiplayer. [E16]

Registrare gli esiti in `docs/TEST_RESULTS.md`, indicando versione del progetto, motore, data, piattaforma, metodo e risultato. **Tutti i test della tabella sono inizialmente DA ESEGUIRE.**

| ID | Prova | Risultato atteso |
|---|---|---|
| T01 | Host e client in processi separati | Collegamento, corpi e armi visibili reciprocamente. |
| T02 | Movimento, salto e crouch | Nessun blocco anomalo nella mappa; stato coerente sugli altri client. |
| T03 | Ingresso di 2, poi 3–4 giocatori | Squadre assegnate correttamente, nessun Pawn duplicato. |
| T04 | Danno host → client e client → host | HP 100 → 64 → 28 → 0 con tre colpi uniformi senza protezione. |
| T05 | Parete fra tiratore e bersaglio | Nessun danno attraverso il muro, anche con canna vicina all'ostacolo. |
| T06 | Raffica e pressioni rapide | Intervallo server di almeno 0,100 s tra colpi autorizzati. |
| T07 | Caricatore vuoto | 30 colpi consumano 30 munizioni; il tentativo successivo non causa danno. |
| T08 | Ricarica parziale | Con 20+90 munizioni, dopo 1,5 s si ottiene 30+80. |
| T09 | Riserva insufficiente | Con 20+5 si ottiene 25+0; nessuna munizione negativa. |
| T10 | Morte durante ricarica | Nessun callback della vecchia arma altera il nuovo equipaggiamento. |
| T11 | Friendly fire e teamkill | Stesso danno; `Deaths +1` alla vittima e `TeamKills +1` al responsabile; `Kills` e punteggi di entrambe le squadre invariati. |
| T12 | Respawn | Nessun rientro prima di 3 s; dopo serve un nuovo input; squadra e statistiche persistono. |
| T13 | Input di respawn | Il click che fa rinascere non spara immediatamente. |
| T14 | Protezione | Dura 4 s; movimento consentito; primo colpo autorizzato la interrompe. |
| T15 | Morte e loot | Una sola morte registrata, niente combattimento da morto, nessun pickup generato. |
| T16 | Limite punti e tempo | Fine partita uguale per tutti; pareggio gestito; nuova partita dopo 10 s. |
| T17 | Ingresso a match avviato | Timer, team, punteggi e stati iniziali corretti. |
| T18 | UI dell'host e respawn | HUD aggiornato anche sul listen server e collegato al nuovo Pawn. |
| T19 | Uscita durante attesa/ricarica | Nessun respawn fantasma, timer rimossi, partita ancora utilizzabile. |
| T20 | IP errato, server pieno, host chiuso | Messaggio comprensibile e ritorno al menu senza blocco. |
| T21 | Breve prova con latenza/perdita simulate | Nessun doppio danno, raffica bloccata attiva o respawn duplicato. |
| T22 | Shipping su secondo computer | Avvio senza Editor, menu operativo, partita LAN completa. |
| T23 | Contenuti e crediti | Asset previsti presenti, licenze registrate, nessun asset sorgente distribuito per errore. |

Per testare il limite punti senza giocare cinquanta eliminazioni ogni volta, usare una configurazione di sviluppo a due punti e una durata breve. Ripristinare i valori della specifica per il pacchetto finale. Se si aggiungono test automatici, privilegiare regole con casi limite reali — trasferimento munizioni, attribuzione punti, soglia del respawn — senza sostituire le prove di rete.

Il gioco base è finito quando tutte le funzioni previste sono implementate e i test applicabili sono superati con evidenza registrata. Non basta che il codice compili o che la singola finestra PIE si apra.

## 13. Backlog operativo da eseguire in ordine

| Attività | Risultato verificabile |
|---|---|
| B01 — Ambiente e progetto | Versione fissata, template C++ compilato, Git/LFS configurati. |
| B02 — Classi e mappe | Framework, Blueprint principali, `L_MainMenu` e blocchi di `L_Depot`. |
| B03 — Movimento e visuali | Due giocatori visibili, input corretto, salto/crouch. |
| B04 — Squadre e stato partita | Assegnazione, spawn, attesa, avvio e timer condiviso. |
| B05 — Arma e danno | Data Asset, fuoco server, proiettili, collisione, HP. |
| B06 — Munizioni e ricarica | Cadenza, clip/riserva, timer e interruzioni corretti. |
| B07 — Morte e respawn | Punti, attesa, richiesta, nuovo Pawn e protezione. |
| B08 — UI e travel | Host/join, HUD, scoreboard, risultato, ritorno e nuova partita. |
| B09 — Asset e mappa | Contenuti scelti integrati, crediti e compatibilità verificati. |
| B10 — Pacchetto e collaudo | Development e Shipping, secondo PC, risultati registrati. |
| B11 — Automazione facoltativa | Build locale riprodotta su runner; artifact scaricabile. |
| B12 — Consegna ai tester | ZIP verificato, istruzioni giocatore, pagina o link di download. |

Non iniziare B09 come sostituto di B03–B08: completare prima il ciclo movimento → combattimento → morte → respawn → risultato con contenuti provvisori. La CI è utile dopo che quel ciclo produce già una build locale valida.

## 14. Istruzioni da consegnare a Codex

Il testo seguente può accompagnare questo file quando si richiede l'implementazione. È un incarico proposto per un passaggio successivo, non un'azione già svolta.

> Implementa il primo prototipo di Eclipse Recoil in un nuovo progetto Unreal seguendo `ECLIPSE_RECOIL_UNREAL_BASE.md`.
>
> Usa la versione Unreal disponibile e registra ambiente e toolchain. Parti dal template First Person C++, con Blueprint per asset e UI. Segui il backlog B01–B10 in ordine e mantieni Alpha/Omega, 100 HP, friendly fire, un fucile a proiettili, 30+90 munizioni, 600 RPM, danno uniforme 36 e ricarica di 1,5 secondi.
>
> Implementa server autorevole per combattimento e partita. Il respawn richiede un input dopo almeno 3 secondi; la protezione dura 4 secondi e termina al primo sparo autorizzato. Conserva i dati del giocatore nel PlayerState attraverso la rinascita del Pawn.
>
> Usa placeholder finché le meccaniche non funzionano. Inserisci soltanto asset disponibili con licenza verificata. Mantieni il repository originale come riferimento; non modificarlo per creare il port.
>
> A ogni fase, produci codice e configurazione coerenti, compila quando gli strumenti lo consentono e registra le verifiche effettive. Se non puoi usare l'Editor per creare o collegare un asset binario, prepara uno script Editor eseguibile quando possibile oppure indica esattamente classe, nome, cartella, proprietà e collegamenti da impostare. Non fingere di aver creato, compilato o testato ciò che non hai potuto eseguire.
>
> I bot di collaudo opzionali sono stati approvati dall'utente; non aggiungere altre armi, matchmaking o funzionalità rinviate. L'ambito corrente è il gioco locale con l'Editor; packaging e distribuzione restano rinviati. Descrivi eventuali passi manuali rimanenti e problemi noti. La pubblicazione esterna segue la successiva decisione dell'utente sulla destinazione.

## 15. Riferimenti

Fonti consultate per preparare la guida: documentazione ufficiale Epic/GitHub/itch.io e codice del progetto originale. Le pagine online possono cambiare: selezionare nella documentazione Epic la versione di Unreal effettivamente utilizzata. Le licenze da applicare sono quelle dei contenuti acquisiti, non un nome di licenza dedotto dal prezzo.

### Progetto originale, revisione fissata

- [R1 — Configurazione TDM][R1]
- [R2 — Configurazione dell'arsenale][R2]
- [R3 — Documentazione gameplay][R3]
- [R4 — Stato del personaggio, respawn e protezione][R4]
- [R5 — Valori predefiniti delle armi][R5]
- [R6 — Gestione server e punteggi][R6]
- [R7 — Vincoli e convenzioni del progetto][R7]
- [R8 — Richiesta di respawn dal client originale][R8]

### Unreal Engine

- [E1 — First Person Template][E1]
- [E2 — Multiplayer Programming Quick Start][E2]
- [E3 — Game Mode and Game State][E3]
- [E4 — Actor Owner and Owning Connection][E4]
- [E5 — Remote Procedure Calls][E5]
- [E6 — Enhanced Input][E6]
- [E7 — Networked Character Movement][E7]
- [E8 — First Person Rendering][E8]
- [E9 — Travelling in Multiplayer][E9]
- [E10 — Packaging Your Project][E10]
- [E11 — Compiling Game Projects][E11]
- [E12 — Linux Development Requirements][E12]
- [E13 — CubeGrid][E13]
- [E14 — Scripting the Unreal Editor Using Python][E14]
- [E15 — Build Operations e UAT][E15]
- [E16 — Testing Multiplayer][E16]
- [E17 — Migrating Assets][E17]

### Versionamento, distribuzione e licenze

- [G1 — Git Large File Storage][G1]
- [G2 — GitHub Actions: self-hosted runners][G2]
- [G3 — GitHub Releases][G3]
- [D1 — itch.io: access control][D1]
- [D2 — itch.io: aggiornamenti con butler][D2]
- [L1 — Fab End User License Agreement][L1]
- [L2 — Epic Content License Agreement][L2]
- [L3 — Creative Commons CC0][L3]
- [L4 — Unreal Engine End User License Agreement][L4]

[R1]: https://github.com/agea/eclipse-recoil/blob/c5e6fad873e4bbddf189c1be4ba555b916bacad5/config/csgopen/tdm.cfg
[R2]: https://github.com/agea/eclipse-recoil/blob/c5e6fad873e4bbddf189c1be4ba555b916bacad5/config/csgopen/arsenal.cfg
[R3]: https://github.com/agea/eclipse-recoil/blob/c5e6fad873e4bbddf189c1be4ba555b916bacad5/doc/csgopen/gameplay.md
[R4]: https://github.com/agea/eclipse-recoil/blob/c5e6fad873e4bbddf189c1be4ba555b916bacad5/src/game/game.h
[R5]: https://github.com/agea/eclipse-recoil/blob/c5e6fad873e4bbddf189c1be4ba555b916bacad5/src/game/weapons.h
[R6]: https://github.com/agea/eclipse-recoil/blob/c5e6fad873e4bbddf189c1be4ba555b916bacad5/src/game/server.cpp
[R7]: https://github.com/agea/eclipse-recoil/blob/c5e6fad873e4bbddf189c1be4ba555b916bacad5/AGENTS.md
[R8]: https://github.com/agea/eclipse-recoil/blob/c5e6fad873e4bbddf189c1be4ba555b916bacad5/src/game/game.cpp
[E1]: https://dev.epicgames.com/documentation/en-us/unreal-engine/first-person-template-in-unreal-engine
[E2]: https://dev.epicgames.com/documentation/en-us/unreal-engine/multiplayer-programming-quick-start-for-unreal-engine
[E3]: https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine
[E4]: https://dev.epicgames.com/documentation/en-us/unreal-engine/actor-owner-and-owning-connection-in-unreal-engine
[E5]: https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-procedure-calls-in-unreal-engine
[E6]: https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine
[E7]: https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine
[E8]: https://dev.epicgames.com/documentation/en-us/unreal-engine/first-person-rendering
[E9]: https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine
[E10]: https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project
[E11]: https://dev.epicgames.com/documentation/en-us/unreal-engine/compiling-game-projects-in-unreal-engine-using-cplusplus
[E12]: https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-requirements-for-unreal-engine
[E13]: https://dev.epicgames.com/documentation/en-us/unreal-engine/cubegrid-tool-in-unreal-engine
[E14]: https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python
[E15]: https://dev.epicgames.com/documentation/en-us/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine
[E16]: https://dev.epicgames.com/documentation/en-us/unreal-engine/testing-multiplayer-in-unreal-engine
[E17]: https://dev.epicgames.com/documentation/en-us/unreal-engine/migrating-assets-in-unreal-engine
[G1]: https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-git-large-file-storage
[G2]: https://docs.github.com/en/actions/concepts/runners/self-hosted-runners
[G3]: https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases
[D1]: https://itch.io/docs/creators/access-control
[D2]: https://itch.io/docs/butler/pushing.html
[L1]: https://www.fab.com/eula
[L2]: https://www.unrealengine.com/eula/content
[L3]: https://creativecommons.org/publicdomain/zero/1.0/deed.en
[L4]: https://www.unrealengine.com/eula/unreal


## Aggiornamento autorizzato dall'utente — 13 ottobre 2026

Prima di proseguire con effetti del fucile, tarare movimento e attraversamento:
personaggio 185 cm; corsa leggera 10 km/h predefinita, Shift cammina a 5 km/h;
inerzia in accelerazione/frenata; salto più basso e rapido. Gradini fino a 30 cm
automatici, salita fino a 80 cm rallentata con arma in mano, arrampicata fino
alle spalle anche quando raggiunte saltando. Interpretazione iniziale delle
spalle: 150 cm sopra i piedi correnti. Laboratorio separato con 20 blocchi
2×2 m alti da 10 a 200 cm a intervalli di 10 cm. Implementazione e risultati
aggiornati in DEVELOPMENT.md e TEST_RESULTS.md; packaging ancora rinviato.

Ulteriore richiesta del 13 ottobre: nascondere la mano sinistra con il fucile
in arrampicata, lieve oscillazione al passo e mouse più sensibile. Taratura
iniziale: scalar mouse 0,18 (+50%), bob cosmetico massimo 0,7 cm verticale.
