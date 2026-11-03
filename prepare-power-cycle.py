"""Allow only boolean L1C XOB votes in the existing bounded RPMh transport."""
import difflib
from pathlib import Path
import subprocess
import sys
import tempfile

TARGET = 'bsd/dev/sweet_rpmh.c'
def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)

def transform(before):
    after = replace(before, 'int cepheus_touch_power_enable(void);',
                    'int cepheus_touch_power_enable(void);\nint cepheus_touch_power_set(int enabled);')
    after = replace(after, 'rh_send(uint32_t address, unsigned mv, bool xob)\n{',
                    'rh_send(uint32_t address, unsigned mv, bool xob)\n{\n\tif (xob && mv > 1u) { return EINVAL; }')
    after = replace(after, 'const uint32_t values[3] = {xob ? 1u : mv, 7, 1};',
                    'const uint32_t values[3] = {mv, 7, 1};')
    after = replace(after, 'CEPHEUS POWER: ldoc1 XOB enable only address=%#x data=1\\n", address + 4',
                    'CEPHEUS POWER: ldoc1 XOB enable-bit address=%#x data=%u\\n", address + 4, mv')
    after = replace(after, 'cepheus_touch_power_enable(void)\n{',
                    'cepheus_touch_power_set(int enabled)\n{\n\tif (enabled != 0 && enabled != 1) { return EINVAL; }')
    after = replace(after, 'error = rh_send(address, 0, true);',
                    'error = rh_send(address, (unsigned)enabled, true);')
    after = replace(after, 'CEPHEUS POWER: ldoc1 enable request result=%d\\n", error',
                    'CEPHEUS POWER: ldoc1 set=%d request result=%d\\n", enabled, error')
    after = replace(after, 'Only ACTIVE enable=1 is sent. No direct SPMI, sleep or wake writes.',
                    'Only ACTIVE enable=0/1 is sent. No voltage/mode, SPMI or sleep/wake writes.')
    after = replace(after, '#endif /* ARM64_BOARD_CONFIG_SWEET */', '''
int
cepheus_touch_power_enable(void)
{
    return cepheus_touch_power_set(1);
}
#endif /* ARM64_BOARD_CONFIG_SWEET */''')
    # XOB sends one command to base+4; the VRM voltage/mode/enable path and
    # idle-slot checks, ACK timeout and quarantine logic remain intact.
    assert 'const unsigned ncmd = xob ? 1u : 3u;' in after
    assert 'const uint32_t offsets[3] = {xob ? 4u : 0u, 8, 4};' in after
    assert 'rh_send(address, mv, false)' in after
    return after

def main():
    patches, power, inherited, output = map(Path, sys.argv[1:])
    if output.exists():
        raise ValueError('Refusing to overwrite output')
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        subprocess.run(['git', 'init', '-q', str(root)], check=True)
        for patch in [*sorted(patches.glob('*.patch')), power, inherited]:
            subprocess.run(['git', '-C', str(root), 'apply', '--whitespace=nowarn',
                            f'--include={TARGET}', str(patch.resolve())], check=True)
        before = (root / TARGET).read_text()
        after = transform(before)
        output.write_text(''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                                    fromfile='a/' + TARGET, tofile='b/' + TARGET)))
        subprocess.run(['git', '-C', str(root), 'apply', '--check', str(output.resolve())], check=True)
    print('L1C boolean enable-bit patch generated and apply-checked')

if __name__ == '__main__':
    main()
