# ONE WORLD — Development Status

Last updated: 2026-11-27

## Current direction

- Active engine: Godot 4.x.
- Active development branch: `one-world/godot-foundation`.
- Godot project entry point: `Godot/project.godot`.
- Renderer: Compatibility (chosen as a conservative starting point for lower-end hardware).
- The previous Unreal Engine project remains preserved under `ParadiseHeat/`; it is inactive and not required to run the Godot project.

## Implemented in the Godot foundation branch

- Godot project configuration and main scene.
- Third-person CharacterBody3D controller with mouse look, WASD movement, sprint and jump.
- Procedural greybox environment with ground, road, simple buildings and collision.
- Six character needs with tunable per-minute decay and restoration.
- NGN wallet with balance checks and guarded spending.
- Basic HUD showing wallet and all six needs.
- Initial Lagos region JSON data.

## Verification status

The project files have been committed to GitHub, but have **not yet been opened or run in the user's local Godot installation**. GDScript parsing, runtime behavior, input bindings, collision and performance remain to be checked. The current environment is a greybox prototype, not final Lagos art or a completed playable vertical slice.

## Preserved Unreal work

The original `ParadiseHeat/` project and its attribution notices remain in the repository as a reference/archive. Do not delete or overwrite this folder during the engine transition.

## Immediate next steps

1. Import `Godot/project.godot` into the installed Godot 4 editor.
2. Run the project and fix any parser/runtime issues reported by the editor.
3. Record the exact Godot version and test the prototype on the target PC.
4. Add interaction prompts and a reusable interaction interface.
5. Add save/load for player needs and wallet.
6. Replace greybox buildings with properly sourced, Lagos-authentic assets.
7. Add NPC pedestrians, routines and a first public transport interaction.

Do not describe the prototype as tested, polished, fully playable or complete until those checks pass.
