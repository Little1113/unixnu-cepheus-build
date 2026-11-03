/* Board-specific supply recovery, based on MiCode cepheus-p-oss
 * fts_chip_powercycle after resolving the inverted C regulator names:
 * vdd_reg = DT avdd = L1C; avdd_reg = DT vdd = GPIO84 supply.
 * The 300-ms off dwell uses the vendor's resetless recovery dwell as a
 * conservative experiment; it is not claimed as a measured minimum.
 * Caller serializes this before claiming the GPIO I2C bus at boot.
 */
#ifndef CEPHEUS_FTS_POWERCYCLE_H
#define CEPHEUS_FTS_POWERCYCLE_H
struct cepheus_fts_powercycle_ops {
    void *ctx;
    int (*l1c)(void *, int enabled);
    void (*vddio)(void *, int enabled);
    void (*reset)(void *, int high);
    void (*wait_ms)(void *, unsigned ms);
};
static int cepheus_fts_powercycle(const struct cepheus_fts_powercycle_ops *ops)
{
    int err = ops->l1c(ops->ctx, 0);
    if (err) { return err; }
    ops->vddio(ops->ctx, 0);
    ops->reset(ops->ctx, 0);
    ops->wait_ms(ops->ctx, 300);
    err = ops->l1c(ops->ctx, 1);
    /* If enabling is uncertain, leave reset asserted and GPIO84 disabled;
     * do not feed the unpowered controller through its bus-supply pins. */
    if (err) { return err; }
    ops->wait_ms(ops->ctx, 1);
    ops->vddio(ops->ctx, 1);
    ops->wait_ms(ops->ctx, 4);  /* live DT regulator startup-delay-us=4000 */
    ops->wait_ms(ops->ctx, 5);
    ops->wait_ms(ops->ctx, 10);
    ops->reset(ops->ctx, 1);
    ops->wait_ms(ops->ctx, 300);
    return 0;
}
#endif
