#!/usr/bin/env python3
"""Local 4-player admission, late join and disconnect integration check."""
from pathlib import Path
import argparse
import os
import subprocess
import time

project = Path(__file__).resolve().parents[1]
editor = Path(os.environ.get("UE_ROOT", "/Users/Shared/Epic Games/UE_5.8")) / "Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
logs = project / "Saved/Testing"
logs.mkdir(parents=True, exist_ok=True)
common = [str(editor), str(project / "RespawnAndRage.uproject"), "-game", "-nullrhi", "-nosound", "-unattended", "-NoSplash", "-Multiprocess", "-stdout", "-FullStdOutLogOutput", "-ExecCmds=t.MaxFPS 30"]
args = argparse.ArgumentParser()
args.add_argument("--bots", action="store_true", help="Start with three targets; human joins must replace them")
with_bots = args.parse_args().bots
log_prefix = "bots-" if with_bots else ""
processes, handles = [], []


def launch(name, args):
    handle = (logs / (log_prefix + name + ".log")).open("w")
    handles.append(handle)
    process = subprocess.Popen(common + args, stdout=handle, stderr=subprocess.STDOUT)
    processes.append(process)
    return process


def wait_for(name, needle, seconds=60, count=1):
    end = time.monotonic()+seconds
    while time.monotonic() < end:
        text = (logs / (log_prefix + name + ".log")).read_text(errors="replace")
        if text.count(needle) >= count:
            return text
        time.sleep(.2)
    raise RuntimeError(f"Missing {needle} in {name}.log")


try:
    map_url = "/Game/EclipseRecoil/Maps/L_Depot?listen" + ("?ERBots=3?ERBotMode=Target" if with_bots else "")
    host = launch("admission-host", [map_url, "-port=17778"])
    wait_for("admission-host", "listening on port 17778")
    if with_bots:
        wait_for("admission-host", "ER_BOT_JOINED name=Bersaglio 3")
    for number in range(1, 4):
        launch("admission-client-"+str(number), ["127.0.0.1:17778"])
        wait_for("admission-host", f"ER_PLAYER_JOINED players={number+1}")
    launch("admission-rejected", ["127.0.0.1:17778"])
    wait_for("admission-rejected", "Server pieno")
    text = (logs / (log_prefix + "admission-host.log")).read_text(errors="replace")
    joins = [line for line in text.splitlines() if "ER_PLAYER_JOINED" in line]
    assert len(joins) == 4, joins
    assert sum("team=Alpha" in line for line in joins) == 2, joins
    assert sum("team=Omega" in line for line in joins) == 2, joins
    assert text.count("ER_MATCH_STARTED") == 1, "Late joins reset the match"
    if with_bots:
        assert text.count("ER_BOT_REMOVED") == 3, "Human joins must replace all three bots"
        print("PASS BOT SLOTS: three bots replaced by joining humans; four-player cap preserved")
    else:
        assert "ER_BOT_JOINED" not in text, "Normal LAN host must have no bots"
    processes[1].terminate()
    processes[1].wait(timeout=10)
    wait_for("admission-host", "ER_PLAYER_LEFT players=3", seconds=35)
    if with_bots:
        wait_for("admission-host", "ER_BOT_JOINED name=Bersaglio 1", seconds=35, count=2)
        print("PASS BOT REFILL: one target returns after a human leaves")
    print("PASS T03: four players, Alpha 2 / Omega 2")
    print("PASS T17: late joins preserve the running match")
    print("PASS T20: fifth player rejected by server")
    print("PASS T19: disconnect removes player without respawn")
    print("Logs:", logs)
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
