#!/usr/bin/env python3
"""Verify ten stationary skin targets on a real listen server and remote peer."""
from pathlib import Path
import os
import subprocess
import time

project = Path(__file__).resolve().parents[1]
editor = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs = project / 'Saved/Testing'
logs.mkdir(parents=True, exist_ok=True)
common = [str(editor), str(project / 'fdm.uproject'), '-game', '-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 60']
processes, handles = [], []
try:
    host_log = logs / 'ten-targets-host.log'
    client_log = logs / 'ten-targets-client.log'
    handle = host_log.open('w'); handles.append(handle)
    host = subprocess.Popen(common + ['/Game/EclipseRecoil/Maps/L_Depot?listen?ERBots=10?ERBotMode=Target?Name=Host', '-port=17781', '-ERTargetSkinsProbe'], stdout=handle, stderr=subprocess.STDOUT)
    processes.append(host)
    deadline = time.monotonic() + 45
    while 'listening on port 17781' not in host_log.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic() > deadline:
            raise RuntimeError('Host did not start')
        time.sleep(.2)
    handle = client_log.open('w'); handles.append(handle)
    client = subprocess.Popen(common + ['127.0.0.1:17781?Name=Client', '-ERTargetSkinsProbe'], stdout=handle, stderr=subprocess.STDOUT)
    processes.append(client)
    host.wait(timeout=70)
    for side, path in [('host', host_log), ('client', client_log)]:
        output = path.read_text(errors='replace')
        marker = f'ER_TARGET_SKINS_PASS side={side} count=10 alpha=31 omega=31'
        if marker not in output or 'ER_TARGET_SKINS_FAIL' in output:
            raise RuntimeError(f'{side}: ten stationary, armed, correct skin meshes were not verified')
        print(marker, flush=True)
    if host.returncode:
        raise RuntimeError('Host probe failed')
    print('TEN TARGETS PASS: all five skins per team, no firing/movement, preserved after human join and replicated meshes.')
finally:
    for process in processes:
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill(); process.wait()
    for handle in handles:
        handle.close()
