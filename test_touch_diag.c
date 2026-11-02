#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "cepheus_touch_diag.h"
int main(void)
{
    struct cepheus_touch_diag d = {0};
    uint8_t e[8] = {0};
    assert(!cepheus_touch_diag_heartbeat(&d));
    assert(!cepheus_touch_diag_event(&d, e));
    assert(d.empty == 1 && d.events == 0);
    e[0] = 0x03;
    assert(cepheus_touch_diag_event(&d, e));
    e[0] = 0xf3;
    assert(cepheus_touch_diag_event(&d, e));
    assert(d.ready == 1 && d.controller_errors == 1);
    e[0] = 0x13; e[1] = 0x21; e[2] = 0x34; e[3] = 0xa2;
    uint8_t original[8]; memcpy(original, e, 8);
    for (unsigned i = 0; i < 1000; ++i) {
        d.polls = i + 1;
        int show = cepheus_touch_diag_event(&d, e);
        assert(show == (i < 62));
        assert(cepheus_touch_diag_heartbeat(&d) == (i == 0 || !((i + 1) & 255u)));
    }
    assert(d.raw_lines == 64 && d.events == 1002);
    assert(memcmp(e, original, 8) == 0 && memcmp(d.last, original, 8) == 0);
    return 0;
}

