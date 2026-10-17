"""Audit the current executable, actual runtime results and fresh captures."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct
root=Path(__file__).resolve().parent.parent
binary=root/'Release/Windows/CeroEstrellas/Binaries/Win64/CeroEstrellas.exe'
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert digest(binary)==digest(root/'Binaries/Win64/CeroEstrellas.exe')
assert all(p.stat().st_mtime<=binary.stat().st_mtime for p in (root/'Source').rglob('*') if p.suffix in ['.cpp','.h','.cs'])
scene=json.loads((root/'Evidence/entrega12_saved_scene_audit.json').read_text(encoding='utf-8-sig'))
assert scene['passed'] and scene['fixed_actors']>600 and not scene['interactive_actors_baked']
backend=json.loads((root/'Evidence/entrega12_backend.json').read_text(encoding='utf-8-sig'))
assert backend['voice']['ready'] and backend['ollama']['models']
assert all(model['size_vram']==0 for model in backend['ollama']['models'])
assert (root/'Evidence/Comparativa12/Comparativa.html').is_file()
editor_dir=root/'Evidence/Entrega12EditorFinal'
editor=json.loads((editor_dir/'smoke_result.json').read_text(encoding='utf-8-sig'))
assert editor['passed'] and editor['version']=='0.12.0' and editor['all_19_PBR_and_screen_textures_available']
editor_checks=int(re.search(r'CE_LOOP_RESULT passed=1 checks=(\d+)',(editor_dir/'editor_runtime.log').read_text(encoding='utf-8-sig')).group(1))
scenarios=[]
for folder,filename in [('Entrega12Closure','smoke_result.json'),('Entrega12Lumen','smoke_result.json'),('Entrega12Dialogue','dialogue_result.json')]:
    directory=root/'Evidence'/folder
    data=json.loads((directory/filename).read_text(encoding='utf-8-sig'))
    assert data['passed'] and data['version']=='0.12.0',folder
    assert data['frame_time_basis']=='wall_clock_between_game_ticks',folder
    identity_path=directory/'binary_identity.json'
    identity=json.loads(identity_path.read_text(encoding='utf-8-sig'))
    assert identity['sha256']==digest(binary) and not identity['render_offscreen'],folder
    assert (identity['requested_width'],identity['requested_height'])==(1920,1080),folder
    log=(directory/('runtime.log' if 'Dialogue' in folder else 'packaged_runtime.log')).read_text(encoding='utf-8-sig')
    assert not re.search(r'Log(?:RHI|D3D12RHI|Material): Error:|Fatal error:|missing usage flag',log),folder
    checks=int(re.search(r'CE_(?:LOOP|DIALOGUE)_RESULT passed=1 checks=(\d+)',log).group(1))
    resources=json.loads((directory/'resources_summary.json').read_text(encoding='utf-8-sig'))
    assert resources['samples']>0 and not resources['other_game_observed'],folder
    shots=[]
    for png in directory.glob('*.png'):
        assert png.stat().st_mtime>=identity_path.stat().st_mtime,png
        assert struct.unpack('>II',png.read_bytes()[16:24])==(1920,1080),png
        shots.append(png.name)
    assert shots,folder
    if folder=='Entrega12Closure':assert data['full_shift_exercised']
    if 'Dialogue' in folder:assert data['validated_live_replies']>=8 and data['played_Piper_voices']>=6
    else:
        assert checks>=167 and data['all_19_PBR_and_screen_textures_available'],folder
        assert data['physically_prepared_espresso_remains_valid_at_serving_input'],folder
        assert data['wet_tablet_remains_visible_without_oversized_generic_prop_stain'],folder
        for name in ['24_salon_ampliado.png','25_clientes_y_dispositivos.png','26_hud_seis_comandas.png','27_movil_en_mano.png','28_tablet_mojada.png']:
            assert name in shots,(folder,name)
    scenarios.append({'folder':folder,'checks':checks,'passed':True,'full_shift_exercised':data.get('full_shift_exercised',False),
                      'frame_time_basis':data['frame_time_basis'],'frame_p50_ms':data['frame_p50_ms'],'frame_p95_ms':data['frame_p95_ms'],
                      'frame_max_ms':data['frame_max_ms'],'resources':resources,'screenshots':shots,'resolution':[1920,1080],
                      'graphics_audit':re.findall(r'CE12_(?:GRAPHICS|RENDER_AUDIT)[^\r\n]*',log),'validated_live_replies':data.get('validated_live_replies')})
manifest={'version':'0.12.0','engine':'5.8.3','created_utc':datetime.now(timezone.utc).isoformat(),'launcher':'Jugar.bat',
          'executable':str(binary.relative_to(root)),'sha256':digest(binary),'matches_compiled_binary':True,
          'source_older_than_binary':True,'human_playthrough':False,'human_microphone_conversation_verified':False,
          'table_count':6,'fixed_scene_audit':'Evidence/entrega12_saved_scene_audit.json','backend_audit':'Evidence/entrega12_backend.json',
          'editor_validation':{'folder':'Entrega12EditorFinal','passed':True,'checks':editor_checks},'scenarios':scenarios,
          'gallery':'Evidence/Comparativa12/Comparativa.html'}
(root/'Evidence/entrega12_delivery.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps({'version':manifest['version'],'sha256':manifest['sha256'],'scenarios':[(s['folder'],s['checks'],len(s['screenshots'])) for s in scenarios]},indent=2))
