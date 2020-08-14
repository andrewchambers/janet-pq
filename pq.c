#include <alloca.h>
#include <inttypes.h>
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

static void result_to_string(void *p, JanetBuffer *buffer) {
  JPQresult *jpqr = (JPQresult *)p;

  janet_buffer_push_cstring(buffer, "PGResult: ");

  if (!jpqr->r) {
    janet_buffer_push_cstring(buffer, "uninit");
    return;
  }

  int status = PQresultStatus(jpqr->r);
  if (status != PGRES_TUPLES_OK && status != PGRES_EMPTY_QUERY &&
      status != PGRES_COMMAND_OK) {
    char *msg = PQresultErrorMessage(jpqr->r);
    janet_buffer_push_cstring(buffer, msg ? msg : "error");
  } else {
    janet_buffer_push_cstring(buffer, "...");
  }
}

static const JanetAbstractType pq_result_type = {
    "pq/result", result_gc,        NULL, NULL, NULL, NULL,
    NULL,        result_to_string, NULL};

static Janet safe_cstringv(char *s) {
  return s ? janet_cstringv(s) : janet_wrap_nil();
}

static Janet safe_ckeywordv(char *s) {
  return s ? janet_ckeywordv(s) : janet_wrap_nil();
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

static Janet jpq_error_pred(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  if (!janet_checkabstract(argv[0], &pq_result_type))
    return janet_wrap_boolean(0);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  int status = PQresultStatus(jpqr->r);
  return janet_wrap_boolean(status != PGRES_TUPLES_OK &&
                            status != PGRES_EMPTY_QUERY &&
                            status != PGRES_COMMAND_OK);
}

static Janet jpq_result_unpack(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  JPQresult *jpqr = (JPQresult *)janet_getabstract(argv, 0, &pq_result_type);
  JanetTable *decode_tab = janet_gettable(argv, 1);

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
        Oid t = PQftype(jpqr->r, j);
        int format = PQfformat(jpqr->r, j);
        if (format != 0)
          janet_panic("BUG: currently only text decoding is supported");
        char *v = PQgetvalue(jpqr->r, i, j);
        int l = PQgetlength(jpqr->r, i, j);

        Janet decoder = janet_table_get(decode_tab, janet_wrap_integer(t));
        if (janet_checktype(decoder, JANET_NIL))
          janet_panicf("no decoder entry for returned row item with oid - %d",
                       t);

        switch (janet_type(decoder)) {
        case JANET_FUNCTION: {
          Janet args[1];
          args[0] = janet_stringv((uint8_t *)v, l);
          JanetFunction *f = janet_unwrap_function(decoder);
          /* XXX should we reenable GC? */
          jv = janet_call(f, 1, args);
          break;
        }
        case JANET_CFUNCTION: {
          Janet args[1];
          args[0] = janet_stringv((uint8_t *)v, l);
          JanetCFunction f = janet_unwrap_cfunction(decoder);
          jv = f(1, args);
          break;
        }
        default:
          /* XXX we could have an abstract type for more efficient c functions
           */
          janet_panic("decoder entry is not a callable function");
        }
      }

      Janet k = safe_ckeywordv(PQfname(jpqr->r, j));
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

static Janet jpq_close(int32_t argc, Janet *argv);

static JanetMethod context_methods[] = {
    {"close", jpq_close}, /* So contexts can be used with 'with' */
    {NULL, NULL}};

static int context_get(void *ptr, Janet key, Janet *out) {
  Context *p = (Context *)ptr;
  return janet_getmethod(janet_unwrap_keyword(key), context_methods, out);
}

static const JanetAbstractType pq_context_type = {
    "pq/context", context_gc, NULL, context_get, NULL,
    NULL,         NULL,       NULL, NULL,        NULL};

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
  if (argc > 10000000)
    janet_panic("too many arguments");

  Context *ctx = (Context *)janet_getabstract(argv, 0, &pq_context_type);
  __ensure_ctx_ok(ctx);

  JPQresult *jpqr =
      (JPQresult *)janet_abstract(&pq_result_type, sizeof(JPQresult));
  jpqr->r = NULL;

  const char *q = janet_getcstring(argv, 1);

  argc -= 2;
  argv += 2;

  Oid *poids;
  int *plengths;
  int *pformats;
  char **pvals;

#define NFAST_PATH 4
  if (argc <= NFAST_PATH) {
    int n;
#define ZALLOCA(P, N)                                                          \
  do {                                                                         \
    n = N;                                                                     \
    P = alloca(n);                                                             \
    memset(P, 0, n);                                                           \
  } while (0)
    ZALLOCA(poids, sizeof(Oid) * argc);
    ZALLOCA(plengths, sizeof(int) * argc);
    ZALLOCA(pformats, sizeof(int) * argc);
    ZALLOCA(pvals, sizeof(char *) * argc);
#undef ZALLOCA
  } else {
    poids = zsmalloc(sizeof(Oid) * argc);
    plengths = zsmalloc(sizeof(int) * argc);
    pformats = zsmalloc(sizeof(int) * argc);
    pvals = zsmalloc(sizeof(char *) * argc);
  }

  for (int i = 0; i < argc; i++) {
    Janet j = argv[i];
    Oid oid = 0;

  try_again:

    switch (janet_type(j)) {
    case JANET_NIL:
      break;
    case JANET_BOOLEAN:
      plengths[i] = 2;
      pvals[i] = janet_smalloc(2);
      pvals[i][0] = janet_unwrap_boolean(j) ? 't' : 'f';
      pvals[i][1] = 0;
      break;
    case JANET_BUFFER: {
      JanetBuffer *b = janet_unwrap_buffer(j);
      pvals[i] = janet_smalloc(b->count);
      memcpy(pvals[i], b->data, b->count);
      plengths[i] = b->count;
      pformats[i] = 1;
      break;
    }
    case JANET_KEYWORD:
    case JANET_STRING: {
      const char *s = (char *)janet_unwrap_string(j);
      size_t l = janet_string_length(s);
      pvals[i] = janet_smalloc(l);
      memcpy(pvals[i], s, l);
      plengths[i] = l;
      pformats[i] = 1;
      break;
    }
    case JANET_NUMBER: {
      double d = janet_unwrap_number(j);
      /* The exact range could be increased to ~52 bits afaik, for now 32 bits
       * only. */
      const char *fmt = (d == floor(d) && d <= ((double)INT32_MAX) &&
                         d >= ((double)INT32_MIN))
                            ? "%.0f"
                            : "%g";
      size_t l = snprintf(NULL, 0, fmt, d);
      pvals[i] = janet_smalloc(l + 1);
      snprintf(pvals[i], l + 1, fmt, d);
      plengths[i] = l;
      break;
    }
    case JANET_ARRAY:
    case JANET_TUPLE:
    raw_encode : {
      JanetView view = janet_getindexed(&j, 0);
      if (view.len != 3)
        janet_panic(
            "arrays and tuples must be an [oid is-binary string] triple");

      if (!janet_checktype(view.items[0], JANET_NUMBER))
        janet_panic("oid must be a number");

      poids[i] = (Oid)janet_unwrap_number(view.items[0]);

      if (!janet_checktype(view.items[1], JANET_BOOLEAN))
        janet_panic("is-binary must be a boolean");

      pformats[i] = (Oid)janet_unwrap_boolean(view.items[1]);

      if (!janet_checktype(view.items[2], JANET_STRING) &&
          !janet_checktype(view.items[2], JANET_BUFFER))
        janet_panic("value in oid pair must be a string or buffer");

      JanetByteView bytes = janet_getbytes(&view.items[2], 0);

      pvals[i] = janet_smalloc(bytes.len + 1);
      memcpy(pvals[i], bytes.bytes, bytes.len);
      pvals[i][bytes.len] = 0;
      plengths[i] = bytes.len;
      break;
    }
    case JANET_ABSTRACT: {
      JanetIntType intt = janet_is_int(j);
      if (intt == JANET_INT_S64) {
        int64_t v = janet_unwrap_s64(j);
        size_t l = snprintf(NULL, 0, "%" PRId64, v);
        pvals[i] = janet_smalloc(l + 1);
        snprintf(pvals[i], l + 1, "%" PRId64, v);
        plengths[i] = l;
        break;
      } else if (intt == JANET_INT_U64) {
        uint64_t v = janet_unwrap_u64(j);
        size_t l = snprintf(NULL, 0, "%" PRIu64, v);
        pvals[i] = janet_smalloc(l + 1);
        snprintf(pvals[i], l + 1, "%" PRIu64, v);
        plengths[i] = l;
        break;
      } else {
        /* fall through to default */
      }
    }
    default: {
      /* XXX: renable janet GC for this call? how do we do that? what do we
         need to root these values? */
      j = janet_mcall("pq/encode", 1, &j);
      if (!janet_checktype(j, JANET_ARRAY) && !janet_checktype(j, JANET_TUPLE))
        janet_panic("method :pq/encode did not return an array or tuple");
      goto raw_encode;
    }
    }
  }

  jpqr->r = PQexecParams(ctx->conn, q, argc, NULL, (const char *const *)pvals,
                         plengths, pformats, 0);

  if (!jpqr->r)
    janet_panic("query failed");

  /* Free in reverse order for sfree's sake */
  for (int i = argc - 1; i > 0; i--) {
    janet_sfree(pvals[i]);
  }
  if (argc > NFAST_PATH) {
    janet_sfree(pvals);
    janet_sfree(pformats);
    janet_sfree(plengths);
    janet_sfree(poids);
  }
#undef NFAST_PATH

  if (PQresultStatus(jpqr->r) == PGRES_TUPLES_OK) {
    size_t pressure = 0;
    int n = PQntuples(jpqr->r);
    int ncols = PQnfields(jpqr->r);
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < ncols; j++) {
        if (!PQgetisnull(jpqr->r, i, j)) {
          pressure += PQgetlength(jpqr->r, i, j);
        }
      }
    }
    janet_gcpressure(pressure);
  }

  return janet_wrap_abstract(jpqr);
}

