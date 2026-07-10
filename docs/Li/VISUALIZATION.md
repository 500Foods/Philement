# Visualization Module Implementation Plan

**Project Context**  
This document outlines a phased implementation plan for a self-contained **Visualization Manager** module. The module will be added to an existing vanilla JS/CSS/HTML application. It will render interactive 3D visualizations (primarily plant growth simulations from a C+Lua backend, with future support for G-code toolpaths) inside a provided `<div>` container.

**Core Architectural Principles**
- Multi-threaded Rust + WebAssembly + native WebGPU ("we control everything" approach)
- High performance and control; minimal reliance on heavy frameworks
- Clean separation between the JS integration layer and the Rust/Wasm rendering core
- Support for time-based scrubbing/evaluation (discrete days with optional interpolation)
- Designed to support both detailed single-plant views and large-scale greenhouse scenes
- Extensible to G-code visualization and offline high-quality video export
- Sequential, gated implementation with explicit deliverables per phase

**Overall Approach**
- The visualization logic lives primarily in a Rust + wgpu compiled to multi-threaded WebAssembly module.
- A thin vanilla JavaScript "Manager" class handles mounting into a `<div>`, passing configuration/data, and bridging to the Wasm core.
- Each phase has a clear **Gate** (deliverable + test) that must be satisfied before proceeding.
- A living **Work History / Lessons Learned** file will be maintained alongside implementation to capture assumptions, discoveries, and trajectory changes.

---

## Phase 1: Environment, Tooling & Project Skeleton

**Purpose**  
Establish a clean, reproducible development environment and project structure for both the Rust/Wasm core and the JavaScript integration layer.

**Why Needed**  
Prevents later friction with build systems, Wasm threading requirements, and integration between Rust and vanilla JS. Early decisions here affect every subsequent phase.

**Implementation Targets**
- Create a new subdirectory for the visualization module (e.g. `visualization/` or `viz-manager/`)
- Set up a Rust workspace with `wasm-bindgen`, `wgpu`, and threading support
- Configure `wasm-pack` or equivalent for building multi-threaded Wasm
- Basic HTML test harness that loads the Wasm module
- Initial `package.json` (or equivalent) for any JS build tooling if needed (keep minimal)
- Document required server headers (`Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy`)

**Gate / Deliverable / Test**
- A minimal "Hello Triangle" WebGPU example renders inside a `<canvas>` element when the page loads.
- Multi-threaded Wasm is confirmed working (simple parallel computation visible in browser dev tools).
- Build process is documented and reproducible on a fresh machine.

---

## Phase 2: Multi-threading & Shared Memory Infrastructure

**Purpose**  
Establish reliable multi-threaded execution in WebAssembly so heavy work (reconciliation, culling, buffer preparation, growth evaluation) can run off the main JS thread.

**Why Needed**  
The visualization must remain responsive during scrubbing, large data processing, and video export. Single-threaded Wasm would block the UI on anything non-trivial.

**Implementation Targets**
- Set up `wasm-bindgen` + `rayon` (or manual worker + `SharedArrayBuffer`) patterns
- Create a simple worker thread that can receive work and return results
- Demonstrate non-blocking behavior from the main thread
- Document the exact header requirements and how to serve them

**Gate / Deliverable / Test**
- A demo that performs a CPU-intensive task (e.g. processing 100k items) in a Wasm worker thread while the main thread remains fully responsive (UI updates continue smoothly).
- Clear documentation of how to enable/disable multi-threading.

---

## Phase 3: WebGPU Context, Device & Basic Pipeline

**Purpose**  
Create the foundational WebGPU rendering infrastructure inside the Wasm module.

**Why Needed**  
All visual output depends on a stable, efficient WebGPU device, queue, and pipeline setup. This is the lowest-level graphics foundation.

**Implementation Targets**
- Initialize WebGPU adapter/device with appropriate features (limits, etc.)
- Create a basic render pipeline (shader, bind group layout)
- Render a simple colored triangle or quad that can be driven from JS
- Handle resize and context loss gracefully

**Gate / Deliverable / Test**
- A colored triangle renders inside the target `<div>` / `<canvas>` and resizes correctly when the container changes size.
- Basic error handling and context recovery is in place.

---

## Phase 4: Camera System (Orbit + Basic Fly Controls)

**Purpose**  
Implement a reusable camera that supports both inspection (orbit) and exploration (fly) modes.

**Why Needed**  
Users need to move around the 3D scene naturally. This is a core interaction primitive required by almost every later feature.

**Implementation Targets**
- Implement a `Camera` struct in Rust with view/projection matrices
- Support orbit (around a target), pan, zoom, and basic fly/WASD movement
- Expose camera parameters to JavaScript so the JS layer can drive it
- Add support for multiple control scheme presets (Unity-style as default, with hooks for others)

**Gate / Deliverable / Test**
- User can orbit, pan, and zoom a simple scene using mouse/trackpad with Unity-like controls.
- Camera matrices are correctly passed into the Wasm renderer each frame.

---

