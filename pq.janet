(import _pq)
(import json)

#PQStatus
(def CONNECTION_OK _pq/CONNECTION_OK)
(def CONNECTION_BAD _pq/CONNECTION_BAD)
#PQtransactionStatus
(def PQTRANS_IDLE _pq/PQTRANS_IDLE)
(def PQTRANS_ACTIVE _pq/PQTRANS_ACTIVE)
(def PQTRANS_INERROR _pq/PQTRANS_INERROR)
(def PQTRANS_INTRANS _pq/PQTRANS_INTRANS)
# PQresultStatus
(def PGRES_EMPTY_QUERY _pq/PGRES_EMPTY_QUERY)
(def PGRES_COMMAND_OK _pq/PGRES_COMMAND_OK)
(def PGRES_TUPLES_OK _pq/PGRES_TUPLES_OK)
(def PGRES_BAD_RESPONSE _pq/PGRES_BAD_RESPONSE)
(def PGRES_FATAL_ERROR _pq/PGRES_FATAL_ERROR)
# PQresultErrorField
(def PG_DIAG_SEVERITY _pq/PG_DIAG_SEVERITY)
(def PG_DIAG_SQLSTATE _pq/PG_DIAG_SQLSTATE)
(def PG_DIAG_MESSAGE_PRIMARY _pq/PG_DIAG_MESSAGE_PRIMARY)
(def PG_DIAG_MESSAGE_DETAIL _pq/PG_DIAG_MESSAGE_DETAIL)
(def PG_DIAG_MESSAGE_HINT _pq/PG_DIAG_MESSAGE_HINT)
(def PG_DIAG_STATEMENT_POSITION _pq/PG_DIAG_STATEMENT_POSITION)
(def PG_DIAG_INTERNAL_POSITION _pq/PG_DIAG_INTERNAL_POSITION)
(def PG_DIAG_INTERNAL_QUERY _pq/PG_DIAG_INTERNAL_QUERY)
(def PG_DIAG_CONTEXT _pq/PG_DIAG_CONTEXT)
(def PG_DIAG_SCHEMA_NAME _pq/PG_DIAG_SCHEMA_NAME)
(def PG_DIAG_TABLE_NAME _pq/PG_DIAG_TABLE_NAME)
(def PG_DIAG_COLUMN_NAME _pq/PG_DIAG_COLUMN_NAME)
(def PG_DIAG_DATATYPE_NAME _pq/PG_DIAG_DATATYPE_NAME)
(def PG_DIAG_CONSTRAINT_NAME _pq/PG_DIAG_CONSTRAINT_NAME)
(def PG_DIAG_SOURCE_FILE _pq/PG_DIAG_SOURCE_FILE)
(def PG_DIAG_SOURCE_LINE _pq/PG_DIAG_SOURCE_LINE)
(def PG_DIAG_SOURCE_FUNCTION _pq/PG_DIAG_SOURCE_FUNCTION)

(def *decoders*
  @{
    16 |(= $0 "t")
    17 _pq/decode-bytea
    18 identity
    19 keyword
    20 int/s64
    21 scan-number
    23 scan-number
    25 identity
    26 scan-number
    114 json/decode
    700 scan-number
    701 scan-number
    1042 identity
    1043 identity
    1700 scan-number
    3802 json/decode
  })

(defn json
  [v]
  [114 false (json/encode v)])

(defn jsonb
  [v]
  [3802 false (json/encode v)])

(def connect _pq/connect)

(def raw-exec _pq/exec)

(defn exec
  "Execute a query against conn.\n\n

   If the result is an error, it is thrown, use error? to check if a thrown error is a pq error, which can be inspected with the result-* functions.\n\n

   Params can be nil|boolean|string|buffer|number|u64|s64|array|tuple.\n\n

   If a param is an array or tuple, this must be a triple of
   [oid:number is-binary:boolean encoded:string|buffer].\n\n

   Otherwise params can have the method :pq/encode returning an encoded triple as described."
  [conn query & params]
  (def result (_pq/exec conn query ;params))
  (when (_pq/error? result)
    (error result))
  result)

