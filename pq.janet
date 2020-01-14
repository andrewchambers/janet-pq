(import _pq)
(import json)

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
    17 identity
    20 int/s64
    21 scan-number
    23 scan-number
    25 identity
    114 json/decode
    700 scan-number
    701 scan-number
    1042 identity
    1043 identity
    3802 json/decode
  })

(defn json
  [v]
  [114 false (json/encode v)])

(defn jsonb
  [v]
  [3802 false (json/encode v)])

(def connect _pq/connect)

(defn exec
  "Run a query against conn.\n\n
   
   Returned rows are decoded by default-decoder-table or the table *decoders*.\n\n
   
   If the result is an error, it is thrown, use error? to check if a thrown error is a pq error,
   which can be inspected with the result-* functions.\n\n

   Params can be nil|boolean|string|buffer|number|u64|s64|array|tuple.\n\n

   If a param is an array or tuple, this must be a triple of 
   [oid:number is-binary:boolean encoded:string|buffer].\n\n
   
   Otherwise params can have the method :pq/encode returning an encoded triple as described."
  [conn query & params]
  (def result (_pq/exec conn query ;params))
  (when (_pq/error? result)
    (error result))
  (_pq/result-unpack result *decoders*))

(defn one
  "Run a query like exec, returning the first result"
  [conn query & params]
  (def r (exec conn query ;params))
  (if (zero? (length r))
    nil
    (first r)))

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
