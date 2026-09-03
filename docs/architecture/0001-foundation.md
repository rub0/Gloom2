# ADR 0001: Modern foundation

Status: accepted

## Decision

Gloom is rebuilt in a new repository while the legacy repository remains a
read-only behavioural and asset reference. The engine implementation uses
C++23. Third-party libraries remain isolated behind interfaces owned by Gloom.

The initial stack is:

- CMake with the Visual Studio generator for orchestration on Windows.
- Visual C++ for C++23 compilation and optimized native code.
- SDL3 for windows, input, displays and platform integration.
- Diligent Engine as the first rendering backend, with Vulkan first and D3D12
  kept available for debugging and vendor SDK interoperability.
- Jolt Physics for multithreaded simulation.
- GameNetworkingSockets for secure transport.

SDL and Diligent are complementary rather than competing choices: SDL owns the
platform layer; Diligent owns the graphics abstraction. Gloom owns the public
interfaces so either backend can be replaced.

## Consequences

- The old code will be ported by behaviour, not copied wholesale.
- Third-party types and headers must not leak into public engine interfaces.
- Upscalers such as FSR and DLSS belong in a future render-feature layer, not in
  the renderer interface itself.
- Each backend must have a testable placeholder or mock implementation.
