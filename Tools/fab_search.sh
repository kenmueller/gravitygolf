#!/bin/sh
# Compact Fab search (free listings):  Tools/fab_search.sh "hedge" [listingTypes] [count]
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$ROOT/Tools/mcp.py" call call_tool "$(python3 -c 'import json,sys;print(json.dumps({"toolset_name":"FabToolset.FabToolset","tool_name":"SearchFabListings","arguments":{"keywords":sys.argv[1],"bFreeOnly":True,"listingTypes":sys.argv[2],"count":int(sys.argv[3])}}))' "$1" "${2:-}" "${3:-10}")" | python3 -c '
import json,sys
d=json.loads(json.load(sys.stdin)["returnValue"])
for r in d["results"]:
    print(r["listingId"], "|", r["title"], "|", r["seller"], "|", r["listingType"], "|", ",".join(o["assetFormat"] for o in r.get("installOptions",[])))
'
