#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "cepheus_fts_info.h"
int main(void)
{
    uint8_t data[208] = {0xa5, 1};
    data[16] = 0x60; data[20] = 0x37; data[22] = 0x3e;
    data[24] = 0x34; data[25] = 0x12;
    data[28] = 5; data[29] = 6; data[30] = 7; data[31] = 1;
    data[64] = 0x78; data[65] = 0x56; data[66] = 0x34; data[67] = 0x12;
    data[80] = 0x38; data[81] = 4; data[82] = 0x24; data[83] = 9;
    data[84] = 32; data[85] = 18;
    data[128] = 0x80; data[129] = 0xef;
    const unsigned offsets[] = {136, 140, 144, 148, 152, 156};
    for (unsigned i = 0; i < 6; ++i) { data[offsets[i]] = (uint8_t)i; data[offsets[i] + 1] = 0x10; }
    struct cepheus_fts_info info = {0};
    assert(cepheus_info_parse(data, sizeof(data), &info));
    assert(info.fw == 0x60 && info.cfg == 0x37 && info.project == 0x3e && info.cx == 0x1234);
    assert(info.cfg_afe == 5 && info.cx_afe == 6 && info.panel_afe == 7 && info.fw_crc == 0x12345678);
    assert(info.x == 1080 && info.y == 2340 && info.tx == 32 && info.rx == 18 && info.debug_addr == 0xef80);
    assert(info.ms_raw == 0x1000 && info.ms_strength == 0x1001 && info.ss_tx_raw == 0x1002 &&
           info.ss_tx_strength == 0x1003 && info.ss_rx_raw == 0x1004 && info.ss_rx_strength == 0x1005);
    struct cepheus_fts_info unchanged = info;
    for (unsigned n = 0; n < sizeof(data); ++n) { assert(!cepheus_info_parse(data, n, &info)); }
    data[0] = 0; assert(!cepheus_info_parse(data, sizeof(data), &info)); data[0] = 0xa5;
    data[1] = 2; assert(!cepheus_info_parse(data, sizeof(data), &info));
    assert(memcmp(&info, &unchanged, sizeof(info)) == 0);
    assert(cepheus_info_range(0xff80, 64) && cepheus_info_range(208, 1));
    assert(!cepheus_info_range(0xff81, 64) && !cepheus_info_range(0, 1));
    assert(!cepheus_info_range(208, 0) && !cepheus_info_range(208, 65));
    uint8_t values[] = {0, 0x80, 0xff, 0xff, 0, 0, 0xff, 0x7f};
    struct cepheus_fts_stats stats;
    assert(cepheus_info_stats(values, sizeof(values), &stats));
    assert(stats.min == -32768 && stats.max == 32767);
    assert(!cepheus_info_stats(values, 0, &stats) && !cepheus_info_stats(values, 7, &stats));
    assert(!cepheus_info_stats(values, 130, &stats));
    return 0;
}
