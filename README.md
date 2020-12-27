# Streem

[![Build Status](https://github.com/matz/streem/workflows/ci/badge.svg)](https://github.com/matz/streem/actions?query=workflow%3Aci)
[![Gitter](https://badges.gitter.im/JoinChat.svg)](https://gitter.im/matz/streem?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Streem is a stream based concurrent scripting language. It is based on a
programming model similar to the shell, with influences from Ruby, Erlang, and
other functional programming languages.

__Note:__ Streem is still in the design stage. It's not working yet.  Stay tuned.

## Compiling

### Installing dependencies

* bison
* flex
* gcc / clang

### Run make

```
make
```

## Examples

In Streem, a simple `cat` program looks like this:

```
stdin | stdout
```

You can try it out by (firstly `cd` to streem top directory):

```
$ bin/streem -e 'stdin | stdout'
```

or

```
$ bin/streem examples/01cat.strm
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
seq(100) | map{x->
  if (x % 15 == 0)     "FizzBuzz"
  else if (x % 3 == 0) "Fizz"
  else if (x % 5 == 0) "Buzz"
  else                 x
} | stdout
```

The second part in the pipeline (`{x ->...}`) is a function
object.  If a function object is connected in the pipeline,
it will be invoked for each element in the stream.

There are more examples under folder `examples/`. Just play with them!

## Contributing

Send a pull request to <https://github.com/matz/streem>.  We consider
you have granted non-exclusive right to your contributed code under
MIT license.  Use <https://github.com/matz/streem/issues> for
discussion.

## License

MIT license (&copy; 2015-2016 Yukihiro Matsumoto)