## Phase 5: Input Handling Layer (JS + Optional Gamepad)

**Purpose**  
Create a clean input abstraction in JavaScript that feeds into the Wasm camera and future interaction systems.

**Why Needed**  
Decouples raw DOM events from the rendering core. Enables easy swapping of control schemes and future gamepad support.

**Implementation Targets**
- Vanilla JS input manager that captures mouse, wheel, keyboard, and pointer events
- Mapping system for different control presets (Unity, Blender, FPS)
- Optional Gamepad API integration (nice-to-have, lower priority)
- Events are translated into camera commands and sent to Wasm

**Gate / Deliverable / Test**
- All basic navigation (orbit/pan/zoom) works smoothly from mouse and keyboard.
- Control scheme can be switched at runtime via a simple API.

---

## Phase 6: Plant Data Model & Server Ingestion

**Purpose**  
Define and implement ingestion of the compact render-oriented data format produced by the C+Lua growth model server.

**Why Needed**  
The visualization must consume the server’s output efficiently. A good data model here affects memory usage, reconciliation speed, and extensibility.

**Implementation Targets**
- Define Rust structs for the compact growth representation (stable IDs, birth times, parameters, etc.)
- Implement deserialization (binary preferred over JSON for performance)
- Basic validation and loading of a sample plant dataset
- Expose loading API to JavaScript

**Gate / Deliverable / Test**
- A sample plant dataset from the server can be loaded into the Wasm module and basic statistics (number of elements, time range) can be queried from JavaScript.

---

## Phase 7: Time-based State Evaluation & Reconciliation

**Purpose**  
Implement the core logic that, given a time value `t`, produces the current visual state of the scene (which elements exist, their current parameters, transforms, etc.).

**Why Needed**  
This is the heart of time scrubbing, day-to-day jumps, and future support for variable model granularity (daily vs weekly). It enables smooth animation and the dual-time-axis video export feature.

**Implementation Targets**
- Efficient evaluation of element state at arbitrary time `t`
- Reconciliation system that computes the minimal set of changes between two times
- Support for both discrete jumps and light interpolation (optional per element)
- Performance suitable for real-time scrubbing on desktop

**Gate / Deliverable / Test**
- Given two different time values, the system can produce the before/after state and a diff of changed elements.
- Scrubbing a slider in JavaScript updates the rendered scene at interactive rates.

---

## Phase 8: Instanced Geometry & Stem/Branch Rendering

**Purpose**  
Implement efficient rendering of many tapered, faceted cylindrical segments (and later other primitives) using instancing and shader-generated geometry.

**Why Needed**  
Plant stems and branches are the dominant geometry. Naive per-mesh rendering will not scale. This is also foundational for G-code extrusion visualization.

**Implementation Targets**
- Instanced rendering pipeline for tapered frustums/cylinders
- Vertex shader that generates faceted geometry on the fly from per-instance attributes (start, end, radius, etc.)
- Basic material system (color, opacity for ghosting)
- Support for consistent facet alignment or deliberate twisting at joints

**Gate / Deliverable / Test**
- Several hundred instanced tapered cylinders render at interactive framerates with correct tapering and joint behavior.
- Changing instance attributes (length, radius, color) updates the scene correctly.

---

## Phase 9: LOD, Frustum Culling & Basic Performance

**Purpose**  
Add level-of-detail and culling so the system can handle thousands of elements and eventually many plants without melting the GPU.

**Why Needed**  
Even with instancing, rendering everything at full detail for large greenhouse scenes or zoomed-out views is wasteful. This phase makes the system production-viable at scale.

**Implementation Targets**
- Simple distance-based or screen-size LOD for segments
- Frustum culling of instances
- Basic instance buffer compaction / visibility buffer
- Performance metrics exposed to JavaScript (draw calls, visible instances)

**Gate / Deliverable / Test**
- A scene with 10,000+ segments renders smoothly when zoomed out, with clear reduction in draw calls as LOD/culling activates.
- Performance stays acceptable on a typical desktop GPU when many elements are off-screen.

---

## Phase 10: JavaScript Visualization Manager (The Public API)

**Purpose**  
Create the thin vanilla JS class that the rest of the application will use to embed and control the visualization.

**Why Needed**  
This is the integration point. It must feel natural in a vanilla JS codebase and hide the complexity of Wasm/WebGPU.

**Implementation Targets**
- `VisualizationManager` class that takes a `container: HTMLElement` (or selector)
- Methods: `loadPlantData(data)`, `setTime(t)`, `setCameraMode(...)`, `resize()`, `dispose()`
- Clean event system for camera changes, loading progress, etc.
- Fullscreen toggle support (secondary priority)
- Error boundaries and graceful degradation messaging

**Gate / Deliverable / Test**
- A developer can mount the visualizer into any `<div>` with a few lines of vanilla JS and drive it via a simple public API.
- The module can be destroyed and re-created without memory leaks.

---

## Phase 11: G-code Data Path (Segment-based Representation)

