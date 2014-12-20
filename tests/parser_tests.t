Streem should recognise the sequence funciton:
  $ echo 'seq(100)' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise an if statement:
  $ echo 'if true {}' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise an if-else statement:
  $ echo 'if true {} else {}' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a mathimatical expression:
  $ echo '((1 + 1) * (99 / 3)) % 33' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a boolean expression:
  $ echo '1 < 2 && 2 <= 3 && (4 >= 5) || ((3 + 3) > 8) && 1 == 1' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a stream expression:
  $ echo 'STDIN | STDOUT' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a block expression:
  $ echo '{ |x| x + 1 }' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a block expression in a stream:
  $ echo 'STDIN | { |s| s + "!" } | STDOUT' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a block expression with a statement in a stream:
  $ echo 'STDIN | { |s| if (s != "exit") { s } else { exit() } } | STDOUT' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a block expression with an if statement and return values in a stream:
  $ echo 'seq(100) | { |x| if x % 15 == 0 { "FizzBuzz" } else if x % 3 == 0 { "Fizz" } else if x % 5 == 0 { "Buzz" } else { x } } | STDOUT' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a block expression with a break statement in a stream:
  $ echo 'seq(100) | { |x| if x % 15 == 0 { break } else if x % 3 == 0 { "Fizz" } else if x % 5 == 0 { "Buzz" } else { x } } | STDOUT' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise nil:
  $ echo 'nil' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise true:
  $ echo 'true' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise false:
  $ echo 'false' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a literal string:
  $ echo '"It works!!"' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a variable assignment using <-:
  $ echo 'foo <- 1' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise a variable assignment using ->:
  $ echo '1 -> foo' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise return statement in lambda:
  $ echo '{ |x| return x + 1 }' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise basic math:
  $ echo '((x + 1) * (9 / 3)) % 10' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise lists:
  $ echo '[1, 2, 3, 4]' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise funciton calls:
  $ echo 'fnc(1, 2, 3)' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise method calls:
  $ echo 'obj.meth(1, 2, 3)' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise map:
  $ echo '[ "foo": 1, "bar": "BAZ" ]' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise empty map:
  $ echo '[ : ]' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise empty array:
  $ echo '[ ]' | $TESTDIR/../bin/streem
  Syntax OK

Streem should recognise multiple statements on one line:
  $ echo 'puts("foo"); true' | $TESTDIR/../bin/streem
  Syntax OK
