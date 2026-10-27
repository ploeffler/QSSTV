"""Exercise production serial PTT mask operations without radio hardware.

Run: python3 tests/test_serial_ptt.py (requires a C++ compiler).
The three GET/SET blocks are extracted from rigcontrol.cpp and executed with
all initial DTR/RTS states and all non-conflicting polarity configurations.
"""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SerialPTTTest(unittest.TestCase):
    def test_initial_idle_transmit_and_release(self):
        source = (ROOT / 'src/rig/rigcontrol.cpp').read_text()
        blocks = re.findall(r'ioctl\(serialP,TIOCMGET,&modemlines\);(.*?)'
                            r'ioctl\(serialP,TIOCMSET,&modemlines\);', source, re.S)
        self.assertEqual(len(blocks), 3)
        functions = '\n'.join('int operation%d(int modemlines, Params catParams) {%s\nreturn modemlines;}'
                              % (index, block) for index, block in enumerate(blocks))
        program = r'''
#include <sys/ioctl.h>
#include <iostream>
struct Params { bool activeDTR, activeRTS, nactiveDTR, nactiveRTS; };
''' + functions + r'''
int main() {
  using Operation = int (*)(int, Params);
  Operation operations[] = {operation0, operation1, operation2};
  const int unrelated = 0x40000000;
  for (int op=0; op<3; ++op)
    for (int dtr=0; dtr<3; ++dtr)
      for (int rts=0; rts<3; ++rts)
        for (int initial=0; initial<4; ++initial) {
          Params p = {dtr==1, rts==1, dtr==2, rts==2};
          int input = unrelated | ((initial&1) ? TIOCM_DTR : 0)
                                | ((initial&2) ? TIOCM_RTS : 0);
          int expected = input;
          const int masks[] = {TIOCM_DTR, TIOCM_RTS};
          const int polarities[] = {dtr,rts};
          for (int line=0; line<2; ++line) {
            if (polarities[line]==0) continue;
            bool high = (polarities[line]==1) ? (op==1) : (op!=1);
            if (high) expected |= masks[line];
            else expected &= ~masks[line];
          }
          int result = operations[op](input,p);
          if (result!=expected) {
            std::cerr << "PTT mismatch: op=" << op << " dtr=" << dtr
                      << " rts=" << rts << " initial=" << initial << '\n';
            return 1;
          }
        }
}
'''
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / 'test.cpp'
            binary = Path(directory) / 'test'
            cpp.write_text(program)
            subprocess.run(['c++', '-std=c++11', '-Wall', '-Wextra', '-Werror',
                            str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
