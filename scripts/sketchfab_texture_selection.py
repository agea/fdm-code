"""Current owner-approved texture selection; pruned 20 October 2026."""

SOURCES = {
    '022d7b8e846345f487fbe751b3974c2b': 'brick_tiled',
    '64e67e4fb8a14ddeae07bd91d50fc846': 'concrete_seamless',
    '9b129e388857498f9fe395a43268c3e8': 'concrete_2k',
    'd52ea063c93a4bbc8c35962c18ee1bf7': 'iron_plates',
    'f7b6f74ef9294c189ce7dd4876d8f6f7': 'wood_05',
    '1698b2f676ef4fa2ac8e2faab21ae83d': 'snow_01',
    '9ed1e321c74c4828baebf084a5d53c80': 'grass_02',
    '08a8a0341f6d4a399d571d0133eec58b': 'asphalt_11',
    'e708ce13ea8843668ff833e347d4612e': 'steel_irregular',
    'b758a52fa80a44bfb0fc9fc0b2beba5e': 'brick_textures',
}
MATERIAL_IDS = {
    key for key in SOURCES.values() if key != 'brick_textures'
} | {'brick_textures_0', 'brick_textures_1', 'brick_textures_2'}
