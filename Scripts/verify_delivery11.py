"""Audit the final package and collect actual scenario results, never old passes."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,struct,re
root=Path(__file__).resolve().parent.parent
binary=root/'Release/Windows/CeroEstrellas/Binaries/Win64/CeroEstrellas.exe'
compiled=root/'Binaries/Win64/CeroEstrellas.exe'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
assert digest(binary)==digest(compiled)
assert all(p.stat().st_mtime<=binary.stat().st_mtime for p in (root/'Source').rglob('*') if p.suffix in ['.cpp','.h','.cs'])
for name,key in [('entrega11_deformation.json','characters'),('entrega11_lods.json','meshes')]:
    audit=json.loads((root/'Evidence'/name).read_text(encoding='utf-8-sig'))
    assert audit['passed'] and len(audit[key])==9,name
scene=json.loads((root/'Evidence/entrega11_saved_scene_audit.json').read_text(encoding='utf-8-sig'))
assert scene['passed'] and scene['fixed_actors']==483 and not scene['interactive_actors_baked']
backend=json.loads((root/'Evidence/entrega11_backend.json').read_text(encoding='utf-8-sig'))
assert backend['voice']['ready'] and backend['ollama']['models']
assert all(model['size_vram']==0 for model in backend['ollama']['models'])
scenarios=[]
for folder,report in [('Entrega11Closure','smoke_result.json'),('Entrega11Dialogue','dialogue_result.json'),('Entrega11Lumen','smoke_result.json')]:
    path=root/'Evidence'/folder/report
    data=json.loads(path.read_text(encoding='utf-8-sig'));assert data['passed'] and data['version']=='0.11.0',folder
    assert data['frame_time_basis']=='wall_clock_between_game_ticks',folder
    identity=json.loads((path.parent/'binary_identity.json').read_text(encoding='utf-8-sig'));assert identity['sha256']==digest(binary),folder
    assert not identity['render_offscreen'] and (identity['requested_width'],identity['requested_height'])==(1920,1080),folder
    log=path.parent/('runtime.log' if report=='dialogue_result.json' else 'packaged_runtime.log')
    log_text=log.read_text(encoding='utf-8-sig')
    assert not re.search(r'Log(?:RHI|D3D12RHI|Material): Error:|Fatal error:',log_text),folder
    checks=int(re.search(r'CE_(?:LOOP|DIALOGUE)_RESULT passed=1 checks=(\d+)',log_text).group(1))
    resources=json.loads((path.parent/'resources_summary.json').read_text(encoding='utf-8-sig'))
    assert resources['samples']>0 and not resources['other_game_observed'],folder
    dimensions=[]
    for png in path.parent.glob('*.png'):
        assert png.stat().st_mtime>=(path.parent/'binary_identity.json').stat().st_mtime,png
        w,h=struct.unpack('>II',png.read_bytes()[16:24]);assert (w,h)==(1920,1080),(png,w,h)
        dimensions.append({'file':png.name,'width':w,'height':h})
    assert dimensions,folder
    scenarios.append({'report':str(path.relative_to(root)),'passed':True,'checks':checks,
                      'frame_p95_ms':data.get('frame_p95_ms'),'frame_max_ms':data.get('frame_max_ms'),'screenshots':dimensions,
                      'frame_p50_ms':data.get('frame_p50_ms'),'frame_samples':data.get('frame_samples'),
                      'frame_time_basis':data['frame_time_basis'],
                      'resources':resources,'graphics_audit':re.findall(r'CE11_(?:GRAPHICS|RENDER_AUDIT)[^\r\n]*',log_text),
                      'full_shift_exercised':data.get('full_shift_exercised',False),'validated_live_replies':data.get('validated_live_replies')})
manifest={'version':'0.11.0','engine':'5.8.3','created_utc':datetime.now(timezone.utc).isoformat(),'launcher':'Jugar.bat',
          'executable':str(binary.relative_to(root)),'sha256':digest(binary),'matches_compiled_binary':True,'source_older_than_binary':True,
          'human_playthrough':False,'human_microphone_conversation_verified':False,'scenarios':scenarios,
          'deformation_report':'Evidence/entrega11_deformation.json','lod_report':'Evidence/entrega11_lods.json',
          'saved_scene_audit':'Evidence/entrega11_saved_scene_audit.json','backend_report':'Evidence/entrega11_backend.json',
          'comparison':'Evidence/Comparativa11/Comparativa.html'}
(root/'Evidence/entrega11_delivery.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in manifest.items() if k!='scenarios'},indent=2))
