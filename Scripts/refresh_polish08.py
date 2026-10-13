"""Refresh saved anatomical profiles and original PBR maps in isolated namespaces."""
from pathlib import Path
import runpy
import unreal
ce_refresh_root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
for ce_refresh_script in ['import_characters.py','refresh_material_textures.py']:
    runpy.run_path(str(ce_refresh_root/'Scripts'/ce_refresh_script),run_name='__main__')
