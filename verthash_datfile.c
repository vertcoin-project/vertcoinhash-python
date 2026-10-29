/* Generation of verthash.dat, the Verthash proof-of-space datafile.
 *
 * A port of vertcoin-core's src/crypto/verthash_datfile.cpp: the graph of h1.c
 * (NewGraph/XiGraphIter), built through a memory map of the file instead of with
 * a seek, read and write per node, and hashed with Bitcoin Core's unrolled
 * SHA3-256 (src/crypto/sha3.cpp), about four times faster than tiny_sha3's.
 * The map lets the OS page the file out, so it needs no 1.2 GiB of memory.
 *
 * keccakf: Copyright (c) 2020-present The Bitcoin Core developers, MIT license,
 * based on https://github.com/mjosaarinen/tiny_sha3 by Markku-Juhani O. Saarinen.
 */
/* ftruncate, mmap, msync and posix_fallocate under -std=c99 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200112L
#endif

#include "verthash_datfile.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define NODE_SIZE 32
/* the datafile is the graph for this index: numXi(17) nodes of NODE_SIZE bytes */
#define GRAPH_INDEX 17
/* XiGraphIter's stack holds at most 4 nodes per level plus 5 */
#define MAX_STACK 256

/* ---- SHA3-256 (Bitcoin Core's KeccakF) ---- */

#define ROTL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))

static void keccakf(uint64_t st[25])
{
    static const uint64_t RNDC[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
        0x000000000000808bULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
        0x000000000000008aULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
        0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
        0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
        0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
    };
    int round;

    for (round = 0; round < 24; ++round) {
        uint64_t bc0, bc1, bc2, bc3, bc4, t;

        /* Theta */
        bc0 = st[0] ^ st[5] ^ st[10] ^ st[15] ^ st[20];
        bc1 = st[1] ^ st[6] ^ st[11] ^ st[16] ^ st[21];
        bc2 = st[2] ^ st[7] ^ st[12] ^ st[17] ^ st[22];
        bc3 = st[3] ^ st[8] ^ st[13] ^ st[18] ^ st[23];
        bc4 = st[4] ^ st[9] ^ st[14] ^ st[19] ^ st[24];
        t = bc4 ^ ROTL64(bc1, 1); st[0] ^= t; st[5] ^= t; st[10] ^= t; st[15] ^= t; st[20] ^= t;
        t = bc0 ^ ROTL64(bc2, 1); st[1] ^= t; st[6] ^= t; st[11] ^= t; st[16] ^= t; st[21] ^= t;
        t = bc1 ^ ROTL64(bc3, 1); st[2] ^= t; st[7] ^= t; st[12] ^= t; st[17] ^= t; st[22] ^= t;
        t = bc2 ^ ROTL64(bc4, 1); st[3] ^= t; st[8] ^= t; st[13] ^= t; st[18] ^= t; st[23] ^= t;
        t = bc3 ^ ROTL64(bc0, 1); st[4] ^= t; st[9] ^= t; st[14] ^= t; st[19] ^= t; st[24] ^= t;

        /* Rho Pi */
        t = st[1];
        bc0 = st[10]; st[10] = ROTL64(t, 1); t = bc0;
        bc0 = st[7]; st[7] = ROTL64(t, 3); t = bc0;
        bc0 = st[11]; st[11] = ROTL64(t, 6); t = bc0;
        bc0 = st[17]; st[17] = ROTL64(t, 10); t = bc0;
        bc0 = st[18]; st[18] = ROTL64(t, 15); t = bc0;
        bc0 = st[3]; st[3] = ROTL64(t, 21); t = bc0;
        bc0 = st[5]; st[5] = ROTL64(t, 28); t = bc0;
        bc0 = st[16]; st[16] = ROTL64(t, 36); t = bc0;
        bc0 = st[8]; st[8] = ROTL64(t, 45); t = bc0;
        bc0 = st[21]; st[21] = ROTL64(t, 55); t = bc0;
        bc0 = st[24]; st[24] = ROTL64(t, 2); t = bc0;
        bc0 = st[4]; st[4] = ROTL64(t, 14); t = bc0;
        bc0 = st[15]; st[15] = ROTL64(t, 27); t = bc0;
        bc0 = st[23]; st[23] = ROTL64(t, 41); t = bc0;
        bc0 = st[19]; st[19] = ROTL64(t, 56); t = bc0;
        bc0 = st[13]; st[13] = ROTL64(t, 8); t = bc0;
        bc0 = st[12]; st[12] = ROTL64(t, 25); t = bc0;
        bc0 = st[2]; st[2] = ROTL64(t, 43); t = bc0;
        bc0 = st[20]; st[20] = ROTL64(t, 62); t = bc0;
        bc0 = st[14]; st[14] = ROTL64(t, 18); t = bc0;
        bc0 = st[22]; st[22] = ROTL64(t, 39); t = bc0;
        bc0 = st[9]; st[9] = ROTL64(t, 61); t = bc0;
        bc0 = st[6]; st[6] = ROTL64(t, 20); t = bc0;
        st[1] = ROTL64(t, 44);

        /* Chi Iota */
        bc0 = st[0]; bc1 = st[1]; bc2 = st[2]; bc3 = st[3]; bc4 = st[4];
        st[0] = bc0 ^ (~bc1 & bc2) ^ RNDC[round];
        st[1] = bc1 ^ (~bc2 & bc3);
        st[2] = bc2 ^ (~bc3 & bc4);
        st[3] = bc3 ^ (~bc4 & bc0);
        st[4] = bc4 ^ (~bc0 & bc1);
        bc0 = st[5]; bc1 = st[6]; bc2 = st[7]; bc3 = st[8]; bc4 = st[9];
        st[5] = bc0 ^ (~bc1 & bc2);
        st[6] = bc1 ^ (~bc2 & bc3);
        st[7] = bc2 ^ (~bc3 & bc4);
        st[8] = bc3 ^ (~bc4 & bc0);
        st[9] = bc4 ^ (~bc0 & bc1);
        bc0 = st[10]; bc1 = st[11]; bc2 = st[12]; bc3 = st[13]; bc4 = st[14];
        st[10] = bc0 ^ (~bc1 & bc2);
        st[11] = bc1 ^ (~bc2 & bc3);
        st[12] = bc2 ^ (~bc3 & bc4);
        st[13] = bc3 ^ (~bc4 & bc0);
        st[14] = bc4 ^ (~bc0 & bc1);
        bc0 = st[15]; bc1 = st[16]; bc2 = st[17]; bc3 = st[18]; bc4 = st[19];
        st[15] = bc0 ^ (~bc1 & bc2);
        st[16] = bc1 ^ (~bc2 & bc3);
        st[17] = bc2 ^ (~bc3 & bc4);
        st[18] = bc3 ^ (~bc4 & bc0);
        st[19] = bc4 ^ (~bc0 & bc1);
        bc0 = st[20]; bc1 = st[21]; bc2 = st[22]; bc3 = st[23]; bc4 = st[24];
        st[20] = bc0 ^ (~bc1 & bc2);
        st[21] = bc1 ^ (~bc2 & bc3);
        st[22] = bc2 ^ (~bc3 & bc4);
        st[23] = bc3 ^ (~bc4 & bc0);
        st[24] = bc4 ^ (~bc0 & bc1);
    }
}

