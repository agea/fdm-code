# Inventario asset — primo prototipo locale

| Origine / autore | Contenuto usato | Percorso | Compatibilità osservata | Condizioni / crediti |
|---|---|---|---|---|
| Template First Person / Arena Shooter, Epic Games, UE 5.8.3 | Manny semplice, animazione rifle idle e locomozione rifle per gli avversari | `Content/Characters/Mannequins/`, `Content/Variant_Shooter/Anims/ABP_TP_Rifle` | Caricato dal progetto originale e dal prototipo; vista FPS verificata con Metal | Contenuto incluso nel template installato. Registrare le condizioni applicabili prima della condivisione dei sorgenti o distribuzione; nessuna pubblicazione effettuata |
| Template Epic, UE 5.8.3 | Mesh skeletal del fucile | `Content/Weapons/Rifle/` | Caricata da Blueprint; orientamento FPS adattato | Stessa provenienza del template; non attribuita una licenza Fab per deduzione |
| Engine Basic Shapes / Epic | Cube e Sphere per blocchi e indicatore team | `/Engine/BasicShapes/` | Collisioni e proiettili verificati in processi locali | Dipendenza dal motore installato |
| Sviluppo Eclipse Recoil locale | Materiali, Blueprint, mappe e input creati dallo script | `Content/EclipseRecoil/` | Generati e salvati con Unreal Editor 5.8.3 | Contenuti propri del prototipo |
| Adattamento locale del template Epic | Posa statica FPS con entrambe le mani allineate al fucile | `Content/EclipseRecoil/Characters/A_ER_Rifle_HipPose` | Salvata via Editor e verificata con Metal | Derivata da `MF_Rifle_Idle_ADS`; stessa provenienza e condizioni del template |

Documentazione della provenienza del template:
https://dev.epicgames.com/documentation/en-us/unreal-engine/first-person-template-in-unreal-engine

I restanti contenuti del template sono stati conservati senza essere attivati dal
GameMode del port. Non sono stati copiati asset del vecchio gioco.

## Libreria Fab e Lyra

Il Launcher Epic è stato aperto e l'installazione 5.8.3 confermata. Il dialogo
iniziale “File di progetto non associati” non ha consentito di completare
l'ispezione della libreria attraverso i controlli UI disponibili durante questa
sessione. Nessun pacchetto Fab esterno è stato importato e nessuna licenza o
versione compatibile è stata presunta. Lyra non è stato trovato tra i progetti
locali ispezionati e non è stato aggiunto come dipendenza.

Per questa fase il materiale del template è sufficiente. La scelta successiva
sarà un pacchetto ambientale modulare adatto al deposito; verificare nome,
autore, URL Fab, licenza acquisita, versioni UE e plugin prima di importarlo.
Mantenere le cartelle originali e migrare solo dipendenze effettivamente usate.


## Laboratorio movimento — 13 ottobre 2026

Mappa propria `Content/EclipseRecoil/Maps/L_MovementLab.umap`: venti cubi
Engine Basic Shapes da 2×2 m, quote 10–200 cm, cartelli TextRender e perimetro.
Input proprio `Content/EclipseRecoil/Input/IA_Walk.uasset`, collegato ai due
Shift nella mappatura esistente. Aggiornati i default di BP_ERCharacter e
BP_ERPlayerController tramite `scripts/setup_movement_lab.py`. Nessun nuovo
asset Fab importato per questa prova.

Rifinitura del passo: aggiornato soltanto lo scalar di `IA_Look` a 0,18 tramite
`scripts/tune_presentation.py`; nessuna nuova animazione o mesh importata.

## Feedback dello sparo — 13 ottobre 2026

| Origine / autore | Contenuto | Percorso | Compatibilità osservata | Condizioni / crediti |
|---|---|---|---|---|
| Adattamento locale del template Epic | Animazione FPS di 100 ms, derivata dalla posa hip-fire, con entrambe le impugnature conservate | `Content/EclipseRecoil/Characters/A_ER_Rifle_Fire` | Generata e salvata da Unreal; pose campionate con spostamento massimo 1,4115 cm e span tra mani invariato | Stessa provenienza della posa template; originale conservato |
| Template Epic, UE 5.8.3 | Suono di sparo provvisorio, già presente nella cartella GrenadeLauncher | `Content/Weapons/GrenadeLauncher/Audio/FirstPersonTemplateWeaponFire02` | Caricato dai default di BP_ERCharacter; nessun asset esterno importato | Contenuto del template installato; non è un campione rifle Fab |
| Sviluppo locale + Engine Basic Shapes | Materiale additivo per il lampo e impostazioni di attenuazione; mesh Sphere del motore | `Content/EclipseRecoil/Weapons/M_ER_MuzzleFlash`, `SA_ER_Rifle`; `/Engine/BasicShapes/Sphere` | Generati e salvati tramite Unreal Python | Materiale e configurazione propri; mesh dipendente dal motore installato |

