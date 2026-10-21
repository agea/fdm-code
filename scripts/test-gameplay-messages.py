#!/usr/bin/env python3
"""Run native catalog/notification/deferred-attacker checks without cooking or publishing."""
from pathlib import Path
import os
import subprocess

project = Path(__file__).resolve().parents[1]
editor = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
log = project / 'Saved/Testing/gameplay-messages-automation.log'
log.parent.mkdir(parents=True, exist_ok=True)
command = [str(editor), str(project / 'RespawnAndRage.uproject'), '/Engine/Maps/Entry',
           '-ExecCmds=Automation RunTests EclipseRecoil.Messages', '-TestExit=Automation Test Queue Empty',
           '-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput']
with log.open('w') as output:
    result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=180)
text = log.read_text(errors='replace')
for line in text.splitlines():
    if 'Test Completed.' in line or 'LogAutomationController: Error:' in line:
        print(line)
passed = 'Result={Success} Name={CatalogAndDelivery}' in text and 'Result={Fail}' not in text
print('Log:', log)
raise SystemExit(0 if result.returncode == 0 and passed else 1)
