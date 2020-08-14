(import sh)
(import process)
(import pq)


(defn cleanup []
  (each d (->>
            (sh/$< find /tmp -maxdepth 1 -type d -name janet-pq-test.tmp.*)
            (string/split "\n")
            (map string/trim)
            (filter (comp not empty?)))
    (print "cleaning up " d "...")
    (sh/$? pg_ctl -s -w -D ,d stop -m immediate)
    (sh/$ rm -rf ,d)))

(var pg-data-dir nil)

(defn connect []
  (pq/connect "postgresql://localhost?dbname=postgres"))

(var tests nil)

(defn run-tests
  []
  # Cleanup any previous runs that didn't have time to clean up.
  (cleanup)

  (set pg-data-dir
       (sh/$<_ mktemp -d /tmp/janet-pq-test.tmp.XXXXX))

  (print "Launching postgres in " pg-data-dir "...")
  (sh/$ pg_ctl -s -D ,pg-data-dir initdb -o --auth=trust)

  (def pg-cfg (string pg-data-dir "/postgresql.conf"))
  (def pg-unix-socket-dir (string pg-data-dir "/_sockets"))
  (os/mkdir pg-unix-socket-dir)

  (->> (slurp pg-cfg)
       (string/replace "#unix_socket_directories = '/run/postgresql'"
                       (string "unix_socket_directories = '" pg-unix-socket-dir "'"))
       (spit pg-cfg))
  (def log (string pg-data-dir "/test-log-file.txt"))
  (try
    (sh/$ pg_ctl -s -w -D ,pg-data-dir start -l ,log)
    ([e f]
      (eprintf "start failed, log:\n%s" (string (slurp log)))
      (propagate e f)))

  (defer (cleanup)
    (tests)))