Impatti proiettile: `Content/EclipseRecoil/Weapons/M_ER_BulletHole` è un decal
monocromatico procedurale con centro scuro, bordo scheggiato, crepe radiali e
rilievo leggero della cavità. Nessuna texture esterna; rigenerabile con
`scripts/setup_bullet_impact.py`.

Corona munizioni/dispersione: disegno Canvas nativo, senza nuovi asset raster,
mesh o font. Il codice HUD del vecchio Eclipse Recoil è stato consultato
come riferimento funzionale; nessuna texture del vecchio gioco è importata.
I nuovi parametri di precisione appartengono a `UERWeaponData` / `DA_Rifle_A`.

Rifinitura della corona: pallini da 3 px disegnati su Canvas, bianco 70% / nero
10%, senza bordo; una fascia con alpha sfumato attenua i bordi. Non richiede
texture o pacchetti esterni.

## Bersaglio mercenario — 13 ottobre 2026

Pacchetto `[ FREE STARTED PACK ] AAA Modular Mercenary System`, Waveforce Studio,
[scheda Fab](https://www.fab.com/listings/57082fbf-3f85-407e-98ef-d858c2eefba7),
importato dall'utente in `Content/Mercenary_Male_WaveforceStudio`. Le condizioni
della licenza acquisita non sono state verificate in questa sessione.

La prova usa `Mesh/SK_MER_ME_Outfit01`, duplicata con il suo skeleton in
`Content/EclipseRecoil/Characters/Mercenary/SK_ER_Mercenary_Target` e
`SKEL_ER_Mercenary_Target`. Originali conservati. La copia contiene i socket
HandGrip_R/L derivati dal Manny del template; il Blueprint proprio
`Content/EclipseRecoil/Core/BP_ERTrainingTarget` usa `ABP_TP_Rifle` esistente.
Compatibilità osservata su UE 5.8.3 / Metal: materiali caricati, posa col fucile
e piedi a terra. Mesh con tre LOD (24.068 / 6.915 / 3.793 vertici) e 16 slot
materiali. Generazione ripetibile con `scripts/setup_mercenary_target.py`.

Livree Alpha/Omega: dieci copie proprie di MaterialInstanceConstant in
`Content/EclipseRecoil/Characters/Mercenary/MI_ER_{Alpha,Omega}_*`, derivate da
Outer01, Pant01, Gloves01, Shoes01 e Hair01 del pacchetto. Parametri esistenti
TintColor_01/02, senza nuove texture; riferimenti degli slot nei default di
BP_ERTrainingTarget. Originali Fab invariati.

Riutilizzo per i bot mobili: BP_ERTrainingTarget e le due livree vengono usati
anche nel modo Patrol. Nessuna nuova mesh, texture o animazione importata.

Risposta mouse progressiva: curva C++ e slider Slate nativi; nessun nuovo asset
e nessuna modifica a IA_Look (scalar 0,18 conservato).

## Mercenario FPS — 13 ottobre 2026

Riutilizzata la mesh propria SK_ER_Mercenary_Target con sole sezioni LowerArm,
Hand, UpperArm e Gloves visibili (slot 2/4/7/11), su tutti i LOD. Nessuna nuova
mesh importata. Due copie proprie `MI_ER_Alpha_FPS_Gloves` e
`MI_ER_Omega_FPS_Gloves` danno il colore di squadra ai guanti, lasciando la pelle
invariata. Originali Fab conservati.

Animazioni proprie in `Content/EclipseRecoil/Characters/Mercenary`:
`A_ER_Mercenary_Rifle_HipPose`, derivata da MF_Rifle_Idle_ADS del template e
risolta sulle proporzioni del mercenario; `A_ER_Mercenary_Rifle_Fire`, ciclo
100 ms derivato dalla nuova posa. Entrambe usano lo skeleton proprio del
mercenario. Generazione tramite `scripts/setup_mercenary_first_person.py` e
solver riutilizzato di `create_rifle_pose.py`. Pose mannequin precedenti intatte.

Rifinitura FPS: rigenerate solo le due animazioni proprie del mercenario con
mani a scala 0,72 e orientamento delle impugnature del mannequin. Nessuna nuova
mesh/materiale; aggiornati i default FPS per impedire l'eredità della scala
della mano sul fucile. Asset Fab e posa world dei bot invariati.

Le animazioni FPS rifinite includono anche scala trasversale delle braccia
0,6, compensata sulle mani per mantenerle a 0,72. Mesh e materiali invariati.

Ripristino temporaneo per screenshot di confronto: i default FPS tornano a
SKM_Manny_Simple e alle pose mannequin precedenti, con i materiali originali.
Nessun asset eliminato; bot world e asset Fab invariati. Script dedicato
`restore_mannequin_first_person.py`.

Ripresa del mercenario FPS dal riferimento utente: rigenerate le due pose
proprie, nuovi default di inquadratura e sole sezioni LowerArm/Hand/Gloves
visibili. Nessun nuovo asset esterno o modifica alla mesh world condivisa.

## Danni e morte — 13 ottobre 2026

`Content/EclipseRecoil/Audio/S_ER_Hit_Impact`: SoundWave proprio mono 44,1 kHz,
160 ms, rumore filtrato e componente grave con inviluppo esponenziale. Generato
da `scripts/generate_hit_sound.py`, importato con `scripts/setup_damage_death.py`.
`SA_ER_Hit` configura la spazializzazione; nessun campione audio esterno.

`Content/EclipseRecoil/Core/BP_ERCorpse`: classe propria ERCorpse, mesh propria
SK_ER_Mercenary_Target, materiali Alpha/Omega esistenti e asset fisico originale
`/Game/Mercenary_Male_WaveforceStudio/Rig/PA_MER_ME`. Posa iniziale ADS del
template esistente; nessuna modifica agli asset Fab o animazione FPS. Default
suono/corpse assegnati a BP_ERCharacter e BP_ERTrainingTarget tramite Unreal.

Correzione compenetrazione FPS: nessun asset aggiunto o modificato. I componenti
esistenti braccia, fucile e fiammata vengono marcati FirstPerson a runtime;
la camera applica una scala di profondità 0.2 mantenendo l'inquadratura.

### Low Poly Weapons fornito dall'utente

Sorgenti in SourceAssets/LowPolyWeapons: meshes.zip, texture.zip, sedici FBX,
T_Gun.TGA, source.json con SHA-256 e report di importazione/associazione.
La provenienza Fab è dichiarata dall'utente; autore, URL della scheda e licenza
non presenti negli ZIP e non verificati. Nessuna pubblicazione del pacchetto.

Runtime in /Game/EclipseRecoil/Weapons/LowPoly: sedici SkeletalMesh con relativi
scheletri, texture T_Gun, materiale M_LowPolyGun, undici DA_* ERWeaponData,
materiali propri M_ER_Smoke/M_ER_Explosion. Sette principali, M1911, tre
granate; Optic_010 su M4 e Optic_02_Black su M107. Le altre ottiche e laser
sono importati e disponibili nell'Editor.

Ventidue pose idle/fire proprie in Characters/Mercenary/Weapons, generate
dal template e dal mercenario già presenti, con solver di impugnatura per
arma. Nuovi modelli/scheletri restano nel formato importato; nessun asset
Fab del mercenario o template modificato. Riferimenti nei DA_* e default
BP_ERCharacter/BP_ERTrainingTarget/BP_ERPlayerController/BP_ERProjectile.
Le partite usano solo uasset locali al progetto.

### Bomb Crack Ground fornito dall'utente

Archivio `bomb-crack-ground-00-free.zip`, FBX originale e otto texture conservati
in `SourceAssets/BombCrackGround`; `source.json` registra dimensioni e SHA-256.
Autore, URL e licenza non inclusi nello ZIP; nessuna distribuzione effettuata.

`scripts/bake_bomb_crack.py` usa Blender per derivare `SK_ER_BombDebris.fbx`:
seleziona 128 frammenti volanti dai 1.416 oggetti originali, esclude il terreno
fermo e le lastre con poco movimento, conserva traiettorie, rotazioni e UV.
Un'unica mesh rigida scheletrica, 1.693 vertici / 2.874 triangoli, due materiali,
92 fotogrammi a 30 Hz. Originali immutati; selezione documentata in bake-report.json.

`scripts/setup_grenade_explosion.py` importa attraverso Unreal in
`Content/EclipseRecoil/Effects/Grenade`: mesh, scheletro, A_ER_BombDebris,
sei texture colore/normal/roughness (limite runtime 1.024px), M_Bomb_Surface,
M_Bomb_Side con dissolvenza dither e BP_ERGrenadeExplosion. Metallness esclusa
per queste schegge non metalliche. Materiali persistenti assegnati tramite
helper editor limitato a quel percorso; nessuna modifica binaria degli uasset.
Il breve lampo usa Sphere del motore e M_ER_Explosion già nel progetto.
BP_ERProjectile riferisce il Blueprint dell'effetto; nessun caricamento da Downloads.

### Bullet Interactive Smoke fornito dall'utente

Progetto originale in versione 5.3, con plugin NiagaraFluids. Archiviati i
21 file dell'effetto e delle impostazioni originali, escludendo le mappe demo,
in `SourceAssets/InteractiveSmoke/InteractiveSmoke_source.zip`; source.json
contiene SHA-256 e dimensioni. Provenienza dalla cartella indicata dall'utente;
autore/licenza non forniti nei file letti, nessuna pubblicazione effettuata.

`scripts/migrate_interactive_smoke.py` esegue la migrazione Unreal del
Blueprint principale e delle dipendenze: undici package in Content/InteractiveSmoke.
Mappe, personaggi e impostazioni del progetto dimostrativo non sostituiscono FDM.
NiagaraFluids attivato nel progetto 5.8.3; modulo nativo dipende da Niagara.

`scripts/setup_interactive_smoke.py` duplica NS_InterSmoke_Sim come
`Effects/Smoke/NS_ER_InteractiveSmoke` e crea BP_ERSmokeCloud con riferimenti
nei default di BP_ERProjectile. Griglia massima 40, dominio 600x600x400 cm,
durata 12 s, colore originale grigio. L'interfaccia del volume genera render
target gestiti per istanza, evitando la texture condivisa del Blueprint demo.
Simulazione/renderers/materiali/texture dell'effetto originale preservati;
inizializzazione e ciclo di vita adattati al networking nativo di FDM.

Il renderer MD_Volume richiede il pass volumetric fog: supporto creato soltanto
a runtime durante le nubi, densità ambientale quasi nulla; nessuna mappa salvata
o modificata. Shader di Niagara e materiali compilati localmente per Metal.

### Twinblast / Drongo per le squadre

Pacchetti ParagonTwinblast e ParagonDrongo già importati dall'utente in Content.
Variante attiva Alpha: TwinBlast_ActionHero (ActionMovie); Omega: Drongo_GDC.
Mesh/skeleton derivati in Characters/Teams/Alpha e Omega; materiali *_Grounded,
IK Rig, retargeter e animazioni adattate di ABP_TP_Rifle nella cartella Teams.
Physics Asset propri PA_ER_Alpha_Ragdoll e PA_ER_Omega_Ragdoll: 16 corpi
anatomici e 15 articolazioni. I Physics Asset Paragon originali sono conservati
come riferimenti, ma non vengono usati per le morti.

Skin/occhi/capelli originali conservati. Materiali di abbigliamento/equipaggiamento
con texture e normali originali, desaturazione/tinta sobria, emissivo azzerato,
metallic 0,05 e roughness 0,82. Armi incorporate Paragon nascoste; Drongo senza
bazooka, boomerang, tubi della maschera e decorazioni d'osso. L'arsenale FDM
rimane l'equipaggiamento visibile. Protesi e proporzioni stilizzate degli eroi
non sono state rimodellate. FPS ancora mercenario con guanti oliva/marrone.

Generazione tramite setup_team_characters.py, ispezione persistente tramite
verify_team_characters.py. La richiesta non comprende acquisizione di nuovi
asset, pubblicazione né verifica delle condizioni di distribuzione dei pacchetti.

Il ragdoll usa sempre LOD0; gli originali Paragon sono mantenuti. Drongo senza
canister_pos e grenade_pos: accessori delle abilità originali che potevano
restare fluttuanti dopo la caduta. Le granate dell'arsenale FDM sono separate.

### Varianti selezionabili — 14 ottobre

Dieci FERTeamAppearance in Characters/Teams/{Alpha,Omega}/Skins con mesh,
materiali e ragdoll propri. Alpha usa ActionHero, ShadowOps, TwinBlast base e
Wasteland; Omega usa Drongo_GDC e Scavenger. Nessuna acquisizione aggiuntiva.
Cinque combinazioni per squadra, con differenze di abbigliamento e accessori;
riutilizzo del rig/animazioni retargettate di ciascun team. Il campo
Attachments contiene casco/zaino agganciati a head/spine_03 e materiali neutri.
Characters/Teams/Gear/SM_ER_FieldHelmet e SM_ER_FieldPack sono geometrie proprie,
generate e salvate attraverso Unreal con MeshDescription. Collisione disattivata.
Dominanti blu/fredde per Alpha e arancio/calde per Omega, dettagli neutri e
carnagioni naturali; guanti FPS aggiornati. Protesi Twinblast, proporzioni e
alcune forme stilizzate originali rimangono una limitazione del realismo.


### Avambracci FPS delle skin — 14 ottobre

Dieci SK_ER_*_Forearms in Characters/Teams/{Alpha,Omega}/FirstPerson,
derivati dalle mesh della skin tramite setup_skin_first_person_forearms.py
e l'helper editor ERFirstPersonForearms.cpp. Contengono solo triangoli di
avambracci/mani; nessun torso, braccio superiore, testa o arma Paragon.
UV e materiali conservati; bind, gerarchia e socket sono quelli di
SK_ER_Mercenary_Target per usare le undici coppie di pose FPS esistenti.
Riferimento FirstPersonMesh in FERTeamAppearance, nei default di player e
bersaglio. Nessuna modifica ai pacchetti vendor o alle mesh world/cadaveri.
Le protesi di TwinBlast rimangono nella geometria della skin scelta.

L'estrazione include tutti i LOD importati; i LOD generati privi di descrizione
vengono ricostruiti dal LOD0 ritagliato usando le loro impostazioni di riduzione.
Il viewmodel FPS mantiene LOD0 e i bounds originali del rig mercenario.


Stationary skin comparison (14 October 2026): reuses all five existing
AlphaSkins and OmegaSkins on BP_ERTrainingTarget. Ten live pawns and
transient per-target team starts are created at runtime; no additional
mesh, material or map asset. Variant numbering is 1–5 per team.


## Provenienza e dipendenze — revisione del 14 ottobre 2026

I tre URL Fab forniti dall'utente sono registrati nei source.json con autore
e nome: Low Poly Weapons VOL.1 / Simon Mercuzot; Bullet Interactive Smoke /
Theos; Bomb Crack_Ground_00 / Paul Yang. Hash e dati di importazione conservati.
La licenza accettata all'acquisizione non è nei file: ASSET_LICENSES.md distingue
Fab/legacy Epic, template UE Examples e possibile CC Attribution del modello
esplosione pubblicato dall'autore su Sketchfab. Nessun asset spostato/rimosso.
Audit completo in Saved/AssetAudit, sintesi versionabile in
ASSET_AUDIT_SUMMARY.json. Gli originali delle skin e il rig/pose FPS mercenario
sono protetti; circa 2,78 GiB di candidati richiedono revisione Unreal.


## Autonomia dal mercenario — 14 ottobre 2026

Nuovo Characters/FirstPerson: SKEL_ER_FirstPerson, due pose base rifle e
22 pose per undici armi. Preview su SK_ER_Twinblast_Alpha_Officer_Forearms,
stesse trasformazioni/bone data fitted, nessuna compatibilità vendor, nessun
post-process FPS. Dieci forearms restano in Teams/{Alpha,Omega}/FirstPerson.
Tutte le mesh/materiali del mercenario sono ora esterne alle dipendenze
correnti; world/corpse fallback usano le skin Alpha. Originali e vecchie
copie in Characters/Mercenary ancora presenti ma eliminabili: 272 package,
547,41 MiB, zero referencer esterni. Conservare FirstPerson e i termini di
provenienza del rig/pose derivati. Audit e ASSET_AUDIT_SUMMARY.json aggiornati.


## Pulizia effettiva — 14 ottobre 2026

272 package Mercenary e 3399 extra Paragon rimossi: ogni package era
coperto da backup verificato e il batch non aveva referencer esterni.
ParagonTwinblast rimasto: 315 package / 1292,64 MiB; ParagonDrongo:
185 package / 840,98 MiB. Conservati sorgenti di tutte le skin e chiusura
completa runtime/authoring. Totale attuale 1292 package; nessun asset
attivo alterato dalla pulizia. Dettagli in ASSET_AUDIT_SUMMARY.json,
CLEANUP_RESULT.json e REPOSITORY_CLEANUP.md. Backup locali riservati
fuori dalla cartella progetto, senza cambio di licenza o upload cloud.


## Separazione codice/asset · 14 ottobre 2026

La scelta successiva dell’utente è un repository pubblico `agea/fdm-code`
e tre submodule privati (personaggi, armi/effetti, mappe/Blueprint comuni).
Il vecchio `agea/fdm` resta privato. I percorsi Unreal sono invariati;
Content/SourceAssets sono collegamenti generati e ignorati. La struttura
attuale e i comandi sono in [ASSET_SUBMODULES.md](ASSET_SUBMODULES.md), che
sostituisce la precedente proposta di un solo submodule. Nessuna licenza
open-source del codice è stata scelta implicitamente.


## Postura prona — 14 ottobre 2026

Nessun nuovo asset binario: `UERPostureMesh` modifica la posa world dei rig team
esistenti dopo la valutazione delle animazioni. Mani/FPS e grip delle armi
restano sugli asset attuali. `ERPostureGallery` genera immagini temporanee sotto
Saved/Testing; non salva o modifica Blueprint, mappe, mesh o animazioni.


## Effetti hitscan — 14 ottobre 2026

- PNG utente «Quattro Impatti sulla Parete Trasparente.png», RGBA 1254x1254,
  conservata intatta come SourceAssets/BulletImpacts/FourBulletImpacts.png
  nel submodule privato weapons. Provenienza: allegato dell'utente; nessuna
  ulteriore licenza dedotta e nessun inserimento nel codice pubblico.
- Effects/Shots/T_ER_BulletImpacts, M_ER_BulletImpactAtlas e
  MI_ER_BulletImpact_1..4: texture alpha e quattro quadranti dello stesso atlas.
- NS_ER_Tracer e M_ER_Tracer: sistema Niagara proprio, costruito dall'emitter
  SingleLoopingParticle incluso in Epic Niagara; renderer sprite SourceMode=Emitter
  con posizione dal componente, allineamento/dimensione in parametri User
  ed emissione dorata nel materiale. Bounds comprendono tutto il segmento.
- NS_ER_Impact: copia privata di Epic Niagara DirectionalBurst, simulazione
  locale e M_ER_ImpactSprite proprio, morbido e caldo; scala/moto compatti
  e rimozione gestiti dal codice. Non è una libreria per tipi di superficie.
- Muzzle flash e audio esistenti conservati. Riferimenti nei Blueprint privati
  BP_ERCharacter (maps) e BP_ERWeapon_Rifle (weapons).
- Ricostruzione: scripts/setup_hitscan_effects.py tramite Unreal dopo la build.


### Revisione discreta dopo screenshot utente

NS_ER_Impact ora usa un solo sprite morbido di 1,6 cm per 35 ms, creato
dall'emitter Epic SingleLoopingParticle e dal materiale proprio
M_ER_ImpactSprite. Il precedente DirectionalBurst non viene più usato.
M_ER_Tracer attenuato; M_ER_MuzzleFlashSoft per il flash ridotto a 18 ms.
BP_ER_SpentCasing usa Engine BasicShapes/Cube e M_ER_CasingDark proprio:
nessun nuovo modello o sorgente esterno. Default CasingClass e materiale
flash in BP_ERCharacter. Gli asset restano nei submodule privati weapons/maps.


Ulteriore feedback del 14 ottobre: ripristinata la visibilità di M_ER_Tracer
(emissione dorata 4/2,2/0,65 e alpha 0,7), larghezza runtime 0,12 cm.
Conservati i materiali tenui di impatto/muzzle flash e il bossolo scuro.


Origine traccianti (14 ottobre): aggiornati solo MuzzleOffset nei data asset
DA_AK74, DA_M4_8, DA_Uzi, DA_SG_M4, DA_M249, DA_M107 e DA_M1911.
Punta dal centro della geometria frontale (ultimi 4mm lungo +X), via Unreal
MeshDescription e fix_weapon_muzzles.py. Nessuna modifica alle mesh Fab,
agli skeleton o alle pose dei grip. Nel solo M_ER_Tracer rimossa la
sfumatura longitudinale che nascondeva l'inizio del segmento; colore,
luminosità e sfumatura trasversale conservati.
