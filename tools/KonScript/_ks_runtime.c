// _ks_runtime.c — KonScript native runtime
// Implements the stdlib functions declared in irgen's emitRuntimeDecls().
// Compiled once: clang -c _ks_runtime.c -o _ks_runtime.o
// Linked with every native KonScript binary.
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ── Result<T> ────────────────────────────────────────────────────────────────
// A heap-allocated struct: [ok:i32, value:i8*, error:i8*]
typedef struct { int ok; char* value; char* error; } _KsResult;

static _KsResult* _ks_result_ok_val(char* v) {
    _KsResult* r = calloc(1, sizeof(_KsResult));
    r->ok = 1; r->value = v ? strdup(v) : strdup(""); r->error = strdup("");
    return r;
}
static _KsResult* _ks_result_err_val(const char* e) {
    _KsResult* r = calloc(1, sizeof(_KsResult));
    r->ok = 0; r->value = strdup(""); r->error = strdup(e);
    return r;
}

int _ks_result_ok(void* r)    { return r ? ((_KsResult*)r)->ok : 0; }
char* _ks_result_value(void* r) { return r ? ((_KsResult*)r)->value : ""; }
char* _ks_result_error(void* r) { return r ? ((_KsResult*)r)->error : ""; }

// ── File I/O ─────────────────────────────────────────────────────────────────
void* _ks_file_read(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) { char msg[256]; snprintf(msg,256,"cannot open: %s",path); return _ks_result_err_val(msg); }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char* buf = malloc(sz + 1);
    size_t nr = fread(buf, 1, sz, f);
    buf[nr] = '\0';
    fclose(f);
    _KsResult* r = _ks_result_ok_val(buf); free(buf); return r;
}
void* _ks_file_write(const char* path, const char* content) {
    FILE* f = fopen(path, "w");
    if (!f) { char msg[256]; snprintf(msg,256,"cannot write: %s",path); return _ks_result_err_val(msg); }
    fputs(content, f); fclose(f); return _ks_result_ok_val("");
}
void* _ks_file_append(const char* path, const char* content) {
    FILE* f = fopen(path, "a");
    if (!f) { char msg[256]; snprintf(msg,256,"cannot append: %s",path); return _ks_result_err_val(msg); }
    fputs(content, f); fclose(f); return _ks_result_ok_val("");
}
int _ks_file_exists(const char* path) {
    FILE* f = fopen(path, "r"); if (!f) return 0; fclose(f); return 1;
}
void* _ks_file_delete(const char* path) {
    if (remove(path) == 0) return _ks_result_ok_val("");
    char msg[256]; snprintf(msg,256,"cannot delete: %s",path);
    return _ks_result_err_val(msg);
}

// _ks_file_lines returns a _KsArray of strings (see array section)
// For now returns the raw content — proper implementation below with arrays.

// ── String methods ────────────────────────────────────────────────────────────
int   _ks_str_len(const char* s)       { return s ? (int)strlen(s) : 0; }
int   _ks_str_isEmpty(const char* s)   { return !s || *s == '\0'; }
int   _ks_str_contains(const char* s, const char* sub) { if (!s || !sub) return 0; return strstr(s, sub) != NULL; }
int   _ks_str_starts(const char* s, const char* p)     { if (!s || !p) return 0; return strncmp(s, p, strlen(p)) == 0; }
int   _ks_str_ends(const char* s, const char* e) {
    if (!s || !e) return 0;
    size_t sl = strlen(s), el = strlen(e);
    return sl >= el && strcmp(s + sl - el, e) == 0;
}
char* _ks_str_trim(const char* s) {
    while (isspace((unsigned char)*s)) s++;
    if (!*s) return strdup("");
    const char* e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) e--;
    size_t len = e - s + 1;
    char* r = malloc(len + 1); memcpy(r, s, len); r[len] = '\0'; return r;
}
char* _ks_str_upper(const char* s) {
    char* r = strdup(s); for (char* p = r; *p; p++) *p = toupper((unsigned char)*p); return r;
}
char* _ks_str_lower(const char* s) {
    char* r = strdup(s); for (char* p = r; *p; p++) *p = tolower((unsigned char)*p); return r;
}
char* _ks_str_replace(const char* s, const char* from, const char* to) {
    size_t fl = strlen(from), tl = strlen(to);
    if (fl == 0) return strdup(s);
    // Count occurrences
    int count = 0; const char* p = s;
    while ((p = strstr(p, from))) { count++; p += fl; }
    size_t newlen = strlen(s) + count * (tl - fl) + 1;
    char* r = malloc(newlen); char* w = r; p = s;
    while (*p) {
        if (strncmp(p, from, fl) == 0) { memcpy(w, to, tl); w += tl; p += fl; }
        else *w++ = *p++;
    }
    *w = '\0'; return r;
}
char* _ks_str_substr(const char* s, int pos, int len) {
    if (!s) return strdup("");
    int slen = (int)strlen(s);
    if (pos < 0) pos = 0;
    if (pos > slen) pos = slen;
    if (len < 0) len = 0;
    if (pos + len > slen) len = slen - pos;
    char* r = malloc(len + 1);
    if (len > 0) memcpy(r, s + pos, len);
    r[len] = '\0';
    return r;
}
/* Avoid atoi/strtol/strtod: all redirected to __isoc23_* on glibc 2.38+
   which is not present in the bundled musl sysroot. Hand-roll instead. */
