from pathlib import Path
import runpy,unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
for name in ['import_characters.py','configure_professional_lods.py','import_details11.py','import_graphics11.py','author_cafe11.py']:
    runpy.run_path(str(root/'Scripts'/name),run_name='__main__')
unreal.log('CE11_CONTENT_READY')
