# Rooms

Shared rules are in [MAPS.md](../MAPS.md).

**Code:** `ERRooms.cpp`.

Sealed half-turn room graph with no arena floor outside it. 4, 6, 8 or 10 rooms.

- Corridors 3.6m wide, at least 560cm high (legacy saved 340 is raised at runtime), four clipped
  corners, slope ≤1:2. Closed shell prisms provide collision.
- Four-room plans: all rooms at rectangle corners, Alpha northwest, Omega southeast, two attack
  paths, optional central hall-to-hall link. Halls at least twice a base's area, 55%+ real floor
  coverage (60% for the large rectangle), 2m apart, 6m approaches.
- Larger graphs add an upper crossing with galleries at 620cm and 310cm switchbacks.
- No single corridor may disconnect bases. Starts and routes are reserved before furnishing.
- Sparse 2.4m timber boards every 5m on runs over 10m; at most two per room.

## Verify

`test-rooms.py` (`--render --offscreen --network`). Native: `EclipseRecoil.Destruction.Rooms*`.
