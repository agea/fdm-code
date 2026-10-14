# Pulizia e separazione del repository

Pulizia eseguita il 14 ottobre 2026. Nessun commit, push o cambio di visibilità.
La configurazione Git locale usa ora una cache LFS esterna al progetto. Il rapporto
distingue spazio locale, file necessari al gioco e materiali per ricostruirli.

## Risultato della pulizia eseguita

- Cartella progetto: circa **4,1 GiB**; Content **2,7 GiB**, .git circa **66 MiB**.
- Rimossi tramite Unreal **272 package Mercenary** e **3.399 package Paragon**
  senza referencer esterni: circa **2,87 GiB** di asset, con backup SHA-256 verificati.
- Conservati tutti i package runtime e gli originali necessari per ricostruire
  le dieci skin; restano 1.292 package, chiusura runtime 823 / authoring 892.
- Puliti log, screenshot vecchi, DDC, registro asset e diagnostica shader.
  Build e Binaries funzionanti conservati per avviare subito il gioco.
- Backup, copie pre-migrazione e fixture di verifica sono fuori dal progetto:
  `/Users/agea/git/fdm/_backups/2026-10-14-cleanup`.
- Cache LFS: `/Users/agea/git/fdm/_cache/lfs`, **4.927 oggetti / 5,43 GiB**, tutti
  verificati SHA-256. Nessun oggetto eliminato. Solo `lfs.storage` nella config
  locale Git è cambiata; i nuovi cloni usano la loro configurazione predefinita.

Le riduzioni della cartella progetto comprendono spostamenti: cache e backup
occupano ancora il disco locale, non sono stati caricati su Drive. Se si sposta
il progetto o il suo cache store, aggiornare `git config --local lfs.storage`
con il percorso corretto; non cancellare `_cache/lfs` come se fosse il backup
statico degli asset. Le quote LFS del remoto non sono state modificate.

Verifiche sul progetto effettivamente ripulito: dieci skin, undici armi,
550 campioni grip, host/client con sparo, morte/cadaveri e respawn; gallery
Metal world e FPS **10/10 ciascuna**. Vedere `CLEANUP_RESULT.json` e TEST_RESULTS.

`scripts/prune_unused_paragon.py` mostra un piano in anteprima. L'opzione
`-ERPruneParagon` richiede un archivio separato già verificato, controlla gli
hash correnti e rifiuta qualsiasi referencer esterno. Un helper Editor limitato
ai due namespace Paragon rimuove package non caricati e aggiorna AssetRegistry:
le vecchie demo World possono causare crash nel percorso standard UE 5.8.
Questo evita di caricare/compilare demo incompatibili e non modifica asset attivi.

I dati dei paragrafi seguenti descrivono l'audit **prima** della pulizia.

## Dove andava lo spazio

Il checkout occupa circa 13 GiB, compresi Git e file generati. I valori `du`
sono arrotondati; quelli degli asset sotto sono dimensioni logiche in MiB.

| Cartella | Spazio locale circa | Cosa fare |
| --- | ---: | --- |
| Content | 5,6 GiB | Conservare i package necessari, esaminare i candidati dell'audit |
| .git | 5,4 GiB | Quasi tutto cache LFS, non cancellare manualmente |
| Intermediate | 1,2 GiB | Rigenerabile, dopo chiusura Unreal; richiede ricompilazione |
| SourceAssets | 128 MiB | Sorgenti/archivi di importazione; separabili dal runtime, da conservare per ricostruire |
| Saved | 97 MiB | Pulire selettivamente log e screenshot, conservare impostazioni/autosalvataggi |
| Binaries | 2,6 MiB | Rigenerabile; cancellandolo gli script di gioco richiedono prima una nuova build |
| DerivedDataCache | 1,6 MiB | Rigenerabile; il prossimo avvio può ricompilare shader |
| Source + Config + scripts + docs | Circa 1,2 MiB | Conservare/versionare; molto piccolo rispetto agli asset |