static Janet jpq_status(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  Context *ctx = (Context *)janet_getabstract(argv, 0, &pq_context_type);
  __ensure_ctx_ok(ctx);
  return janet_wrap_integer(PQstatus(ctx->conn));
}

static Janet jpq_transaction_status(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  Context *ctx = (Context *)janet_getabstract(argv, 0, &pq_context_type);
  __ensure_ctx_ok(ctx);
  return janet_wrap_integer(PQtransactionStatus(ctx->conn));
}

static Janet jpq_close(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  Context *ctx = (Context *)janet_getabstract(argv, 0, &pq_context_type);
  context_close(ctx);
  return janet_wrap_nil();
}

static Janet jpq_escape_literal(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  Context *ctx = (Context *)janet_getabstract(argv, 0, &pq_context_type);
  char *input = (char *)janet_getstring(argv, 1);
  char *output = PQescapeLiteral(ctx->conn, input, janet_string_length(input));
  if (!output)
    janet_panicv(safe_cstringv(PQerrorMessage(ctx->conn)));
  const uint8_t *result = janet_string((uint8_t *)output, strlen(output));
  PQfreemem(output);
  return janet_wrap_string(result);
}

static Janet jpq_escape_identifier(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  Context *ctx = (Context *)janet_getabstract(argv, 0, &pq_context_type);
  char *input = (char *)janet_getstring(argv, 1);
  char *output =
      PQescapeIdentifier(ctx->conn, input, janet_string_length(input));
  if (!output)
    janet_panicv(safe_cstringv(PQerrorMessage(ctx->conn)));
  const uint8_t *result = janet_string((uint8_t *)output, strlen(output));
  PQfreemem(output);
  return janet_wrap_string(result);
}

