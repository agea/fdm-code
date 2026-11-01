#!/usr/bin/env python3
"""Run native kit/mine invariants and a real15-kit host/client replication fixture."""
from pathlib import Path
import argparse,os,subprocess,time,re
from client_shutdown import settle_clients
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--network',action='store_true')
parser.add_argument('--player-team',action='store_true',help='Verify remote deferred team change, cancellation, protected suicide and respawn')
parser.add_argument('--render',action='store_true')
parser.add_argument('--drongo-masks',action='store_true',help='Capture the two restored masks plus the helmet variant')
parser.add_argument('--clothing',action='store_true',help='Capture front/back walking clothing and masks on all affected skins')
parser.add_argument('--layout-only',action='store_true',help='Capture one Player page with the largest equipment list')
parser.add_argument('--width',type=int,default=1440)
parser.add_argument('--height',type=int,default=1000)
parser.add_argument('--mines',action='store_true')
args=parser.parse_args()
project=Path(__file__).resolve().parents[1]
editor=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=project/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
common=[str(editor),str(project/'RespawnAndRage.uproject'),'-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput']
if not args.network and not args.render:
    log=logs/'loadout-automation.log'
    with log.open('w') as out:
        result=subprocess.run(common+['/Engine/Maps/Entry','-nullrhi','-ExecCmds=Automation RunTests EclipseRecoil.Loadout','-TestExit=Automation Test Queue Empty','-ReportExportPath='+str(logs/'loadout-report')],stdout=out,stderr=subprocess.STDOUT,timeout=180)
    text=log.read_text(errors='replace')
    print('\n'.join(line for line in text.splitlines() if 'Test Completed.' in line or 'Error:' in line))
    raise SystemExit(0 if result.returncode==0 and all('Result={Success} Name={'+name+'}' in text for name in ['Catalog','ClaymoreLifecycle']) else 1)
processes=[];files=[]
try:
    capture_started=time.time()
    tag='loadout-'+('render' if args.render else 'network')
    hostlog=logs/(tag+'-host.log');clientlog=logs/(tag+'-client.log')
    hostflags=['-windowed',f'-ResX={args.width}',f'-ResY={args.height}','-ERLoadoutGallery'] if args.render else ['-nullrhi']
    if args.mines:hostflags.append('-ERLoadoutMines')
    if args.player_team:hostflags.append('-ERPlayerTeamProbe')
    if args.render and args.mines:hostflags.append('-ERClaymoreLaserCapture')
    if args.layout_only:hostflags.append('-ERPlayerLayoutCapture')
    if args.clothing:hostflags.append('-ERClothingGallery')
    if args.drongo_masks:hostflags.append('-ERDrongoMaskGallery')
    files.append(hostlog.open('w'))
    host=subprocess.Popen(common+['/Game/EclipseRecoil/Maps/L_Depot?listen?Name=LoadoutHost','-game','-ERLoadoutProbe','-port=17825','-ExecCmds=t.MaxFPS 60,r.DynamicRes.OperationMode 0,r.ScreenPercentage 100']+hostflags,stdout=files[-1],stderr=subprocess.STDOUT);processes.append(host)
    deadline=time.monotonic()+60
    while 'listening on port17825' not in hostlog.read_text(errors='replace').replace('port 17825','port17825'):
        if host.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Host did not listen: '+str(hostlog))
        time.sleep(.2)
    files.append(clientlog.open('w'))
    client=subprocess.Popen(common+['127.0.0.1:17825?Name=LoadoutClient','-game','-ini:Game:[RespawnAndRage.Loadout.smg]:Reserve=999','-nullrhi','-ExecCmds=t.MaxFPS 60'],stdout=files[-1],stderr=subprocess.STDOUT);processes.append(client)
    host.wait(timeout=300)
    server=hostlog.read_text(errors='replace');remote=clientlog.read_text(errors='replace')
    print('\n'.join(line for line in server.splitlines() if 'ER_LOADOUT_' in line))
    all_remote=set(range(1,16)).issubset({int(i) for i in re.findall(r'ER_LOADOUT_OBSERVED side=client phase=(\d+)\b',remote)})
    print('ER_LOADOUT_REMOTE_ALL15',all_remote)
    success=host.returncode==0 and 'ER_LOADOUT_COMPLETE result=PASS' in server and 'ER_LOADOUT_FAIL' not in server and all_remote
    if args.player_team:
        success &= 'ER_PLAYER_TEAM_COMPLETE result=PASS' in server and 'ER_PLAYER_TEAM_REMOTE result=PASS' in remote
    if args.mines:
        success &= 'ER_LOADOUT_MINES_REMOTE absolute_deadlines=PASS' in remote and 'ER_LOADOUT_MINES staggered_expiry_death_respawn_variant=PASS' in server
    if args.render:
        captures=all((logs/'Loadouts'/f'{"Clothing" if args.clothing or args.drongo_masks else "Loadout"}_{i}.png').exists() and (logs/'Loadouts'/f'{"Clothing" if args.clothing or args.drongo_masks else "Loadout"}_{i}.png').stat().st_mtime>=capture_started for i in (range(10,16) if args.drongo_masks else range(28) if args.clothing else [3] if args.layout_only else range(14)));print('ER_LOADOUT_CAPTURES',captures);success &= captures
        material_ok=not any('Claymore/' in line and 'Failed to compile Material' in line for line in server.splitlines())
        print('ER_CLAYMORE_METAL_MATERIAL',material_ok);success &= material_ok
        if args.mines:
            beam=logs/'Loadouts/ClaymoreLasers.png'
            beam_ok=beam.exists() and beam.stat().st_mtime>=capture_started
            print('ER_CLAYMORE_LASER_CAPTURE',beam_ok);success &= beam_ok
    raise SystemExit(0 if success else 1)
finally:
    settle_clients(processes,files)
    for proc in processes:
        if proc.poll() is None:
            proc.terminate()
            try:proc.wait(timeout=5)
            except subprocess.TimeoutExpired:proc.kill();proc.wait()
    for file in files:file.close()
