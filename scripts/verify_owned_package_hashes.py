"""Check saved baselines, accepting only the explicitly recorded palette migration.
A migrated file must match both the older receipt's exact before hash and the
current receipt's exact after hash. All other package hashes remain strict.
"""
import hashlib,json

def verify_owned_package_hashes(project,expected):
 receipt=project/'Saved/Testing/MutedLegacyPalette/changed.json'
 migrations={r['file']:r for r in json.loads(receipt.read_text())} if receipt.exists() else {}
 for rel,before in expected.items():
  actual=hashlib.sha256((project/rel).read_bytes()).hexdigest()
  if actual==before:continue
  row=migrations.get(rel)
  assert row and row['before_hash']==before and row['after_hash']==actual,rel
