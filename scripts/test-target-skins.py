#!/usr/bin/env python3
"""Verify all stationary skin targets on a real listen server and remote peer."""
from pathlib import Path
import os
import subprocess
import time
import runpy
from client_shutdown import settle_clients
count=len(runpy.run_path(str(Path(__file__).with_name("character_catalog.py")))["NAMES"]["Alpha"])

project = Path(__file__).resolve().parents[1]
editor = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs = project / 'Saved/Testing'
logs.mkdir(parents=True, exist_ok=True)
common = [str(editor), str(project / 'RespawnAndRage.uproject'), '-game', '-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 60']
processes, handles = [], []
try:
    host_log = logs / 'skin-targets-host.log'
    client_log = logs / 'skin-targets-client.log'
    handle = host_log.open('w'); handles.append(handle)
    host = subprocess.Popen(common + [f'/Game/EclipseRecoil/Maps/L_Depot?listen?ERBots={count*2}?ERBotMode=Target?ERFullArsenalTest=1?Name=Host', '-port=17781', '-ERTargetSkinsProbe'], stdout=handle, stderr=subprocess.STDOUT)
    processes.append(host)
    deadline = time.monotonic() + 45
    while 'listening on port 17781' not in host_log.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic() > deadline:
            raise RuntimeError('Host did not start')
        time.sleep(.2)
    handle = client_log.open('w'); handles.append(handle)
    client = subprocess.Popen(common + ['127.0.0.1:17781?Name=Client', '-ERTargetSkinsProbe'], stdout=handle, stderr=subprocess.STDOUT)
    processes.append(client)
    host.wait(timeout=150)
    for side, path in [('host', host_log), ('client', client_log)]:
        output = path.read_text(errors='replace')
        marker = f'ER_TARGET_SKINS_PASS side={side} count={count*2} alpha={(1<<count)-1} omega={(1<<count)-1} weapons=126'
        if marker not in output or 'ER_TARGET_SKINS_FAIL' in output:
            raise RuntimeError(f'{side}: all stationary, armed, correct skin meshes were not verified')
        print(marker, flush=True)
    if host.returncode:
        raise RuntimeError('Host probe failed')
    print(f'TARGETS PASS: all {count} skins per team, stationary and replicated after human join.')
finally:
    settle_clients(processes, handles)
    for process in processes:
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill(); process.wait()
    for handle in handles:
        handle.close()
