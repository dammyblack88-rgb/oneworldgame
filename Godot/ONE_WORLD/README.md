# ONE WORLD — Godot project

This is the new Godot 4 implementation workspace. It is intentionally isolated under `Godot/ONE_WORLD`; the inherited Unreal project remains in `ParadiseHeat/` and is preserved on this branch.

## Open and run

1. Install a current stable Godot 4.x Standard build from https://godotengine.org/download/.
2. Extract the repository and open the `Godot/ONE_WORLD/project.godot` file in Godot.
3. Press F6 only after opening the main scene, or press F6? Use F6 for the current scene; press F6 to run current scene and F5 to run the project main scene.
4. Controls: WASD / arrow keys to move, Space to jump, mouse to look, Esc to release the mouse; click to capture it again.

## Current scope

- A minimal 3D test scene with a controllable capsule character and third-person camera.
- Six needs with configurable per-minute decay and restore/set methods.
- Integer wallet with safe affordability and spending methods.

This is a starter implementation, not yet a Lagos environment or a complete playable game. The first scene is a technical movement test. No compile/import/runtime test has been performed in this repository-writing session; verify it in Godot before treating it as working.

## Next milestone

Add a minimal Lagos district blockout, an interaction ray, a HUD for needs and money, and a save/load path. Keep all content data-driven and avoid placeholder claims.
