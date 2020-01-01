#include <janet.h>
#include <libpq-fe.h>

typedef struct {
  PGresult *r;
} JPQresult;

static int result_gc(void *p, size_t s) {
  (void)s;
  JPQresult *jpqr = (JPQresult *)p;
  if (jpqr->r) {
    PQclear(jpqr->r);
    jpqr->r = NULL;
  }
  return 0;
}

static const JanetAbstractType pq_result_type = {
    "pq.result", result_gc, NULL, NULL, NULL, NULL, NULL, NULL};

static Janet safe_cstringv(char *s) {
  s ? janet_cstringv(s) : janet_wrap_nil();
}

static Janet jpq_result_ntuples(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);

  return janet_wrap_number(PQntuples(jpqr->r));
}

static Janet jpq_result_nfields(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);

  return janet_wrap_number(PQnfields(jpqr->r));
}

static Janet jpq_result_fname(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  int col = janet_getinteger(argv, 1);

  return safe_cstringv(PQfname(jpqr->r, col));
}

static Janet jpq_result_fnumber(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  const char *s = janet_getcstring(argv, 1);

  return janet_wrap_number(PQfnumber(jpqr->r, s));
}

static Janet jpq_result_ftype(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  int col = janet_getinteger(argv, 1);

  return janet_wrap_number(PQftype(jpqr->r, col));
}

static Janet jpq_result_fformat(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  int col = janet_getinteger(argv, 1);

  return janet_wrap_number(PQfformat(jpqr->r, col));
}

static Janet jpq_result_status(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  return janet_wrap_number(PQresultStatus(jpqr->r));
}

static Janet jpq_result_error_message(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  return safe_cstringv(PQresultErrorMessage(jpqr->r));
}

static Janet jpq_result_error_field(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  int code = janet_getinteger(argv, 1);
  return safe_cstringv(PQresultErrorField(jpqr->r, code));
}

static Janet jpq_result_unpack(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  Janet decode_tab = argv[1];

  int status = PQresultStatus(jpqr->r);

  if (status == PGRES_EMPTY_QUERY || status == PGRES_COMMAND_OK)
    return janet_wrap_nil();

  if (status != PGRES_TUPLES_OK)
    janet_panicv(argv[0]);

  int n = PQntuples(jpqr->r);
  int ncols = PQnfields(jpqr->r);

  JanetArray *a = janet_array(n);

  for (int i = 0; i < n; i++) {
    JanetTable *t = janet_table(ncols);

    for (int j = 0; j < ncols; j++) {
      Janet jv;

      if (PQgetisnull(jpqr->r, i, j)) {
        jv = janet_wrap_nil();
      } else {
        Oid t = PQftype(jpqr->r, i);
        char *v = PQgetvalue(jpqr->r, i, j);
        int l = PQgetlength(jpqr->r, i, j);

        Janet decoder = janet_get(decode_tab, janet_wrap_integer(t));
        if (janet_checktype(decoder, JANET_NIL))
          janet_panicf("no decoder entry for oid '%d'", t);


        switch (janet_type(decoder)) {
        case JANET_FUNCTION: {
          Janet args[2];
          args[0] = janet_wrap_number((double)t);
          args[1] = janet_stringv(v, l);
          JanetFunction *f = janet_unwrap_function(decoder);
          /* XXX should we reenable GC? */
          jv = janet_call(f, 2, args);
          break;
        }
        case JANET_CFUNCTION: {
          Janet args[2];
          args[0] = janet_wrap_number((double)t);
          args[1] = janet_stringv(v, l);
          JanetCFunction f = janet_unwrap_cfunction(decoder);
          jv = f(2, args);
          break;
        }
        default:
          /* XXX we could have an abstract type for more efficient c functions
           */
          janet_panic("decoder entry is not a callable function");
        }
      }

      Janet k = safe_cstringv(PQfname(jpqr->r, j));

      janet_table_put(t, k, jv);
    }

    janet_array_push(a, janet_wrap_table(t));
  }

  return janet_wrap_array(a);
}

typedef struct {
  PGconn *conn;
} Context;

static void context_close(Context *ctx) {
  if (ctx->conn) {
    PQfinish(ctx->conn);
    ctx->conn = NULL;
  }
}

static int context_gc(void *p, size_t s) {
  (void)s;
  Context *ctx = (Context *)p;
  context_close(ctx);
  return 0;
}

static const JanetAbstractType pq_context_type = {
    "pq.context", context_gc, NULL, NULL, NULL, NULL, NULL, NULL};

