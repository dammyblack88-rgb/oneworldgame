#!/usr/bin/env python3
"""Compile the project and prepare locally acquired assets. Never downloads licensed packs."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'ParadiseHeat' / 'ParadiseHeat.uproject'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=('template', 'city'), default='template')
    parser.add_argument('--ue-root', type=Path, default=Path(os.environ.get('UE_ROOT', '/Users/Shared/UE_5.8')))
    parser.add_argument('--skip-build', action='store_true', help='Use an already compiled editor module')
    args = parser.parse_args()
    engine = args.ue_root.resolve()
    if sys.platform == 'darwin':
        build = engine / 'Engine/Build/BatchFiles/Mac/Build.sh'
        editor = engine / 'Engine/Binaries/Mac/UnrealEditor-Cmd'
        command = ['bash', str(build), 'ParadiseHeatEditor', 'Mac', 'Development', f'-Project={PROJECT}', '-WaitMutex', '-architecture=arm64', '-MaxParallelActions=2', '-NoUBA']
    elif sys.platform == 'win32':
        build = engine / 'Engine/Build/BatchFiles/Build.bat'
        editor = engine / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
        command = [str(build), 'ParadiseHeatEditor', 'Win64', 'Development', f'-Project={PROJECT}', '-WaitMutex']
    else:
        parser.error('This setup supports macOS and Windows hosts.')
    if not editor.is_file() or not build.is_file():
        parser.error('Unreal toolchain not found; pass --ue-root pointing to the Unreal Engine installation.')
    env = dict(os.environ, PARADISE_HEAT_ROOT=str(ROOT), PARADISE_HEAT_PROFILE=args.profile)
    if args.profile == 'city':
        required = [
            'Content/CitySampleBuildings',
            'Content/VehicleVarietyVol2/Skeletons/SK_Sedan_01a.uasset',
            'Content/VehicleVarietyVol2/Skeletons/SK_SUV_01a.uasset',
            'Content/VehicleVarietyVol2/Skeletons/SK_BoxTruck_01a.uasset',
            'Content/Scanned3DPeoplePack/RP_Character/rp_nathan_rigged_003_ue4/rp_nathan_rigged_003_ue4.uasset',
            'Content/Mz3/Mesh/SK_Mz3.uasset',
        ]
        raw = ['Street_Lights_Pack_01_fbx/Street_Light_A_08.fbx', 'Street_Lights_Pack_01_Textures/Street_Light_A_BaseMap.png', 'PolyHaven/tree_small_02/tree_small_02_1k.gltf']
        missing = [str(PROJECT.parent / p) for p in required if not (PROJECT.parent / p).exists()]
        missing += [str(ROOT / 'SourceAssets' / p) for p in raw if not (ROOT / 'SourceAssets' / p).is_file()]
        if missing:
            parser.error('Acquire the dependencies in Readme.md first. Missing:\n' + '\n'.join(missing))
    (ROOT / 'artifacts/setup').mkdir(parents=True, exist_ok=True)
    (ROOT / 'artifacts/car-geometry').mkdir(parents=True, exist_ok=True)
    if not args.skip_build:
        subprocess.run(command, cwd=ROOT, env=env, check=True)
    marker = ROOT / 'artifacts/setup' / (args.profile + '.ready')
    marker.unlink(missing_ok=True)
    subprocess.run([str(editor), str(PROJECT), '/Game/ThirdPerson/Lvl_ThirdPerson', f'-ExecutePythonScript={ROOT / "Scripts/Editor/prepare_project.py"}', '-unattended', '-nullrhi', '-nosplash', '-stdout', '-FullStdOutLogOutput'], cwd=ROOT, env=env, check=True)
    marker = ROOT / 'artifacts/setup' / (args.profile + '.ready')
    if not marker.is_file():
        raise RuntimeError('Unreal did not complete preparation; inspect its log above.')
    print(f'Prepared {args.profile} profile. Open {PROJECT} and press Play.')

if __name__ == '__main__':
    main()
