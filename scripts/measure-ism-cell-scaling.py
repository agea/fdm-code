#!/usr/bin/env python3
"""Measure the current one-ISM-instance-plus-Chaos-body-per-block path at finer
block sizes in the real L_DestructionLab game world. Nothing is saved.

Run (UE_ROOT as in the other scripts):
  UnrealEditor RespawnAndRage.uproject /Game/EclipseRecoil/Maps/L_DestructionLab -game -nullrhi \
    -nosound -unattended -NoSplash -stdout -windowed -ResX=640 -ResY=360 \
    "-ExecCmds=py $PWD/scripts/measure-ism-cell-scaling.py"
Prints ER_SCALE json lines: instances, regenerate ms, RSS delta, single removal ms.
"""
import unreal, time, os, subprocess, json, statistics
def rss(): return int(subprocess.check_output(['ps','-o','rss=','-p',str(os.getpid())]).strip())*1024
world=None
for w in unreal.ObjectIterator(unreal.World):
    if 'L_DestructionLab' in w.get_path_name() and unreal.GameplayStatics.get_player_controller(w,0): world=w
room=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ERBlockRoom)[0]
pawn=unreal.GameplayStatics.get_player_pawn(world,0)
origin=room.get_actor_location()
out=[]
def run(size, chunk):
    unreal.SystemLibrary.collect_garbage()
    before=rss()
    cfg=room.get_editor_property('config'); cfg.block_size=unreal.Vector(*size); cfg.chunk_size=chunk
    room.set_editor_property('config',cfg)
    t=time.perf_counter(); ok=room.regenerate(); gen=(time.perf_counter()-t)*1000
    after=rss()
    comps=room.get_components_by_class(unreal.InstancedStaticMeshComponent)
    inst=sum(c.get_instance_count() for c in comps)
    times=[]
    for k in range(48):
        x=origin.x-230+k*9.7; z=origin.z+15+(k%10)*27
        hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,origin.y,z),unreal.Vector(x,origin.y-400,z),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[pawn],unreal.DrawDebugTrace.NONE,True)
        if hit is None or hit.to_tuple()[9] is None: continue
        t=time.perf_counter(); d=unreal.GameplayStatics.apply_point_damage(room,1e6,unreal.Vector(0,-1,0),hit,None,None,unreal.DamageType); dt=(time.perf_counter()-t)*1000
        if d>0: times.append(dt)
    r=dict(size=size,chunk=chunk,ok=ok,instances=inst,components=len(comps),regenerate_ms=round(gen,2),rss_delta_mib=round((after-before)/2**20,2),
           single_removals=len(times),single_ms_median=round(statistics.median(times),4) if times else None,single_ms_max=round(max(times),4) if times else None)
    unreal.log('ER_SCALE '+json.dumps(r)); out.append(r)
for size in [(50,20,25),(10,20,10),(5,20,10),(5,20,5)]:
    for rep in range(3): run(size,400)
for rep in range(2): run((5,20,5),100)
run((50,20,25),400)
unreal.log('ER_SCALE_DONE')
unreal.SystemLibrary.execute_console_command(world,'quit')
