Streem should recognise the sequence funciton:
  $ echo 'seq(100)' | $TESTDIR/bin/streem
  Syntax OK

Streem should recognise an if statement:
  $ echo 'if true {}' | $TESTDIR/bin/streem
  Syntax OK

Streem should recognise an if-else statement:
  $ echo 'if true {} else {}' | $TESTDIR/bin/streem
  Syntax OK

Streem should recognise a mathimatical expression:
  $ echo '((1 + 1) * (99 / 3)) % 33' | $TESTDIR/bin/streem
  Syntax OK

Streem should recognise a boolean expression:
  $ echo '1 < 2 && 2 <= 3 && (4 >= 5) || ((3 + 3) > 8)' | $TESTDIR/bin/streem
  Syntax OK

Streem should recognise a stream expression:
  $ echo 'STDIN | STDOUT' | $TESTDIR/bin/streem
  Syntax OK

Streem should recognise a block expression:
  $ echo '{ |x| x + 1 }' | $TESTDIR/bin/streem
  Syntax OK

Streem should recognise a block expression in a stream:
  $ echo 'STDIN | { |s| s + "!" } | STDOUT' | $TESTDIR/bin/streem
  Syntax OK

Streem should recognise a block expression with a statement in a stream:
  $ echo 'STDIN | { |s| if (s != "exit") { s } else { exit() } } | STDOUT' | $TESTDIR/bin/streem
  Syntax OK

