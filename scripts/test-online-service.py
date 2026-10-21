#!/usr/bin/env python3
"""Real EOS login/create/search/P2P-listen/travel/cancel/cleanup on this Mac.

An Epic browser sign-in may require human interaction. This single-account
probe does not certify a remote peer connection or cross-network NAT traversal.
"""
from pathlib import Path
import configparser
import os
import subprocess

root = Path(__file__).resolve().parents[1]
config = configparser.ConfigParser()
config.read(root / 'Config/EOS.local.ini')
assert config.get('EOS', 'ClientSecret', fallback=''), 'Configure the ignored EOS.local.ini first.'
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
out = root / 'Saved/Testing/OnlineService'
out.mkdir(parents=True, exist_ok=True)
log = out / 'probe.log'
with log.open('w') as output:
    process = subprocess.Popen([
        str(exe), str(root / 'RespawnAndRage.uproject'), '/Game/EclipseRecoil/Maps/L_MainMenu',
        '-game', '-nullrhi', '-nosound', '-NoSplash', '-Multiprocess',
        '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 60',
        '-UserDir=' + str(out / 'user'), '-EROnlineServiceProbe',
    ], stdout=output, stderr=subprocess.STDOUT)
    try:
        code = process.wait(timeout=620)
    except BaseException:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill(); process.wait()
        raise
text = log.read_text(errors='replace')
for line in text.splitlines():
    if 'ER_EOS_PROBE_' in line or 'ER_ONLINE_ERROR' in line or 'ER_ONLINE_LOGIN success' in line or 'ER_ONLINE_QUIT' in line or 'ER_ONLINE_SHUTDOWN' in line:
        print(line, flush=True)
assert code == 0, f'Game exited with status {code}; inspect {log}'
assert 'ER_EOS_PROBE_COMPLETE result=PASS' in text and 'ER_EOS_PROBE_FAIL' not in text, str(log)
assert 'ER_ONLINE_QUIT cleaned=1' in text and 'ER_ONLINE_SHUTDOWN instance_released' in text, str(log)
print('EOS service integration: PASS (one account; no remote peer proof)')
