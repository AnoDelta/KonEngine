/* ks_runtime.c — KonScript runtime library
   Implements the _ks_array_*, _ks_str_*, _ks_hashmap_* functions
   declared in IRGen-emitted LLVM IR.

   Compile:
     clang -O2 -c ks_runtime.c -o ks_runtime.o
   Then link ks_runtime.o alongside the compiled KonScript IR.

   Cross-compile example (Linux → Windows):
     clang --target x86_64-pc-windows-msvc -O2 -c ks_runtime.c -o ks_runtime.obj

   For bare-metal (kernel), replace malloc/free with your allocator and
   stub out the string/file functions you don't need.
*/

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
   Array — dynamic array of i8* elements (type-erased)
   Layout: { i8** data, int32_t len, int32_t cap }
   ------------------------------------------------------------------------- */
typedef struct {
    int8_t**  data;
    int32_t   len;
    int32_t   cap;
} KsArray;

int8_t* _ks_array_new(int32_t initial_cap) {
    KsArray* a = (KsArray*)malloc(sizeof(KsArray));
    if (!a) return NULL;
    a->cap  = initial_cap > 0 ? initial_cap : 8;
    a->len  = 0;
    a->data = (int8_t**)malloc(sizeof(int8_t*) * (size_t)a->cap);
    return (int8_t*)a;
}

void _ks_array_push(int8_t* arr, int8_t* elem) {
    KsArray* a = (KsArray*)arr;
    if (a->len == a->cap) {
        a->cap  *= 2;
        a->data  = (int8_t**)realloc(a->data, sizeof(int8_t*) * (size_t)a->cap);
    }
    a->data[a->len++] = elem;
}

int8_t* _ks_array_pop(int8_t* arr) {
    KsArray* a = (KsArray*)arr;
    if (a->len == 0) return NULL;
    return a->data[--a->len];
}

int32_t _ks_array_len(int8_t* arr) {
    if (!arr) return 0;
    return ((KsArray*)arr)->len;
}

int8_t* _ks_array_get(int8_t* arr, int32_t i) {
    KsArray* a = (KsArray*)arr;
    if (!a || i < 0 || i >= a->len) return NULL;
    return a->data[i];
}

int8_t  _ks_array_has(int8_t* arr, int8_t* elem) {
    KsArray* a = (KsArray*)arr;
    for (int32_t i = 0; i < a->len; i++)
        if (a->data[i] == elem) return 1;
    return 0;
}

/* -------------------------------------------------------------------------
   String helpers (simple heap-allocated C strings)
   ------------------------------------------------------------------------- */
static int8_t* ks_strdup(const char* s) {
    size_t n = strlen(s) + 1;
    int8_t* p = (int8_t*)malloc(n);
    memcpy(p, s, n);
    return p;
}

int32_t _ks_str_len(int8_t* s)  { return (int32_t)strlen((char*)s); }

int8_t* _ks_str_trim(int8_t* s) {
    char* p = (char*)s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    char* q = p + strlen(p);
    while (q > p && (q[-1]==' '||q[-1]=='\t'||q[-1]=='\n'||q[-1]=='\r')) q--;
    size_t n = (size_t)(q - p);
    int8_t* out = (int8_t*)malloc(n + 1);
    memcpy(out, p, n); out[n] = 0;
    return out;
}

int8_t* _ks_str_upper(int8_t* s) {
    size_t n = strlen((char*)s);
    int8_t* out = (int8_t*)malloc(n + 1);
    for (size_t i = 0; i <= n; i++)
        out[i] = (s[i] >= 'a' && s[i] <= 'z') ? s[i] - 32 : s[i];
    return out;
}

int8_t* _ks_str_lower(int8_t* s) {
    size_t n = strlen((char*)s);
    int8_t* out = (int8_t*)malloc(n + 1);
    for (size_t i = 0; i <= n; i++)
        out[i] = (s[i] >= 'A' && s[i] <= 'Z') ? s[i] + 32 : s[i];
    return out;
}

int8_t  _ks_str_contains(int8_t* s, int8_t* sub) {
    return strstr((char*)s, (char*)sub) != NULL;
}
int8_t  _ks_str_starts(int8_t* s, int8_t* p) {
    return strncmp((char*)s, (char*)p, strlen((char*)p)) == 0;
}
int8_t  _ks_str_ends(int8_t* s, int8_t* e) {
    size_t sl = strlen((char*)s), el = strlen((char*)e);
    return sl >= el && strcmp((char*)s + sl - el, (char*)e) == 0;
}

int8_t* _ks_str_substr(int8_t* s, int32_t start, int32_t len) {
    int8_t* out = (int8_t*)malloc((size_t)len + 1);
    memcpy(out, s + start, (size_t)len); out[len] = 0;
    return out;
}

int8_t* _ks_str_replace(int8_t* s, int8_t* from, int8_t* to) {
    /* simple single-pass replace */
    char* p = strstr((char*)s, (char*)from);
    if (!p) return ks_strdup((char*)s);
    size_t pre = (size_t)(p - (char*)s);
    size_t fl  = strlen((char*)from);
    size_t tl  = strlen((char*)to);
    size_t sl  = strlen((char*)s);
    size_t nl  = sl - fl + tl;
    int8_t* out = (int8_t*)malloc(nl + 1);
    memcpy(out, s, pre);
    memcpy(out + pre, to, tl);
    memcpy(out + pre + tl, p + fl, sl - pre - fl + 1);
    return out;
}