Content è presente due volte in forme diverse: i package nel checkout e gli
oggetti LFS nella cache Git. Non sono entrambi dati cancellabili: LFS serve
anche per commit, ripristini e versioni indicizzate. `git lfs prune --dry-run`
ha proposto soltanto **28 oggetti / 816 KB**, mantenendone 4.847: qui una normale
prune recupererebbe pochissimo. Non è stata eseguita la cancellazione.

Le cartelle rigenerabili sono già ignorate da `.gitignore`; pulirle libera
disco locale, non riduce automaticamente il repository remoto. Anche togliere
un asset dal nuovo commit non rimuove vecchie versioni da Git/LFS.

## Dipendenze e candidati

`scripts/audit-assets.py` ha letto 4.963 package dal progetto e dal registro
Unreal. Radici runtime: tre mappe EclipseRecoil, Blueprint Core/cataloghi,
DA_Rifle_A e riferimenti letterali nativi/config. Esplorazione transitiva di
riferimenti hard, soft e management. Le dipendenze editor possono gonfiare il
risultato rispetto a un gioco cooked.

| Gruppo | Totale MiB | Raggiunto dal gioco MiB | Solo ricostruzione MiB | Candidati MiB |
| --- | ---: | ---: | ---: | ---: |
| ParagonTwinblast | 2.365,0 | 1.269,1 | 23,6 | 1.072,4 |
| ParagonDrongo | 2.163,6 | 829,0 | 12,0 | 1.322,6 |
| Mercenary_Male_WaveforceStudio | 537,2 | 0 | 0 | 537,2 |
| Tutto Content | 5.722,2 | 2.648,0 | 79,1 | 2.994,2 |

Altri 0,95 MiB sono protetti per revisione separata: external actors/objects
e due package non indicizzati. Il totale teorico conservando runtime,
ricostruzione e questi file è circa **2,66 GiB**, prima dei controlli di
caricamento. I candidati rappresentano circa **2,92 GiB** potenzialmente
separabili; non sono un elenco di cancellazione automatica.

I risparmi maggiori sono demo e mannequin duplicati del mercenario, audio e
FX di Drongo, FX e contenuti globali extra di Twinblast. Gli originali
ShadowOps, Wasteland e Scavenger sono protetti come sorgenti delle skin anche
quando non raggiunti dal gioco: gli script ne costruiscono i percorsi.

Il **mercenario è ora eliminabile**: la migrazione successiva ha rimosso tutti

i riferimenti runtime e di authoring correnti ai due alberi Mercenary. Il rig
fitted è in `Characters/FirstPerson/SKEL_ER_FirstPerson`; 24 pose sono nello
stesso percorso, con preview su avambracci Alpha e senza compatibilità verso
il rig vendor. I Blueprint usano mesh Twinblast/Drongo e non conservano i
vecchi materiali di fallback. L'attuale rig FPS non necessita del post-process
vendor. Le copie di rig/pose mantengono la loro provenienza; l'autonomia delle
dipendenze non cambia i termini del materiale derivato.

**272 package / circa 547 MiB** tra `Content/Mercenary_Male_WaveforceStudio`
e `Content/EclipseRecoil/Characters/Mercenary` non hanno più referencer esterni.
Una copia di prova priva di entrambe le cartelle ha superato rete, dieci skin,
550 campioni di grip salvati e rendering FPS. La migrazione ha prima lasciato le cartelle recuperabili; la successiva pulizia
le ha rimosse dopo aver verificato un archivio completo esterno.

`scripts/remove_legacy_mercenary.py` è un controllo Unreal in anteprima;
verifica tutti i referencer esterni, anche per asset non usati dal gioco.
Per eseguire la rimozione, usarlo nel commandlet aggiungendo esplicitamente
`-ERDeleteLegacyMercenary`. Senza quel flag non cancella nulla. Conserva
`Characters/FirstPerson` e rifiuta la rimozione se compaiono referencer esterni.
`verify_no_mercenary.py` è la verifica salvata in sola lettura; i vecchi script
sono documentati in `scripts/legacy`, fuori dal workflow di ricostruzione.

