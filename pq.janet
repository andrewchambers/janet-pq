(import _pq)

(defn decode-bool
  [oid is-binary s]
  (= s "t"))

(defn decode-string
  [oid is-binary s]
  s)

(def default-decoder-table
  @{
    16 decode-bool
    17 decode-string
    25 decode-string
  })

(def connect _pq/connect)

(defn exec
  [conn query & params]
  (def result (_pq/exec conn query ;params))
  (def status (_pq/result/status result))
  (unless
    (or (= status _pq/PGRES_TUPLES_OK)
        (= status _pq/PGRES_EMPTY_QUERY)
        (= status _pq/PGRES_COMMAND_OK))
    (error result))
  (_pq/result/unpack result (dyn :pq/decoder-table default-decoder-table)))

(def close _pq/close)

