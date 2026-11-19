#!/usr/bin/env python3
"""Minimal client for the Unreal Editor's MCP server (Streamable HTTP), for scripting from a shell.

    Tools/mcp.py tools                      list native tools
    Tools/mcp.py call <tool> '<json args>'  call a tool
    Tools/mcp.py python <file.py>           run an editor Python file via execute_python
"""

import json
import os
import sys
import urllib.request

URL = os.environ.get("UE_MCP_URL", "http://127.0.0.1:8000/mcp")
HEADERS = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}


def post(payload, session=None):
    headers = dict(HEADERS)
    if session:
        headers["Mcp-Session-Id"] = session
    request = urllib.request.Request(URL, data=json.dumps(payload).encode(), headers=headers, method="POST")
    with urllib.request.urlopen(request, timeout=600) as response:
        session = response.headers.get("Mcp-Session-Id", session)
        body = response.read().decode()
    if not body.strip():
        return None, session
    if body.lstrip().startswith("{"):
        return json.loads(body), session
    # Server-sent events: take the last data line that holds a JSON-RPC message.
    message = None
    for line in body.splitlines():
        if line.startswith("data:"):
            message = json.loads(line[5:].strip())
    return message, session


def connect():
    init, session = post({
        "jsonrpc": "2.0", "id": 1, "method": "initialize",
        "params": {"protocolVersion": "2025-06-18", "capabilities": {},
                   "clientInfo": {"name": "gravitygolf-cli", "version": "1"}},
    })
    post({"jsonrpc": "2.0", "method": "notifications/initialized"}, session)
    return init, session


def call(session, name, arguments):
    result, _ = post({"jsonrpc": "2.0", "id": 2, "method": "tools/call",
                      "params": {"name": name, "arguments": arguments}}, session)
    return result


def show(result):
    if result and "result" in result:
        for block in result["result"].get("content", []):
            if block.get("type") == "text":
                print(block["text"])
            elif block.get("type") == "image" and os.environ.get("MCP_SAVE_IMAGE"):
                import base64
                with open(os.environ["MCP_SAVE_IMAGE"], "wb") as image:
                    image.write(base64.b64decode(block["data"]))
                print(f"saved {os.environ['MCP_SAVE_IMAGE']}")
            else:
                print(f"[{block.get('type')} block]")
        if result["result"].get("isError"):
            sys.exit(1)
    else:
        print(json.dumps(result, indent=2))
        sys.exit(1)


def main():
    init, session = connect()
    command = sys.argv[1] if len(sys.argv) > 1 else "info"
    if command == "info":
        print(json.dumps(init["result"]["serverInfo"], indent=2))
        print(json.dumps(init["result"].get("_meta", {}), indent=2))
    elif command == "tools":
        result, _ = post({"jsonrpc": "2.0", "id": 2, "method": "tools/list"}, session)
        for tool in result["result"]["tools"]:
            print(f"{tool['name']}: {tool.get('description', '')[:120]}")
    elif command == "call":
        show(call(session, sys.argv[2], json.loads(sys.argv[3]) if len(sys.argv) > 3 else {}))
    elif command == "python":
        code = open(sys.argv[2]).read()
        show(call(session, "call_tool", {"tool_name": "execute_python", "arguments": {"code": code}}))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
