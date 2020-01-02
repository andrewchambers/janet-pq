(import sh)
(import process)
(import pq)

(defmacro with-unconditional
  [action body]
  (def b (tuple 'do body (tuple action)))
  (with-syms [err fiber]
    ~(try
       ,b
     ([,err ,fiber] (,action) (,propagate ,err ,fiber)))))

(defmacro assert
  [cond]
  ~(unless ,cond (error "fail")))

(defn cleanup []
  (each d (->>
            (sh/$$ ["find" "/tmp" "-maxdepth" "1" "-type" "d" "-name" "janet-pq-test.tmp.*"])
            (string/split "\n")
            (map string/trim)
            (filter (comp not empty?)))
    (print "cleaning up " d "...")
    (sh/$? ["pg_ctl" "-s" "-w" "-D" d "stop" "-m" "immediate"])
    (sh/$ ["rm" "-rf" d])))

(var pg-data-dir nil)

(defn connect []
  (pq/connect "postgresql://localhost?dbname=postgres"))

(var tests nil)

(defn run-tests
  []
  # Cleanup any previous runs that didn't have time to clean up.
  (cleanup)

  (set pg-data-dir 
    (string (sh/$$_ ["mktemp" "-d" "/tmp/janet-pq-test.tmp.XXXXX"])))
  (print "Launching postgres in " pg-data-dir "...")

  (sh/$ ["pg_ctl" "-s" "-D" pg-data-dir "initdb" "-o" "--auth=trust"])
  (sh/$ ["pg_ctl" "-s" "-w" "-D" pg-data-dir  "start" "-l" (string pg-data-dir "/test-log-file.txt")])

  (with-unconditional cleanup
      (tests)))

(set tests (fn []
  (print "running tests...")
  (def conn (connect))
  
  (defn round-trip-test
    [test-case]
    (pq/exec conn (string "create table roundtrip (a text, b " (test-case :coltype) ");"))
    (pq/exec conn "insert into roundtrip(a, b) values($1, $2);" "t" (test-case :val))
    (def v ((first (pq/exec conn "select * from roundtrip where a = $1;" "t")) "b"))
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
  (round-trip-test {:coltype "boolean" :val {:pq/encode (fn [&] [16 false "f"]) } :expected false})
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
  # json
  (round-trip-test {:coltype "json" :val (pq/json @{"hello" "json"}) :expected @{"hello" "json"}})
  # jsonb
  (round-trip-test {:coltype "jsonb" :val (pq/jsonb @{"hello" "json"}) :expected @{"hello" "json"}})
  

  # test slow path where we smalloc params.
  (do
    (pq/exec conn "create table t(a text, b text, c text, d text, e text, f text, g text, h text);")
    (pq/exec conn "insert into t(a,b,c,d,e,f,g,h) values($1,$2,$3,$4,$5,$6,$7,$8);"
                  "a" "b" "c" "d" "e" "f" "g" "h")
    (assert (deep= (pq/exec conn "select * from t;")
                   @[@{"a" "a" "b" "b" "c" "c" "d" "d" "e" "e" "f" "f" "g" "g" "h" "h"}]))
    (pq/exec conn "drop table t;"))

  # escaping functions.
  (assert (= (pq/escape-literal conn "123") "'123'"))
  (assert (= (pq/escape-identifier conn "123") "\"123\""))

  ))


(run-tests)