Una copia pre-migrazione dei package modificati è conservata localmente in
`Saved/AssetAudit/MercenaryDetachBackup`. Dopo il backup privato e la revisione
può essere rimossa separatamente: non è una dipendenza del gioco né entra
nel repository. Il test in `BuildArtifacts/no-mercenary-test` usa hardlink;
non eseguire import/setup/salvataggi lì. Non contiene le due cartelle obsolete.

File del rapporto:

- `docs/ASSET_AUDIT_SUMMARY.json`: sintesi versionabile, radici e dimensioni.
- `Saved/AssetAudit/asset-audit.json`: grafo raggiunto e stato per package.
- `Saved/AssetAudit/candidates-review.csv`: candidati da valutare.
- `Saved/AssetAudit/runtime-manifest.txt`: package raggiunti dal gioco.
- `Saved/AssetAudit/authoring-manifest.txt`: ulteriori dipendenze per ricostruzione.
- `Saved/AssetAudit/protected-review.txt`: file esclusi dalla pulizia automatica.

Il registro non prova l'assenza di usi dinamici e non sostituisce la verifica
degli script, delle mappe modificate e dei redirector. La rimozione degli asset
va fatta **attraverso Unreal**, su una copia protetta, poi riapertura delle tre
mappe, verifica dieci skin/FPS/cadaveri, armi/fumo/esplosioni e test LAN. Non
cancellare `.uasset` seguendo semplicemente il CSV e non usare `force delete`.

Ripetere l'audit con il modulo Editor compilato:

```sh
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/fdm.uproject" -run=pythonscript -script="$PWD/scripts/audit-assets.py" \
  -unattended -nullrhi -nosound -NoSplash
```

## Script di pulizia

Da questa cartella, a Unreal/compilatori chiusi:

```sh
python3 scripts/clean-project.py
python3 scripts/clean-project.py --apply
python3 scripts/clean-project.py --profile logs screenshots --older-than 7
python3 scripts/clean-project.py --profile logs screenshots cache build
```

Ogni comando senza `--apply` mostra soltanto l'anteprima. Profilo predefinito:
log `.log` in Saved/Logs, Saved/Testing e root. Screenshot è separato e include
le immagini di riferimento/test; conservarle se servono a confrontare la
grafica. Cache rimuove la DDC del progetto; build seleziona Intermediate e
Binaries. Non gestisce la cache globale Unreal né `.git/lfs`.

L'anteprima ha misurato circa **64 MiB di log** e **1.403 MiB** con tutti i
profili: lo spazio reale recuperato dipende da file successivi e filesystem.
Lo script rifiuta file tracciati/indicizzati e processi Unreal/build attivi,
non segue symlink, conserva Saved/Config, Autosaves, AssetAudit, SourceAssets
e Content. Le directory possono restare vuote dopo la pulizia.

## Struttura consigliata

**Un repository pubblico per codice + un repository privato per Content**,
con un archivio privato dei download/import originali. Un submodule privato
montato direttamente a `Content/` conserva i percorsi `/Game/...` usati da
Blueprint e script. `SourceAssets/` può essere ripristinato da un archivio
Drive quando si lavora alla ricostruzione, senza essere necessario per avviare
il gioco già importato. Entrambi devono avere una versione/manifest coerente
con il commit del codice.

