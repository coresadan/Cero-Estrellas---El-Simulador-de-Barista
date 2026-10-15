"""Loopback-only bounded Whisper/Piper service. All inference uses CPU.

POST /transcribe audio/wav -> {text}; POST /speak {text} -> PCM16 WAV.
No audio is persisted. Microphone opens only after the in-game Record button.
"""
import argparse, io, json, threading, wave
from collections import OrderedDict
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parent.parent
MODELS = ROOT / 'Tools/LocalAI/Models'
VOICE = MODELS / 'es_ES-davefx-medium.onnx'
WHISPER = MODELS / 'WhisperTiny'
MAX_BYTES = 4 * 1024 * 1024

def download():
    from huggingface_hub import snapshot_download
    MODELS.mkdir(parents=True, exist_ok=True)
    snapshot_download('Systran/faster-whisper-tiny', local_dir=str(WHISPER))
    for suffix in ['', '.json']:
        target = Path(str(VOICE) + suffix)
        if not target.exists():
            urllib.request.urlretrieve('https://huggingface.co/rhasspy/piper-voices/resolve/main/es/es_ES/davefx/medium/es_ES-davefx-medium.onnx' + suffix, target)
    # Preserve voice dataset/model licence alongside the original model files.
    card = MODELS / 'PIPER_MODEL_CARD.txt'
    if not card.exists():
        urllib.request.urlretrieve('https://huggingface.co/rhasspy/piper-voices/resolve/main/es/es_ES/davefx/medium/MODEL_CARD', card)

class Handler(BaseHTTPRequestHandler):
    def send(self, code, data, content='application/json', headers=None):
        if isinstance(data, dict): data = json.dumps(data, ensure_ascii=False).encode('utf-8')
        self.send_response(code); self.send_header('Content-Type', content)
        for key,value in (headers or {}).items(): self.send_header(key,value)
        self.send_header('Content-Length', str(len(data))); self.end_headers(); self.wfile.write(data)

    def do_GET(self):
        if self.path == '/health': self.send(200, {'ready': True, 'stt': 'whisper-tiny-int8-cpu', 'tts': 'piper-cpu', 'tts_threads': 1})
        else: self.send(404, {'error': 'Ruta desconocida'})

    def do_POST(self):
        size = int(self.headers.get('Content-Length', '0'))
        if size <= 0 or size > MAX_BYTES: return self.send(413, {'error': 'Entrada demasiado grande'})
        data = self.rfile.read(size)
        try:
            if self.path == '/transcribe':
                with wave.open(io.BytesIO(data)) as wav:
                    if wav.getnframes() / wav.getframerate() > 16: raise ValueError('Audio demasiado largo')
                with self.server.stt_lock:
                    segments, info = self.server.whisper.transcribe(io.BytesIO(data), language='es', beam_size=1, vad_filter=True)
                    text = ' '.join(s.text.strip() for s in segments).strip()[:500]
                self.send(200, {'text': text})
            elif self.path == '/speak':
                text = json.loads(data)['text']
                if not isinstance(text, str) or not 1 <= len(text) <= 350: raise ValueError('Texto invalido')
                with self.server.tts_lock:
                    hit=text in self.server.tts_cache
                    if hit: pcm=self.server.tts_cache.pop(text)
                    else:
                        buf = io.BytesIO()
                        with wave.open(buf, 'wb') as wav: self.server.piper.synthesize_wav(text, wav)
                        pcm=buf.getvalue()
                    self.server.tts_cache[text]=pcm
                    while len(self.server.tts_cache)>64 or sum(map(len,self.server.tts_cache.values()))>16*1024*1024:
                        self.server.tts_cache.popitem(last=False)
                self.send(200, pcm, 'audio/wav', {'X-Voice-Cache':'hit' if hit else 'miss'})
            else: self.send(404, {'error': 'Ruta desconocida'})
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            # Normal cancellation when the player closes the panel or starts another turn.
            return
        except Exception as exc:
            self.send(400, {'error': str(exc)[:200]})

if __name__ == '__main__':
    args = argparse.ArgumentParser(); args.add_argument('--download', action='store_true'); opts = args.parse_args()
    if opts.download: download()
    else:
        from faster_whisper import WhisperModel
        from piper import PiperVoice
        from piper.config import PiperConfig
        import onnxruntime as ort
        service = ThreadingHTTPServer(('127.0.0.1', 11435), Handler)
        service.tts_lock=threading.Lock(); service.stt_lock=threading.Lock(); service.tts_cache=OrderedDict()
        service.whisper = WhisperModel(str(WHISPER), device='cpu', compute_type='int8', cpu_threads=4, num_workers=1)
        # Keep synthesis from taking every CPU core while the language model
        # and Unreal are running. The complete text is prepared ahead of time.
        tts_options = ort.SessionOptions()
        tts_options.intra_op_num_threads = 1; tts_options.inter_op_num_threads = 1
        tts_options.execution_mode = ort.ExecutionMode.ORT_SEQUENTIAL
        tts_options.add_session_config_entry('session.intra_op.allow_spinning', '0')
        service.piper = PiperVoice(
            config=PiperConfig.from_dict(json.loads(Path(str(VOICE)+'.json').read_text(encoding='utf-8'))),
            session=ort.InferenceSession(str(VOICE),sess_options=tts_options,providers=['CPUExecutionProvider']))
        print('CE_VOICE_READY CPU 127.0.0.1:11435', flush=True)
        service.serve_forever()
