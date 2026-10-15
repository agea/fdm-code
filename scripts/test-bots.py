#!/usr/bin/env python3
"""Run real listen-server/client tests for both optional training bot modes."""
from pathlib import Path
import os
import re
import subprocess
import time

project = Path(__file__).resolve().parents[1]
engine = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8'))
editor = engine / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs = project / 'Saved/Testing'
logs.mkdir(parents=True, exist_ok=True)
common = [str(editor),str(project/'RespawnAndRage.uproject'),'-game','-nullrhi','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']


def run_mode(mode, port):
    processes, handles = [], []
    host_path = logs / ('bots-'+mode.lower()+'-host.log')
    client_path = logs / ('bots-'+mode.lower()+'-client.log')
    try:
        handle = host_path.open('w'); handles.append(handle)
        host = subprocess.Popen(common+[f'/Game/EclipseRecoil/Maps/L_Depot?listen?ERLegacyArsenal=1?ERBots=1?ERBotMode={mode}?Name=Host',f'-port={port}','-ERBotProbe'],stdout=handle,stderr=subprocess.STDOUT)
        processes.append(host)
        deadline = time.monotonic()+45
        while time.monotonic()<deadline:
            if f'listening on port {port}' in host_path.read_text(errors='replace'):
                break
            if host.poll() is not None:
                raise RuntimeError(mode+' host exited before listening')
            time.sleep(.2)
        else:
            raise RuntimeError(mode+' host did not listen')
        handle = client_path.open('w'); handles.append(handle)
        client = subprocess.Popen(common+[f'127.0.0.1:{port}?Name=Client','-ERBotProbe'],stdout=handle,stderr=subprocess.STDOUT)
        processes.append(client)
        deadline=time.monotonic()+90
        while time.monotonic()<deadline and host.poll() is None:
            time.sleep(.2)
        if host.poll() is None:
            raise RuntimeError(mode+' bot probe timed out')
        output=host_path.read_text(errors='replace')
        client_output=client_path.read_text(errors='replace')
        markers=[line for line in output.splitlines() if 'ER_BOT_TEST_' in line or 'ER_BOT_PROBE_COMPLETE' in line]
        markers += [line for line in client_output.splitlines() if 'ER_BOT_CLIENT_PASS' in line]
        print('\n'.join(markers),flush=True)
        if host.returncode != 0 or 'ER_BOT_PROBE_COMPLETE result=PASS' not in output or 'ER_BOT_TEST_FAIL' in output:
            raise RuntimeError(mode+' server test failed')
        for marker in ['pawn_team_health_replicated','bot_death_score_replicated']:
            if 'ER_BOT_CLIENT_PASS '+marker not in client_output:
                raise RuntimeError(mode+' missing client replication check: '+marker)
        if f'ER_BOT_PAWN_CLASS mode={mode} class=/Game/EclipseRecoil/Core/BP_ERTrainingTarget.BP_ERTrainingTarget_C' not in output:
            raise RuntimeError(mode+' missing training pawn class')
        for side, text in [('host', output), ('client', client_output)]:
            for team,mesh in [('Alpha','SK_ER_Twinblast_Alpha'),('Omega','SK_ER_Drongo_Omega')]:
                if not re.search(rf'ER_TEAM_APPEARANCE team={team} mesh=/Game/EclipseRecoil/Characters/Teams/{team}/[^\n]*{mesh}', text):
                    raise RuntimeError(mode+' missing team character '+team+' on '+side)
        print(mode+' TEAM APPEARANCE PASS: Twinblast/Drongo on host/client.', flush=True)
    finally:
        for process in processes:
            if process.poll() is None:
                process.terminate()
        for process in processes:
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill(); process.wait()
        for handle in handles:
            handle.close()


if __name__ == '__main__':
    for mode,port in [('Patrol',17779),('Target',17780)]:
        run_mode(mode,port)
    print('BOT INTEGRATION PASS: both modes and remote-client replication. Logs:',logs)
