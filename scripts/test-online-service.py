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
import time

root = Path(__file__).resolve().parents[1]
config = configparser.ConfigParser()
config.read(root / 'Config/EOS.local.ini')
assert config.get('EOS', 'ClientSecret', fallback=''), 'Configure the ignored EOS.local.ini first.'
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
out = root / 'Saved/Testing/OnlineService'
out.mkdir(parents=True, exist_ok=True)
fingerprints = []

def launch_search_probe():
    log = out / 'search-process.log'
    with log.open('w') as output:
        process = subprocess.Popen([
            str(exe), str(root / 'RespawnAndRage.uproject'), '/Game/EclipseRecoil/Maps/L_MainMenu',
            '-game', '-nullrhi', '-nosound', '-NoSplash', '-Multiprocess', '-unattended',
            '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 30',
            '-UserDir=' + str(out / 'search-user'), '-EROnlineSearchProbe',
            # Deliberately differ from the host: the old implicit EOS bucket
            # hid this server even though RR_VERSION was identical.
            '-BuildIdOverride=987654321',
        ], stdout=output, stderr=subprocess.STDOUT)
        try:
            code = process.wait(timeout=110)
        except BaseException:
            process.terminate()
            try: process.wait(timeout=10)
            except subprocess.TimeoutExpired: process.kill(); process.wait()
            raise
    result = log.read_text(errors='replace')
    assert code == 0 and 'ER_EOS_SEARCH_PROCESS result=PASS' in result, str(log)
    print('Separate process: named Internet server discovered with differing engine build ID', flush=True)

for attempt in range(2):
    log = out / ('probe.log' if attempt == 0 else 'probe-relaunch.log')
    with log.open('w') as output:
        process = subprocess.Popen([
        str(exe), str(root / 'RespawnAndRage.uproject'), '/Game/EclipseRecoil/Maps/L_MainMenu',
        '-game', '-nullrhi', '-nosound', '-NoSplash', '-Multiprocess', '-unattended',
        '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 60',
        '-UserDir=' + str(out / 'user'), '-EROnlineServiceProbe',
        *(['-EROnlineDiscoveryHost'] if attempt == 0 else []),
        ], stdout=output, stderr=subprocess.STDOUT)
        try:
            if attempt == 0:
                deadline = time.monotonic() + 150
                while 'ER_EOS_DISCOVERY_HOST ready' not in log.read_text(errors='replace'):
                    assert process.poll() is None and time.monotonic() < deadline, str(log)
                    time.sleep(.25)
                launch_search_probe()
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
for log in (out / 'probe.log', out / 'probe-relaunch.log'):
    if 'policy_missing_action' in log.read_text(errors='replace'):
        print('EOS policy: INCOMPLETE (backend denied a required action; inspect private service logs)')
        break
