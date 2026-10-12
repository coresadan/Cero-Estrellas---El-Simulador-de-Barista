"""UE Python entrypoint: refresh only the 0.6 assets, retaining the cafe map."""
import os, unreal
root=unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
for name in ['import_characters.py','import_polish.py']:
    path=os.path.join(root,'Scripts',name)
    exec(compile(open(path,encoding='utf-8').read(),path,'exec'),{'__file__':path})
