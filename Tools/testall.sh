#!/bin/sh
# Everything that can be checked without hands, in one go. Needs the editor open (Tools/editor.sh) and takes about
# twenty minutes. Each line is one check; the last line says whether all of them passed.
#   Tools/testall.sh            (add "web" to also run the browser tests against the last web build)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FAILED=""
check() { # name, command...
	NAME="$1"
	shift
	if OUT="$("$@" 2>&1)"; then
		printf 'ok    %s\n' "$NAME"
	else
		printf 'FAIL  %s\n%s\n' "$NAME" "$(echo "$OUT" | tail -5)"
		FAILED="$FAILED $NAME"
	fi
}
check "sim tests" "$ROOT/Tests/Sim/run.sh"
for COURSE in terraces saltmirror canyon lantern returning brief colonnade stones glass periscope leapfrog afteryou wires; do
	check "proof: $COURSE" "$ROOT/Tests/Sim/solve.sh" "$COURSE"
done
check "every hole solves in the engine" "$ROOT/Tools/smoke.sh"
check "grilles stop a carried machine" "$ROOT/Tools/grilletest.sh"
check "the Workshop" "$ROOT/Tools/workshoptest.sh"
check "two players stay in step" sh -c "'$ROOT/Tools/nettest.sh' | grep -q DIFF && exit 1 || exit 0"
check "two players in the Workshop" "$ROOT/Tools/nettest_workshop.sh"
check "two players and the grilles" "$ROOT/Tools/nettest_grille.sh"
check "host and join, two copies of the game" "$ROOT/Tools/hosttest.sh"
if [ "$1" = "web" ]; then
	check "browser: every hole, the Workshop, play, phone" "$ROOT/Tools/webtest.sh" tour workshop workshop-phone play phone phone-menus phone-machines
fi
[ -z "$FAILED" ] && echo "ALL PASSED" || echo "FAILED:$FAILED"
[ -z "$FAILED" ]
