# ONE WORLD — Region Data Contract

## Purpose

Every playable region must be described as content data rather than hard-coded assumptions in shared gameplay systems. Lagos is the first region pack. The same contract should support other Nigerian states, the FCT and later countries.

## Region identity

Each region should define:

- Stable machine ID (lowercase, underscore-separated).
- Display name and administrative type.
- Parent country and, where applicable, state/FCT.
- Local time zone and language/localization keys.
- Currency and regional economic tuning references.
- Map bounds, streaming cells and travel connections.
- Districts, landmarks, roads, transport routes and service locations.
- Ambient audio, weather profile and environmental references.
- NPC population profiles, occupations and schedule presets.
- Asset manifest with source, licence, attribution and version.

## Content rules

- Use stable IDs; never use display names as database keys.
- Store coordinates in a documented world coordinate system and define conversion at import boundaries.
- Keep state/country-specific prices and rules in tuning data.
- Do not assume every region uses Lagos transport, slang, architecture or climate.
- Region packs must declare dependencies and minimum game/content versions.
- Missing optional content should degrade gracefully; missing required content should fail validation with a useful message.

## Initial region

- ID: `ng_lagos`
- Display name: Lagos
- Country: Nigeria
- Role: first playable region
- Scope: one small representative district for the first vertical slice

## Expansion target

Nigeria has 36 states plus the Federal Capital Territory. Each becomes its own region/state data entry as content is produced and validated. The presence of a data entry does not imply that a fully playable map exists.
