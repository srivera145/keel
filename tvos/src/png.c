/* KEEL OS — minimal PNG writer (own deflate: LZ77 + fixed Huffman). */
#include "png.h"

typedef struct { uint8_t *buf; size_t len, cap; uint32_t bits; int nbits; } BitOut;

static void bo_byte(BitOut *b, uint8_t v) {
    if (b->len == b->cap) { b->cap = b->cap ? b->cap * 2 : 1 << 16; b->buf = realloc(b->buf, b->cap); if (!b->buf) abort(); }
    b->buf[b->len++] = v;
}
static void bo_bits(BitOut *b, uint32_t v, int n) {
    b->bits |= v << b->nbits;
    b->nbits += n;
    while (b->nbits >= 8) { bo_byte(b, (uint8_t)b->bits); b->bits >>= 8; b->nbits -= 8; }
}
static uint32_t rev(uint32_t v, int n) {
    uint32_t r = 0;
    for (int i = 0; i < n; i++) { r = (r << 1) | (v & 1); v >>= 1; }
    return r;
}
static void put_lit(BitOut *b, int v) {
    if (v < 144) bo_bits(b, rev(0x30 + v, 8), 8);
    else if (v < 256) bo_bits(b, rev(0x190 + v - 144, 9), 9);
    else if (v < 280) bo_bits(b, rev(v - 256, 7), 7);
    else bo_bits(b, rev(0xC0 + v - 280, 8), 8);
}
static const int LBASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const int LEXT[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const int DBASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static const int DEXT[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static void put_match(BitOut *b, int len, int dist) {
    int li = 28;
    while (LBASE[li] > len) li--;
    put_lit(b, 257 + li);
    if (LEXT[li]) bo_bits(b, (uint32_t)(len - LBASE[li]), LEXT[li]);
    int di = 29;
    while (DBASE[di] > dist) di--;
    bo_bits(b, rev((uint32_t)di, 5), 5);
    if (DEXT[di]) bo_bits(b, (uint32_t)(dist - DBASE[di]), DEXT[di]);
}

static uint8_t *zlib_compress(const uint8_t *in, size_t n, size_t *outlen) {
    BitOut b = {0};
    bo_byte(&b, 0x78); bo_byte(&b, 0x01);
    bo_bits(&b, 1, 1); /* final */
    bo_bits(&b, 1, 2); /* fixed huffman */
    enum { HBITS = 16, WIN = 32768 };
    int32_t *head = xmalloc(sizeof(int32_t) << HBITS);
    int32_t *prev = xmalloc(sizeof(int32_t) * WIN);
    for (int i = 0; i < (1 << HBITS); i++) head[i] = -1;
    size_t i = 0;
    while (i < n) {
        int best = 0, bdist = 0;
        if (i + 3 <= n) {
            uint32_t h = ((in[i] << 16) | (in[i + 1] << 8) | in[i + 2]) * 2654435761u >> (32 - HBITS);
            int32_t cand = head[h];
            int chain = 16;
            while (cand >= 0 && (int)(i - cand) <= WIN - 1 && chain-- > 0) {
                int l = 0, maxl = (int)MINI(258, n - i);
                while (l < maxl && in[cand + l] == in[i + l]) l++;
                if (l > best) { best = l; bdist = (int)(i - cand); if (l == maxl) break; }
                cand = prev[cand & (WIN - 1)];
            }
            prev[i & (WIN - 1)] = head[h];
            head[h] = (int32_t)i;
        }
        if (best >= 3) {
            put_match(&b, best, bdist);
            for (int k = 1; k < best && i + k + 3 <= n; k++) {
                size_t j = i + k;
                uint32_t h = ((in[j] << 16) | (in[j + 1] << 8) | in[j + 2]) * 2654435761u >> (32 - HBITS);
                prev[j & (WIN - 1)] = head[h];
                head[h] = (int32_t)j;
            }
            i += best;
        } else {
            put_lit(&b, in[i]);
            i++;
        }
    }
    put_lit(&b, 256);
    if (b.nbits) bo_bits(&b, 0, 8 - b.nbits);
    uint32_t a = 1, s2 = 0;
    for (size_t k = 0; k < n; k++) { a = (a + in[k]) % 65521; s2 = (s2 + a) % 65521; }
    uint32_t ad = (s2 << 16) | a;
    bo_byte(&b, ad >> 24); bo_byte(&b, ad >> 16); bo_byte(&b, ad >> 8); bo_byte(&b, ad);
    free(head);
    free(prev);
    *outlen = b.len;
    return b.buf;
}

static uint32_t crc_table[256];
static uint32_t crc(const uint8_t *p, size_t n, uint32_t c) {
    if (!crc_table[1])
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t v = i;
            for (int k = 0; k < 8; k++) v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1;
            crc_table[i] = v;
        }
    c = ~c;
    for (size_t i = 0; i < n; i++) c = crc_table[(c ^ p[i]) & 255] ^ (c >> 8);
    return ~c;
}
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    uint8_t hdr[8] = {len >> 24, len >> 16, len >> 8, len, type[0], type[1], type[2], type[3]};
    fwrite(hdr, 1, 8, f);
    if (len) fwrite(data, 1, len, f);
    uint32_t c = crc(hdr + 4, 4, 0);
    c = crc(data, len, c);
    uint8_t cb[4] = {c >> 24, c >> 16, c >> 8, c};
    fwrite(cb, 1, 4, f);
}

int png_write(const char *path, const uint32_t *px, int w, int h, int stride) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    size_t rowb = (size_t)w * 3 + 1;
    uint8_t *raw = xmalloc(rowb * h);
    for (int y = 0; y < h; y++) {
        uint8_t *r = raw + rowb * y;
        r[0] = 1; /* Sub filter */
        const uint32_t *s = px + (size_t)y * stride;
        uint8_t pr = 0, pg = 0, pb = 0;
        for (int x = 0; x < w; x++) {
            uint8_t cr = (s[x] >> 16) & 255, cg = (s[x] >> 8) & 255, cb = s[x] & 255;
            r[1 + 3 * x] = cr - pr; r[2 + 3 * x] = cg - pg; r[3 + 3 * x] = cb - pb;
            pr = cr; pg = cg; pb = cb;
        }
    }
    size_t zl;
    uint8_t *z = zlib_compress(raw, rowb * h, &zl);
    static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = {w >> 24, w >> 16, w >> 8, w, h >> 24, h >> 16, h >> 8, h, 8, 2, 0, 0, 0};
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z, (uint32_t)zl);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw);
    free(z);
    return 1;
}
