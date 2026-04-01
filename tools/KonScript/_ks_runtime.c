// _ks_runtime.c — KonScript native runtime
// Implements the stdlib functions declared in irgen's emitRuntimeDecls().
// Compiled once: clang -c _ks_runtime.c -o _ks_runtime.o
// Linked with every native KonScript binary.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ── Result<T> ────────────────────────────────────────────────────────────────
// A heap-allocated struct: [ok:i32, value:i8*, error:i8*]
typedef struct { int ok; char* value; char* error; } _KsResult;

static _KsResult* _ks_result_ok_val(char* v) {
    _KsResult* r = malloc(sizeof(_KsResult));
    r->ok = 1; r->value = v ? strdup(v) : strdup(""); r->error = strdup("");
    return r;
}
static _KsResult* _ks_result_err_val(const char* e) {
    _KsResult* r = malloc(sizeof(_KsResult));
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
    char* buf = malloc(sz + 1); fread(buf, 1, sz, f); buf[sz] = '\0'; fclose(f);
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
int   _ks_str_contains(const char* s, const char* sub) { return strstr(s, sub) != NULL; }
int   _ks_str_starts(const char* s, const char* p)     { return strncmp(s, p, strlen(p)) == 0; }
int   _ks_str_ends(const char* s, const char* e) {
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
    char* r = malloc(len + 1); memcpy(r, s + pos, len); r[len] = '\0'; return r;
}
int   _ks_str_toInt(const char* s)   { return atoi(s); }
float _ks_str_toFloat(const char* s) { return (float)atof(s); }

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
