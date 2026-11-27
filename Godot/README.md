# ONE WORLD — Godot 4 foundation

**LIVE · EXPLORE · BELONG**  
*One universe. Countless lives.*

Built by Afolabi Oluwadamilare Simeon — SimeonJr Studios.

## Open the project

1. Install/use Godot 4.x (standard build; no .NET version required for this GDScript prototype).
2. In Godot Project Manager, choose **Import**.
3. Select this folder's `project.godot`.
4. Open the project and press **F6** only if a scene is selected; otherwise press **F5** to run the project.

## Current prototype scope

- Godot 4 project using the Compatibility renderer as a lower-hardware starting point.
- Third-person CharacterBody3D controller: WASD, mouse look, Shift sprint, Space jump, Escape releases mouse.
- Procedurally generated placeholder street, road, ground and building collision.
- Six character needs: Hunger, Energy, Hygiene, Social, Fun and Bladder.
- Simple NGN wallet with safe spend checks.
- HUD showing money and needs.
- Lagos region data starter file.

## Important status

This is an initial Godot project foundation and greybox prototype, not a finished game. It has not yet been run in the user's local Godot installation, and no claim of runtime verification is made. The street/buildings are temporary blockout geometry, not authentic final Lagos assets.

The previous Unreal Engine project and its source are intentionally retained under `../ParadiseHeat` for provenance/reference. They are not required to run this Godot project. Do not delete or overwrite the Unreal source during the transition.

## Next milestones

1. Run and validate this project in the installed Godot version.
2. Add interaction prompts and an interaction interface.
3. Add save/load for player needs and wallet.
4. Add authentic Lagos street assets and a small playable district.
5. Add NPC pedestrians, daily routines and first public transport interaction.
6. Expand simulation systems only after profiling on target hardware.