int8_t* _ks_str_split(int8_t* s, int8_t* delim) {
    /* returns a KsArray of strings */
    int8_t* arr = _ks_array_new(4);
    char* copy  = strdup((char*)s);
    char* tok   = strtok(copy, (char*)delim);
    while (tok) {
        _ks_array_push(arr, ks_strdup(tok));
        tok = strtok(NULL, (char*)delim);
    }
    free(copy);
    return arr;
}

int32_t _ks_str_toInt(int8_t* s)   { return (int32_t)atoi((char*)s); }
float   _ks_str_toFloat(int8_t* s) { return (float)atof((char*)s); }

/* -------------------------------------------------------------------------
   HashMap — simple open-addressing hash map (string keys → i8* values)
   ------------------------------------------------------------------------- */
typedef struct { int8_t* key; int8_t* val; } KsMapEntry;
typedef struct { KsMapEntry* entries; int32_t cap; int32_t len; } KsMap;

#define KS_MAP_INIT_CAP 16

static uint32_t ks_hash(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
    return h;
}

int8_t* _ks_hashmap_new(void) {
    KsMap* m = (KsMap*)malloc(sizeof(KsMap));
    m->cap     = KS_MAP_INIT_CAP;
    m->len     = 0;
    m->entries = (KsMapEntry*)calloc((size_t)m->cap, sizeof(KsMapEntry));
    return (int8_t*)m;
}

void _ks_hashmap_set(int8_t* mp, int8_t* key, int8_t* val) {
    KsMap* m = (KsMap*)mp;
    uint32_t h = ks_hash((char*)key) % (uint32_t)m->cap;
    while (m->entries[h].key && strcmp((char*)m->entries[h].key, (char*)key) != 0)
        h = (h + 1) % (uint32_t)m->cap;
    if (!m->entries[h].key) { m->entries[h].key = key; m->len++; }
    m->entries[h].val = val;
}

int8_t* _ks_hashmap_get(int8_t* mp, int8_t* key) {
    KsMap* m = (KsMap*)mp;
    uint32_t h = ks_hash((char*)key) % (uint32_t)m->cap;
    while (m->entries[h].key) {
        if (strcmp((char*)m->entries[h].key, (char*)key) == 0) return m->entries[h].val;
        h = (h + 1) % (uint32_t)m->cap;
    }
    return NULL;
}

int8_t  _ks_hashmap_has(int8_t* mp, int8_t* key) { return _ks_hashmap_get(mp, key) != NULL; }
int32_t _ks_hashmap_len(int8_t* mp) { return ((KsMap*)mp)->len; }

/* -------------------------------------------------------------------------
   File I/O
   ------------------------------------------------------------------------- */
int8_t* _ks_file_read(int8_t* path) {
    FILE* f = fopen((char*)path, "rb");
    if (!f) return ks_strdup("");
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    int8_t* buf = (int8_t*)malloc((size_t)sz + 1);
    fread(buf, 1, (size_t)sz, f); buf[sz] = 0; fclose(f);
    return buf;
}

int8_t* _ks_file_write(int8_t* path, int8_t* content) {
    FILE* f = fopen((char*)path, "wb");
    if (!f) return ks_strdup("error");
    fputs((char*)content, f); fclose(f);
    return ks_strdup("ok");
}

int8_t* _ks_file_append(int8_t* path, int8_t* content) {
    FILE* f = fopen((char*)path, "ab");
    if (!f) return ks_strdup("error");
    fputs((char*)content, f); fclose(f);
    return ks_strdup("ok");
}

int8_t  _ks_file_exists(int8_t* path) {
    FILE* f = fopen((char*)path, "r");
    if (!f) return 0; fclose(f); return 1;
}

int8_t* _ks_file_delete(int8_t* path) {
    remove((char*)path); return ks_strdup("ok");
}

int8_t* _ks_file_lines(int8_t* path) {
    int8_t* content = _ks_file_read(path);
    return _ks_str_split(content, (int8_t*)"\n");
}

/* -------------------------------------------------------------------------
   Result type
   ------------------------------------------------------------------------- */
typedef struct { int8_t ok; int8_t* value; } KsResult;

int8_t* _ks_result_ok_make(int8_t* val) {
    KsResult* r = (KsResult*)malloc(sizeof(KsResult));
    r->ok = 1; r->value = val; return (int8_t*)r;
}
int8_t  _ks_result_ok(int8_t* r)    { return ((KsResult*)r)->ok; }
int8_t* _ks_result_value(int8_t* r) { return ((KsResult*)r)->value; }
int8_t* _ks_result_error(int8_t* r) {
    return ((KsResult*)r)->ok ? ks_strdup("") : ((KsResult*)r)->value;
}

/* String concatenation */
int8_t* _ks_str_concat(int8_t* a, int8_t* b) {
    if (!a) a = (int8_t*)"";
    if (!b) b = (int8_t*)"";
    size_t la = strlen((char*)a), lb = strlen((char*)b);
    int8_t* out = (int8_t*)malloc(la + lb + 1);
    memcpy(out, a, la);
    memcpy(out + la, b, lb);
    out[la + lb] = 0;
    return out;
}
