"""Run the device-free KG-STV RX/TX widget test with production application objects.
Usage: python3 tests/run_kgstv_ui_test.py /path/to/build
The build must contain a generated QSSTV Makefile and its dependencies.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('build', type=Path)
args = p.parse_args()
build = args.build.resolve()
source = Path(__file__).with_name('test_kgstv_ui.cpp').resolve()
if (build / 'test_kgstv_ui.o').exists() and not (build / 'test_kgstv_ui.d').exists():
    (build / 'test_kgstv_ui.o').unlink()
recipe = '''include Makefile
-include test_kgstv_ui.d
.PHONY: verify-kgstv-ui
verify-kgstv-ui: $(filter-out main.o,$(OBJECTS)) test_kgstv_ui.o
\t$(LINK) $(LFLAGS) -fno-lto -o test_kgstv_ui test_kgstv_ui.o $(filter-out main.o,$(OBJECTS)) $(OBJCOMP) $(LIBS)
test_kgstv_ui.o: $(KGSTV_UI_TEST_SOURCE) Makefile
\t$(CXX) -c $(CXXFLAGS) $(DEFINES) $(INCPATH) -MMD -MP -MF test_kgstv_ui.d -o $@ "$(KGSTV_UI_TEST_SOURCE)"
'''
with tempfile.NamedTemporaryFile(mode='w', suffix='.mk') as f:
    f.write(recipe)
    f.flush()
    subprocess.run(['make', '-j2', '-f', f.name, 'verify-kgstv-ui',
                    'KGSTV_UI_TEST_SOURCE=' + str(source)], cwd=build, check=True)
env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
command = [str(build / 'test_kgstv_ui')]
subprocess.run(command, cwd=build, env=env, check=True)
