# Math

Streem has a lot Mathematical functions defined by the C standard.  

## Constants

### PI

The mathematical constant π = 3.141592…, to available precision.

```
# Output: 3.1415926353898
print(PI)
```

### E

The mathematical constant e = 2.718281…, to available precision.  

```
# Output: 2.718281828459
print(E)
```

## Number-theoretic and representation functions

### ceil(x)

Return the ceiling of x.  

```
# Output: 46
print(ceil(45.54))
```

### fabs(x)

Return the absolute value of x.  

```
# Output: 2
print(fabs(-2))
```

### gcd(x, y)

Return the greatest common divisor of the x and y.Note x and y must be integers.

```
Output: 2
print(gcd(4,10))
```

### trunc(x)

Return the Real value x truncated to an Integral.  

```
Output: 9
print(trunc(9.13))
```  

### int(x)

Same as trunc(x).  

### floor(x)

Return the floor of x.  

```
# Output: 2
print(floor(2.7))
```

### round(x)

Return the rounding of x.  

```
# Output: 10
print(round(9.76))
```

### frexp(x, y)

Decompose x into mantissa and exponent. y is the exponent.

```
# Output: 0.512500
print(frexp(16.4,5))
```

### ldexp(x, y)

Return the power y of x times 2. (x * (2 ^ y))

```
# Output: 6.00000
print(ldexp(6,3))
```

## Trigonometric functions

### sin(x)

Return the sine of x.  

### cos(x)

Return the cosine of x.  

### tan(x)

Return the tangent of x.  

### asin(x)

Return the arc sine of x.  

### acos(x)

Return the arc cosine of x.  

### atan(x)

Return the arc tangent of x.  

### hypot(x, y)

Return the `sqrt(x * x + y * y)`(Euclidean norm).  

```
# Output: 5
print(hypot(3,4))
```  

## Hyperbolic functions

### asinh(x)

Return the inverse hyperbolic sine of x.  

### acosh(x)

Return the inverse hyperbolic cosine of x.  

### atanh(x)

Return the inverse hyperbolic tangent of x.  

### cosh(x)

Return the hyperbolic cosine of x.  

### sinh(x)

Return the hyperbolic sine of x.  

### tanh(x)

Return the hyperbolic tangent of x.  

## logarithmic functions

### exp(x)

Return e raised to the power x.  

### log(x)

Return the natural logarithm of x.  

### log2(x)

Return the base-2 logarithm of x.  

### log10(x)

Return the base-10 logarithm of x.  

## Power functions

### pow(x,y)

Return x raised to the power y.

```
# Output: 25
print(pow(5,2))
```  

### sqrt(x)

Return the square root of x.

```
# Output: 5
print(sqrt(25))
```  
