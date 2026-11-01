#!/usr/bin/env python3
"""Actual Middle Eastern Alleys door, underpass, stair and bridge traversal; optional LAN/Metal."""
import argparse, json, os, re, subprocess, time
from pathlib import Path
from client_shutdown import settle_clients

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--render', action='store_true')
p.add_argument('--architecture', action='store_true', help='Real weapon, HE, controlled facade/roof cuts, pawn traversal, support collapse and peer reconstruction')
p.add_argument('--network', action='store_true')
p.add_argument('--gallery', action='store_true', help='Capture held alley cameras after shader/asset compilation, without the gameplay probe')
p.add_argument('--plans', action='store_true', help='Capture four square aerial views of alley seeds 42, 73, 511 and 7919')
p.add_argument('--seed', type=int, default=42, help='Starting seed for the gameplay probe')
p.add_argument('--passage', action='store_true', help='Capture the lower passage, ramp and intact/breached road')
p.add_argument('--tower', action='store_true', help='Capture the district watchtower and ladder only')
p.add_argument('--modules', action='store_true', help='Capture procedural house exteriors, interiors and climbable roof rims')
args = p.parse_args()
if sum([args.plans,args.tower,args.passage,args.modules])>1:p.error('--plans, --tower, --passage and --modules are separate captures')
if args.modules:args.gallery=True
if args.passage:args.gallery=True
if args.tower:args.gallery=True
if args.plans:args.gallery=True
if args.architecture and args.gallery:p.error('--architecture cannot run with --gallery')
if args.gallery and args.network:p.error('--gallery requires standalone execution')
if args.gallery:args.render=True
root = Path(__file__).resolve().parents[1]
out = root / 'Saved/Testing'
out.mkdir(parents=True, exist_ok=True)
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
label = 'alleys' + ('-network' if args.network else '-local') + ('-metal' if args.render else '')
if args.gallery:label += '-gallery'
if args.architecture:label += '-architecture'
if args.plans:label += '-plans'
if args.tower:label += '-tower'
if args.passage:label += '-passage'
if args.modules:label += '-modules'
children, handles, paths = [], [], {}
receipt=out/(label+'-late-ready')
if args.network:receipt.unlink(missing_ok=True)
initial_receipt=out/(label+'-initial-ready')
if args.network and args.architecture:initial_receipt.unlink(missing_ok=True)

architecture_images=[out/('MiddleEasternDestruction-'+name+'.png') for name in ['Facade-Intact','Facade-Breached','Roof-Intact','Roof-Breached','House-Collapsed']]
module_images=[out/('MiddleEasternModules-'+name+'.png') for name in ['Houses','Interior','Roof-Rims']]
if args.architecture and args.render:
    for image in architecture_images:image.unlink(missing_ok=True)
if args.modules:
    for image in module_images:image.unlink(missing_ok=True)

def launch(role, url, render=False, extra=()):
    paths[role] = out / (label + '-' + role + '.log')
    handle = paths[role].open('w'); handles.append(handle)
    command = [str(exe), str(root / 'RespawnAndRage.uproject'), url, '-game', '-ERDustTemplateProbe',
               '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout',
               '-FullStdOutLogOutput', '-windowed', '-ResX=1280', '-ResY=720',
               '-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1,p.NetShowCorrections 1']
    if args.architecture:command.append('-ERAlleyArchitecture')
    if args.plans:
        command[command.index('-ResX=1280')]='-ResX=1536'
        command[command.index('-ResY=720')]='-ResY=1536'
    if not render: command.append('-nullrhi')
    if args.gallery:
        command[-1] += ',r.Shadow.Virtual.MaxPhysicalPages ' + ('4096' if args.plans else '2048')
        command.append('-ERAlleyGallery')
        if args.plans:command.extend(['-ERAlleyPlans','-forceres'])
        if args.tower:command.append('-ERAlleyTowerOnly')
        if args.passage:command.append('-ERAlleyPassageOnly')
        if args.modules:command.append('-ERAlleyModulesOnly')
    child = subprocess.Popen(command + list(extra), cwd=root, stdout=handle, stderr=subprocess.STDOUT)
    children.append(child)
    return child

def read(role): return paths[role].read_text(errors='replace')

def wait(marker,role='host'):
    end = time.monotonic() + 650
    while time.monotonic() < end:
        current=read(role)
        if marker in current: return
        if 'ER_BLOCK_NET_INCOMPATIBLE' in current or 'Attempted to send bunch exceeding' in read('host'):raise RuntimeError(role+' rejected map reconstruction or the host exceeded a network payload limit')
        if children[0].poll() is not None: raise RuntimeError('Host stopped before ' + marker)
        time.sleep(.3)
    raise RuntimeError('Timed out before ' + marker)

