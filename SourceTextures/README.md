# Superficies originales de Cero Estrellas

`Scripts/generate_material_textures.py` genera 19 texturas de 512 × 512 sin imágenes externas. La semilla es fija para poder reproducirlas. Se importan con mipmaps en `/Game/Prototype/Textures`.

- `*_A`: multiplicador de albedo en sRGB. El parámetro `Tint` conserva la paleta de cada objeto.
- `*_N`: normal tangente DirectX, compresión Normalmap, sin sRGB.
- `*_ORM`: oclusión, rugosidad y metalicidad en R/G/B; compresión Masks, sin sRGB.
- `Screen_A`: interfaz ficticia de inversión para los portátiles; material emissive moderado.

Madera, piedra, tela, metal cepillado, piel y pintura tienen mapas separados. Los materiales no necesitan ruido procedural por píxel para estas superficies. Las coordenadas UV proyectadas de las mallas también se regeneran con `Scripts/model_cafe.py`; los detalles se mueven con el objeto.

Tras cambiar solo las texturas, `Scripts/refresh_material_textures.py` permite reimportarlas sin regenerar las mallas ni los materiales. Se ejecuta mediante `UnrealEditor-Cmd.exe -run=pythonscript -script=...`, usando rutas absolutas con `/` para evitar que Unreal interprete `\r` dentro de una ruta entrecomillada. Después se empaqueta con `Scripts/package.ps1 -IterativeCook`; `-ReuseCook` dejaría los recursos anteriores en el ejecutable.