static Janet jpq_connect(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  const char *u = janet_getcstring(argv, 0);

  Context *ctx = (Context *)janet_abstract(&pq_context_type, sizeof(Context));

  ctx->conn = PQconnectdb(u);
  if (!ctx->conn)
    janet_panic("unable to create connection");

  if (PQstatus(ctx->conn) != CONNECTION_OK) {
    /* GC cleans this up as we assigned to the type. */
    janet_panicf("connection failed: %s", PQerrorMessage(ctx->conn));
  }

  return janet_wrap_abstract(ctx);
}

static void __ensure_ctx_ok(Context *ctx) {
  if (ctx->conn == NULL)
    janet_panic("pq context is disconnected");
}

static void *zsmalloc(size_t n) {
  void *p = janet_smalloc(n);
  memset(p, 0, n);
  return p;
}

static Janet jpq_exec(int32_t argc, Janet *argv) {
  if (argc < 2)
    janet_panic("expected at least a pq context and a query string");
  if (argc > 10000)
    janet_panic("too many arguments");

  Context *ctx = (Context *)janet_getabstract(argv, 0, &pq_context_type);
  __ensure_ctx_ok(ctx);

  JPQresult *jpqr =
      (JPQresult *)janet_abstract(&pq_result_type, sizeof(JPQresult));
  jpqr->r = NULL;

  const char *q = janet_getcstring(argv, 1);

  argc -= 2;
  argv += 2;

  Oid *poids = zsmalloc(sizeof(Oid) * argc);
  int *plengths = zsmalloc(sizeof(int) * argc);
  int *pformats = zsmalloc(sizeof(int) * argc);
  char **pvals = zsmalloc(sizeof(char *) * argc);

  for (int i = 0; i < argc; i++) {
    Janet j = argv[i];
    Oid oid = 0;

  try_again:

    switch (janet_type(j)) {
    case JANET_NIL:
      break;
    case JANET_BUFFER: {
      JanetBuffer *b = janet_unwrap_buffer(j);
      pvals[i] = janet_smalloc(b->count);
      memcpy(pvals[i], b->data, b->count);
      plengths[i] = b->count;
      pformats[i] = 1;
      break;
    }
    case JANET_STRING: {
      const char *s = janet_unwrap_string(j);
      size_t l = janet_string_length(s);
      pvals[i] = janet_smalloc(l);
      memcpy(pvals[i], s, l);
      plengths[i] = l;
      pformats[i] = 1;
      break;
    }
    case JANET_NUMBER: {
      double d = janet_unwrap_number(j);
      size_t l = snprintf(NULL, 0, "%f", d);
      pvals[i] = janet_smalloc(l + 1);
      snprintf(pvals[i], l + 1, "%f", d);
      plengths[i] = l;
      break;
    }
    case JANET_ARRAY:
    case JANET_TUPLE: {

      JanetView view = janet_getindexed(&j, 0);
      if (view.len != 2)
        janet_panic("arrays and tuples must be an [oid, string] pair");

      if (janet_checktype(view.items[0], JANET_NUMBER))
        janet_panic("oid must be a number");

      poids[i] = (Oid)janet_unwrap_number(view.items[0]);

      if (!janet_checktype(view.items[1], JANET_STRING) &&
          !janet_checktype(view.items[1], JANET_BUFFER))
        janet_panic("value in oid pair must be a string or buffer");

      j = view.items[1];

      goto try_again;
    }
    case JANET_ABSTRACT: {
      JanetIntType intt = janet_is_int(j);
      if (intt == JANET_INT_S64) {
        int64_t v = janet_unwrap_s64(j);
        size_t l = snprintf(NULL, 0, "%ld", v);
        pvals[i] = janet_smalloc(l + 1);
        snprintf(pvals[i], l + 1, "%ld", v);
        plengths[i] = l;
        break;
      } else if (intt == JANET_INT_U64) {
        uint64_t v = janet_unwrap_u64(j);
        size_t l = snprintf(NULL, 0, "%lu", v);
        pvals[i] = janet_smalloc(l + 1);
        snprintf(pvals[i], l + 1, "%lu", v);
        plengths[i] = l;
        break;
      } else {
        /* fall through to default */
      }
    }
    default:
      /* XXX: renable janet GC for this call? how do we do that? what do we
         need to root these values? */
      j = janet_mcall("pq/to-string", 1, &j);
      if (!janet_checktype(j, JANET_STRING) &&
          !janet_checktype(j, JANET_BUFFER))
        janet_panic("method :pq/to-string did not return a string or buffer");

      Janet joid = janet_mcall("pq/type-oid", 1, &j);
      if (!janet_checktype(j, JANET_NUMBER))
        janet_panic("method :pq/type-oid did not return a number");

      poids[i] = (Oid)janet_unwrap_number(joid);

      goto try_again;
    }
  }

  jpqr->r = PQexecParams(ctx->conn, q, argc, NULL, (const char *const *)pvals,
                         plengths, NULL, 0);

  if (!jpqr->r)
    janet_panic("query failed");

  /* Free in reverse order for sfree's sake */
  for (int i = argc - 1; i > 0; i--) {
    janet_sfree(pvals[i]);
  }
  janet_sfree(pvals);
  janet_sfree(pformats);
  janet_sfree(plengths);
  janet_sfree(poids);

  return janet_wrap_abstract(jpqr);
}

