#!/usr/bin/env python3
"""Read-only check of the downloaded packs and locally rebuilt city dependencies."""
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[1]
CONTENT = ROOT / 'ParadiseHeat/Content'
REQUIRED = [
    'CitySampleBuildings',
    'VehicleVarietyVol2/Skeletons/SK_Sedan_01a.uasset',
    'Scanned3DPeoplePack/RP_Character/rp_nathan_rigged_003_ue4/rp_nathan_rigged_003_ue4.uasset',
    'Mz3/Mesh/SK_Mz3.uasset',
    'Characters/ScannedPlayer/BP_NathanPlayer.uasset',
    'Vehicles/Player/Entry/SK_Sedan_Entry.uasset',
    'Vehicles/Player/Entry/SK_Nathan_CockpitArms.uasset',
    'Environment/StreetLights/Street_Light_A_08.uasset',
    'Environment/StreetTrees',
    'Environment/CityDistrict10x/Materials',
    'Environment/CityDistrict10x/DayMaterials',
]
missing = [p for p in REQUIRED if not (CONTENT / p).exists()]
for path in REQUIRED:
    print(('MISSING ' if path in missing else 'OK      ') + '/Game/' + path.removesuffix('.uasset'))
if missing:
    print('\nFollow the asset table in Readme.md, then run Scripts/setup.py --profile city.')
    sys.exit(1)
print('\nCity dependency files are present. This does not certify gameplay or license ownership.')
