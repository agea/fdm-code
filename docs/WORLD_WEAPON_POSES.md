# Pose armi in terza persona

Revisione: 26 ottobre 2026.

## Obiettivo

Il riferimento principale è la posa neutra del personaggio mascherato fornita
dall'utente: calcio sotto il braccio destro, mano destra sull'impugnatura,
mano sinistra sul paramano e arma circa all'altezza del gomito destro. La pistola usa una presa
a due mani. Quando lo scope è attivo, arma, mani e calcio salgono verso la linea
dell'occhio; il passaggio è interpolato e mantiene l'orientamento dell'aim.

## Implementazione

`UERWeaponData` conserva un profilo esclusivamente world con scala assoluta e
contatti locali per calcio, palmo destro, palmo sinistro e supporto pistola.
`ERWorldWeaponPose` collega l'arma a `ER_Palm_R` senza ereditare la scala diversa
dei corpi. Per le armi lunghe, `UERPostureMesh` ruota il busto di 30 gradi in
neutro / 15 gradi in ADS, aggiungendo 25 / 30 gradi alle clavicole: rotazione
complessiva 55 / 45 gradi. Il collo compensa la torsione del busto per mantenere
il viso orientato verso la mira; segue l'IK a due ossa di entrambe le braccia.
Vista dall'alto, la spalla destra che riceve il calcio resta arretrata e quella
di supporto avanzata. L'asse polso-nocche destro punta in avanti con una lieve
inclinazione verso il basso, invece della precedente rotazione verticale che
faceva pendere la mano. Il palmo sinistro tocca il lato inferiore del paramano,
con asse polso-nocche verso +Y dell'arma e normale verso +Z. Il precedente
asse -Y ripiegava la mano indietro: la revisione 4 lo ruota di 180 gradi
mantenendo lo stesso contatto. La revisione 5 sposta l'appoggio fino a 1,5 cm
verso il corpo, misurando nuovamente la superficie inferiore alla stessa
posizione longitudinale; sulle sezioni strette riduce lo spostamento.
La revisione 6 avanza poi quella posizione di 4 cm world verso la canna,
misurando nuovamente il paramano e conservando il ricentramento laterale.
La revisione 7 aggiunge altri 2 cm verso la canna (6 cm totali dalla revisione
5) e 2 cm verso la sinistra del personaggio, -Y dell'arma, per allineare il
pollice al paramano. Quest'ultimo è un offset di seduta del palmo rispetto
alla superficie misurata, conservando il verso del polso.
La revisione 8 ruota il polso di 30 gradi in senso antiorario nella vista
dall'alto fornita dall'utente: asse polso-nocche da +Y verso +X dell'arma
(yaw 60 gradi), normale del palmo ancora +Z e stessa posizione d'appoggio.
Solo per le cinque varianti Phase, `FERTeamAppearance.WorldSupportPalmOffsetCm`
aggiunge `(0,2,0)` cm negli assi dell'arma: la mano rientra sotto il paramano.
Le altre skin hanno offset zero e conservano la presa approvata; i profili
condivisi delle armi e la pistola non cambiano. L'offset è convertito dalla
scala dell'arma da `ERWorldWeaponPose::SupportContact`.
L'animazione continua a controllare locomozione, gambe,
testa e dita. Il profilo neutro usa il 90% della lunghezza del braccio
per portare l'asse del fucile circa all'altezza del gomito; il calcio resta
all'esterno del busto, sotto il braccio. La linea del calcio è a metà
della larghezza originale delle spalle meno 1 cm dal centro delle spalle,
in direzione del fianco destro: 4 cm più interna della precedente linea
troppo vicina al braccio. La pistola conserva la precedente posa delle
clavicole. Lo scope miscela verso la spalla e
l'occhio. Il sistema usa `GetBaseAimRotation`, quindi è coerente su server e
client anche mirando in alto o in basso. Gli accessori field-pack/backpack sono
stati rimossi dalle varianti selezionabili e dai corpi corrispondenti.

Scale approvate: Pistol0,92; SMG0,92; Shotgun0,90; Assault1,00; Carbine0,95;
Sniper0,78; Launcher0,95. I contatti sono ricavati dalla geometria effettiva,
non da offset condivisi tra modelli diversi.

## Confini

La modifica non altera rig, pose, trasformazioni o animazioni FPS; non cambia
reload, muzzle, danni, armor ratio, intervalli di fuoco, spread, scope o collisioni.
`scripts/setup_world_weapon_pose.py` salva solo i sette DataAsset di presentazione,
crea un backup e confronta i campi protetti prima e dopo la migrazione.

Le immagini dell'utente e i profili misurati restano nei submodule privati degli
asset. Gli originali non sono usati a runtime.

## Verifica

- `python3 scripts/test-world-poses.py`: 20 skin x13 casi su server e client.
- `python3 scripts/test-reference-weapons.py`: slot, impugnature, reload e combat.
- `python3 scripts/test-scopes.py`: replica e comportamento scope/ADS.
- `python3 scripts/capture-world-pose-gallery.py`: 80 catture Metal, quattro
  angolazioni per ognuno dei 20 personaggi, più quattro tavole riepilogative.

Le tolleranze automatiche sono0,5cm per il palmo destro,1,5cm per il sinistro,
dot prodotto arma/aim superiore a0,995, errore angolare del palmo destro inferiore
a 1 grado e scala world esatta. Per le armi lunghe, il prodotto scalare tra
direzione avambraccio sinistro e asse polso-nocche deve essere positivo:
una mano ripiegata all'indietro fallisce il controllo anche se tocca l'arma.
Le catture Metal
sono in `Saved/Testing/WorldPoseAll`.
