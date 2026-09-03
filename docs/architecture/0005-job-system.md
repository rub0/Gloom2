# ADR 0005: Engine job system and scheduler boundary

## Status

Accepted

## Context

Scene preparation, asset processing and future gameplay systems need bounded
parallel work without depending on a middleware scheduler. Waiting naively on a
worker can deadlock a fixed-size pool when nested work occupies every thread.

Jolt can use an external scheduler, but its graph grows while jobs are already
running and its barriers have specific reference-lifetime rules. The official
[JobSystem documentation](https://jrouwe.github.io/JoltPhysics/class_job_system.html)
warns that unrelated physics work during a barrier wait can cause races or
deadlocks. The maintainer recommends deriving from `JobSystemWithBarrier` and
forwarding immediately runnable jobs when integrating an engine pool
([discussion 1726](https://github.com/jrouwe/JoltPhysics/discussions/1726)).

## Decision

Gloom owns a C++23 scheduler using move-only jobs, persistent `std::jthread`
workers and reusable task groups. A waiting thread executes available queued
work before sleeping, so nested task-group waits can make forward progress.
Exceptions are captured per group and rethrown by `wait` after all group work
has completed.

The scheduler records submitted/completed jobs, work executed by a waiting
thread, peak queue depth, aggregate job execution time and aggregate wait time.
The visual demo uses two reusable groups to update physics-derived poses and
build disjoint ranges of the immutable render snapshot.

Jolt keeps `JobSystemThreadPool` for this milestone. We will not write a thin,
untested adapter that ignores Jolt's dynamic graph and barrier contract. A
future adapter must derive from `JPH::JobSystemWithBarrier`, be stress-tested,
and demonstrate a measurable advantage over the separate pool.

## Consequences

- Engine code can schedule backend-independent parallel work and wait safely
  from either the frame thread or another job.
- Task groups are tied to the scheduler that created them and must not outlive
  that scheduler.
- Snapshot writes are partitioned into disjoint ranges; Diligent submission
  remains on the frame thread.
- Metrics are diagnostic aggregates, not a substitute for a sampling profiler.
- The application uses two engine workers for its small scene to avoid spawning
  a large second pool while Jolt still owns its workers.
