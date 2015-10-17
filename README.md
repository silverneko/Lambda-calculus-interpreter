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
let x y in z
-- evaluate `z` in the context of `x` being bounded to `y`
:let x y
-- bound `x` to `y` (this changes the global context)
-- because this is not an complete expression it can only be used at the outermost layer
-- see `testcase/fibtest` for example
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
