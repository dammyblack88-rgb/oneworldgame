# ONE WORLD — Development Status

Last updated: 2026-11-27

## Active direction

- Active development branch: `one-world/godot-transition`.
- Active engine direction: Godot 4, using the Compatibility renderer as a conservative starting point for lower-end hardware.
- Godot project: `Godot/ONE_WORLD/project.godot`.
- The inherited Unreal project remains preserved under `ParadiseHeat/`; earlier Unreal foundation work remains on `one-world/foundation`.
- No Unreal project files have been deleted or rewritten by this transition.

## Implemented in the Godot transition branch

- Initial Godot project configuration and main scene.
- Basic third-person CharacterBody3D movement, mouse look, jump and camera rig.
- Player state component with six needs (hunger, energy, hygiene, social, fun and bladder), configurable per-minute decay, clamped restore/set methods and lowest-need query.
- Integer wallet with guarded spending, affordability checks and add-funds method.
- Setup and scope notes in `Godot/ONE_WORLD/README.md`.

## Important limitations

- The Godot project has not yet been opened, imported, run or tested in the Godot editor in this environment.
- No compiled or packaged build has been produced.
- The scene is a technical movement test, not a Lagos environment.
- No HUD, save/load, interaction framework, NPCs, transport, traffic, multiplayer or production assets are implemented yet.
- The Godot scripts and scene need editor validation before being treated as working.

## Next steps

1. User installs Godot 4 Standard (no .NET build required for the current GDScript project).
2. Open `Godot/ONE_WORLD/project.godot` in Godot and run the main scene.
3. Fix any import/parser/runtime issues found in the editor.
4. Add a small Lagos district blockout and an interaction framework.
5. Add a readable needs/wallet HUD and save/load.
6. Introduce a first activity loop and pedestrian prototype.
7. Establish repeatable smoke tests and a performance baseline.

Do not describe ONE WORLD as fully playable or production-ready until these checks pass.
