# Inventario asset — primo prototipo locale

| Origine / autore | Contenuto usato | Percorso | Compatibilità osservata | Condizioni / crediti |
|---|---|---|---|---|
| Template First Person / Arena Shooter, Epic Games, UE 5.8.3 | Manny semplice, animazione rifle idle e locomozione rifle per gli avversari | `Content/Characters/Mannequins/`, `Content/Variant_Shooter/Anims/ABP_TP_Rifle` | Caricato dal progetto originale e dal prototipo; vista FPS verificata con Metal | Contenuto incluso nel template installato. Registrare le condizioni applicabili prima della condivisione dei sorgenti o distribuzione; nessuna pubblicazione effettuata |
| Template Epic, UE 5.8.3 | Mesh skeletal del fucile | `Content/Weapons/Rifle/` | Caricata da Blueprint; orientamento FPS adattato | Stessa provenienza del template; non attribuita una licenza Fab per deduzione |
| Engine Basic Shapes / Epic | Cube e Sphere per blocchi e indicatore team | `/Engine/BasicShapes/` | Collisioni e proiettili verificati in processi locali | Dipendenza dal motore installato |
| Sviluppo Respawn & Rage locale | Materiali, Blueprint, mappe e input creati dallo script | `Content/EclipseRecoil/` | Generati e salvati con Unreal Editor 5.8.3 | Contenuti propri del prototipo |
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
mesh o font. Il codice HUD del vecchio Respawn & Rage è stato consultato
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
Mappe, personaggi e impostazioni del progetto dimostrativo non sostituiscono Respawn & Rage.
NiagaraFluids attivato nel progetto 5.8.3; modulo nativo dipende da Niagara.

`scripts/setup_interactive_smoke.py` duplica NS_InterSmoke_Sim come
`Effects/Smoke/NS_ER_InteractiveSmoke` e crea BP_ERSmokeCloud con riferimenti
nei default di BP_ERProjectile. Griglia massima 40, dominio 600x600x400 cm,
durata 12 s, colore originale grigio. L'interfaccia del volume genera render
target gestiti per istanza, evitando la texture condivisa del Blueprint demo.
Simulazione/renderers/materiali/texture dell'effetto originale preservati;
inizializzazione e ciclo di vita adattati al networking nativo di Respawn & Rage.

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
bazooka, boomerang, tubi della maschera e decorazioni d'osso. L'arsenale Respawn & Rage
rimane l'equipaggiamento visibile. Protesi e proporzioni stilizzate degli eroi
non sono state rimodellate. FPS ancora mercenario con guanti oliva/marrone.

Generazione tramite setup_team_characters.py, ispezione persistente tramite
verify_team_characters.py. La richiesta non comprende acquisizione di nuovi
asset, pubblicazione né verifica delle condizioni di distribuzione dei pacchetti.

Il ragdoll usa sempre LOD0; gli originali Paragon sono mantenuti. Drongo senza
canister_pos e grenade_pos: accessori delle abilità originali che potevano
restare fluttuanti dopo la caduta. Le granate dell'arsenale Respawn & Rage sono separate.

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

La scelta successiva dell’utente è un repository pubblico `respawn-and-rage/core`
e tre submodule privati (personaggi, armi/effetti, mappe/Blueprint comuni).
Il vecchio archivio privato originale resta privato. I percorsi Unreal sono invariati;
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


## Laboratorio distruzione — 14 ottobre 2026

Nuova mappa propria /Game/EclipseRecoil/Maps/L_DestructionLab, tre materiali
M_ER_Block_{Drywall,Brick,Concrete} e DA_ER_BlockMaterials in Art/Destruction,
generati via Unreal da scripts/setup_destruction_lab.py. Salvati nel mount
privato Assets/maps; nessun asset binario aggiunto al codice pubblico.
Pareti runtime ISM da /Engine/BasicShapes/Cube; pavimento statico con la stessa
mesh. Riutilizzati GameMode, personaggio, armi e input del progetto.
Nessun asset Fab aggiuntivo, detrito o plugin di distruzione importato.


## Milestone 2 distruzione — 14 ottobre 2026