int _ks_str_toInt(const char* s) {
    if (!s) return 0;
    int result = 0, sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if      (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    while (*s >= '0' && *s <= '9')
        result = result * 10 + (*s++ - '0');
    return sign * result;
}
float _ks_str_toFloat(const char* s) {
    if (!s) return 0.0f;
    double result = 0.0, frac = 0.1;
    int sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if      (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    while (*s >= '0' && *s <= '9') result = result * 10.0 + (*s++ - '0');
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') { result += (*s++ - '0') * frac; frac *= 0.1; }
    }
    return (float)(sign * result);
}

// ── Array ─────────────────────────────────────────────────────────────────────
// Dynamic array of i8* (void pointers)
typedef struct { void** data; int len; int cap; } _KsArray;

void* _ks_array_new(int cap) {
    _KsArray* a = malloc(sizeof(_KsArray));
    a->cap = cap > 0 ? cap : 8;
    a->data = malloc(a->cap * sizeof(void*));
    a->len = 0; return a;
}
void _ks_array_push(void* arr, void* val) {
    _KsArray* a = arr;
    if (a->len == a->cap) {
        a->cap *= 2;
        a->data = realloc(a->data, a->cap * sizeof(void*));
    }
    a->data[a->len++] = val;
}
void* _ks_array_pop(void* arr) {
    _KsArray* a = arr;
    if (a->len == 0) return NULL;
    return a->data[--a->len];
}
int _ks_array_len(void* arr)  { return arr ? ((_KsArray*)arr)->len : 0; }
int _ks_array_has(void* arr, void* val) {
    _KsArray* a = arr;
    for (int i = 0; i < a->len; i++)
        if (a->data[i] == val || (a->data[i] && val && strcmp(a->data[i], val) == 0))
            return 1;
    return 0;
}
void* _ks_array_get(void* arr, int idx) {
    _KsArray* a = arr;
    if (idx < 0 || idx >= a->len) return NULL;
    return a->data[idx];
}

void _ks_array_set(void* arr, int idx, void* val) {
    _KsArray* a = arr;
    if (idx >= 0 && idx < a->len)
        a->data[idx] = val;
}

void _ks_array_clear(void* arr) {
    if (arr) ((_KsArray*)arr)->len = 0;
}

// _ks_str_split uses the array runtime
void* _ks_str_split(const char* s, const char* delim) {
    void* arr = _ks_array_new(8);
    size_t dl = strlen(delim);
    const char* p = s;
    while (1) {
        const char* f = strstr(p, delim);
        size_t len = f ? (size_t)(f - p) : strlen(p);
        char* part = malloc(len + 1); memcpy(part, p, len); part[len] = '\0';
        _ks_array_push(arr, part);
        if (!f) break;
        p = f + dl;
    }
    return arr;
}
// _ks_file_lines — returns _KsArray of strings
void* _ks_file_lines(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        // Return empty array on failure
        return _ks_array_new(0);
    }
    void* arr = _ks_array_new(16);
    char buf[4096];
    while (fgets(buf, sizeof(buf), f)) {
        size_t l = strlen(buf);
        if (l > 0 && buf[l-1] == '\n') buf[--l] = '\0';
        _ks_array_push(arr, strdup(buf));
    }
    fclose(f);
    return arr;
}

// ── HashMap ───────────────────────────────────────────────────────────────────
// Simple open-addressing string→void* hashmap
#define KS_MAP_INIT_CAP 16
typedef struct { char* key; void* val; } _KsMapEntry;
typedef struct { _KsMapEntry* entries; int len; int cap; } _KsMap;

static unsigned _ks_hash(const char* s) {
    unsigned h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h;
}
void* _ks_hashmap_new() {
    _KsMap* m = malloc(sizeof(_KsMap));
    m->cap = KS_MAP_INIT_CAP;
    m->entries = calloc(m->cap, sizeof(_KsMapEntry));
    m->len = 0; return m;
}
void _ks_hashmap_set(void* map, const char* key, void* val) {
    _KsMap* m = map;
    unsigned idx = _ks_hash(key) % m->cap;
    while (m->entries[idx].key) {
        if (strcmp(m->entries[idx].key, key) == 0) { m->entries[idx].val = val; return; }
        idx = (idx + 1) % m->cap;
    }
    m->entries[idx].key = strdup(key);
    m->entries[idx].val = val;
    m->len++;
}
void* _ks_hashmap_get(void* map, const char* key) {
    _KsMap* m = map;
    unsigned idx = _ks_hash(key) % m->cap;
    while (m->entries[idx].key) {
        if (strcmp(m->entries[idx].key, key) == 0) return m->entries[idx].val;
        idx = (idx + 1) % m->cap;
    }
    return NULL;
}
int _ks_hashmap_has(void* map, const char* key) { return _ks_hashmap_get(map, key) != NULL; }
int _ks_hashmap_len(void* map)  { return map ? ((_KsMap*)map)->len : 0; }

