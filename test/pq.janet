(import sh)
(import process)
(import build/pq :as pq)

(defmacro with-unconditional
  [action body]
  (def b (tuple 'do body (tuple action)))
  (with-syms [err fiber]
    ~(try
       ,b
     ([,err ,fiber] (,action) (,propagate ,err ,fiber)))))

(defn cp
  [from to]
  (sh/$ ["cp" from to]))

(defn cleanup []
  (each d (->>
            (sh/$$ ["find" "/tmp" "-maxdepth" "1" "-type" "d" "-name" "janet-pq-test.tmp.*"])
            (string/split "\n")
            (map string/trim)
            (filter (comp not empty?)))
    (print "cleaning up " d " ...")
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
  (pq/ping conn)))

(run-tests)