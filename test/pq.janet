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
  (pq/connect (string "postgresql://localhost?dbname=postgres")))

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
  (pq/exec conn "create table foo (a text, b boolean);")
  
  (pq/exec conn "insert into foo(a, b) values($1, $2);" "t1" true)
  (unless
    (= (freeze (pq/exec conn "select * from foo where a = $1;" "t1"))
       [{"a" "t1" "b" true}])
    (error "fail"))

  (pq/exec conn "insert into foo(a, b) values($1, $2);" "t3" {:pq/marshal (fn [&] [16 false "t"])})
  (unless
    (= (freeze (pq/exec conn "select * from foo where a = $1;" "t3"))
       [{"a" "t3" "b" true}])
    (error "fail"))

  (pq/exec conn "insert into foo(a, b) values($1, $2);" "t2" [16 false "f"])
  (unless
    (= (freeze (pq/exec conn "select * from foo where a = $1;" "t2"))
       [{"a" "t2" "b" false}])
    (error "fail"))))

(run-tests)