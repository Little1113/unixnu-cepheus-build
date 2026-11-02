/* Passive, bounded event accounting. No I/O or changes to touch decisions. */
#ifndef CEPHEUS_TOUCH_DIAG_H
#define CEPHEUS_TOUCH_DIAG_H
struct cepheus_touch_diag {
    uint64_t polls, events, empty, errors, ready, controller_errors;
    unsigned raw_lines;
    uint8_t last[8];
};
static int cepheus_touch_diag_event(struct cepheus_touch_diag *d, const uint8_t e[8])
{
    for (unsigned i = 0; i < 8; ++i) { d->last[i] = e[i]; }
    if (!e[0]) { ++d->empty; return 0; }
    ++d->events;
    if (e[0] == 0x03) { ++d->ready; }
    if (e[0] == 0xf3) { ++d->controller_errors; }
    if (d->raw_lines >= 64) { return 0; }
    ++d->raw_lines;
    return 1;
}
static int cepheus_touch_diag_heartbeat(const struct cepheus_touch_diag *d)
{
    return d->polls == 1 || (d->polls && !(d->polls & 255u));
}
#endif

