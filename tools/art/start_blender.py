"""Start the project-local Blender MCP addon in the portable Blender UI."""

from pathlib import Path
import shutil

import addon_utils
import bpy


root = Path(__file__).resolve().parents[2]
bundled = root / '.cache/blender-mcp-env/Lib/site-packages/blender_mcp/bundled/addon.py'
scripts = root / '.cache/blender-profile/scripts'
addons = scripts / 'addons'
addons.mkdir(parents=True, exist_ok=True)
shutil.copy2(bundled, addons / 'blender_mcp.py')
bpy.utils.refresh_script_paths()
addon_utils.modules_refresh()
addon_utils.enable('blender_mcp', default_set=True, persistent=True)
prefs = bpy.context.preferences.addons.get('blender_mcp')
if prefs is None:
    raise RuntimeError('Blender MCP addon did not load')
prefs.preferences.telemetry_consent = False
bpy.context.preferences.view.show_splash = False
for field in ('polyhaven', 'hyper3d', 'hunyuan3d', 'sketchfab', 'polypizza'):
    setattr(bpy.context.scene, 'blendermcp_use_' + field, False)
bpy.context.scene.blendermcp_port = 9876
bpy.context.scene.blendermcp_auto_start_server = False
bpy.ops.wm.save_userpref()
bpy.ops.blendermcp.start_server()
if not bpy.context.scene.blendermcp_server_running:
    raise RuntimeError('Blender MCP socket did not start')
print('GLOOM_BLENDER_READY: localhost:9876; telemetry OFF; external providers OFF', flush=True)
