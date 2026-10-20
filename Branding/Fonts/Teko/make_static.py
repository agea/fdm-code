#!/usr/bin/env python3
"""Create Unreal's static SemiBold instance; requires fonttools (build-time only)."""
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
root = Path(__file__).resolve().parent
source = root / 'Fonts/Teko-Variable.ttf'
font = TTFont(source)
instance = instantiateVariableFont(font, {'wght': 600}, inplace=False, updateFontNames=True)
assert 'fvar' not in instance, 'The runtime font must be a static instance'
assert all(codepoint in instance.getBestCmap() for codepoint in range(32, 127))
instance.save(root / 'Fonts/Teko-SemiBold.ttf')
print('Teko SemiBold, static weight 600:', instance['name'].getDebugName(2))
