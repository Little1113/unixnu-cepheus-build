/* ST-FTS resume sequence from MiCode cepheus-p-oss fts_resume_work and
 * fts_system_reset. Caller supplies uint8_t and errno definitions.
 * All callbacks run on the one touch worker; no controller flash writes.
 */
#ifndef CEPHEUS_FTS_RESUME_H
#define CEPHEUS_FTS_RESUME_H
struct cepheus_fts_resume_ops {
    void *ctx;
    void (*reset)(void *, int high);
    void (*wait_ms)(void *, unsigned);
    int (*read_event)(void *, uint8_t event[8]);
    int (*scan)(void *, int enabled);
};

static int
cepheus_fts_resume(const struct cepheus_fts_resume_ops *ops)
{
    ops->reset(ops->ctx, 0);
    ops->wait_ms(ops->ctx, 10);
    ops->reset(ops->ctx, 1);
    ops->wait_ms(ops->ctx, 300);
    /* Reset starts with scanning disabled. Consume startup/error events
     * until Controller Ready (0x03); never infer readiness from I2C ACK.
     */
    for (unsigned attempt = 0; attempt < 100; ++attempt) {
        uint8_t event[8] = {0};
        int err = ops->read_event(ops->ctx, event);
        if (err) { return err; }
        if (event[0] == 0x03) { return ops->scan(ops->ctx, 1); }
        ops->wait_ms(ops->ctx, 10);
    }
    return ETIMEDOUT;
}
#endif