static Janet jpq_close(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  Context *ctx = (Context *)janet_getabstract(argv, 0, &pq_context_type);
  context_close(ctx);
  return janet_wrap_nil();
}

static const JanetReg cfuns[] = {
    {"connect", jpq_connect,
     "(pq/connect url)\n\n"
     "Connect to a postgres server or raise an error."},
    {"exec", jpq_exec,
     "(pq/exec ctx query params)\n\n"
     "Execute a query, returning a pq.result value."
     "Params can be nil|string|buffer|number|u64|s64."
     "If a param is an array or tuple, this must be a set of [TypeOID "
     "String|Buffer]."
     "Otherwise params must handle the methods :pq/to-string and :pq/to-oid."},
    {"close", jpq_close,
     "(pq/close ctx)\n\n"
     "Close a postgres context."},
    {"result/ntuples", jpq_result_ntuples, NULL},
    {"result/nfields", jpq_result_nfields, NULL},
    {"result/fname", jpq_result_fname, NULL},
    {"result/fnumber", jpq_result_fnumber, NULL},
    {"result/ntuples", jpq_result_ftype, NULL},
    {"result/ftype", jpq_result_fformat, NULL},
    {"result/fformat", jpq_result_status, NULL},
    {"result/error-message", jpq_result_error_message, NULL},
    {"result/error-field", jpq_result_error_field, NULL},
    {"result/unpack", jpq_result_error_field, NULL},

    {NULL, NULL, NULL}};

JANET_MODULE_ENTRY(JanetTable *env) {

  janet_cfuns(env, "pq", cfuns);

#define DEF_CONSTANT_INT(X) janet_def(env, #X, janet_wrap_integer(X), NULL)

  /* PQresultStatus */
  DEF_CONSTANT_INT(PGRES_EMPTY_QUERY);
  DEF_CONSTANT_INT(PGRES_COMMAND_OK);
  DEF_CONSTANT_INT(PGRES_TUPLES_OK);
  DEF_CONSTANT_INT(PGRES_BAD_RESPONSE);
  DEF_CONSTANT_INT(PGRES_FATAL_ERROR);
  /* PQresultErrorField */
  DEF_CONSTANT_INT(PG_DIAG_SEVERITY);
  DEF_CONSTANT_INT(PG_DIAG_SQLSTATE);
  DEF_CONSTANT_INT(PG_DIAG_MESSAGE_PRIMARY);
  DEF_CONSTANT_INT(PG_DIAG_MESSAGE_DETAIL);
  DEF_CONSTANT_INT(PG_DIAG_MESSAGE_HINT);
  DEF_CONSTANT_INT(PG_DIAG_STATEMENT_POSITION);
  DEF_CONSTANT_INT(PG_DIAG_INTERNAL_POSITION);
  DEF_CONSTANT_INT(PG_DIAG_INTERNAL_QUERY);
  DEF_CONSTANT_INT(PG_DIAG_CONTEXT);
  DEF_CONSTANT_INT(PG_DIAG_SCHEMA_NAME);
  DEF_CONSTANT_INT(PG_DIAG_TABLE_NAME);
  DEF_CONSTANT_INT(PG_DIAG_COLUMN_NAME);
  DEF_CONSTANT_INT(PG_DIAG_DATATYPE_NAME);
  DEF_CONSTANT_INT(PG_DIAG_CONSTRAINT_NAME);
  DEF_CONSTANT_INT(PG_DIAG_SOURCE_FILE);
  DEF_CONSTANT_INT(PG_DIAG_SOURCE_LINE);
  DEF_CONSTANT_INT(PG_DIAG_SOURCE_FUNCTION);

#undef DEF_CONSTANT_INT
}
