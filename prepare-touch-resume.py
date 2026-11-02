"""Apply-check the display notification hook after pinned performance patch."""
import difflib
from pathlib import Path
import subprocess
import sys
import tempfile

FB = 'iokit/Kernel/arm/SweetFramebuffer.cpp'

def main():
    patches, performance, output = map(Path, sys.argv[1:])
    if output.exists():
        raise ValueError('Refusing to overwrite output')
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        subprocess.run(['git', 'init', '-q', str(root)], check=True)
        for patch in [*sorted(patches.glob('*.patch')), performance]:
            subprocess.run(['git', '-C', str(root), 'apply', '--whitespace=nowarn',
                            f'--include={FB}', str(patch.resolve())], check=True)
        before = (root / FB).read_text()
        anchor = '#include <pexpert/pexpert.h>'
        assert before.count(anchor) == 1
        after = before.replace(anchor, anchor + '\nextern "C" void cepheus_touch_display_power(int on);')
        anchor = '\t\t\tIOLockUnlock(fFB->fLock);\n\t\t\tIOLog("SWEET FB: display %s\\n", off ? "off" : "on");'
        assert after.count(anchor) == 1
        # Publish while still holding the framebuffer lock so concurrent
        # power requests cannot reorder the touch state. This is just an
        # atomic store; the touch worker performs all hardware operations.
        after = after.replace(anchor, '\t\t\tcepheus_touch_display_power(off ? 0 : 1);\n' + anchor)
        output.write_text(''.join(difflib.unified_diff(
            before.splitlines(True), after.splitlines(True),
            fromfile='a/' + FB, tofile='b/' + FB)))
        subprocess.run(['git', '-C', str(root), 'apply', '--check', str(output.resolve())], check=True)
    print('generated and apply-checked', output)

if __name__ == '__main__':
    main()
