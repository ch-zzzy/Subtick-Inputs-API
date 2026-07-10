# Subtick Inputs API

An API that lets consumer mods register inputs at sub-tick resolution
while keeping the player's physics faithful to vanilla.

The API does nothing on its own. A consumer mod (like [Superb Input Precision](https://github.com/ch-zzzy/Superb-Input-Precision)) uses it to deliver finer input precision.
Vanilla does all the physics, but the API applies Y-displacement correction on top of it so that an input processed partway through a tick lands where vanilla would have put it at that moment.

## How it works

Vanilla runs its normal physics ticks unmodified. For each tick, `processInputs`
measures how the player's velocity and acceleration would have differed had an input
been processed at its true sub-tick timing, and accumulates that difference into a
Y-displacement correction that's applied on top of vanilla's Y-displacement. This avoids the
intermediate-velocity error that splitting a tick introduces for accelerating
gamemodes, while still placing inputs at sub-tick precision.

Wave is handled differently. Since wave has no gravity, velocity is constant within
each tick, so a wave click *can* be handled by splitting the tick,
running each segment through vanilla's own update. Rotation and collision are
also re-run per-segment like CBF and CBS.

## API surface

Everything lives in the `subtickinputs` namespace.
A `prelude` sub-namespace is provided to pull in the common symbols at once.

### `Config` singleton

`Config::get()` returns the shared instance.

- `getInputHz()` / `setInputHz(float)`: input check rate (overriden by Instantaneous Input)
- `isInstantInputsEnabled()` / `setInstantInputsEnabled()`: whether input processing is
snapped to the input check rate grid, or happens immediately
- `isVelocityUnroundingEnabled()` / `setVelocityUnroundingEnabled(bool)`: whether y-velocity 3dp rounding is bypassed
- `isApiDisabled()`: master-switch state, already checked in `useVanilla()`

### `Config` listeners

Register a callback to be notified when a config changes, including changes made by
other mods. Useful for keeping local stuff in sync with the shared variables.

- `listenForInputHzChanges`: callback receives the new `float` input hz
- `listenForInstantInputsChanges`: callback receives the new `bool` state
- `listenForVelocityUnroundingChanges`: callback receives the new `bool` state

Usage is very similar to geode's listener, for example:

```cpp
listenForInputHzChanges([](float newHz) {
    // react to the new value
});
```

If you're syncing `Config` to a Geode setting (e.g. so the settings menu checkbox stays
correct even when another mod changes the value), call Geode's
`setSettingValue` from inside the listener:

```cpp
listenForVelocityUnroundingChanges([](bool enabled) {
    geode::Mod::get()->setSettingValue<bool>("<your-velocity-unrounding-key-here>", enabled);
});
```

### `useVanilla()`

Returns `true` when the API should let vanilla run without modification: API disabled,
null PlayLayer, first frame after pause/death/init, player died, platformer mode, or
RobTop's replay mode. Dependent mods should check this before calling any API function.

> `physics::useVanillaPhysics()` is the old name for this and is deprecated, it will be
> removed in v1.0.0.

### `inputs` namespace

- `processInputs(float dt)`: processes both players' queued inputs for the current tick (dispatching each via `handleButton` and `updateJump(0)`) and accumulates each player's sub-tick Y-displacement adjustment for `SIPlayerObject::update` to apply. `dt` is the tick duration passed to `processQueuedButtons`.

> `processInputs(PlayerObject*, float dt)` is the old signature and is deprecated; it will be removed in v1.0.0.

## Using it as a dependency

Add it to your `mod.json`:

```json
"dependencies": {
    "chizz.subtick-inputs-api": {
        "version": ">=v0.4.2",
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