static int decode_nibble(uint8_t b) {
  if (b >= '0' && b <= '9')
    return b - '0';
  if (b >= 'a' && b <= 'f')
    return 10 + b - 'a';
  if (b >= 'A' && b <= 'F')
    return 10 + b - 'A';
  return 0;
}

static Janet jpq_decode_bytea(int32_t argc, Janet *argv) {
  const uint8_t *in;
  uint8_t *out;

  janet_fixarity(argc, 1);
  JanetByteView b = janet_getbytes(argv, 0);
  size_t nbytes = (size_t)b.len;

  if (nbytes < 2 || b.bytes[0] != '\\' || b.bytes[1] != 'x')
    janet_panic("bytea encoded data should begin with '\\x'");

  nbytes -= 2;
  in = b.bytes + 2;
  out = janet_smalloc(nbytes / 2);

  size_t nout = 0;
  for (size_t i = 0; i < nbytes; i += 2) {
    uint8_t c1 = decode_nibble(in[i]);
    uint8_t c2 = decode_nibble((i + 1 < nbytes) ? in[i + 1] : 0);
    out[nout++] = (c1 << 4) | c2;
  }

  Janet s = janet_stringv(out, nout);
  janet_sfree(out);
  return s;
}

#define upstream_doc "See libpq documentation at https://www.postgresql.org."

