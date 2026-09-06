# ADR 0066: Original character motion

Status: accepted; milestone 65.

Recover the actual AvatarController update, including its native 16 ms cadence,
rather than interpreting archetype momentum values as SI velocities. Keep the
modern fixed 60 Hz authority and normalize geometric retention over elapsed
time. A pure gameplay function is shared by server movement and prediction;
Jolt continues to resolve the imported Factory collision mesh.

Character profiles are resolved from server-owned selections. Dodge is a
one-shot input flag transported with existing input redundancy, acknowledged
and replayed like jump. Protocol 15 rejects prior peers. Factory's movement
contract revision is included in its scene fingerprint.

Preserve the former blockout laboratory rules for their existing tests. The
Factory path uses the original capsule proportions and per-character movement.
Tests compare the pure motion step against an independent transcription at
16 ms, exercise time-normalized response, wire validation, and real Factory
jump/landing. See [GAMEPLAY_MIGRATION.md](../GAMEPLAY_MIGRATION.md) for conversion
details and explicit differences from PhysX and multi-key dodge behavior.