static uint64_t read_le64(const uint8_t *p)
{
    return (uint64_t)p[0] | (uint64_t)p[1] << 8 | (uint64_t)p[2] << 16 | (uint64_t)p[3] << 24 |
           (uint64_t)p[4] << 32 | (uint64_t)p[5] << 40 | (uint64_t)p[6] << 48 | (uint64_t)p[7] << 56;
}

static void write_le64(uint8_t *p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++)
        p[i] = (uint8_t)(v >> (8 * i));
}

/* SHA3-256 of a message shorter than its 136-byte rate (one block), as Core's
 * SHA3_256().Write(in).Finalize(out). The node hashes are 64 to 128 bytes. */
static void sha3_256_short(const uint8_t *in, size_t len, uint8_t out[32])
{
    uint64_t st[25] = {0};
    uint8_t last[8] = {0};
    size_t i, words = len / 8;

    for (i = 0; i < words; i++)
        st[i] ^= read_le64(in + 8 * i);
    memcpy(last, in + 8 * words, len % 8);
    last[len % 8] ^= 0x06;
    st[words] ^= read_le64(last);
    st[16] ^= 0x8000000000000000ULL;
    keccakf(st);
    for (i = 0; i < 4; i++)
        write_le64(out + 8 * i, st[i]);
}

/* ---- the graph ---- */

struct graph
{
    uint8_t *data;
    int64_t pow2;
    uint8_t pk[NODE_SIZE];
    int64_t total;
    verthash_progress_fn progress;
    void *ctx;
    int64_t next_report;
    int stopped;
};

static int64_t num_xi(int64_t index)
{
    return ((int64_t)1 << index) * (index + 1) * index;
}

static int64_t log2_floor(int64_t x)
{
    int64_t r = 0;
    for (; x > 1; x >>= 1)
        r++;
    return r;
}

/* nodes are stored at id & ~pow2 (h1.c's bfsToPost) */
static uint8_t *get_node(struct graph *g, int64_t id)
{
    return g->data + (id & ~g->pow2) * NODE_SIZE;
}

