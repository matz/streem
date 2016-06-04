# Streem
[![Build Status](https://travis-ci.org/matz/streem.svg?branch=master)](https://travis-ci.org/matz/streem)
[![Gitter](https://badges.gitter.im/Join Chat.svg)](https://gitter.im/matz/streem?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Streem is a stream based concurrent scripting language. It is based on a
programming model similar to the shell, with influences from Ruby, Erlang, and
other functional programming languages.

__Note:__ Streem is still in the design stage. It's not working yet.  Stay tuned.

## Examples
In Streem, a simple `cat` program looks like this:

```
stdin | stdout
```

Streem is a (sort of) DSL for data flows.  Above code means
building data-flow connection between `stdin` and `stdout`.
Actual data processing will be done in the event loop
invoked after program execution.

For another example, a simple FizzBuzz will look like this:

```
# seq(100) returns a stream of numbers from 1 to 100.
# A function object in pipeline works as a map function.
# stdout is an output destination.
seq(100) | map{|x|
  print("x=",x)
  if (x % 15 == 0)     "FizzBuzz"
  else if (x % 3 == 0) "Fizz"
  else if (x % 5 == 0) "Buzz"
  else                 x
} | stdout
```

The second part in the pipeline (`{x ->...}`) is a function
object.  If a function object is connected in the pipeline,
it will be invoked for each element in the stream.

## Compiling

### Installing dependencies

* bison
* flex
* gcc / clang

### Run make
```
make
```

Please note that Streem will not run any scripts yet, but you can check parse files and check syntax.

## Contributing
Send a pull request to <http://github.com/matz/streem>.  We consider
you have granted non-exclusive right to your contributed code under
MIT license.  Use <http://github.com/matz/streem/issues> for
discussion.

## License
MIT license (&copy; 2015-2016 Yukihiro Matsumoto)