In Art/Destruction: M_ER_BlockImpact_{Drywall,Brick,Concrete}, tre materiali
decal creati da espressioni proprie via Unreal, senza nuove texture esterne.
DA_ER_BlockMaterials riceve nuovi parametri di gameplay e riferimenti ai
materiali/decal e all'esistente Effects/Shots/NS_ER_Impact.
L_DestructionLab aggiunge tre ERBlockTarget (Cube Epic + TextRender HP) e un
riferimento a Effects/Grenade/BP_ERGrenadeExplosion già presente per FX debug.
Tutti gli asset nuovi/modificati rimangono in Assets/maps; gli asset riusati
di armi/effetti restano nei loro percorsi privati e non sono modificati.
Nessun nuovo pacchetto Fab, mesh, texture sorgente o audio importato.


## Texture PBR e piano procedurale — 14 ottobre 2026

ZIP 2K originale e 84 file verificati nel mount SourceAssets/ProceduralTextures
(Assets/maps); inventario e provenienza in PROCEDURAL_TEXTURES.md. Materiali
PBR in Art/ProceduralTextures, normali DX, tiling continuo in centimetri, nessuna
displacement geometrica. Surface del catalogo sostituite senza cambiare HP/danni/FX.
Nuova L_ProceduralLab con generatore nativo runtime, pavimenti piastrelle/legno e
controsoffitto OfficeCeiling001. Vetro parametrico dedicato in libreria; finestre
M3 vuote. Legno assi, pietra e metallo pronti come superfici, senza nuovi
profili balistici; asfalto sul basamento esterno indistruttibile. Tutte le modifiche binarie sono effettuate esclusivamente via Unreal.


## Milestone 4 — 14 ottobre 2026

Nessun nuovo asset binario o modifica artistica. Ottimizzazioni native runtime
ISM/identità, profiler e stress runner; riusate le mappe/materiali/FX privati
già presenti. CSV/log/trace Unreal Insights in Saved/Testing ignorati dal codice;
ricevuta aggregata del benchmark in docs/DESTRUCTION_BENCHMARK.json.

### Milestone 6 material bindings (15 October)

`DA_ER_BlockMaterials` now binds Wood to existing
`M_ER_PBR_wood_planks`, Stone to `M_ER_PBR_stone_wall_02`, Metal to
`M_ER_PBR_rusty_metal_04`, and Glass to `M_ER_Glass` in Art/ProceduralTextures.
The additive Unreal migration adds missing profiles/fills unset surfaces while
preserving original and artist-edited data. Seven gameplay profiles; no new
texture download or source archive, geometry/model import, or new map package.
Stairs, floor slabs and panes are generated from the existing normalized engine
cube at runtime. Catalog save remains in the private maps asset mount.

### Menu/session update — 15 October 2026

Session configuration, team bot counts and laboratory reload reserve are native
code changes only. No `.uasset`/`.umap` or licensed source asset was modified.


### Killhouse procedural template — 15 October 2026

New private assets created/saved through Unreal:
`EclipseRecoil/Art/MapTemplates/DA_Killhouse.uasset` and
`EclipseRecoil/Maps/L_Killhouse.umap`. Reference analysis and authored construction
JSON plus provenance notes are in maps `SourceAssets/MapTemplates`.

Runtime art reuses Engine/BasicShapes/Cube, existing Wood/Concrete/Metal catalog
surfaces, `M_ER_PBR_concrete_wall_006` for the arena floor and
`M_ER_PBR_rusty_metal_04` for the shell/ceiling. Weapon/FX/team assets are reused
unchanged. No CS:GO models or textures and no Factory vendor packages imported.
Industrial mesh substitution is deferred while the user's pack downloads.
The public code adds a reusable data-asset schema, bounded template generator,
read-only analyzer, additive Unreal setup and verification scripts.


### Factory fixed geometry and balanced Killhouse — 15 October 2026

The supplied `FactoryEnvironmentCollect` project contains8.7GB of content.
A selected dependency closure was migrated/renamed/saved through Unreal into
private maps `Content/EclipseRecoil/Art/FactoryEnvironment`:15 root meshes,
70 packages including materials/functions/textures,546,899,871 bytes. Selection,
inspection and migration receipts live in private SourceAssets/FactoryEnvironment.