(defn all
  "Return all results from a query.\n\n

   Like exec, with the addition that returned rows are
   decoded by matching the returned oid by to the
   corresponding decoder function in the table *decoders*.\n\n
  "
  [conn query & params]
  (def result (_pq/exec conn query ;params))
  (when (_pq/error? result)
    (error result))
  (_pq/result-unpack result *decoders*))

(defn row
  "Run a query like exec, returning the first result"
  [conn query & params]
  (if-let [rows (all conn query ;params)]
    (if (empty? rows)
      nil
      (first rows))))

(defn col
  "Run a query that returns a single column with many rows
   and return an array with all columns unpacked"
  [conn query & params]
  (map |(first (values $)) (all conn query ;params)))

(defn val
  "Run a query returning a single value and return that value or nil."
  [conn query & params]
  (if-let [r (row conn query ;params)
           v (values r)]
    (when (not (empty? v))
      (first v))))

(def status _pq/status)

(def transaction-status _pq/transaction-status)

(def close _pq/close)

(def escape-literal _pq/escape-literal)

(def escape-identifier _pq/escape-identifier)

(def error? _pq/error?)

(def result-ntuples _pq/result-ntuples)

(def result-nfields _pq/result-nfields)

(def result-fname _pq/result-fname)

(def result-fnumber _pq/result-fnumber)

(def result-ftype _pq/result-ftype)

(def result-fformat _pq/result-fformat)

(def result-status _pq/result-status)

(def result-error-message _pq/result-error-message)

(def result-error-field _pq/result-error-field)

(def result-unpack _pq/result-unpack)

(defn in-transaction? [conn] (= (transaction-status conn) PQTRANS_INTRANS))

(defn rollback
  [conn &opt v]
  (signal 0 [conn [:rollback v]]))

(defn commit
  [conn &opt v]
  (signal 0 [conn [:commit v]]))

(defn txn*
  "function form of txn"
  [conn options ftx]
  (def retry (get options :retry false))
  (def mode (get options :mode ""))
  (try
    (do
      (exec conn (string "begin " mode ";"))
      (def fb (fiber/new ftx :i0123))
      (def v (resume fb))
      (match [v (fiber/status fb)]
        # XXX https://github.com/janet-lang/janet/issues/297
        # matching nil explicitly here until this issue is resolved.
        [nil :dead]
          (do
            (exec conn "commit;")
            v)
        [v :dead]
          (do
            (exec conn "commit;")
            v)
        ([[c [action v]] :user0] (= c conn))
          (do
            (case action
              :commit
                (exec conn "commit;")
              :rollback
                (exec conn "rollback;")
              (error "misuse of txn*"))
            v)
        (do
          (exec conn "rollback;")
          (propagate v fb))))
  ([err f]
    (if (and
          retry
          (error? err)
          (= (result-error-field err PG_DIAG_SQLSTATE) "40001"))
      (txn* conn options ftx)
      (do
        (unless (= (status conn) CONNECTION_BAD)
          (exec conn "rollback;"))
        (propagate err f))))))

(defmacro txn
  `
    Run body in an sql transaction with options.

    Valid option table entries are:

    :retry (default false)
    :mode (default "") Query fragment used to begin the transaction.

    The transaction is rolled back when:

    - An error is raised.
    - If pq/rollback is called.

    The transaction is retried if

    - The an error is raised and the error is a serializability error (\"40001\").

    Returns the last form or the value of any inner calls to rollback.

    Examples:

      (pq/txn conn {:mode "ISOLATION LEVEL SERIALIZABLE" :retry true} ...)
      (pq/txn conn (pq/rollback :foo))
  `
  [conn options & body]
  ~(,txn* ,conn ,options (fn [] ,(tuple 'do ;body))))
