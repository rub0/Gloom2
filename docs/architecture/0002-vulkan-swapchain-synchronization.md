# ADR 0002: Vulkan swapchain synchronization

## Status

Accepted

## Context

Diligent Core revision `b036337d` selected its draw-complete semaphore using a
frame-cycling index. Vulkan presentation does not guarantee that swapchain
images are acquired in a simple round-robin order. A semaphore associated with
an earlier presentation could therefore be signalled again before the
presentation engine had finished waiting on it, triggering
`VUID-vkQueueSubmit-pSignalSemaphores-00067`.

## Decision

Pin Diligent Core to commit `a255d24d365e13edeade3b02676f444a493bc883`,
the upstream fix for DiligentCore issue 682. The fix selects the draw-complete
semaphore by acquired back-buffer index, giving each swapchain image its own
semaphore.

Keep a graphics regression test named `gloom.vulkan_sync`. It runs an unthrottled
600-frame render sequence with VSync disabled and a minimize/restore cycle, and
rejects Vulkan validation errors in the process output.

## Consequences

Fresh builds use a reproducible Diligent revision containing the fix. Dependency
upgrades must continue to pass the synchronization stress test with Vulkan
validation enabled.
