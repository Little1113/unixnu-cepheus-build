#include <assert.h>
#include <errno.h>
#include "cepheus_fts_powercycle.h"
struct fake { unsigned step, elapsed; int disable_error, enable_error; };
static int l1c(void *ctx, int enabled)
{
    struct fake *f = ctx;
    if (!enabled) {
        assert(f->step == 0 && f->elapsed == 0);
        ++f->step;
        return f->disable_error;
    }
    assert(f->step == 4 && f->elapsed == 300);
    ++f->step;
    return f->enable_error;
}
static void vddio(void *ctx, int enabled)
{
    struct fake *f = ctx;
    assert((!enabled && f->step == 1) || (enabled && f->step == 6 && f->elapsed == 301));
    ++f->step;
}
static void reset(void *ctx, int high)
{
    struct fake *f = ctx;
    assert((!high && f->step == 2) || (high && f->step == 10 && f->elapsed == 320));
    ++f->step;
}
static void wait_ms(void *ctx, unsigned ms)
{
    struct fake *f = ctx;
    assert((f->step == 3 && ms == 300) || (f->step == 5 && ms == 1) ||
           (f->step == 7 && ms == 4) || (f->step == 8 && ms == 5) ||
           (f->step == 9 && ms == 10) || (f->step == 11 && ms == 300));
    f->elapsed += ms;
    ++f->step;
}
static int run(struct fake *f)
{
    const struct cepheus_fts_powercycle_ops ops = {f, l1c, vddio, reset, wait_ms};
    return cepheus_fts_powercycle(&ops);
}
int main(void)
{
    struct fake f = {0};
    assert(run(&f) == 0 && f.step == 12 && f.elapsed == 620);
    f = (struct fake){.disable_error = EIO};
    assert(run(&f) == EIO && f.step == 1 && f.elapsed == 0);
    f = (struct fake){.enable_error = ETIMEDOUT};
    assert(run(&f) == ETIMEDOUT && f.step == 5 && f.elapsed == 300);
    return 0;
}
