# janet-pq
Bindings to libpq.

# quick-examples

```
(import pq)
(def conn (pq/connect "postgresql://localhost?dbname=postgres"))
(pq/exec "create table users(name text, data jsonb);")
(pq/exec "insert into users(name, data) values($1, $2);" "ac" (pq/jsonb @{"some" "data"}))
(pq/exec "select * from users where name = $1;" "ac")
{"name" "ac" "data" @{"some" "data"}}
```
