#!/usr/bin/env python3
"""Opt-in local integration test using two actual Unreal game processes.

No cook, packaging or distribution. Outputs are stored in Saved/Testing.
The replicated probe only exists when -ERMovementProbe is explicitly supplied.
"""
from pathlib import Path
import os
import subprocess
import sys
import time

project = Path(__file__).resolve().parents[1]
engine = Path(os.environ.get("UE_ROOT", "/Users/Shared/Epic Games/UE_5.8"))
editor = engine / "Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
logs = project / "Saved/Testing"
logs.mkdir(parents=True, exist_ok=True)
common = [str(editor), str(project / "fdm.uproject"), "-game", "-nullrhi", "-nosound", "-unattended", "-NoSplash", "-Multiprocess", "-stdout", "-FullStdOutLogOutput", "-ExecCmds=t.MaxFPS 60"]
processes = []
handles = []
try:
    host_log = (logs / "movement-host.log").open("w")
    handles.append(host_log)
    host = subprocess.Popen(common + ["/Game/EclipseRecoil/Maps/L_MovementLab?listen?Name=Host", "-port=17781", "-ERMovementProbe"], stdout=host_log, stderr=subprocess.STDOUT)
    processes.append(host)
    deadline = time.monotonic() + 45
    while time.monotonic() < deadline:
        output = (logs / "movement-host.log").read_text(errors="replace")
        if "listening on port 17781" in output:
            break
        if host.poll() is not None:
            raise RuntimeError("Host exited before listening; inspect movement-host.log")
        time.sleep(.2)
    else:
        raise RuntimeError("Host did not listen within 45 seconds")
    client_log = (logs / "movement-client.log").open("w")
    handles.append(client_log)
    client = subprocess.Popen(common + ["127.0.0.1:17781?Name=Client"], stdout=client_log, stderr=subprocess.STDOUT)
    processes.append(client)
    deadline = time.monotonic()+100
    while time.monotonic()<deadline and host.poll() is None:
        time.sleep(.2)
    if host.poll() is None:
        raise RuntimeError("Movement probe timed out")
    output = (logs / "movement-host.log").read_text(errors="replace")
    markers = [line for line in output.splitlines() if "ER_MOVEMENT_" in line or "ER_MOVEMENT_COMPLETE" in line]
    print("\n".join(markers))
    success = "ER_MOVEMENT_COMPLETE PASS" in output and "ER_MOVEMENT_FAIL" not in output
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