static const JanetReg cfuns[] = {
    {"connect", jpq_connect,
     "(pq/connect url)\n\n"
     "Connect to a postgres server or raise an error."},
    {"exec", jpq_exec, "See pq/exec"},
    {"close", jpq_close,
     "(pq/close ctx)\n\n"
     "Close a pq context."},
    {"error?", jpq_error_pred,
     "(pq/error? result)\n\n"
     "Check if an object is a pq.result containing an error."},
    {"transaction-status", jpq_transaction_status, upstream_doc},
    {"status", jpq_status, upstream_doc},
    {"escape-literal", jpq_escape_literal, upstream_doc},
    {"escape-identifier", jpq_escape_identifier, upstream_doc},
    {"result-ntuples", jpq_result_ntuples, upstream_doc},
    {"result-nfields", jpq_result_nfields, upstream_doc},
    {"result-fname", jpq_result_fname, upstream_doc},
    {"result-fnumber", jpq_result_fnumber, upstream_doc},
    {"result-ftype", jpq_result_ftype, upstream_doc},
    {"result-fformat", jpq_result_fformat, upstream_doc},
    {"result-status", jpq_result_status, upstream_doc},
    {"result-error-message", jpq_result_error_message, upstream_doc},
    {"result-error-field", jpq_result_error_field, upstream_doc},
    {"result-unpack", jpq_result_unpack, upstream_doc},
    {"decode-bytea", jpq_decode_bytea, "(pq/decode-bytea buf)"},
    {NULL, NULL, NULL}};

JANET_MODULE_ENTRY(JanetTable *env) {

  janet_cfuns(env, "pq", cfuns);
  janet_register_abstract_type(&pq_context_type);
  janet_register_abstract_type(&pq_result_type);

#define DEF_CONSTANT_INT(X) janet_def(env, #X, janet_wrap_integer(X), NULL)

  /* PQStatus */
  DEF_CONSTANT_INT(CONNECTION_OK);
  DEF_CONSTANT_INT(CONNECTION_BAD);
  /* PQransactionStatus */
  DEF_CONSTANT_INT(PQTRANS_IDLE);
  DEF_CONSTANT_INT(PQTRANS_ACTIVE);
  DEF_CONSTANT_INT(PQTRANS_INERROR);
  DEF_CONSTANT_INT(PQTRANS_INTRANS);
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