The current template uses solid metal panels for the modular shed and roof,
metal beams/columns, platform decks, grill treads, platform pillars, whole cargo
containers, water containers and barrels. The foundation keeps the project PBR
surface. Legacy trial selections remain in the same private library; no licensed
asset enters the public code tree. Project Wood/Concrete destruction surfaces
and combat data are unchanged. The screenshot supplied by the user is preserved
privately in SourceAssets/MapTemplates/killhouse_reference.png for provenance.

DA_Killhouse revision2 and L_Killhouse were saved via Unreal. Spawn symmetry,
rear-access bases, internal stairs/open roof wells, parapets and denser paired
cover are authored in the private construction manifest. Vendor art stays on
fixed geometry; the reusable schema prevents it on destructible records.


### Killhouse solid surfaces and variation recipe — 15 October 2026

Revision3 saves only DA_Killhouse through Unreal. Private construction recipe:
SourceAssets/MapTemplates/killhouse_variations.json; migration receipt:
SourceAssets/FactoryEnvironment/variation_upgrade.json. No additional vendor
packages imported. Factory dependencies remain shared across seeds/maps.
Walkable base/tower roofs and treads now use Engine/BasicShapes/Cube with the
existing project Metal surface, preserving visible/collision continuity. Factory
platform/grill meshes remain in the private library and are no longer stretched
into walkable slabs. Fixed cargo/tanks/barrels/beams/columns/guards and modular
shed panels reuse the imported palette. The destruction catalog/materials and
L_Killhouse package retain their pre-migration hashes.


### Narrow-parapet traversal correction — 15 October 2026

Native movement and opt-in probes only; no `.umap`, `.uasset`, Factory asset or
material catalog changes. The existing100cm-high,20cm-thick parapets now supply
a supported assisted-climb landing through the generic ledge detector.


### Movement-only bot demonstrations — 15 October 2026

Native controller/probe changes only. Reuses the normal team meshes, jump/vault
movement and procedural prone/posture presentation. No binary asset or map edited;
the existing L_MovementLab provides the traversal integration-test fixtures.


### Dust Mirror / MOUT_Arabian — 15 October 2026

Private maps now contain DA_DustMirror and L_DustMirror, plus
Art/ArabianEnvironment (27 packages: three selected village/door meshes, the
stone-floor material and their material/texture closure). Source is the
user-imported MOUT_Arabian folder in FactoryEnvironmentCollect. The dependency
closure was staged and renamed/saved with Unreal; original source packages were
not modified. SourceAssets/ArabianEnvironment/migration_receipt.json records
paths, hashes and bytes. SourceAssets/MapTemplates/dust_mirror_{analysis,template}.json
records Source2 provenance and the authored procedural assembly. The original
project destruction catalog is unchanged. Detailed reconstruction/limits and
verification commands are in MAP_TEMPLATES.md.

The four imported Substance material instances have missing parents.
setup_arabian_materials.py reconstructs four owned world-aligned PBR parents
from their supplied color/normal/roughness/AO/metallic textures and reparents
only private copies. The final closure has31 packages (27 vendor plus4 owned);
source vendor materials remain untouched. The migration receipt hashes describe
the initial migration, before this explicit PBR repair. pbr_repair.json records
the material adaptation.


### Dust town revision3 and wooden gates — 15 October 2026

The private MapTemplates/DustTown folder adds three project-authored static
meshes (curved arch surround, left/right curved timber leaf) imported from
explicitly triangulated OBJ sources, plus five owned town materials (plaster,
paving, roof, iron and natural timber). They reuse the existing supplied
MOUT_Arabian and CC0 procedural texture libraries; no new vendor pack or Source2
render asset is imported. Runtime uses solid matching slabs, rotated wooden
collision planks and visual-only facade hardware. Reference images, recipe,
source hashes and art receipt remain in private SourceAssets/MapTemplates/DustTown.
DA_DustMirror revision3 stores the town graph/palette. L_DustMirror retains its
lighting/gameplay and adds the complete outer patrol circuit. Shared destruction
material catalog and Killhouse maps/template are unchanged.


