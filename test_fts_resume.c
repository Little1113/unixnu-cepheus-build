#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include "cepheus_fts_resume.h"

struct fake {
    unsigned resets, waits, elapsed, reads, scans, ready_at;
    int read_error, scan_error;
};
static void reset(void *ctx, int high)
{
    struct fake *f = ctx;
    assert(f->scans == 0 && f->reads == 0);
    assert((f->resets == 0 && high == 0 && f->elapsed == 0) ||
           (f->resets == 1 && high == 1 && f->elapsed == 10));
    ++f->resets;
}
static void wait_ms(void *ctx, unsigned ms)
{
    struct fake *f = ctx;
    f->elapsed += ms;
    ++f->waits;
}
static int read_event(void *ctx, uint8_t e[8])
{
    struct fake *f = ctx;
    assert(f->resets == 2 && f->elapsed >= 310 && f->scans == 0);
    ++f->reads;
    if (f->read_error) { return f->read_error; }
    /* Include the observed startup error before Ready. It is logged by
     * the caller, not treated as proof that the controller is ready. */
    e[0] = f->reads == f->ready_at ? 0x03 : (f->reads == 1 ? 0xf3 : 0);
    return 0;
}
static int scan(void *ctx, int enabled)
{
    struct fake *f = ctx;
    assert(enabled == 1 && f->reads == f->ready_at && f->ready_at > 0);
    ++f->scans;
    return f->scan_error;
}
static int run(struct fake *f)
{
    const struct cepheus_fts_resume_ops ops = {f, reset, wait_ms, read_event, scan};
    return cepheus_fts_resume(&ops);
}
int main(void)
{
    struct fake f = {.ready_at = 2};
    assert(run(&f) == 0 && f.scans == 1 && f.elapsed == 320);
    f = (struct fake){.ready_at = 100};
    assert(run(&f) == 0 && f.reads == 100 && f.scans == 1);
    f = (struct fake){0};
    assert(run(&f) == ETIMEDOUT && f.scans == 0 && f.reads == 100 && f.elapsed == 1310);
    f = (struct fake){.read_error = EIO};
    assert(run(&f) == EIO && f.scans == 0 && f.reads == 1);
    f = (struct fake){.ready_at = 1, .scan_error = EBUSY};
    assert(run(&f) == EBUSY && f.scans == 1);
    return 0;
}
