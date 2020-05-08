# pq

## pq/*decoders*
type: table

## pq/CONNECTION_BAD
type: number

## pq/CONNECTION_OK
type: number

## pq/PGRES_BAD_RESPONSE
type: number

## pq/PGRES_COMMAND_OK
type: number

## pq/PGRES_EMPTY_QUERY
type: number

## pq/PGRES_FATAL_ERROR
type: number

## pq/PGRES_TUPLES_OK
type: number

## pq/PG_DIAG_COLUMN_NAME
type: number

## pq/PG_DIAG_CONSTRAINT_NAME
type: number

## pq/PG_DIAG_CONTEXT
type: number

## pq/PG_DIAG_DATATYPE_NAME
type: number

## pq/PG_DIAG_INTERNAL_POSITION
type: number

## pq/PG_DIAG_INTERNAL_QUERY
type: number

## pq/PG_DIAG_MESSAGE_DETAIL
type: number

## pq/PG_DIAG_MESSAGE_HINT
type: number

## pq/PG_DIAG_MESSAGE_PRIMARY
type: number

## pq/PG_DIAG_SCHEMA_NAME
type: number

## pq/PG_DIAG_SEVERITY
type: number

## pq/PG_DIAG_SOURCE_FILE
type: number

## pq/PG_DIAG_SOURCE_FUNCTION
type: number

## pq/PG_DIAG_SOURCE_LINE
type: number

## pq/PG_DIAG_SQLSTATE
type: number

## pq/PG_DIAG_STATEMENT_POSITION
type: number

## pq/PG_DIAG_TABLE_NAME
type: number

## pq/PQTRANS_ACTIVE
type: number

## pq/PQTRANS_IDLE
type: number

## pq/PQTRANS_INERROR
type: number

## pq/PQTRANS_INTRANS
type: number

## pq/_pq/CONNECTION_BAD
type: number

## pq/_pq/CONNECTION_OK
type: number

## pq/_pq/PGRES_BAD_RESPONSE
type: number

## pq/_pq/PGRES_COMMAND_OK
type: number

## pq/_pq/PGRES_EMPTY_QUERY
type: number

## pq/_pq/PGRES_FATAL_ERROR
type: number

## pq/_pq/PGRES_TUPLES_OK
type: number

## pq/_pq/PG_DIAG_COLUMN_NAME
type: number

## pq/_pq/PG_DIAG_CONSTRAINT_NAME
type: number

## pq/_pq/PG_DIAG_CONTEXT
type: number

## pq/_pq/PG_DIAG_DATATYPE_NAME
type: number

## pq/_pq/PG_DIAG_INTERNAL_POSITION
type: number

## pq/_pq/PG_DIAG_INTERNAL_QUERY
type: number

## pq/_pq/PG_DIAG_MESSAGE_DETAIL
type: number

## pq/_pq/PG_DIAG_MESSAGE_HINT
type: number

## pq/_pq/PG_DIAG_MESSAGE_PRIMARY
type: number

## pq/_pq/PG_DIAG_SCHEMA_NAME
type: number

## pq/_pq/PG_DIAG_SEVERITY
type: number

## pq/_pq/PG_DIAG_SOURCE_FILE
type: number

## pq/_pq/PG_DIAG_SOURCE_FUNCTION
type: number

## pq/_pq/PG_DIAG_SOURCE_LINE
type: number

## pq/_pq/PG_DIAG_SQLSTATE
type: number

## pq/_pq/PG_DIAG_STATEMENT_POSITION
type: number

## pq/_pq/PG_DIAG_TABLE_NAME
type: number

## pq/_pq/PQTRANS_ACTIVE
type: number

## pq/_pq/PQTRANS_IDLE
type: number

## pq/_pq/PQTRANS_INERROR
type: number

## pq/_pq/PQTRANS_INTRANS
type: number

## pq/_pq/close
type: cfunction