Dust town revision4 changes DA_DustMirror's sampled dimensions and saves42
patrol actors in L_DustMirror. Existing owned arch/leaves/materials are reused
unchanged: opposite swings and diagonal passage are native transforms. The
explicit compact migration retains recipe/protected-package hashes in private
SourceAssets/MapTemplates/DustTown/compact_migration.json.


User-approved storage cleanup,15 October2026: completed Factory/Arabian scratch
projects and Source2 Dust analysis exports removed from Saved; active private
art/maps preserved. The duplicate texture ZIP was removed while all88 extracted
source files and provenance remain. At that checkpoint Belica and Phase were explicitly reserved
for future character imports and must not be pruned. See REPOSITORY_CLEANUP.md.

### Phase / Lt. Belica — 15 October 2026

User-supplied Epic Paragon packages, previously reserved locally in Content,
are mounted unchanged into private Assets/characters/Content/ParagonPhase and
ParagonLtBelica (3,276 original files at import). The later approval/cleanup
below supersedes the initial complete-source reservation. The private character repository also holds owned Belica/Phase team
skeletons, rifle retargeters/animations, ten derived meshes/material sets,
anatomical ragdolls and extracted/rebound FPS forearms. Base Belica, PolarStrike,
Biohazard and HeavyArmor provide Alpha silhouettes; Phase base/Kitty/Pandapack
provide Omega silhouettes, with project helmet/pack geometry and decorative
parts hidden. SpiderWitch is unused. Blue/navy and rust/orange clothing retains
neutral lower clothing/gear and natural face/hair treatment. Supplied skirt,
boots and futuristic armor geometry remain a limitation of the source models.

Core Blueprint defaults in private maps reference the full twenty-skin catalog.
Setup: scripts/setup_phase_belica.py; saved verification:
scripts/verify_phase_belica.py. Original SHA-256 inventory is retained locally
in Saved/Testing/phase-belica-original-hashes.json; no license receipt has been
independently verified or new public asset redistribution authorized.

Phase field finish hides the long hair sections, retains short natural hair,
turns lace sleeves into opaque field fabric and fits the helmet. The separate
refinement marker preserves later artist edits. Original vendor meshes remain
unchanged; supplied skirts/cropped jackets are still a source-model limitation.

Omega 9 (PhaseEngineer) uses a private fitted helmet mesh/material in its own
skin folder. Its shell has outward-facing polygon winding and a two-sided matte
material; the original shared gear is preserved. The rim is raised above the
brow. `fix_phase_engineer_helmet.py` applies this correction once to Character,
TrainingTarget and Corpse defaults, preserving later art edits.

Phase corpse defaults use an explicit collision-free physical-pose renderer;
the original sixteen-body ragdoll still handles only WorldStatic contacts. The
visual pose is copied in one batch after physics and frozen once settled. The
private PhysicsAssets use conservative limb/leg limits (45/35 degrees, with
smaller knee/ankle/wrist limits) to avoid excessively curled poses. Core
Blueprint defaults select this behavior; no additional vendor asset is required.

### Final character catalog and cleanup — 15 October 2026

All twenty labels now use `character · role`, defined in
scripts/character_catalog.py (Twinblast/Belica Alpha, Drongo/Phase Omega).
Character, TrainingTarget and Corpse catalogs agree; selection indices are stable.
The user's later approval supersedes the complete Phase/Belica source reservation.

Removed 3,210 unused character packages (2,329,822,505 bytes / 2.170 GiB) through
Unreal, including original variants/demos/effects and obsolete owned retarget
inputs. Current characters Content decreased from 6,428,771,571 to 4,098,883,603
bytes. Required source textures/materials and shared dependencies retain their
namespaces. The independent FirstPerson rig/poses are explicitly protected.

Only redundant defaults/editor references changed: owned mesh material defaults
now match the active appearance, hidden slots use a neutral material, skeleton
and animation/BlendSpace previews use owned meshes, and disabled capsule-shadow
asset links are empty. Visible materials, mesh geometry, retarget pose data,
physics profiles and weapon grips were preserved. Thirty-four assets with
outside referencers remain; they are shared with template/project content.
The current audit has no further removable character package.