try:
    url = f'/Game/EclipseRecoil/Maps/L_MiddleEasternAlleys?Name=Host?Primary=4?MapSeed={args.seed}'
    if args.network: url += '?listen'
    extra=['-port=17793', '-MULTIHOME=127.0.0.1']
    if args.network:extra.append('-ERDustLateReceipt='+str(receipt))
    if args.network and args.architecture:extra.append('-ERArchitectureInitialReceipt='+str(initial_receipt))
    host = launch('host', url, args.render, extra)
    if args.network:
        wait('ER_DUST_STATE phase=1 ')
        launch('initial', '127.0.0.1:17793?Name=Initial?Primary=0')
        if args.architecture:
            wait('ER_DUST_CLIENT_PASS phase=1 ','initial')
            initial_receipt.write_text('phase1\n')
        wait('ER_DUST_STATE phase=6 ')
        launch('late', '127.0.0.1:17793?Name=Late?Primary=0')
        wait('ER_DUST_CLIENT_PASS phase=6 ', 'late')
        receipt.write_text('phase6\n')
    host.wait(timeout=1000)
    text = read('host')
    good = host.returncode == 0 and 'ER_DUST_COMPLETE result=PASS' in text and 'ER_DUST_FAIL' not in text
    states = {int(a): int(b) for a, b in re.findall(r'ER_DUST_STATE phase=(\d+) digest=(\d+)', text)}
    if args.network:
        for role, phases in [('initial', [1, 6, 7]), ('late', [6, 7])]:
            client = read(role)
            for phase in phases:
                matched = f'ER_DUST_CLIENT_PASS phase={phase} digest={states.get(phase)} ' in client
                print(role, phase, 'PASS' if matched else 'FAIL'); good &= matched
            good &= 'ER_DUST_CLIENT_FAIL' not in client and 'ER_BLOCK_NET_INCOMPATIBLE' not in client
            # The remote pawn stands on the Short A deck, a runtime FixedMeshes group.
            # Corrections relative to a group the client cannot resolve are silently dropped.
            deck = 'ER_DUST_CLIENT_BASE_PASS phase=' + ('6' if role == 'initial' else '7') + ' ' in client
            dropped = client.count('could not resolve the new relative movement base')
            print(role, 'deck_movement_base', 'PASS' if deck else 'FAIL', 'unresolved_movement_base_corrections=' + str(dropped))
            good &= deck and not dropped
            for line in client.splitlines():
                if 'ER_DUST_CLIENT_BASE_' in line: print(line)
        unsupported = [line for line in text.splitlines() if 'SupportsObject' in line and 'NOT Supported' in line]
        print('host_unsupported_objects=' + str(len(unsupported)))
        for line in unsupported[:5]: print(line)
        good &= not unsupported
    for line in text.splitlines():
        if 'ER_DUST_' in line: print(line)
    report = dict(result='PASS' if good else 'FAIL', network=args.network, render=args.render, gallery=args.gallery,
                  states=states, logs={role: str(path) for role, path in paths.items()})
    if args.plans:
        aerial = re.findall(r'ER_DUST_AERIAL seed=(\d+) size=(.*?) digest=(\d+)', text)
        report['aerial'] = [dict(seed=int(seed), size=size, digest=int(digest),
                                image=str(out / f'MiddleEasternAlleys-Aerial-{seed}.png')) for seed,size,digest in aerial]
        features={int(seed):dict(accesses=int(access),bends=int(bends),plazas=int(plazas),towers=int(towers),walls=int(walls),open_depth=int(depth)) for seed,access,bends,plazas,towers,walls,depth in re.findall(r'ER_DISTRICT_AERIAL seed=(\d+) accesses=(\d+) bends=(\d+) plazas=(\d+) towers=(\d+) walls=(\d+) open_depth=(\d+)',text)}
        passages={int(seed):dict(underpass_axis=int(axis),low_cover=int(cover),center_cover=int(center)) for seed,axis,cover,center in re.findall(r'ER_PASSAGE_AERIAL seed=(\d+) axis=(\d+) cover=(\d+) center_cover=(\d+)',text)}
        for view in report['aerial']:
            view.update(features.get(view['seed'],{}))
            if features:view['image']=str(out / f'MiddleEasternCore-Aerial-{view["seed"]}.png')
            if passages:
                view.update(passages.get(view['seed'],{}));view['image']=str(out / f'MiddleEasternPassages-Aerial-{view["seed"]}.png')
        good &= len(aerial)==4 and all(Path(view['image']).is_file() for view in report['aerial'])
        report['result']='PASS' if good else 'FAIL'
    if args.architecture and args.render:
        report['architecture_images']=[str(path) for path in architecture_images]
        good &= all(path.is_file() for path in architecture_images)
        report['result']='PASS' if good else 'FAIL'
    if args.modules:
        report['module_images']=[str(path) for path in module_images]
        good &= all(Path(path).is_file() for path in report['module_images'])
        report['result']='PASS' if good else 'FAIL'
    (out / (label + '-report.json')).write_text(json.dumps(report, indent=2) + '\n')
    raise SystemExit(0 if good else 1)
except (RuntimeError, subprocess.TimeoutExpired) as error:
    print('FAIL', error)
    print('\n'.join(read('host').splitlines()[-20:])); raise SystemExit(1)
finally:
    settle_clients(children, handles)
    for child in children:
        if child.poll() is None: child.terminate()
    for child in children:
        try: child.wait(timeout=10)
        except subprocess.TimeoutExpired: child.kill(); child.wait()
    for handle in handles: handle.close()
    if args.network:receipt.unlink(missing_ok=True)
    if args.network and args.architecture:initial_receipt.unlink(missing_ok=True)
