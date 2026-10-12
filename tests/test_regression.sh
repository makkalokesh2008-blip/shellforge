#!/bin/bash
cd /home/lokesh/shellforge-push
SF=./shellforge
P=0; F=0
chk() {
    local name=$1 inp=$2 pat=$3
    local out
    out=$(printf '%s
exit
' "$inp" | timeout 8 $SF 2>&1) || true
    if echo "$out" | grep -qF "$pat"; then
        echo "  PASS: $name"; ((P++)) || true
    else
        echo "  FAIL: $name  [want: $pat]"
        echo "        got: $(echo "$out" | tail -3)"
        ((F++)) || true
    fi
}
echo '=== ShellForge Tests ==='
chk 'help'       'help'             'ShellForge'
chk 'help-exit'  'help'             'exit'
chk 'echo'       'echo hello world' 'hello world'
chk 'pwd'        'pwd'              '/'
out=$(printf 'cd /tmp
pwd
exit
' | timeout 8 $SF 2>&1)
echo "$out" | grep -q '/tmp' && { echo '  PASS: cd+pwd'; ((P++)) || true; } || { echo '  FAIL: cd+pwd'; ((F++)) || true; }
chk 'uname'      'uname'            'Linux'
chk 'pipe-wc'    'echo hello world | wc -w'   '2'
chk 'pipe-3'     'echo a b c | wc -w | cat'   '3'
TMP=$(mktemp -d /tmp/sftest_XXXXXX)
printf 'echo testdata > %s/r1.txt
cat %s/r1.txt
exit
' "$TMP" "$TMP" | timeout 8 $SF 2>&1 | grep -q 'testdata' && { echo '  PASS: out-redir'; ((P++)) || true; } || { echo '  FAIL: out-redir'; ((F++)) || true; }
printf 'echo L1 > %s/a.txt
echo L2 >> %s/a.txt
cat %s/a.txt
exit
' "$TMP" "$TMP" "$TMP" | timeout 8 $SF 2>&1 | grep -q 'L2' && { echo '  PASS: append-redir'; ((P++)) || true; } || { echo '  FAIL: append-redir'; ((F++)) || true; }
echo inputdata > "$TMP/inp.txt"
printf 'cat < %s/inp.txt
exit
' "$TMP" | timeout 8 $SF 2>&1 | grep -q 'inputdata' && { echo '  PASS: in-redir'; ((P++)) || true; } || { echo '  FAIL: in-redir'; ((F++)) || true; }
rm -rf "$TMP"
chk 'single-q'   "echo 'hello world'"  'hello world'
chk 'double-q'   'echo "two words"'    'two words'
chk 'var-HOME'   'echo $HOME'          '/home'
chk 'var-braced' 'echo ${HOME}'        '/home'
out=$(printf 'no_such_cmd_xyz
echo still_alive
exit
' | timeout 8 $SF 2>&1)
echo "$out" | grep -q 'still_alive' && { echo '  PASS: err-recovery'; ((P++)) || true; } || { echo '  FAIL: err-recovery'; ((F++)) || true; }
out=$(printf 'cd /no_such
echo ok_after
exit
' | timeout 8 $SF 2>&1)
echo "$out" | grep -q 'ok_after' && { echo '  PASS: cd-err-recovery'; ((P++)) || true; } || { echo '  FAIL: cd-err-recovery'; ((F++)) || true; }
echo "=== DONE: $P passed, $F failed ==="
[ $F -eq 0 ]
