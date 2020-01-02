(import _pq)

(defn decode-bool
  [oid s]
  (= s "t"))

(defn decode-number
  [oid s]
  (scan-number s))

(defn decode-s64
  [oid s]
  (int/s64 s))

(defn decode-string
  [oid s]
  s)

(def default-decoder-table
  @{
    16 decode-bool
    17 decode-string
    20 decode-s64
    21 decode-number
    23 decode-number
    25 decode-string
  })

(def connect _pq/connect)

(defn exec
  "Run a query against conn.\n\n
   
   Returned rows are decoded by default-decoder-table or the dynamic table :pq/decoder-table.\n\n
   
   If the result is an error, it is thrown, use error? to check if a thrown error is a pq error,
   which can be inspected with the result/* functions.\n\n

   Params can be nil|boolean|string|buffer|number|u64|s64|array|tuple.\n\n

   If a param is an array or tuple, this must be a triple of 
   [oid:number is-binary:boolean encoded:string|buffer].\n\n
   
   Otherwise params can have the method :pq/marshal returning an encoded triple as described."
  [conn query & params]
  (def result (_pq/exec conn query ;params))
  (def status (_pq/result/status result))
  (_pq/result/unpack result (dyn :pq/decoder-table default-decoder-table)))

(def close _pq/close)

(def error? _pq/error?)

(def result/ntuples _pq/result/ntuples)

(def result/nfields _pq/result/nfields)

(def result/fname _pq/result/fname)

(def result/fnumber _pq/result/fnumber)

(def result/ftype _pq/result/ftype)

(def result/fformat _pq/result/fformat)

(def result/status _pq/result/status)

(def result/error-message _pq/result/error-message)

(def result/error-field _pq/result/error-field)

(def result/unpack _pq/result/unpack)