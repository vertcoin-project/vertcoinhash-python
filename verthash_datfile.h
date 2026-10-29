#ifndef VERTHASH_DATFILE_INCLUDED
#define VERTHASH_DATFILE_INCLUDED

#include <stdint.h>

/* Size of verthash.dat, the Verthash proof-of-space datafile (graph index 17). */
#define VERTHASH_DATAFILE_SIZE 1283457024LL

/* How many times a generation reports its progress (and the final 1.0). */
#define VERTHASH_PROGRESS_STEPS 40

/* Called with the fraction done; a nonzero return stops the generation. */
typedef int (*verthash_progress_fn)(void *ctx, double fraction);

/* verthash_create_datafile's result when progress stopped it */
#define VERTHASH_STOPPED (-1)

/* Generates the datafile into buf, which holds VERTHASH_DATAFILE_SIZE bytes.
 * Same output as h1.c. Returns 0, or VERTHASH_STOPPED. progress may be NULL. */
int verthash_generate_datafile(uint8_t *buf, verthash_progress_fn progress, void *ctx);

/* Generates the datafile through a memory map of path + ".tmp", then renames
 * that to path. Returns 0, VERTHASH_STOPPED, or an errno value (GetLastError()
 * on Windows); the temporary file is removed unless it succeeded. */
#ifdef _WIN32
#include <wchar.h>
int verthash_create_datafile(const wchar_t *path, verthash_progress_fn progress, void *ctx);
#else
int verthash_create_datafile(const char *path, verthash_progress_fn progress, void *ctx);
#endif

#endif
