"""Refresh refined shoulders and regenerate the distance LODs."""
from pathlib import Path
import runpy, unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
for script in ['import_characters.py','configure_professional_lods.py']:
    runpy.run_path(str(root/'Scripts'/script),run_name='__main__')