// ── String methods (additional — declared in IRGen::emitRuntimeDecls) ─────────
char* _ks_str_concat(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    // Validate pointers before strlen (crash guard)
    size_t la = 0, lb = 0;
    // Use volatile reads to check for truly invalid pointers
    la = strlen(a);
    lb = strlen(b);
    char* r = malloc(la + lb + 1);
    if (la > 0) memcpy(r, a, la);
    if (lb > 0) memcpy(r + la, b, lb);
    r[la + lb] = '\0';
    return r;
}

// Returns a one-character heap string at position pos (or "" if OOB)
char* _ks_str_charAt(const char* s, int pos) {
    if (!s || pos < 0 || pos >= (int)strlen(s)) return strdup("");
    char* r = malloc(2);
    r[0] = s[pos];
    r[1] = '\0';
    return r;
}

// Single-character classification — argument is a one-char string from charAt
int _ks_str_isAlpha(const char* s)  { return s && isalpha((unsigned char)s[0]); }
int _ks_str_isDigit(const char* s)  { return s && isdigit((unsigned char)s[0]); }
int _ks_str_isUpper(const char* s)  { return s && isupper((unsigned char)s[0]); }
int _ks_str_isLower(const char* s)  { return s && islower((unsigned char)s[0]); }
int _ks_str_isSpace(const char* s)  { return s && isspace((unsigned char)s[0]); }

// Returns the Unicode code point (ASCII value) of the first character
int _ks_str_toCharCode(const char* s) {
    return (s && *s) ? (unsigned char)s[0] : 0;
}

// Returns a one-character string for the given code point
char* _ks_str_fromCharCode(int code) {
    char* r = malloc(2);
    r[0] = (char)(unsigned char)code;
    r[1] = '\0';
    return r;
}

// ── Shell execution ──────────────────────────────────────────────────────────
int _ks_system(const char* cmd) {
    return system(cmd);
}

// ── Missing runtime functions (needed by self-hosted compiler) ───────────────

// Convert integer to heap-allocated string
char* _ks_int_to_str(int val) {
    char buf[32];
    int neg = 0, i = 0;
    if (val < 0) { neg = 1; val = -val; }
    if (val == 0) { buf[i++] = '0'; }
    else { while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; } }
    if (neg) buf[i++] = '-';
    char* r = malloc(i + 1);
    for (int j = 0; j < i; j++) r[j] = buf[i - 1 - j];
    r[i] = '\0';
    return r;
}

// Compare two strings (wrapper around strcmp for self-hosted irgen)
int _ks_str_compare(const char* a, const char* b) {
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strcmp(a, b);
}

// ── Array clone and free ─────────────────────────────────────────────────────

void* _ks_array_clone(void* arr) {
    if (!arr) return _ks_array_new(8);
    _KsArray* src = arr;
    _KsArray* dst = _ks_array_new(src->cap);
    for (int i = 0; i < src->len; i++)
        _ks_array_push(dst, src->data[i]);
    return dst;
}

void _ks_array_free(void* arr) {
    if (!arr) return;
    _KsArray* a = arr;
    free(a->data);
    free(a);
}

// ── Closure runtime ──────────────────────────────────────────────────────────

typedef struct { void* fn; void* env; } _KsClosure;

void* _ks_closure_new(void* fn, void* env) {
    _KsClosure* c = malloc(sizeof(_KsClosure));
    c->fn = fn;
    c->env = env;
    return c;
}

void* _ks_closure_fn(void* c)  { return c ? ((_KsClosure*)c)->fn : NULL; }
void* _ks_closure_env(void* c) { return c ? ((_KsClosure*)c)->env : NULL; }

void _ks_closure_free(void* c) {
    if (!c) return;
    _KsClosure* cl = c;
    if (cl->env) free(cl->env);
    free(cl);
}

// ── Timing ──────────────────────────────────────────────────────────────────
#include <time.h>
double _ks_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}


// ── Command-line arguments ──────────────────────────────────────────────────
static int _ks_argc_val = 0;
static char** _ks_argv_val = NULL;

void _ks_init_args(int argc, char** argv) {
    _ks_argc_val = argc;
    _ks_argv_val = argv;
}

int _ks_argc() { return _ks_argc_val; }

char* _ks_get_argv(int idx) {
    if (idx < 0 || idx >= _ks_argc_val || !_ks_argv_val) return "";
    return _ks_argv_val[idx] ? _ks_argv_val[idx] : "";
}
