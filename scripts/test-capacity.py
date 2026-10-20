#!/usr/bin/env python3
"""Six real humans in a configurable-capacity lobby, crowded spawn and reconnect."""
from pathlib import Path
import argparse
import os
import socket
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--industrial', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
out = root / 'Saved/Testing' / ('CapacityIndustrial' if args.industrial else 'CapacityDepot')
out.mkdir(parents=True, exist_ok=True)
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
port, discovery = (17961, 17962) if args.industrial else (17957, 17958)
common = [str(exe), str(root / 'RespawnAndRage.uproject'), '-game', '-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 30', f'-ERDiscoveryPort={discovery}']
processes, handles = [], []

def launch(role, extra, probe=True, url='/Game/EclipseRecoil/Maps/L_MainMenu'):
    handle = (out / (role + '.log')).open('w')
    handles.append(handle)
    flags = ['-ERCapacityProbe', '-ERCapacityLimit=6', '-ERCapacityMap=' + ('0' if args.industrial else '1')] if probe else []
    process = subprocess.Popen(common + [url, '-UserDir=' + str(out / ('user-' + role))] + flags + extra, stdout=handle, stderr=subprocess.STDOUT)
    processes.append(process)
    return process

def read(role):
    return (out / (role + '.log')).read_text(errors='replace')

def wait(role, marker, seconds=180):
    end = time.monotonic() + seconds
    while marker not in read(role):
        if time.monotonic() > end:
            raise RuntimeError(f'{role}: missing {marker}; see {out}')
        if role == 'host' and processes[0].poll() is not None:
            raise RuntimeError('host exited: ' + str(out))
        time.sleep(.25)

try:
    host = launch('host', ['-ERCapacityHost', f'-port={port}'])
    wait('host', 'ER_LAN_BEACON bound=0.0.0.0:')
    clients = [launch('client' + str(i), [f'-ERCapacityClient={i}']) for i in range(1, 6)]
    wait('host', 'ER_CAPACITY_INITIAL result=PASS')
    assert 'ER_CAPACITY_FAIL' not in read('host')
    for i in range(1, 6):
        wait('client' + str(i), 'ER_CAPACITY_CLIENT_SPAWNED')
        assert 'ER_CAPACITY_FAIL' not in read('client' + str(i))
    print('Six humans, including six on one team, pass lobby/travel/spawn checks.', flush=True)
    # Full servers must also stop advertising, including their reserved slots.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as beacon:
        beacon.settimeout(1)
        beacon.sendto(b'RRLAN1?0123456789abcdef0123456789abcdef', ('127.0.0.1', discovery))
        try:
            beacon.recvfrom(2048)
            raise AssertionError('full server still advertised')
        except TimeoutError:
            pass
    launch('rejected', [], probe=False, url=f'127.0.0.1:{port}?Name=Overflow?ERMaxPlayers=999?MaxPlayers=999')
    wait('rejected', 'Server full: maximum 6 players.')
    print('Seventh player rejected; client URL cannot increase server capacity.', flush=True)
    clients[0].terminate()
    clients[0].wait(timeout=15)
    wait('host', 'ER_PLAYER_LEFT players=5')
    launch('replacement', ['-ERCapacityClient=99'])
    wait('replacement', 'ER_LAN_FOUND')
    wait('replacement', 'ER_CAPACITY_CLIENT_SPAWNED')
    wait('host', 'ER_CAPACITY_COMPLETE result=PASS')
    assert 'ER_CAPACITY_FAIL' not in read('host')
    print('\n'.join(line for line in read('host').splitlines() if 'ER_CAPACITY_' in line), flush=True)
    print('Configured capacity integration: PASS', out, flush=True)
finally:
    for process in processes:
        if process.poll() is None:
            process.terminate()
    for process in processes:
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    for handle in handles:
        handle.close()