**Purpose**  
Add support for visualizing G-code toolpaths using a similar segment-based, time-aware data model.

**Why Needed**  
Extends the visualization engine to a second major domain (3D printing) with significant overlap in rendering and time-scrubbing needs.

**Implementation Targets**
- Parser or ingestion path that converts G-code into compact extrusion segments (start, end, diameter, time range, layer, etc.)
- Reuse of the instanced rendering pipeline (adapted for lines/extrusions)
- Time-based visibility for "already printed" vs "future" segments with ghosting
- Layer scrubbing support

**Gate / Deliverable / Test**
- A sample G-code file can be loaded and scrubbed through time, showing progressive deposition with correct transparency for future segments.

---

## Phase 12: Waypoint & Bezier Camera Path System

**Purpose**  
Implement a system for defining camera waypoints and generating smooth fly-through paths.

**Why Needed**  
Enables cinematic presentations and automated video rendering. This is a key feature for the offline video export use case.

**Implementation Targets**
- Data structures for waypoints (position + target/look-at)
- Bezier (or Catmull-Rom) path evaluation
- Animation controller that can play/pause/scrub along the path
- Integration with the time-based simulation evaluation (dual time axes)

**Gate / Deliverable / Test**
- User can define 4–5 waypoints via a simple JS API; the camera smoothly follows the generated path while the scene state updates according to simulation time.

---

## Phase 13: Offline Video Export Pipeline (Batched + ffmpeg.wasm)

**Purpose**  
Implement high-quality frame-by-frame video rendering with batching for memory efficiency and resumability.

**Why Needed**  
Supports the requested cinematic fly-through video export feature (including dual-time-axis rendering) without requiring server GPU resources.

**Implementation Targets**
- Frame capture at high internal resolution
- Integration with multi-threaded `ffmpeg.wasm` for batched MP4 segment encoding
- Final concatenation of segments
- Progress reporting, pause/resume, and cancellation support
- Consistent encoding settings across batches to minimize boundary artifacts

**Gate / Deliverable / Test**
- A short (10–15 second) fly-through can be exported to an MP4 file using the batched approach.
- The resulting video plays back smoothly with no obvious artifacts at batch boundaries.

---

## Phase 14: Fullscreen, Polish & Cross-browser Hardening

**Purpose**  
Add fullscreen support (especially useful on iOS) and perform final cross-browser testing and polish.

**Why Needed**  
Improves usability on tablets and ensures the module works reliably across the target browsers (Chrome, Firefox, Safari desktop + mobile).

**Implementation Targets**
- Fullscreen API integration with proper handling for iOS Safari quirks
- Final pass on input handling, camera feel, and error messages
- Performance and memory profiling on target platforms
- Documentation of known limitations and browser-specific behaviors

**Gate / Deliverable / Test**
- The visualization can be toggled into fullscreen on desktop and iOS devices with acceptable behavior.
- All core features work on Chrome, Firefox, and Safari (desktop and mobile) with no major regressions.

---

## Phase 15: Lessons Learned / Work History System

**Purpose**  
Create a living document and process for capturing discoveries, changed assumptions, and trajectory adjustments as implementation proceeds.

**Why Needed**  
Long, multi-phase projects inevitably encounter surprises. Without a structured way to record and propagate lessons, later phases can repeat mistakes or work against earlier decisions.

**Implementation Targets**
- Create `WORK_HISTORY.md` (or similar) in the visualization module directory
- Define a lightweight format for phase retrospectives (what worked, what didn’t, changed assumptions, impact on future phases)
- Establish the habit of updating the history at the end of each phase (or when significant discoveries occur)
- Make the history file easy for the coordinating model (and future developers) to consume

**Gate / Deliverable / Test**
- `WORK_HISTORY.md` exists with entries for Phases 1–3 (minimum).
- The process for updating it is documented and has been used at least once during early phases.

---

## Sequencing & Dependency Notes

- Phases 1–5 form the **foundational layer** and should be completed before heavy domain logic.
- Phases 6–9 focus on the **plant growth domain** and core visualization power.
- Phase 10 creates the **public JS API** — this can begin once Phase 4–5 are stable.
- Phases 11–13 add **extended capabilities** (G-code, waypoints, video export) and can run somewhat in parallel once the core is solid.
- Phase 14 is **polish and validation**.
- Phase 15 runs continuously as a parallel supporting process.

**Recommended Starting Sequence**
1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10 → (11 | 12 | 13 in parallel or sequential) → 14 + ongoing 15

---

## Success Criteria for the Overall Module

When complete, a developer should be able to:

1. Drop the visualization module into their vanilla JS app.
2. Mount it into any `<div>`.
3. Load plant growth data (or G-code) produced by the backend.
4. Scrub time interactively with good performance.
5. Navigate the scene with familiar controls.
6. Trigger high-quality offline video renders of camera paths.
7. Extend or maintain the system without fighting heavy framework abstractions.

This phased plan is designed to be handed to an implementation model that can then expand each phase with concrete file-level steps, integration points with the existing codebase, and exact test criteria.