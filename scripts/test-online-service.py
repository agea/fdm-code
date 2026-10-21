#!/usr/bin/env python3
"""Real account-free EOS access and service lifecycle on this Mac, twice.

Checks Device ID persistence and the real Connect refresh handler. This single
device probe does not certify remote peer connectivity or cross-network NAT.
"""
from pathlib import Path
import configparser
import os
import re
import subprocess

root = Path(__file__).resolve().parents[1]
config = configparser.ConfigParser()
config.read(root / 'Config/EOS.local.ini')
assert config.get('EOS', 'ClientSecret', fallback=''), 'Configure the ignored EOS.local.ini first.'
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
out = root / 'Saved/Testing/OnlineService'
out.mkdir(parents=True, exist_ok=True)
fingerprints = []
for attempt in range(2):
    log = out / ('probe.log' if attempt == 0 else 'probe-relaunch.log')
    with log.open('w') as output:
        process = subprocess.Popen([
        str(exe), str(root / 'RespawnAndRage.uproject'), '/Game/EclipseRecoil/Maps/L_MainMenu',
        '-game', '-nullrhi', '-nosound', '-NoSplash', '-Multiprocess', '-unattended',
        '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 60',
        '-UserDir=' + str(out / 'user'), '-EROnlineServiceProbe',
        ], stdout=output, stderr=subprocess.STDOUT)
        try:
            code = process.wait(timeout=300)
        except BaseException:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill(); process.wait()
            raise
    text = log.read_text(errors='replace')
    for line in text.splitlines():
        if any(marker in line for marker in ('ER_EOS_PROBE_', 'ER_EOS_GUEST_', 'ER_ONLINE_ERROR', 'ER_ONLINE_LOGIN success', 'ER_ONLINE_QUIT', 'ER_ONLINE_SHUTDOWN')) and 'fingerprint=' not in line:
            print(line, flush=True)
    assert code == 0, f'Game exited with status {code}; inspect {log}'
    assert 'ER_EOS_PROBE_COMPLETE result=PASS' in text and 'ER_EOS_PROBE_FAIL' not in text, str(log)
    assert 'ER_ONLINE_QUIT cleaned=1' in text and 'ER_ONLINE_SHUTDOWN instance_released' in text, str(log)
    assert 'ER_ONLINE_LOGIN success mode=device_id' in text and 'ER_EOS_GUEST_REFRESH success' in text, str(log)
    assert 'ER_EOS_GUEST_REFRESH failed' not in text, str(log)
    ids = set(re.findall(r'ER_EOS_PROBE_IDENTITY fingerprint=([A-Fa-f0-9]+)', text))
    assert len(ids) == 1, 'Guest identity must survive leave/cancel/rehost'
    fingerprints.append(ids.pop())
    if attempt == 1:
        assert 'ER_EOS_GUEST_DEVICE reused' in text, 'Relaunch must reuse the existing Device ID'
assert fingerprints[0] == fingerprints[1], 'Guest identity must persist across process restarts'
print('EOS guest service: PASS (persistent Device ID, refresh, two launches; no remote peer proof)')
