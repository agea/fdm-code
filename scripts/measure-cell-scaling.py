#!/usr/bin/env python3
"""Measure 5cm cell-mode generation in the real L_DestructionLab game world as the
lab room grows (more cells and chunks). Nothing is saved.

Run (UE_ROOT as in the other scripts):
  UnrealEditor RespawnAndRage.uproject /Game/EclipseRecoil/Maps/L_DestructionLab -game -nullrhi \\
    -nosound -unattended -NoSplash -stdout -windowed -ResX=640 -ResY=360 \\
    "-ExecCmds=py $PWD/scripts/measure-cell-scaling.py"
Prints ER_CELL_SCALE json lines (chunk components, regenerate ms, RSS delta);
the matching ER_BLOCK_GENERATED lines give the cell counts.
"""
import unreal, time, os, subprocess, json, statistics
def rss(): return int(subprocess.check_output(['ps','-o','rss=','-p',str(os.getpid())]).strip())*1024
world=None
for w in unreal.ObjectIterator(unreal.World):
    if 'L_DestructionLab' in w.get_path_name() and unreal.GameplayStatics.get_player_controller(w,0): world=w
room=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ERBlockRoom)[0]
for interior,height in [(500,300),(1000,300),(1500,500),(2500,500)]:
    times=[]; deltas=[]; chunks=0
    for rep in range(3):
        cfg=room.get_editor_property('config'); cfg.interior_size=interior; cfg.height=height
        room.set_editor_property('config',cfg); room.set_editor_property('cell_mode',True)
        unreal.SystemLibrary.collect_garbage(); before=rss()
        t=time.perf_counter(); ok=room.regenerate(); times.append((time.perf_counter()-t)*1000)
        deltas.append((rss()-before)/2**20)
        chunks=len(room.get_components_by_class(unreal.DynamicMeshComponent))
    r=dict(interior=interior,height=height,ok=ok,chunk_components=chunks,regenerate_ms_median=round(statistics.median(times),2),rss_delta_mib=[round(d,1) for d in deltas])
    unreal.log('ER_CELL_SCALE '+json.dumps(r))
unreal.log('ER_CELL_SCALE_DONE')
unreal.SystemLibrary.execute_console_command(world,'quit')
