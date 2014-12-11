# Streem - stream based concurrent scripting language

Streem is a concurrent scripting language based on a programming model
similar to shell, with influences from Ruby, Erlang and other
functional programming languages.

In Streem, a simple `cat` program looks like this:

```
STDIN | STDOUT
```

And a simple FizzBuzz will look like this:

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

# Note

Streem is still in the design stage. It's not working yet.  Stay tuned.

# Requirement

* bison
* flex
* gcc / clang

# How to compile

```
cd src
make
```

# License

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
