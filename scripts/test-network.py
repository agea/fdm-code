#!/usr/bin/env python3
"""Opt-in local integration test using two actual Unreal game processes.

No cook, packaging or distribution. Outputs are stored in Saved/Testing.
The replicated probe only exists when -ERNetworkProbe is explicitly supplied.
"""
from pathlib import Path
import os
import subprocess
import sys
import time
import argparse

parser = argparse.ArgumentParser(description=__doc__)
group = parser.add_mutually_exclusive_group()
group.add_argument('--shot-feedback', action='store_true', help='Check authorized shot feedback on host and remote client')
group.add_argument('--accuracy', action='store_true', help='Check real spread, burst recovery and owner replication')
parser.add_argument('--skins', action='store_true', help='Validate non-default cosmetics, RPC selection and corpse identity')
args = parser.parse_args()

project = Path(__file__).resolve().parents[1]
engine = Path(os.environ.get("UE_ROOT", "/Users/Shared/Epic Games/UE_5.8"))
editor = engine / "Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
logs = project / "Saved/Testing"
logs.mkdir(parents=True, exist_ok=True)
common = [str(editor), str(project / "fdm.uproject"), "-game", "-nullrhi", "-nosound", "-unattended", "-NoSplash", "-Multiprocess", "-stdout", "-FullStdOutLogOutput", "-ExecCmds=t.MaxFPS 60"]
if args.shot_feedback:
    common.append('-ERShotAudit')
port = '17783' if args.accuracy else '17777'
label = 'accuracy' if args.accuracy else 'network'
probe = '-ERAccuracyProbe' if args.accuracy else '-ERNetworkProbe'
if args.skins: common.append('-ERSkinProbe')
processes = []
handles = []
try:
    host_log = (logs / (label + '-host.log')).open("w")
    handles.append(host_log)
    host = subprocess.Popen(common + ["/Game/EclipseRecoil/Maps/L_Depot?listen?Name=Host" + ("?AlphaSkin=3?OmegaSkin=2" if args.skins else ""), '-port=' + port, probe], stdout=host_log, stderr=subprocess.STDOUT)
    processes.append(host)
    deadline = time.monotonic() + 45
    while time.monotonic() < deadline:
        output = (logs / (label + '-host.log')).read_text(errors="replace")
        if 'listening on port ' + port in output:
            break
        if host.poll() is not None:
            raise RuntimeError('Host exited before listening; inspect ' + label + '-host.log')
        time.sleep(.2)
    else:
        raise RuntimeError("Host did not listen within 45 seconds")
    client_log = (logs / (label + '-client.log')).open("w")
    handles.append(client_log)
    client = subprocess.Popen(common + ['127.0.0.1:' + port + '?Name=Client' + ('?AlphaSkin=1?OmegaSkin=4' if args.skins else '')], stdout=client_log, stderr=subprocess.STDOUT)
    processes.append(client)
    deadline = time.monotonic()+100
    while time.monotonic()<deadline and host.poll() is None:
        time.sleep(.2)
    if host.poll() is None:
        raise RuntimeError("Network probe timed out")
    output = (logs / (label + '-host.log')).read_text(errors="replace")
    markers = [line for line in output.splitlines() if "ER_TEST_" in line or "ER_NETWORK_PROBE_COMPLETE" in line or 'ER_ACCURACY_' in line]
    print("\n".join(markers))
    success = "ER_NETWORK_PROBE_COMPLETE result=PASS" in output and "ER_TEST_FAIL" not in output
    if args.accuracy:
        remote = (logs / (label + '-client.log')).read_text(errors='replace')
        expected = ['owner_base_data', 'owner_ammo_bloom_replicated', 'owner_clock_recovers_bloom']
        success = 'ER_ACCURACY_COMPLETE result=PASS' in output and 'ER_ACCURACY_FAIL' not in output
        for marker in expected:
            passed = 'ER_ACCURACY_CLIENT_PASS ' + marker in remote
            print('ER_ACCURACY_' + ('PASS' if passed else 'FAIL'), marker)
            success = success and passed
    if args.shot_feedback:
        remote = (logs / 'network-client.log').read_text(errors='replace')
        for label, log in [('host', output), ('client', remote)]:
            events = [line for line in log.splitlines() if 'ER_SHOT_FEEDBACK' in line]
            valid_assets = events and all('animation=1 sound=1 flash=1' in line for line in events)
            perspectives = all(any('local=' + value in line for line in events) for value in ['0', '1'])
            passed = valid_assets and perspectives
            print('ER_TEST_' + ('PASS' if passed else 'FAIL'), label + '_shot_assets_and_both_perspectives', 'events=' + str(len(events)))
            success = success and passed
    remote = (logs / ('accuracy-client.log' if args.accuracy else 'network-client.log')).read_text(errors='replace')
    if not args.accuracy:
        for label, passed in [
            ('damage_owner_audio_and_direction', any('ER_DAMAGE_FEEDBACK' in line and 'sound=1' in line and 'direction=' in line for line in remote.splitlines())),
            ('corpse_replicated_and_settled', 'ER_CORPSE_CREATED' in remote and 'physics=1' in remote and 'ER_CORPSE_SETTLED' in remote),
            ('team_mesh_animation_sections_replicated', 'ER_TEAM_CLIENT_PASS phase=2' in remote and 'ER_TEAM_CLIENT_FAIL' not in remote),
            ('both_team_corpses_replicated', all(any('ER_CORPSE_CREATED' in line and 'mesh=' in line and mesh in line for line in remote.splitlines()) for mesh in ['SK_ER_Drongo_Omega','SK_ER_Twinblast_Alpha'])),
        ]:
            print('ER_TEST_' + ('PASS' if passed else 'FAIL'), label)
            success = success and passed
    if args.skins:
        for marker in ['Skin_preferences_replicated', 'Skin_pending_until_respawn', 'Skin_corpse_preserves_old_selection', 'Skin_new_selection_on_respawn', 'Skin_invalid_index_rejected']:
            passed = 'ER_TEST_PASS ' + marker in output
            print('ER_TEST_' + ('PASS' if passed else 'FAIL'), marker)
            success = success and passed
    print("Logs:", logs)
    sys.exit(0 if success else 1)
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
