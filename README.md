# janet-pq
Bindings to libpq.

# quick-examples

Basic usage:

```
(import pq)
(def conn (pq/connect "postgresql://localhost?dbname=postgres"))
(pq/exec conn "create table users(name text, data jsonb);")
(pq/exec conn "insert into users(name, data) values($1, $2);" "ac" (pq/jsonb @{"some" "data"}))
(pq/all conn "select * from users where name = $1;" "ac")
[{"name" "ac" "data" @{"some" "data"}}]
```

Custom type encoding/decoding:

```
(import pq)
...

# directly insert custom encoding.
(pq/exec conn "insert into tab(x) values($1);" [TYPEOID ISBINARY BYTES])

# use a method for custom type encoding.
(pq/exec conn "insert into tab(x) values($1);" {:pq/marshal (fn [self] [TYPEOID ISBINARY BYTES])})

# Add a custom type decoder.
(put pg/*decoders* TYPEOID custom-decoder)
(pq/all conn "select * from tab;")

```


Error handling:

```
(import pq)
...
(try
  ...
  ([err] (when (pq/error? err)
    (def msg (pq/result-error-field err ...)))))
```

# Special thanks

[Jon Staab](https://github.com/staab) - The author of the first janet postgres library from which this was inspired, and a core contributor to this library.