| Opzione | Vantaggio | Limite |
| --- | --- | --- |
| Content come submodule privato con LFS | Commit fissato dal progetto, aggiornamenti e rollback chiari | Credenziali e storage/banda LFS; asset esclusi dai normali cloni pubblici |
| Archivio privato Drive per Content + SourceAssets | Evita versionare GB in Git; comodo per backup/download originali | Va ripristinato localmente, con versione e SHA-256; niente rollback Git automatico |
| Cartella esterna locale collegata a Content | Non cambia `/Game` se il collegamento conserva l'albero | Portabilità e symlink/junction, accesso locale continuo; non riduce da sola LFS/crono |
| Più submodule per ogni pacchetto | Permessi e aggiornamenti separati per pacchetto | Molti incroci di dipendenze; non riduce i byte totali o i vincoli di licenza |

Preferisco un solo submodule Content: le copie Teams/EclipseRecoil continuano
a riferirsi a materiali/skeleton/texture dei pacchetti originali. Dividere
subito per personaggio complica il ripristino e non evita i costi LFS.
[GitHub descrive il conteggio delle versioni LFS](https://docs.github.com/en/billing/concepts/product-billing/git-lfs).

Drive deve conservare archivi o una copia di backup riservata; per lavorare,
ripristinare su disco locale tutto Content con lo stesso albero. Evitare
package disponibili solo on-demand e modifiche contemporanee ai binari in
una cartella sincronizzata. Non caricare Saved, Intermediate o cache Git.
Conservare originali non modificati, hash, link autore e prova di licenza
fuori dal ramo runtime; non duplicare ZIP e file estratti nel repository attivo.

## Repository pubblico e licenze

Il codice può essere separato dai contenuti e pubblicato dopo la scelta della
licenza del codice e il mantenimento degli avvisi Epic. Gli asset Fab e le
nostre mesh/materiali derivati restano riservati salvo prova di termini più
permissivi. Un Drive pubblico o un submodule pubblico non risolve la licenza.
I template UE hanno regole specifiche per gli Examples, mentre l'esplosione
ha una possibile licenza CC-BY: vedere [ASSET_LICENSES.md](ASSET_LICENSES.md)
per le fonti e i punti ancora da confermare.

HEAD contiene un solo commit iniziale; i pacchetti Fab/Paragon/mercenario sono
principalmente aggiunte già nell'indice, non in quel commit. Le modifiche
attuali sono migliaia: **non committare indiscriminatamente l'indice per
preparare un repository pubblico**. Il controllo locale non certifica cosa
sia già presente sul server remoto o nei suoi oggetti LFS.

`scripts/export-code.py --apply` crea `BuildArtifacts/code-only`: copia locale
con allowlist di sorgenti/config/script/doc, senza Content, SourceAssets,
cache, binari o `.git`. Include un manifest SHA-256 e un avviso sugli asset
necessari; la sua `.gitignore` esclude anche Content/SourceAssets ripristinati.
Per registrare Content come submodule privato usare l'aggiunta esplicita con
`git submodule add -f` nel nuovo repository dopo aver configurato il remoto.
È una base da revisionare per un nuovo repository, non un gioco
autonomo. Non sceglie MIT/Apache o altre licenze e non pubblica nulla. Creare
un repository nuovo evita trascinare la vecchia cronologia di asset. Se si
vuole riusare il remoto attuale, prima verificare anche la sua cronologia/LFS.

Ordine pratico: conservare un backup completo privato; revisionare/prunare
gli asset in Unreal e verificare il gioco; fissare il bundle privato; creare
la storia del codice pubblico e collegare il Content privato. La separazione
non richiede rinominare o migrare i package UE se si mantiene `Content/`.


## Separazione codice/asset · 14 ottobre 2026

La scelta successiva dell’utente è un repository pubblico `agea/fdm-code`
e tre submodule privati (personaggi, armi/effetti, mappe/Blueprint comuni).
Il vecchio `agea/fdm` resta privato. I percorsi Unreal sono invariati;
Content/SourceAssets sono collegamenti generati e ignorati. La struttura
attuale e i comandi sono in [ASSET_SUBMODULES.md](ASSET_SUBMODULES.md), che
sostituisce la precedente proposta di un solo submodule. Nessuna licenza
open-source del codice è stata scelta implicitamente.