Restore archived inputs before using historical generation scripts. Ordinary
play and saved-asset verification need no restoration. The private rollback
archive contains removed files and pre-change versions of modified files, with
SHA-256 verification; see CHARACTER_CLEANUP_RESULT.json and REPOSITORY_CLEANUP.md.


## Requested Sketchfab weapons — 15 October 2026

Nine downloaded original-format model archives and their API metadata, author
credits, license snapshots and SHA-256 receipts are preserved privately under
weapons SourceAssets/SketchfabWeapons. The arsenal imports ten weapon meshes
plus the Milkor GL_slug1 projectile, project-owned data/materials and authored
mechanical animations. Source arm geometry remains archived; FPS hands use the
existing fitted skin rig with adapted source motion. No character packages or
existing low-poly assets are removed. Runtime packages use Weapons/Sketchfab;
character/training-target/controller defaults remain in the existing maps mount.

The original request and English translation are in docs/references/weapons.it.md
and docs/WEAPONS_REFERENCE_EN.md. See WEAPON_IMPORT.md for model/statistic mappings,
source frames, rebuild steps and the retained legacy catalog. The Milkor model
is CC BY-NC; other per-asset terms and the grenade free-standard snapshot remain
recorded with the original inputs. No public binary commit or asset publication
is included. REFERENCE_WEAPON_RESULTS.json records the saved-asset audit.

Presentation revision: active gun derivatives are V3 (V4 for the 30 Hz AR15/SMG),
with owned A_*_Hands_Idle/Fire/Reload clips in each weapon folder. The Milkor
scope is excluded only from its derivative; source geometry and both UV layers
remain archived. Runtime UV0 consolidates populated source layers per material.
No source archive, legacy weapon or skin geometry was replaced or deleted.

The subsequent palm correction adds only `ER_Palm_L/R` socket metadata to the
twenty owned world meshes and twenty owned FPS meshes. Original HandGrip sockets
remain. Private `palm-contact-report.json` records geometry-derived contact frames;
weapon data stores sampled idle-deformed underside contacts and a dedicated FPS
pistol attachment. The final pistol hand clips retain a relaxed arm,
place the weapon in the lower right at closer depth than the assault, and
fit the three thumb joints around the grip with3.5mm palm embed. Attachment and
motion fitting use the weapon component's actual absolute scale1. The final
supplied-image revision removes the earlier roll-7° cant, converges the bore
on the central camera ray at150cm and sets muzzle depth46cm, at
viewport(58.2%,64.25%) in16:9.
The grip is upright, the palm/thumb contact sits5cm higher on the backstrap
and little forearm appears. Its geometry
targets are recorded privately in pistol-presentation.json. Character vertices,
materials and source archives remain.

Grenade audio (16 October): owned
`/Game/EclipseRecoil/Effects/Grenade/Audio/S_ER_GrenadeExplosion`,
`SA_ER_GrenadeExplosion`, and `SC_ER_GrenadeExplosion` are saved in the
private weapons submodule and bound to BP_ERGrenadeExplosion. Source is
qubodup's Freesound162363 public HQ preview; retained MP3, converted mono
PCM16 WAV, source page, hashes and credit live in
`Assets/weapons/SourceAssets/Freesound/162363`. See ASSET_LICENSES.md.

Spawn protection gray revision (16 October): removed generated translucent
Characters/Protection variants. Runtime uses the existing engine
EngineDebugMaterials/LevelColorationLitMaterial and one transient gray MID per character.
No new persistent material, texture or mesh packages; original skin surfaces
are restored from component/slot snapshots. Core character/training Blueprint
defaults reference only the engine base for this effect.

XM1014 mechanics (16 October): only existing DA_Shotgun data updated
(6x20, armor1.6, range.70, anatomical-hit opt-in,3.2deg pellet pattern,
.25deg standing aim inaccuracy). Existing ragdoll physics bodies also serve
explicit server pellet queries. No new mesh, physics asset or animation packages.

English presentation (16 October): renamed player-facing DisplayName metadata
on22 existing weapon data assets and20 selectable appearances across character,
training and corpse Blueprint defaults. Generic active slot names come from
scripts/presentation_labels.py; English skin labels from character_catalog.py.
No new binary packages or changes to model/source provenance or asset paths.
