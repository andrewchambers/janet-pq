# janet-pq
Bindings to libpq.

# quick-examples

Basic usage:

```
(import pq)
(def conn (pq/connect "postgresql://localhost?dbname=postgres"))
(pq/exec "create table users(name text, data jsonb);")
(pq/exec "insert into users(name, data) values($1, $2);" "ac" (pq/jsonb @{"some" "data"}))
(pq/exec "select * from users where name = $1;" "ac")
[{"name" "ac" "data" @{"some" "data"}}]
```

Custom type encoding/decoding:

```
(import pq)
...

# directly insert custom encoding.
(pq/exec "insert into tab(x) values($1);" [TYPEOID ISBINARY BYTES])

# use a method for custom type encoding.
(pq/exec "insert into tab(x) values($1);" {:pq/marshal (fn [self] [TYPEOID ISBINARY BYTES])})

# Add a custom type decoder.
(put pg/default-decoder-table TYPEOID custom-decoder)
(pq/exec "select * from tab;")

# Dynamic decoder table
(with-dyns [:pq/decoder-table t]
  ...)

```


Error handling:

```
(import pq)
...
(try
  ...
  ([err] (when (pq/error? err)
    (def msg (pq/result/error-field err ...)))))
```
