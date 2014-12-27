# Streem - stream based concurrent scripting language
[![Build Status](https://travis-ci.org/matz/streem.svg?branch=master)](https://travis-ci.org/matz/streem)
[![Gitter](https://badges.gitter.im/Join Chat.svg)](https://gitter.im/matz/streem?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Streem is a concurrent scripting language based on a programming model
similar to shell, with influences from Ruby, Erlang and other
functional programming languages.

__Note:__ Streem is still in the design stage. It's not working yet.  Stay tuned.

## Examples
In Streem, a simple `cat` program looks like this:

```
STDIN | STDOUT
```

Streem is a (sort of) DSL for data flows.  Above code means
building data-flow connection between `STDIN` and `STDOUT`.
Actual data processing will be done in the event loop
invoked after program execution.

For another example, a simple FizzBuzz will look like this:

```
seq(100) | {|x|
  if x % 15 == 0 {
    "FizzBuzz"
  }
  else if x % 3 == 0 {
    "Fizz"
  }
  else if x % 5 == 0 {
    "Buzz"
  }
  else {
    x
  }
} | STDOUT
```

The second part in the pipeline (`{|x|...}`) is a function
object.  If a function object is connected in the pipeline,
it will be invoked for each elements in the stream.

## Compiling

### Install dependencies
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
Streem is distributed under MIT license.

Copyright (c) 2015 Yukihiro Matsumoto

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