(set
  tests
  (fn []
    (print "running tests...")
    (def conn (connect))

    (defn round-trip-test
      [test-case]
      (pq/exec conn (string "create table roundtrip (a text, b " (test-case :coltype) ");"))
      (pq/exec conn "insert into roundtrip(a, b) values($1, $2);" "t" (test-case :val))
      (def v (pq/val conn "select b from roundtrip where a = $1;" "t"))
      (def expected (get test-case :expected (get test-case :val)))
      (unless (deep= v expected)
        (error
          (with-dyns [:out (buffer/new 0)]
            (print "expected:")
            (print (type expected))
            (pp expected)
            (print "got:")
            (print (type v))
            (pp v)
            (string (dyn :out)))))
      (pq/exec conn "drop table roundtrip;"))

    # nil
    (round-trip-test {:coltype "boolean"})
    # booleans
    (round-trip-test {:coltype "boolean" :val true})
    (round-trip-test {:coltype "boolean" :val false})
    (round-trip-test {:coltype "boolean" :val [16 false "t"] :expected true})
    (round-trip-test {:coltype "boolean" :val [16 false "f"] :expected false})
    (round-trip-test {:coltype "boolean" :val {:pq/encode (fn [&] [16 false "f"])} :expected false})
    # smallint -32768 to +32767
    (round-trip-test {:coltype "smallint" :val -32768})
    (round-trip-test {:coltype "smallint" :val 32767})
    (round-trip-test {:coltype "smallint" :val 0})
    # integer -2147483648 to +2147483647
    (round-trip-test {:coltype "integer" :val -2147483648})
    (round-trip-test {:coltype "integer" :val 2147483647})
    (round-trip-test {:coltype "integer" :val 0})
    # bigint    -9223372036854775808 to +9223372036854775807
    #(round-trip-test {:coltype "bigint" :val (int/s64 "-9223372036854775808")})
    #(round-trip-test {:coltype "bigint" :val (int/s64 "9223372036854775807")})
    # smallserial  1 to 32767
    (round-trip-test {:coltype "smallserial" :val 1})
    (round-trip-test {:coltype "smallserial" :val 32767})
    # serial 1 to 2147483647
    (round-trip-test {:coltype "serial" :val 1})
    (round-trip-test {:coltype "serial" :val 2147483647})
    # bigserial 1 to 9223372036854775807
    (round-trip-test {:coltype "bigserial" :val (int/s64 "1")})
    (round-trip-test {:coltype "bigserial" :val (int/s64 "9223372036854775807")})
    # real
    (round-trip-test {:coltype "real" :val 1})
    (round-trip-test {:coltype "real" :val -1})
    (round-trip-test {:coltype "real" :val 1.123})
    # double precision
    (round-trip-test {:coltype "double precision" :val 1})
    (round-trip-test {:coltype "double precision" :val -1})
    (round-trip-test {:coltype "double precision" :val 1.123})
    # text
    (round-trip-test {:coltype "text" :val "hello"})
    (round-trip-test {:coltype "text" :val "😀😀😀"})
    # varchar
    (round-trip-test {:coltype "varchar(128)" :val "hello"})
    (round-trip-test {:coltype "varchar(128)" :val "😀😀😀"})
    # char(n)
    (round-trip-test {:coltype "char(8)" :val "hello" :expected "hello   "})
    #bytea
    (round-trip-test {:coltype "bytea" :val "hello" :expected "hello"})
    (round-trip-test {:coltype "bytea" :val @"hello" :expected "hello"})
    (each n [0 1 2 63 64]
      (def randbuf (os/cryptorand n))
      (round-trip-test {:coltype "bytea" :val randbuf :expected (string randbuf)}))
    # json
    (round-trip-test {:coltype "json" :val (pq/json @{"hello" "json"}) :expected @{"hello" "json"}})
    # jsonb
    (round-trip-test {:coltype "jsonb" :val (pq/jsonb @{"hello" "json"}) :expected @{"hello" "json"}})

    # test slow path where we smalloc params.
    (do
      (pq/exec conn "create table t(a text, b text, c text, d text, e text, f text, g text, h text);")
      (pq/exec conn "insert into t(a,b,c,d,e,f,g,h) values($1,$2,$3,$4,$5,$6,$7,$8);"
               "a" "b" "c" "d" "e" "f" "g" "h")
      (assert (deep= (pq/all conn "select * from t;")
                     @[@{:a "a" :b "b" :c "c" :d "d" :e "e" :f "f" :g "g" :h "h"}]))
      (pq/exec conn "drop table t;"))

    # escaping functions.
    (assert (= (pq/escape-literal conn "123") "'123'"))
    (assert (= (pq/escape-identifier conn "123") "\"123\""))

    (do
      (pq/exec conn "create table t(a text);")
      (pq/exec conn "insert into t(a) values($1);" "a")
      (assert (deep= (pq/row conn "select * from t limit 1;")
                     @{:a "a"}))
      (assert (nil? (pq/row conn "select * from t where a = 'b';")))
      (pq/exec conn "drop table t;"))

    # Test various ways of closing.
    (do
      (def conn1 (connect))
      (def conn2 (connect))
      (var conn3 (connect))
      (pq/close conn1)
      (:close conn2)
      (set conn3 nil)
      (gccollect))

    # Test txn macro

    (assert (not (pq/in-transaction? conn)))

    # basic commit
    (assert
      (=
        0
        (pq/txn conn {}
                (assert (pq/in-transaction? conn))
                (pq/exec conn "create table t(a text);")
                (pq/val conn "select count(*)::float from t;"))))

    # manual commit
    (assert
      (=
        :success
        (pq/txn conn {}
                (pq/exec conn "create table t2(b text);")
                (pq/commit conn :success))))

    # error rollback
    (protect
      (pq/txn conn {}
              (pq/exec conn "drop table t;")
              (error "fudge")))

    (assert (zero? (pq/val conn "select count(*)::float from t;")))

    # user signal rollback
    (each sig [0 1 2 3]
      (defn action []
        (pq/txn conn {}
                (pq/exec conn "drop table t;")
                (signal sig :val)))
      (def fb (fiber/new action :t))
      (resume fb))

    (assert (zero? (pq/val conn "select count(*)::float from t;")))

    # far return rollback
    (label outer
           (pq/txn conn {}
                   (pq/exec conn "drop table t;")
                   (return outer)))

    (assert (zero? (pq/val conn "select count(*)::float from t;")))

    # manual rollback
    (assert
      (=
        7
        (pq/txn conn {}
                (assert (zero? (pq/val conn "select count(*)::float from t;")))
                (pq/exec conn "drop table t;")
                (pq/rollback conn 7)
                123)))
    (assert (not (pq/in-transaction? conn)))

    # custom enum type

    (pq/exec conn "CREATE TYPE mood AS ENUM ('sad', 'ok', 'happy');")
    (pq/exec conn "create table person(name text, current_mood mood);")
    (put pq/*decoders* (pq/val conn "SELECT oid FROM pg_type WHERE typname = 'mood';") keyword)
    (pq/exec conn "insert into person values($1, $2);" "a" "happy")
    (pq/exec conn "insert into person values($1, $2);" "b" :sad)
    (assert (= (pq/val conn "select current_mood from person where name = 'a'") :happy))
    (assert (= (pq/val conn "select current_mood from person where name = 'b'") :sad))))

(run-tests)
