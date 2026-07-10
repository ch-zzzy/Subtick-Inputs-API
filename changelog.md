# Changelog

## v0.4.2

- (potentially) fixed some clicks being weird on the ground
- added debug logging

## v0.4.1

- fixed listeners' respective events

## v0.4.0

- added listeners for the configs similar to geode's listenForSettingChanges
- added updateRotation for tick splits, might fix some wave stuff, might not

## v0.3.1

- added non-windows support, no velocity unrounding for those platforms for now

## v0.3.0

- replaced the y-displacement midhook with a full reimplementation of PlayerObject::update, potential for non-windows support 😮 (backup incase it goes wrong)
- fixed some wave stuff, added subtick collisions for them
- removed the SafetyHook dependency
- general refactoring

## v0.2.0

- removed processInputs PlayerObject* param
- fixed p2 input doing nothing in non-dual mode

## v0.1.3

- rounded getGravPerTick to 3dp if velocity unrounding is not enabled

## v0.1.2

- updated mod.json and README, and slightly simplified the midhook

## v0.1.1

- added cheat tag

## v0.1.0

- public beta (i've barely tested this at all lol)
