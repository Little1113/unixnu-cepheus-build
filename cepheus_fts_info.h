/* Read-only FTM5 system-info layout from MiCode cepheus-p-oss ftsCore.c.
 * Caller supplies integer types and bool. No controller commands here.
 */
#ifndef CEPHEUS_FTS_INFO_H
#define CEPHEUS_FTS_INFO_H
#define CEPHEUS_SYSINFO_SIZE 208u
#define CEPHEUS_CHANNEL_LIMIT 64u
struct cepheus_fts_info {
    uint16_t fw, cfg, project, cx, cx_project, x, y, debug_addr;
    uint8_t cfg_afe, cx_afe, panel_afe, protocol, tx, rx;
    uint32_t fw_crc, cfg_crc;
    uint16_t ms_raw, ms_strength, ss_tx_raw, ss_tx_strength, ss_rx_raw, ss_rx_strength;
};
struct cepheus_fts_stats {
    int min, max;
    uint32_t hash;
};
static uint16_t cepheus_info_u16(const uint8_t *p)
{
    return (uint16_t)((unsigned)p[0] | ((unsigned)p[1] << 8));
}
static uint32_t cepheus_info_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static bool cepheus_info_parse(const uint8_t *p, unsigned n, struct cepheus_fts_info *out)
{
    if (!p || !out || n != CEPHEUS_SYSINFO_SIZE || p[0] != 0xa5 || p[1] != 1) {
        return false;
    }
    struct cepheus_fts_info v = {
        .fw = cepheus_info_u16(p + 16), .cfg = cepheus_info_u16(p + 20),
        .project = cepheus_info_u16(p + 22), .cx = cepheus_info_u16(p + 24),
        .cx_project = cepheus_info_u16(p + 26),
        .cfg_afe = p[28], .cx_afe = p[29], .panel_afe = p[30], .protocol = p[31],
        .fw_crc = cepheus_info_u32(p + 64), .cfg_crc = cepheus_info_u32(p + 68),
        .x = cepheus_info_u16(p + 80), .y = cepheus_info_u16(p + 82),
        .tx = p[84], .rx = p[85], .debug_addr = cepheus_info_u16(p + 128),
        .ms_raw = cepheus_info_u16(p + 136), .ms_strength = cepheus_info_u16(p + 140),
        .ss_tx_raw = cepheus_info_u16(p + 144), .ss_tx_strength = cepheus_info_u16(p + 148),
        .ss_rx_raw = cepheus_info_u16(p + 152), .ss_rx_strength = cepheus_info_u16(p + 156)
    };
    /* Still print versions when firmware reports bad geometry; the caller
     * validates each read range separately and never trusts firmware lengths. */
    *out = v;
    return true;
}
static bool cepheus_info_range(uint16_t addr, unsigned words)
{
    return addr >= CEPHEUS_SYSINFO_SIZE && words > 0 && words <= CEPHEUS_CHANNEL_LIMIT &&
           (uint32_t)addr + 2u * words <= 0x10000u;
}
static int cepheus_info_s16(const uint8_t *p)
{
    unsigned v = cepheus_info_u16(p);
    return v >= 0x8000u ? (int)v - 65536 : (int)v;
}
static bool cepheus_info_stats(const uint8_t *p, unsigned bytes, struct cepheus_fts_stats *out)
{
    if (!p || !out || bytes == 0 || (bytes & 1u) || bytes > 2u * CEPHEUS_CHANNEL_LIMIT) {
        return false;
    }
    struct cepheus_fts_stats v = {32767, -32768, 2166136261u};
    for (unsigned i = 0; i < bytes; i += 2) {
        int value = cepheus_info_s16(p + i);
        if (value < v.min) { v.min = value; }
        if (value > v.max) { v.max = value; }
        v.hash = (v.hash ^ p[i]) * 16777619u;
        v.hash = (v.hash ^ p[i + 1]) * 16777619u;
    }
    *out = v;
    return true;
}
#endif
