#!/usr/bin/env python3
"""Boot a Development package with a pistol-equipped player, six bots and native graphics defaults."""
import argparse
import json
from pathlib import Path
import re
import statistics
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--executable', type=Path, required=True)
parser.add_argument('--render', action='store_true', help='Use Metal offscreen instead of the null renderer')
parser.add_argument('--shader-model', choices=('sm5', 'sm6'), help='Force a Metal shader path (requires --render)')
parser.add_argument('--seconds', type=int, default=20)
parser.add_argument('--width', type=int, default=1920)
parser.add_argument('--height', type=int, default=1080)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
if args.shader_model and not args.render:
    parser.error('--shader-model requires --render')
if args.seconds < 10:
    parser.error('Allow at least ten seconds for map loading and animation evaluation')
out = args.output or Path(__file__).resolve().parents[1] / 'Saved/Testing/packaged-runtime'
out.mkdir(parents=True, exist_ok=True)
report_name = ('metal-' + args.shader_model if args.shader_model else 'metal') if args.render else 'null'
log = out / (report_name + '.log')
with tempfile.TemporaryDirectory(prefix='rr-package-user-') as user:
    command = [str(args.executable.resolve()),
               '/Game/EclipseRecoil/Maps/L_Depot?Primary=1?ERAlphaBots=3?EROmegaBots=3?ERBotMode=Target',
               '-unattended', '-nosound', '-NoSplash', '-stdout', '-FullStdOutLogOutput',
               '-UserDir=' + user, '-Seconds=' + str(args.seconds), '-ERPerformanceAudit']
    command += ['-RenderOffscreen', '-windowed', '-ResX=' + str(args.width), '-ResY=' + str(args.height)] if args.render else ['-nullrhi']
    if args.shader_model:
        command.append('-' + args.shader_model)
    with log.open('w') as stream:
        process = subprocess.Popen(command, stdout=stream, stderr=subprocess.STDOUT)
        try:
            code = process.wait(timeout=args.seconds + 90)
        except subprocess.TimeoutExpired:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            raise RuntimeError('Packaged game timed out; inspect ' + str(log))
text = log.read_text(errors='replace')
checks = dict(clean_exit=code == 0,
              native_graphics_profile='ER_RENDER_PROFILE aa=2 gi=0 reflections=0 shadows=1' in text,
              pistol_evaluated='ER_WEAPON_MODEL name=Pistol' in text,
              six_bots=text.count('ER_BOT_JOINED') >= 6,
              no_fatal=not any(m in text for m in ['Assertion failed', 'Fatal error', 'Critical error']))
fps = [int(n) for n in re.findall(r'ER_PERFORMANCE (\d+) FPS', text)]
if args.render:
    checks['real_frames'] = len(fps) >= 5
selected = re.search(r'Metadata set : shaderplatform="METAL_(SM[56])"', text)
selected_shader_model = selected.group(1).lower() if selected else None
if args.shader_model:
    checks['requested_shader_model'] = selected_shader_model == args.shader_model
report = dict(result='PASS' if all(checks.values()) else 'FAIL', exit_code=code, checks=checks,
              renderer='Metal' if args.render else 'null',
              shader_model=args.shader_model,
              selected_shader_model=selected_shader_model,
              fps_median=statistics.median(fps[5:]) if args.render and len(fps) > 5 else None,
              log=str(log), command=command)
(out / (report_name + '.json')).write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
raise SystemExit(0 if all(checks.values()) else 1)
