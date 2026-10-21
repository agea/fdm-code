#!/usr/bin/env python3
"""Boot the real game with EOS enabled but no private configuration; test LAN fallback."""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parents[1]
if (root / 'Config/EOS.local.ini').exists():
    raise SystemExit('This probe requires an unconfigured checkout. Preserve EOS.local.ini and use an isolated workspace.')
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
out = root / 'Saved/Testing/OnlineUnconfigured'
out.mkdir(parents=True, exist_ok=True)
log = out / 'probe.log'
with log.open('w') as output:
    result = subprocess.run([
        str(exe), str(root / 'RespawnAndRage.uproject'), '/Game/EclipseRecoil/Maps/L_MainMenu',
        '-game', '-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess',
        '-stdout', '-FullStdOutLogOutput', '-ExecCmds=t.MaxFPS 60',
        '-UserDir=' + str(out / 'user'), '-port=17961', '-ERDiscoveryPort=17962',
        '-EROnlineUnconfiguredProbe',
    ], stdout=output, stderr=subprocess.STDOUT, timeout=100)
text = log.read_text(errors='replace')
for line in text.splitlines():
    if 'ER_LOBBY_PASS' in line or 'ER_LOBBY_FAIL' in line or 'ER_ONLINE_UNCONFIGURED_COMPLETE' in line:
        print(line)
assert result.returncode == 0 and 'ER_ONLINE_UNCONFIGURED_COMPLETE result=PASS' in text and 'ER_LOBBY_FAIL' not in text, str(log)
assert all(label in text for label in (
    'unconfigured_online_host_fails_without_travel', 'unconfigured_online_search_is_actionable',
    'online_cancel_restores_idle', 'lan_host_uses_ip_with_eos_plugin_enabled', 'lan_has_no_eos_dependency',
)), str(log)
print('Unconfigured online / LAN fallback: PASS')
