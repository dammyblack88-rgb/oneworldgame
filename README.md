# ONE WORLD

**LIVE · EXPLORE · BELONG**  
*One universe. Countless lives.*

ONE WORLD is a living-world game by **SimeonJr Studios**. Development begins in Lagos, Nigeria, with a data-driven world architecture intended to expand across all 36 Nigerian states and the Federal Capital Territory, then internationally.

## Current engine direction

**Godot 4 is the selected engine for active development.** The new project is in `Godot/` and uses GDScript with the Compatibility renderer as a lower-hardware starting point.

- Engine: Godot 4.x
- Primary implementation: GDScript
- Active development branch: `one-world/godot-foundation`
- Project entry: `Godot/project.godot`

The original Unreal Engine project is retained under `ParadiseHeat/` for provenance and reference. It is not the active project and is not required to run the Godot version. No Unreal source has been deleted.

## Product pillars

1. **Living society:** residents with routines, needs, work, relationships and goals.
2. **Nigerian authenticity:** Lagos-first neighbourhoods, transport, markets, food, language and culture.
3. **Emergent street life:** traffic, public transport, vendors, accidents, arguments and breakdowns.
4. **Player agency:** jobs, hustles, education, businesses, housing, construction, vehicles and social life.
5. **Connected expansion:** reusable regional data and systems for new states and countries.

## First playable milestone: Lagos vertical slice

Build a small, coherent and testable Lagos district before claiming city-wide coverage. The target loop is:

- Create and control a player character.
- Explore a Lagos-inspired neighbourhood.
- Track six needs: hunger, energy, hygiene, social, fun and bladder.
- Earn and spend money through simple activities.
- Interact with local places and residents.
- Introduce pedestrian routines and a first public-transport interaction.
- Save and restore player progress.

## Development principles

- Preserve upstream code and attribution; use focused commits and feature branches.
- Never present placeholders or untested systems as finished gameplay.
- Track provenance and licences for every external asset.
- Establish performance budgets before scaling NPC counts.
- Keep regional content data-driven; Lagos is the first region, not the entire world.

## Repository map

- `Godot/` — active Godot project and starter prototype.
- `ParadiseHeat/` — retained Unreal project/reference; inactive for current development.
- `Docs/ATTRIBUTION.md` — inherited third-party notices.
- `Docs/ONE_WORLD_ROADMAP.md` — product milestones.
- `Docs/DEVELOPMENT_STATUS.md` — implementation and verification status.
- `Docs/REGION_DATA_CONTRACT.md` — regional content conventions.

**Built by Afolabi Oluwadamilare Simeon — SimeonJr Studios.**