[pq.c#L461](pq.c#L461)

```
    (pq/close ctx)
    
    Close a pq context.
```

## pq/_pq/connect
type: cfunction

[pq.c#L231](pq.c#L231)

```
    (pq/connect url)
    
    Connect to a postgres server or raise an error.
```

## pq/_pq/error?
type: cfunction

[pq.c#L117](pq.c#L117)

```
    (pq/error? result)
    
    Check if an object is a pq.result containing an error.
```

## pq/_pq/escape-identifier
type: cfunction

[pq.c#L480](pq.c#L480)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/escape-literal
type: cfunction

[pq.c#L468](pq.c#L468)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/exec
type: cfunction

[pq.c#L260](pq.c#L260)

```
    See pq/exec
```

## pq/_pq/result-error-field
type: cfunction

[pq.c#L110](pq.c#L110)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-error-message
type: cfunction

[pq.c#L104](pq.c#L104)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-fformat
type: cfunction

[pq.c#L90](pq.c#L90)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-fname
type: cfunction

[pq.c#L66](pq.c#L66)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-fnumber
type: cfunction

[pq.c#L74](pq.c#L74)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-ftype
type: cfunction

[pq.c#L82](pq.c#L82)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-nfields
type: cfunction

[pq.c#L59](pq.c#L59)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-ntuples
type: cfunction

[pq.c#L52](pq.c#L52)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-status
type: cfunction

[pq.c#L98](pq.c#L98)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/result-unpack
type: cfunction

[pq.c#L126](pq.c#L126)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/status
type: cfunction

[pq.c#L447](pq.c#L447)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/_pq/transaction-status
type: cfunction

[pq.c#L454](pq.c#L454)

```
    See libpq documentation at https://www.postgresql.org.
```

## pq/all
type: function

[pq.janet#L86](pq.janet#L86)

```
    (all conn query & params)
    
    Return all results from a query.
    
    Like exec, with the addition that returned rows are decoded by matching the returned oid
    by to the corresponding decoder function in the table *decoders*.
    
    
```

## pq/close
type: cfunction

[pq.c#L461](pq.c#L461)

## pq/col
type: function

[pq.janet#L107](pq.janet#L107)

```
    (col conn query & params)
    
    Run a query that returns a single column with many rows and return an array with all
    columns unpacked
```

## pq/commit
type: function

[pq.janet#L159](pq.janet#L159)

```
    (commit conn &opt v)
    
    
```

## pq/connect
type: cfunction

[pq.c#L231](pq.c#L231)

## pq/error?
type: cfunction

[pq.c#L117](pq.c#L117)

## pq/escape-identifier
type: cfunction

[pq.c#L480](pq.c#L480)

## pq/escape-literal
type: cfunction

[pq.c#L468](pq.c#L468)

## pq/exec
type: function

[pq.janet#L69](pq.janet#L69)

```
    (exec conn query & params)
    
    Execute a query against conn.
    
    If the result is an error, it is thrown, use error? to check if a thrown error is a pq
    error, which can be inspected with the result-* functions.
    
    Params can be nil|boolean|string|buffer|number|u64|s64|array|tuple.
    
    If a param is an array or tuple, this must be a triple of [oid:number is-binary:boolean
    encoded:string|buffer].
    
    Otherwise params can have the method :pq/encode returning an encoded triple as described.
```

## pq/in-transaction?
type: function

[pq.janet#L153](pq.janet#L153)

```
    (in-transaction? conn)
    
    
```

## pq/json
type: function

[pq.janet#L57](pq.janet#L57)

```
    (json v)
    
    
```

## pq/json/decode
type: cfunction

```
    (json/decode json-source &opt keywords nils)
    
    Returns a janet object after parsing JSON. If keywords is truthy, string keys will be
    converted to keywords. If nils is truthy, null will become nil instead of the keyword
    :null.
```

## pq/json/encode
type: cfunction

```
    (json/encode x &opt tab newline buf)
    
    Encodes a janet value in JSON (utf-8). tab and newline are optional byte sequence which
    are used to format the output JSON. if buf is provided, the formated JSON is append to buf
    instead of a new buffer. Returns the modifed buffer.
```

## pq/jsonb
type: function

[pq.janet#L61](pq.janet#L61)

```
    (jsonb v)
    
    
```

## pq/raw-exec
type: cfunction

[pq.c#L260](pq.c#L260)

## pq/result-error-field
type: cfunction

[pq.c#L110](pq.c#L110)

## pq/result-error-message
type: cfunction

[pq.c#L104](pq.c#L104)

## pq/result-fformat
type: cfunction

[pq.c#L90](pq.c#L90)

## pq/result-fname
type: cfunction

[pq.c#L66](pq.c#L66)

## pq/result-fnumber
type: cfunction

[pq.c#L74](pq.c#L74)

## pq/result-ftype
type: cfunction

[pq.c#L82](pq.c#L82)

## pq/result-nfields
type: cfunction

[pq.c#L59](pq.c#L59)

## pq/result-ntuples
type: cfunction

[pq.c#L52](pq.c#L52)

## pq/result-status
type: cfunction

[pq.c#L98](pq.c#L98)

## pq/result-unpack
type: cfunction

[pq.c#L126](pq.c#L126)

## pq/rollback
type: function

[pq.janet#L155](pq.janet#L155)

```
    (rollback conn &opt v)
    
    
```

## pq/row
type: function

[pq.janet#L99](pq.janet#L99)

```
    (row conn query & params)
    
    Run a query like exec, returning the first result
```

## pq/status
type: cfunction

[pq.c#L447](pq.c#L447)

## pq/transaction-status
type: cfunction

[pq.c#L454](pq.c#L454)

## pq/txn
type: function

[pq.janet#L207](pq.janet#L207)

```
    (txn conn options & body)
    
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
    
```

## pq/txn*
type: function

[pq.janet#L163](pq.janet#L163)

```
    (txn* conn options ftx)
    
    function form of txn
```

## pq/val
type: function

[pq.janet#L113](pq.janet#L113)

```
    (val conn query & params)
    
    Run a query returning a single value and return that value or nil.
```


Docs generated by: https://github.com/andrewchambers/janet-md-doc
