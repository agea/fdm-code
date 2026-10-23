#!/usr/bin/env python3
"""Exercise three automatic map changes with a real connected LAN client."""
import os
from pathlib import Path
import re
import subprocess
import time

root = Path(__file__).resolve().parents[1]
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs = root / 'Saved/Testing'
logs.mkdir(parents=True, exist_ok=True)
common = [str(exe), str(root / 'RespawnAndRage.uproject'), '-game', '-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 60', '-MULTIHOME=0.0.0.0']
processes = []
host_log = logs / 'round-rotation-host.log'
with host_log.open('w') as host_out, (logs / 'round-rotation-client.log').open('w') as client_out:
    try:
        host = subprocess.Popen(common + ['/Game/EclipseRecoil/Maps/L_CentralBuilding?listen?Name=Host?Team=1?ERMaxPlayers=7?ERAlphaBots=0?EROmegaBots=0', '-port=17948', '-ERSessionProbe', '-ERRoundRotationProbe'], stdout=host_out, stderr=subprocess.STDOUT)
        processes.append(host)
        deadline = time.monotonic() + 120
        while 'ER_MATCH_STARTED' not in host_log.read_text(errors='replace'):
            if host.poll() is not None or time.monotonic() > deadline:
                raise RuntimeError(f'Host failed to start: {host_log}')
            time.sleep(.2)
        client = subprocess.Popen(common + ['127.0.0.1:17948?Name=Client?Team=0'], stdout=client_out, stderr=subprocess.STDOUT)
        processes.append(client)
        host.wait(timeout=240)
        output = host_log.read_text(errors='replace')
        rounds = re.findall(r'ER_ROTATION_ROUND index=\d+ map=(\w+) humans=2', output)
        assert host.returncode == 0 and 'ER_ROTATION_COMPLETE result=PASS' in output and 'ER_SESSION_FAIL' not in output, str(host_log)
        assert len(rounds) == 4 and all(a != b for a, b in zip(rounds, rounds[1:])), rounds
        assert all(level in {'L_CentralBuilding', 'L_MiddleEasternAlleys', 'L_FireTraining'} for level in rounds), rounds
        print('PASS: two humans, preserved teams/capacity, three map changes: ' + ' -> '.join(rounds))
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
