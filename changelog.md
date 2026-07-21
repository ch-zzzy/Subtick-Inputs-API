# Changelog

## v0.6.0

- reworked processInputs again, should hopefully fix the mod doing effectively nothing below 240fps (which kinda defeated the purpose)
- added a setting to force disable CBS+COS from within the API, so dependent mods don't need to handle it
- added isApiEnabled(), deprecated isApiDisabled()
- changed gravity precision to better match vanilla (i'll thoroughly check this someday to find other issues)
- lots of refactoring

## v0.5.0

- restructured the input processing model
- added debug logging
- bump geode version

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