/* Go's binary.PutVarint (zigzag), zero-padded to NODE_SIZE (h1.c's WriteVarInt) */
static void write_varint(uint8_t *out, int64_t val)
{
    uint64_t uval = ((uint64_t)val) << 1;
    int i = 0;
    if (val < 0)
        uval = ~uval;
    memset(out, 0, NODE_SIZE);
    while (uval >= 0x80) {
        out[i++] = (uint8_t)uval | 0x80;
        uval >>= 7;
    }
    out[i] = (uint8_t)uval;
}

/* Hashes node id from the public key, the id and up to two parents, straight into
 * the graph. Parents always come before id, so they never overlap the node written. */
static void new_node(struct graph *g, int64_t id, const uint8_t *parent0, const uint8_t *parent1)
{
    uint8_t in[NODE_SIZE * 4];
    size_t len = NODE_SIZE * 2;

    memcpy(in, g->pk, NODE_SIZE);
    write_varint(in + NODE_SIZE, id);
    if (parent0) {
        memcpy(in + len, parent0, NODE_SIZE);
        len += NODE_SIZE;
    }
    if (parent1) {
        memcpy(in + len, parent1, NODE_SIZE);
        len += NODE_SIZE;
    }
    sha3_256_short(in, len, get_node(g, id));
}

/* reports progress each time another VERTHASH_PROGRESS_STEPS-th of the nodes is done */
static void report(struct graph *g, int64_t count)
{
    int64_t done = count - g->pow2;
    if (g->progress == NULL || g->stopped || done < g->next_report || done >= g->total)
        return;
    while (g->next_report <= done)
        g->next_report += g->total / VERTHASH_PROGRESS_STEPS;
    if (g->progress(g->ctx, (double)done / (double)g->total))
        g->stopped = 1;
}

static void butterfly_graph(struct graph *g, int64_t index, int64_t *count)
{
    int64_t num_level, per_level, begin, level, i;

    if (index == 0)
        index = 1;
    num_level = 2 * index;
    per_level = (int64_t)1 << index;
    begin = *count - per_level;
    for (level = 1; level < num_level && !g->stopped; level++) {
        int64_t shift = index - level;
        if (level > num_level / 2)
            shift = level - num_level / 2;
        for (i = 0; i < per_level; i++) {
            int64_t prev = ((i >> shift) & 1) == 0 ? i + ((int64_t)1 << shift) : i - ((int64_t)1 << shift);
            new_node(g, *count, get_node(g, begin + (level - 1) * per_level + prev), get_node(g, *count - per_level));
            (*count)++;
        }
        report(g, *count);
    }
}

static void xi_graph_iter(struct graph *g, int64_t index)
{
    int64_t stack[MAX_STACK];
    int graph_stack[MAX_STACK];
    int stack_size = 0;
    int64_t count = g->pow2;
    int64_t i;

    for (i = 0; i < ((int64_t)1 << index); i++) {
        new_node(g, count, NULL, NULL);
        count++;
    }
    if (index == 1) {
        butterfly_graph(g, index, &count);
        return;
    }

    for (i = 0; i < 5; i++) {
        stack[stack_size] = index;
        graph_stack[stack_size] = 4 - (int)i;
        stack_size++;
    }
    while (stack_size > 0 && !g->stopped) {
        int64_t pow2index, half;
        int graph;

        stack_size--;
        index = stack[stack_size];
        graph = graph_stack[stack_size];
        pow2index = (int64_t)1 << index;
        half = (int64_t)1 << (index - 1);

        if (graph == 0) {
            int64_t sources = count - pow2index;
            for (i = 0; i < half; i++) {
                new_node(g, count, get_node(g, sources + i), get_node(g, sources + i + half));
                count++;
            }
        } else if (graph <= 3) {
            /* the first Xi graph, the second Xi graph and the second butterfly graph
             * are each a chain of nodes with one parent */
            int64_t first = count;
            for (i = 0; i < half; i++) {
                new_node(g, first + i, get_node(g, first - half + i), NULL);
                count++;
            }
        } else {
            int64_t sinks = count;
            int64_t sources = sinks + pow2index - num_xi(index);
            for (i = 0; i < half; i++) {
                const uint8_t *parent0 = get_node(g, sinks - half + i);
                new_node(g, sinks + i, parent0, get_node(g, sources + i));
                new_node(g, sinks + i + half, parent0, get_node(g, sources + i + half));
                count += 2;
            }
        }
        report(g, count);

        if (graph == 0 || graph == 3 || ((graph == 1 || graph == 2) && index == 2)) {
            butterfly_graph(g, index - 1, &count);
        } else if (graph == 1 || graph == 2) {
            if (stack_size + 5 > MAX_STACK)
                abort(); /* cannot happen for index 17 */
            for (i = 0; i < 5; i++) {
                stack[stack_size] = index - 1;
                graph_stack[stack_size] = 4 - (int)i;
                stack_size++;
            }
        }
    }
}

