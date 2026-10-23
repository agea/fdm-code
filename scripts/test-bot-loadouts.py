#!/usr/bin/env python3
"""Actual six-bot preset selection/replication across initial life, respawn and round reset."""
from pathlib import Path
import os
import subprocess
import time

project = Path(__file__).resolve().parents[1]
editor = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs = project / 'Saved/Testing'
logs.mkdir(parents=True, exist_ok=True)
common = [str(editor), str(project / 'RespawnAndRage.uproject'), '-game', '-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 60', '-ERBotLoadoutProbe']


def run(mode, port):
    processes, handles = [], []
    paths = {side: logs / f'bot-loadouts-{mode.lower()}-{side}.log' for side in ('host', 'client')}
    try:
        def start(side, flags):
            handle = paths[side].open('w')
            handles.append(handle)
            process = subprocess.Popen(common + flags, stdout=handle, stderr=subprocess.STDOUT)
            processes.append(process)
            return process
        host = start('host', [f'/Game/EclipseRecoil/Maps/L_Depot?listen?ERAlphaBots=3?EROmegaBots=3?ERBotMode={mode}?Name=Host', f'-port={port}'])
        deadline = time.monotonic() + 60
        while f'listening on port {port}' not in paths['host'].read_text(errors='replace'):
            if host.poll() is not None or time.monotonic() > deadline:
                raise RuntimeError(f'{mode}: listen failed')
            time.sleep(.2)
        start('client', [f'127.0.0.1:{port}?Name=Client'])
        host.wait(timeout=120)
        outputs = {side: path.read_text(errors='replace') for side, path in paths.items()}
        for side, output in outputs.items():
            for phase in (1, 2, 3):
                marker = f'ER_BOT_LOADOUT_PASS side={side} phase={phase} count=6 mask='
                if marker not in output:
                    raise RuntimeError(f'{mode}: missing {marker}; inspect {paths[side]}')
            print(f'{mode} {side}: six valid preset kits, fresh respawn and round reset PASS', flush=True)
        if host.returncode or 'ER_BOT_LOADOUT_COMPLETE result=PASS' not in outputs['host'] or 'ER_BOT_LOADOUT_FAIL' in outputs['host']:
            raise RuntimeError(f'{mode}: probe failed')
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


if __name__ == '__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--mode',choices=['Target','Movement','Patrol']);args=parser.parse_args()
    for mode, port in (('Target', 17831), ('Movement', 17832), ('Patrol', 17833)):
        if args.mode and args.mode!=mode:continue
        run(mode, port)
    print('BOT PRESET LOADOUTS PASS: selected modes, closed kits, both LAN peers.')
