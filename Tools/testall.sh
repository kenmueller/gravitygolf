#!/bin/sh
# Everything that can be checked without hands, in one go. Needs the editor open (Tools/editor.sh) and takes about
# twenty minutes. Each line is one check; the last line says whether all of them passed.
#   Tools/testall.sh            (add "web" to also run the browser tests against the last web build)
# The whole script is one function, called on its last line: the shell reads all of it before running any of it, so
# editing this file while it runs cannot change or break the run in progress.
main() {
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FAILED=""
alive() { ps -axo comm | grep -q "MacOS/UnrealEditor$"; }
check() { # name, command...
	NAME="$1"
	shift
	if OUT="$("$@" 2>&1)"; then
		printf 'ok    %s\n' "$NAME"
		return
	fi
	# The engine preview now and then dies as play starts (an assertion in a lock, twice in sixty starts on the day
	# this was written). That is not this check failing: bring the editor back and run the check once more.
	if ! alive; then
		printf '      (the editor crashed during "%s": restarting it and trying once more)\n' "$NAME"
		CRASHES=$((CRASHES + 1))
		"$ROOT/Tools/editor.sh" >/dev/null 2>&1
		if OUT="$("$@" 2>&1)"; then
			printf 'ok    %s\n' "$NAME"
			return
		fi
	fi
	printf 'FAIL  %s\n%s\n' "$NAME" "$(echo "$OUT" | tail -5)"
	FAILED="$FAILED $NAME"
}
CRASHES=0
check "sim tests" "$ROOT/Tests/Sim/run.sh"
for COURSE in terraces saltmirror canyon lantern returning brief colonnade stones glass periscope leapfrog afteryou waithere handover wires; do
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
	check "browser: every hole, the Workshop, play, phone" "$ROOT/Tools/webtest.sh" tour workshop workshop-phone play phone phone-menus phone-machines web-menus
	check "mouse look, and the pointer's first jump ignored" "$ROOT/Tools/mousetest.sh"
	check "a gamepad walks and turns" "$ROOT/Tools/padtest.sh"
	check "a browser joins a desktop host" "$ROOT/Tools/webjointest.sh"
	check "a phone-size browser joins a desktop host, by touch" env GG_JOIN_PHONE=1 "$ROOT/Tools/webjointest.sh"
fi
[ $CRASHES -gt 0 ] && echo "(the editor crashed $CRASHES time(s) and was restarted)"
[ -z "$FAILED" ] && echo "ALL PASSED" || echo "FAILED:$FAILED"
[ -z "$FAILED" ]
}
main "$@"; exit
