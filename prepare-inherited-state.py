"""Add read-only inherited RPMh snapshots after the pinned 0084 patch."""
import difflib
from pathlib import Path
import subprocess
import sys
import tempfile

TARGET = 'bsd/dev/sweet_rpmh.c'
DUMP = '''
/* Read only the already mapped, validated normal TCS bank. This exposes
 * inherited ACTIVE/SLEEP/WAKE payloads without changing another owner's vote. */
static void
cepheus_dump_inherited_power(uint32_t address)
{
    for (unsigned t = 0; t < RH_NORMAL_N; ++t) {
        uint32_t b = t * RH_TCS_STRIDE;
        uint32_t enabled = rh_read(rh_tcs, b + RH_ENABLE);
        printf("CEPHEUS TCS: t=%u control=%#x enable=%#x wait=%#x status=%#x\\n",
            t, rh_read(rh_tcs, b + RH_CONTROL), enabled,
            rh_read(rh_tcs, b + RH_WAIT), rh_read(rh_tcs, b + RH_STATUS));
        for (unsigned c = 0; c < rh_ncpt; ++c) {
            uint32_t cmd = b + c * RH_CMD_STRIDE;
            if ((enabled & (1u << c)) &&
                (rh_read(rh_tcs, cmd + RH_ADDR) & ~15u) == address) {
                printf("CEPHEUS TCS LDOC1: t=%u c=%u addr=%#x data=%#x msg=%#x status=%#x\\n",
                    t, c, rh_read(rh_tcs, cmd + RH_ADDR), rh_read(rh_tcs, cmd + RH_DATA),
                    rh_read(rh_tcs, cmd + RH_MSGID), rh_read(rh_tcs, cmd + RH_CMD_STATUS));
            }
        }
    }
}
'''

def main():
    patches, power, output = map(Path, sys.argv[1:])
    if output.exists():
        raise ValueError('Refusing to overwrite output')
    assert 'rh_write(' not in DUMP and 'rh_sync(' not in DUMP and 'rh_send(' not in DUMP
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        subprocess.run(['git', 'init', '-q', str(root)], check=True)
        for patch in [*sorted(patches.glob('*.patch')), power]:
            subprocess.run(['git', '-C', str(root), 'apply', '--whitespace=nowarn',
                            f'--include={TARGET}', str(patch.resolve())], check=True)
        before = (root / TARGET).read_text()
        anchor = 'int\ncepheus_touch_power_enable(void)'
        assert before.count(anchor) == 1
        after = before.replace(anchor, DUMP + '\n' + anchor)
        anchor = '\t\tif (!error) { error = rh_send(address, 0, true); }'
        assert after.count(anchor) == 1
        after = after.replace(anchor, '\t\tif (!error) { cepheus_dump_inherited_power(address); }\n' + anchor)
        output.write_text(''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                                    fromfile='a/' + TARGET, tofile='b/' + TARGET)))
        subprocess.run(['git', '-C', str(root), 'apply', '--check', str(output.resolve())], check=True)
    print('Read-only power snapshot patch generated and apply-checked')

if __name__ == '__main__':
    main()
