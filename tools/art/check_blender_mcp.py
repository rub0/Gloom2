"""Exercise real MCP initialize/list/call over stdio, optionally executing a reviewed Blender script."""

import argparse
import asyncio
import base64
import json
import os
from pathlib import Path
import sys

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client


async def run(script, goal, screenshot):
    root = Path(__file__).resolve().parents[2]
    environment = dict(os.environ)
    environment.update(DISABLE_TELEMETRY='true', BLENDER_MCP_SAFE_MODE='1', BLENDER_HOST='localhost', BLENDER_PORT='9876')
    environment['BLENDERMCP_ADDONS_DIR'] = str(root / '.cache/blender-profile/scripts/addons')
    server = StdioServerParameters(command=str(Path(sys.executable).with_name('blender-mcp.exe')), env=environment, cwd=str(root))
    async with stdio_client(server) as (reader, writer):
        async with ClientSession(reader, writer) as session:
            initialized = await session.initialize()
            print('MCP server:', initialized.serverInfo.model_dump_json())
            available = await session.list_tools()
            names = [tool.name for tool in available.tools]
            print('MCP tools:', json.dumps(names))
            result = await session.call_tool('get_scene_info', {'user_prompt': goal})
            if result.isError:
                raise RuntimeError(str(result))
            scene = json.loads('\n'.join(block.text for block in result.content if block.type == 'text'))
            if 'object_count' not in scene:
                raise RuntimeError('MCP did not return valid scene information')
            print('Scene:', result.model_dump_json())
            if script:
                result = await session.call_tool('execute_blender_code', {'code': Path(script).read_text(encoding='utf-8'), 'user_prompt': goal})
                if result.isError:
                    raise RuntimeError(str(result))
                content = '\n'.join(block.text for block in result.content if block.type == 'text')
                print(content)
                if not content.startswith('Code executed successfully:'):
                    raise RuntimeError('Blender execution was not successful')
            if screenshot:
                result = await session.call_tool('get_viewport_screenshot', {'max_size': 1280, 'user_prompt': goal})
                images = [block for block in result.content if block.type == 'image']
                if result.isError or len(images) != 1:
                    raise RuntimeError('Blender did not return a viewport image')
                screenshot.parent.mkdir(parents=True, exist_ok=True)
                screenshot.write_bytes(base64.b64decode(images[0].data, validate=True))
                print('MCP viewport saved:', screenshot)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--script', type=Path)
    parser.add_argument('--screenshot', type=Path, help='Save the image returned by the MCP viewport tool')
    parser.add_argument('--goal', required=True, help='User request associated with this validation')
    arguments = parser.parse_args()
    asyncio.run(run(arguments.script, arguments.goal, arguments.screenshot))
