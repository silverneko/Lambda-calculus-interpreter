# Lambda-calculus-interpreter

Untyped lambda calculus

Project of the [BYOHC-Workshop](https://github.com/CindyLinz/BYOHC-Workshop).

## How to use
```bash
$ make
$ ./main
```
or
```bash
$ ./main < testcase/fibtest
```

Type `Ctrl-D` to exit interactive shell.

## Syntax
### Lambda
```
\x y
```

### Beta reduce
```
(\x y) z
```

Expressions are left associative.
- `a b c d` evaluates to `((a b) c) d`
- `a \x y z` evaluates to `a (\x (y z))`

### Comments
```
-- Comments start with two minuses and it must occupy a whole line
```

### Semantic sugars
```
a $ b c d
-- evaluates to `a ((b c) d)` but not `((a b) c) d`
-- use dollar sign to omit parentheses
let x y z
-- evaluates `z` in the context of `x` being bounded to `y`
:let x y
-- bound `x` to `y` (this changes the global context)
-- because this is not an complete expression it can only be used at the outermost layer
-- see `testcase/fibtest` for example
```

### Example
```
:let zero 0
(
let x 12 $
let y zero $
+ x $ + y 3
)
-- will show 15
```

## TODO
- [x] Parsing
- [x] To json
- [ ] From json
- [x] Substitution
- [x] Normal form
- [x] Weak normal form
- [ ] Blah Blah
- [ ] Refactor
