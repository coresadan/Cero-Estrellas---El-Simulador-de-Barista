"""Refresh exact customer recordings without reimporting geometry."""
from pathlib import Path
import unreal, json
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
tools=unreal.AssetToolsHelpers.get_asset_tools(); tasks=[]
for row in json.loads((root/'SourceAudio/Dialogue/lines.json').read_text(encoding='utf-8')):
    name=row['asset']
    if unreal.EditorAssetLibrary.does_asset_exist('/Game/Prototype/Audio/Dialogue/'+name): continue
    task=unreal.AssetImportTask(); task.filename=str(root/'SourceAudio/Dialogue'/(name+'.wav'))
    task.destination_path='/Game/Prototype/Audio/Dialogue'; task.destination_name=name
    task.automated=True; task.save=True; tasks.append(task)
tools.import_asset_tasks(tasks)
assert all(t.imported_object_paths for t in tasks)
unreal.log('CE10_ADDITIONAL_SPEECH_READY '+str(len(tasks)))
