[![Paradise Heat — Becodemy tutorial](thumbnail.png)](https://www.youtube.com/@BecodemyOfficial)

# Paradise Heat
An editable Unreal Engine city-game source project. This repository contains the C++ gameplay systems, project configuration, original map layouts, template content and local asset-preparation tools. Third-party packs and restricted derivatives are downloaded or generated on your own machine rather than redistributed here.

**Choose a starting point:** the template profile runs without the large city packs. The city profile requires the downloads below and local asset preparation. This public edition omits the unresolved motorcycle model, background music and engine recordings; it is not an identical copy of the private production build. The asset table below lists the required downloads and license notes.

## Requirements

- Unreal Engine **5.8.4**, installed through [Epic Games Launcher](https://www.unrealengine.com/download).
- macOS on Apple Silicon with Xcode and Metal tools. The original game was built with Xcode 27.2. A native Windows host needs Visual Studio C++ tools and the Windows SDK; Windows gameplay has not been verified.
- Python 3 to run the setup launcher. Unreal supplies the Python runtime used by the editor scripts.
- [Git LFS](https://git-lfs.com/) to retrieve the included Unreal template/map assets when cloning.
- The engine plugins declared in the project, including Python Editor Scripting, Editor Scripting Utilities, Geometry Scripting, Chaos Vehicles and StateTree. Engine/plugin binaries are not bundled.

## 1. Get the source

Clone or download this repository. When cloning, install Git LFS first, then run these commands from the repository folder:

```bash
git lfs install --local
git lfs pull
```

The project file is `ParadiseHeat/ParadiseHeat.uproject`. Its targets are `ParadiseHeat` and `ParadiseHeatEditor`; the C++ module, native classes and source folder now use `ParadiseHeat`. Compatibility redirects allow older saved maps and Blueprints to load.

## 2. Run the template profile first

Close Unreal Editor before running setup. On macOS:

```bash
python3 Scripts/setup.py --profile template --ue-root /Users/Shared/UE_5.8
UE_ROOT=/Users/Shared/UE_5.8 bash Scripts/run-mac-editor.sh
```

On Windows PowerShell:

```powershell
py Scripts/setup.py --profile template --ue-root 'C:\Program Files\Epic Games\UE_5.8'
```

Then open `ParadiseHeat/ParadiseHeat.uproject` and press **Play**. Setup compiles the editor module, assigns the supplied mannequin player, and selects `/Game/ThirdPerson/Lvl_ThirdPerson`. First-launch shader compilation can take time. Use `--skip-build` only when the editor target has already been compiled for the current source.

**Controls:** WASD moves, Shift runs, Space jumps, the mouse/trackpad controls the camera, and Escape opens settings/releases the cursor. Click Resume to return to gameplay. E interacts with available vehicles in the city profile.

## 3. Acquire the city assets yourself

Acquire each pack under your own eligible license. Free acquisition does not allow uploading its editable files into this repository. Prices and availability may change.

| Asset / preview and download | Creator | Install under `ParadiseHeat/Content/` | Download/license notes |
|---|---|---|---|
| [City Sample Buildings](https://www.fab.com/listings/008fe959-5511-428e-93bd-f99b1179f6d5) | Epic Games | `CitySampleBuildings/` | Free tiers; listing also says UE-only |
| [Vehicle Variety Pack Volume 2](https://www.fab.com/listings/591e3b3f-9d49-4cd2-8e28-d471c1a10cab) | Switchboard Studios | `VehicleVarietyVol2/` | Free Personal and Professional offers |
| [Scanned 3D People Pack](https://www.fab.com/listings/2e872a96-686e-4955-8256-82fa19904f92) | Renderpeople | `Scanned3DPeoplePack/` | Free legacy UE Marketplace offer |
| [Mz3](https://www.fab.com/listings/3cceba5f-7f8a-4ee8-b566-7208d9f72411) | Ruslan Akpayev | `Mz3/` | Free current Standard offers; older engine package |
| [Free Street Lights Pack 01](https://www.fab.com/listings/8b69b10c-3856-4e41-a58c-d72aa5de88d3) | Sat Productions | Imported by setup into `Environment/StreetLights/` | Personal tier free; Professional tier paid |
| [Tree Small 02](https://polyhaven.com/a/tree_small_02) | Rico Cilliers / Poly Haven | Imported by setup into `Environment/StreetTrees/` | CC0, free |

For Unreal-format packs, use Fab/Launcher **Add to Project**. If a pack is supplied as a project, create/open that project and use **Asset Actions → Migrate** into this repository's `ParadiseHeat/Content` directory. Keep the original top-level folder names. Do not introduce `Content/Content`, rename `VehicleVarietyVol2`, or put these packs in `Plugins`.

Vehicle Variety is supplied as a project; migrate its `VehicleVarietyVol2` folder. Mz3 and Scanned People may need a compatible temporary project followed by migration/conversion to UE 5.8. Do not resave legacy PhysX vehicle Blueprints just to suppress compatibility warnings; the current game uses their mesh assets and its own vehicle systems.

For the source-format downloads, extract the files into the following **local, ignored** directory:

```text
SourceAssets/
  Street_Lights_Pack_01_fbx/
    Street_Light_A_08.fbx
    ... other supplied FBX files
  Street_Lights_Pack_01_Textures/
    Street_Light_A_BaseMap.png
    ... other supplied textures
  PolyHaven/
    tree_small_02/
      tree_small_02_1k.gltf
      ... accompanying binary geometry files
      textures/
        ... the 1K glTF textures
        leaves_alpha_1k.png
```

Download the tree's **1K glTF** and all referenced files. Include its leaf-alpha PNG, named `leaves_alpha_1k.png` locally. The original download file information is available at [Poly Haven's file manifest](https://api.polyhaven.com/files/tree_small_02). The preparation tool expects the exact names above.

## 4. Prepare and run the city

Close Unreal Editor, finish the downloads, then run:

```bash
python3 Scripts/setup.py --profile city --ue-root /Users/Shared/UE_5.8
python3 Scripts/check_dependencies.py
UE_ROOT=/Users/Shared/UE_5.8 bash Scripts/run-mac-editor.sh
```

On Windows use `py` and your Windows engine path, then open the `.uproject` manually. Once setup succeeds, open `/Game/Maps/CityDistrict10x_Day` and press Play.

City setup imports the locally acquired lamps/tree, creates the Nathan player and retargets template locomotion, rebuilds entry bodies/doors/steering wheels/mirrors and cockpit arms from your own installed packs, and recreates local window-material overrides. Generated content stays ignored by Git. The original template and district layouts remain in the repository; source geometry exports are generated under `artifacts/`, not shipped.

Preparation can take time. Its mesh selections were authored against the pack versions used by this project; if a publisher changes topology, an assertion can stop preparation rather than silently generating the wrong doors or mirrors. Read the first error, check the original folder names/version and keep the generated content private. `check_dependencies.py` checks files, not ownership, completeness of every reference, or gameplay quality.

The public setup pipeline passed local reconstruction using the developer's acquired pack versions. A fresh-machine city walkthrough is still required before the tutorial can claim a fully verified setup. Do not interpret passing Python syntax checks or the file checker as proof of that walkthrough.

## Optional material omitted from this edition

- **Motorcycle model:** [original joshuaseaver / Triumph Sketchfab page](https://sketchfab.com/3d-models/triumph-bonneville-bobber-1200cc-motorcycle-e82585ac4eab493185556f51c810894c). Current free access and redistribution rights are unresolved. The motorcycle C++ implementation is included, but this edition does not require or automatically download the model. Ambient traffic uses cars when the bike artwork is absent.
- **Background music and engine loops:** not included until their provenance is established. Vehicle/control systems can run without these recordings. Supply audio you have the right to use locally if desired.
- **Selected pedestrian scream:** the included clip comes from [JohnsonBrandEditing's CC0 recording](https://freesound.org/people/JohnsonBrandEditing/sounds/173944/), distributed in the [Aargh! compilation](https://opengameart.org/content/aargh-male-screams). The compilation also has other, non-CC0 clips; only the selected CC0 conversion is included.

## How the code fits together

- `Source/ParadiseHeat/ParadiseHeat.cpp`: loading artwork/music integration and primary module.
- `ParadiseHeatCharacter` and `ParadiseHeatPlayerController`: movement, camera, input, pause and interaction.
- `VehicleInteraction.inl`, `CarEntryPose.inl`, `CarMirrors.inl`: car entry, driver pose, cockpit and mirrors.
- `Vehicles/Motorcycle/`: assisted two-wheel physics, mounting, falls and recovery; artwork is an optional external dependency.
- `Environment/`: district geometry, route traffic/pedestrians and impact reactions.
- `Settings/`: graphics/input/audio settings, pause UI and native music-file selection.
- `Scripts/Editor/`: source-only preparation operations. These run in Unreal, after the packs have been acquired locally.

## Build a Mac application

After city setup succeeds:

```bash
UE_ROOT=/Users/Shared/UE_5.8 bash Scripts/build-mac.sh
```

This cooks the city profile and outputs `artifacts/mac/Paradise Heat.app`, with the PH icon and local signature. Use a new `OUTPUT_DIR` for another release. This is not Apple notarization. The Windows helper builds a Development package and remains unverified on Windows.

## Sharing this repository

Third-party downloads, regenerated derivatives and unresolved audio are excluded by `.gitignore`. Never force-add those paths. Ordinary Finder compression of a working project after installing assets would include the ignored assets again. Preview the source export with `python3 Scripts/export_source.py --list`, then use `python3 Scripts/export_source.py --output /path/outside/repository/ParadiseHeat-source.zip` to export files under the current Git ignore policy. This helper does not replace a license audit for new assets. Git LFS stores the included binary files; it does not grant redistribution rights.

No engine installation, marketplace archive or plugin binary is included. Keep Epic and other third-party notices. The PH artwork is project branding; no blanket license to third-party assets is implied. Check the applicable license before adding or redistributing any new assets.
