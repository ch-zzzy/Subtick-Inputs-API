# Subtick Inputs API

An API that lets consumer mods register inputs at sub-tick resolution while keeping the player's physics faithful to vanilla.

The API does nothing on its own. A consumer mod (like [Superb Input Precision](https://github.com/ch-zzzy/Superb-Input-Precision)) uses it to deliver finer input precision. Vanilla does all the physics, but the API applies Y-displacement correction on top of it so that an input processed partway through a tick lands where vanilla would have put it at that moment.

## How it works

Vanilla runs its normal physics ticks. The API midhooks `PlayerObject::update` to inject a per-player Y-displacement adjustment, computed from the player's inputs for that tick (an impulse term plus an acceleration term). This avoids the intermediate-velocity error that tick-splitting introduces for accelerating gamemodes, while still giving sub-tick input placement.

## API surface

Everything lives in the `subtickinputs` namespace. A `prelude` sub-namespace is provided to pull in the common symbols at once.

### `Config` singleton

`Config::get()` returns the shared instance.

- `getInputHz()` / `setInputHz(float)`: input check rate (overriden by Instantaneous Input)
- `isInstantInputsEnabled()` / `setInstantInputsEnabled()`: whether input processing is snapped to the input check rate grid, or happens immediately
- `isVelocityUnroundingEnabled()` / `setVelocityUnroundingEnabled(bool)`: whether y-velocity 3dp rounding is bypassed
- `isApiDisabled()`: master-switch state, already checked in `useVanillaPhysics()`

### `physics` namespace

- `useVanillaPhysics()`: returns `true` when the API should let vanilla run without modifications (API disabled, null PlayLayer, mod disabled, first frame after pause/death/init, player died, platformer, or RobTop's replay mode). Dependent mods should check this before using any of the api's features.

### `inputs` namespace

- `processInputs(PlayerObject*, float dt)`: processes the player's queued inputs for the current tick (dispatching each via `handleButton` and `updateJump(0)`) and accumulates the sub-tick Y-displacement adjustment into `m_yDispAdjustment` for the midhook to apply. `dt` is the tick duration passed to `processQueuedButtons`.

## Using it as a dependency

Add it to your `mod.json`:

```json
"dependencies": {
    "chizz.subtick-inputs-api": {
        "version": ">=v0.4.0",
        "required": true
    }
}
```

Then include the header and drive it from your input hook:

```cpp
#include <chizz.subtick-inputs-api/include/SubtickInputs.hpp>
using namespace subtickinputs::prelude;

// in your processQueuedButtons hook, per player:
if (!useVanilla()) {
    processInputs(dt);
}
GJBaseGameLayer::processQueuedButtons(dt, clearInputQueue);
```

While the api is in beta (v0.x.x), minor version changes should be treated as breaking changes. While effort will be made for backwards compatibility, such as overloads, stability is not guaranteed.