int verthash_generate_datafile(uint8_t *buf, verthash_progress_fn progress, void *ctx)
{
    static const char seed[] = "Verthash Proof-of-Space Datafile";
    struct graph g;

    g.data = buf;
    g.total = num_xi(GRAPH_INDEX);
    g.pow2 = (int64_t)1 << (log2_floor(g.total) + 1);
    g.progress = progress;
    g.ctx = ctx;
    g.next_report = g.total / VERTHASH_PROGRESS_STEPS;
    g.stopped = 0;
    sha3_256_short((const uint8_t *)seed, sizeof(seed) - 1, g.pk);
    xi_graph_iter(&g, GRAPH_INDEX);
    if (g.stopped)
        return VERTHASH_STOPPED;
    if (progress && progress(ctx, 1.0))
        return VERTHASH_STOPPED;
    return 0;
}

/* ---- the file ---- */

#ifdef _WIN32

/* Generates into the file at path through a memory map. */
static int generate_into(const wchar_t *path, verthash_progress_fn progress, void *ctx)
{
    HANDLE file, mapping;
    LARGE_INTEGER size;
    uint8_t *data;
    int err;

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return (int)GetLastError();
    /* not sparse: the disk space is allocated here, so a full disk fails now */
    size.QuadPart = VERTHASH_DATAFILE_SIZE;
    if (!SetFilePointerEx(file, size, NULL, FILE_BEGIN) || !SetEndOfFile(file)) {
        err = (int)GetLastError();
        CloseHandle(file);
        return err;
    }
    mapping = CreateFileMappingW(file, NULL, PAGE_READWRITE, (DWORD)(size.QuadPart >> 32),
                                 (DWORD)(size.QuadPart & 0xffffffff), NULL);
    if (mapping == NULL) {
        err = (int)GetLastError();
        CloseHandle(file);
        return err;
    }
    data = (uint8_t *)MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    /* the view keeps the mapping open */
    CloseHandle(mapping);
    if (data == NULL) {
        err = (int)GetLastError();
        CloseHandle(file);
        return err;
    }
    err = verthash_generate_datafile(data, progress, ctx);
    if (!FlushViewOfFile(data, 0) && !err)
        err = (int)GetLastError();
    UnmapViewOfFile(data);
    if (!err && !FlushFileBuffers(file))
        err = (int)GetLastError();
    CloseHandle(file);
    return err;
}

int verthash_create_datafile(const wchar_t *path, verthash_progress_fn progress, void *ctx)
{
    size_t n = wcslen(path);
    wchar_t *tmp = (wchar_t *)malloc((n + 5) * sizeof(wchar_t));
    int err;

    if (tmp == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    memcpy(tmp, path, n * sizeof(wchar_t));
    memcpy(tmp + n, L".tmp", 5 * sizeof(wchar_t));
    err = generate_into(tmp, progress, ctx);
    if (!err && !MoveFileExW(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        err = (int)GetLastError();
    if (err)
        DeleteFileW(tmp);
    free(tmp);
    return err;
}

#else

/* Generates into the file at path through a memory map. */
static int generate_into(const char *path, verthash_progress_fn progress, void *ctx)
{
    const off_t size = (off_t)VERTHASH_DATAFILE_SIZE;
    uint8_t *data;
    int fd, err;

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return errno;
#if defined(__linux__)
    /* Allocate the disk space now: writing to a sparse mapping on a full disk
     * would kill the process with SIGBUS. Some filesystems cannot. */
    err = posix_fallocate(fd, 0, size);
    if (err == EOPNOTSUPP || err == EINVAL)
        err = 0;
    if (err) {
        close(fd);
        return err;
    }
#endif
    if (ftruncate(fd, size) != 0) {
        err = errno;
        close(fd);
        return err;
    }
    data = (uint8_t *)mmap(NULL, (size_t)size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        err = errno;
        close(fd);
        return err;
    }
    err = verthash_generate_datafile(data, progress, ctx);
    if (msync(data, (size_t)size, MS_SYNC) != 0 && !err)
        err = errno;
    munmap(data, (size_t)size);
    if (!err && fsync(fd) != 0)
        err = errno;
    if (close(fd) != 0 && !err)
        err = errno;
    return err;
}

int verthash_create_datafile(const char *path, verthash_progress_fn progress, void *ctx)
{
    size_t n = strlen(path);
    char *tmp = (char *)malloc(n + 5);
    int err;

    if (tmp == NULL)
        return ENOMEM;
    memcpy(tmp, path, n);
    memcpy(tmp + n, ".tmp", 5);
    err = generate_into(tmp, progress, ctx);
    if (!err && rename(tmp, path) != 0)
        err = errno;
    if (err)
        unlink(tmp);
    free(tmp);
    return err;
}

#endif
