# Lambda-calculus-interpreter

Untyped lambda calculus

Project of the [BYOHC-Workshop](https://github.com/CindyLinz/BYOHC-Workshop).

## How to use
### Make
```bash
$ make
```

```bash
$ ./main [source pathname]
# ./main samplecode/iotest
```

## Syntax
### Lambda
```
\x y
```

### Beta reduce
```
(\x y) z  -- evaluates `y` with `x` bounded to `z`
```

Expressions are left associative.
- `a b c d` evaluates to `((a b) c) d`
- `a \x y z` evaluates to `a (\x (y z))`

### IO
```
-- print a character
-- putChar :: Char -> IO ()
putChar 'a'

-- Monadic bind
-- >>= :: IO a -> (a -> IO b) -> IO b
-- Sequencial compose
-- >>  :: IO a -> IO b -> IO b

-- Run an IO monad with runIO
runIO (>> (putChar '4') (putChar '2'))
```

### Comments
```
-- Comments start with two minuses
-- Only have single-line comments right now
```

### Semantic sugars
```
let x y in z
--evaluates `z` in the context of `x` being bounded to `y`
-- i.e. `(\x z) y`
```

### Example
```
let helloworld (: 'H' (: 'e' (: 'l' (: 'l' (: 'o' (: ',' (: ' ' (: 'w' (: 'o' (: 'r' (: 'l' (: 'd' (: '!' []))))))))))))) in
let fibs Y(\fibs (: 1 (: 1 (zipWith + fibs (tail fibs))))) in
let main (>> 
    (putStrLn helloworld)
    (sequence (map (\x putStrLn (showInt x)) (take 30 fibs)))
  )
in runIO main
-- See complete code in `samplecode/prelude` and `samplecode/iotest`
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
