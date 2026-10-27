"""Run experimental native and WAV KG-STV tests in a built offline-tool directory.
Usage: python3 tests/run_kgstv_tests.py /path/to/kgstv-offline-build
Uses that qmake project's production objects; never opens radio/audio devices.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', type=Path)
args = parser.parse_args()
build = args.build.resolve()
source = Path(__file__).with_name('test_kgstv.cpp').resolve()
subprocess.run(['make', '-j2'], cwd=build, check=True)
for stem in ('test_kgstv', 'test_kgstv_controller'):
    if (build / (stem + '.o')).exists() and not (build / (stem + '.d')).exists():
        (build / (stem + '.o')).unlink()
recipe = '''include Makefile
-include test_kgstv.d test_kgstv_controller.d
.PHONY: verify-kgstv
verify-kgstv: $(filter-out main.o,$(OBJECTS)) test_kgstv.o test_kgstv_controller.o
\t$(LINK) $(LFLAGS) -fno-lto -o test_kgstv test_kgstv.o $(filter-out main.o,$(OBJECTS)) $(OBJCOMP) $(LIBS)
\t$(LINK) $(LFLAGS) -fno-lto -o test_kgstv_controller test_kgstv_controller.o $(filter-out main.o,$(OBJECTS)) $(OBJCOMP) $(LIBS)
test_kgstv_controller.o: $(KGSTV_CONTROLLER_TEST_SOURCE) Makefile
\t$(CXX) -c $(CXXFLAGS) $(DEFINES) $(INCPATH) -MMD -MP -MF test_kgstv_controller.d -o $@ "$(KGSTV_CONTROLLER_TEST_SOURCE)"
test_kgstv.o: $(KGSTV_TEST_SOURCE) Makefile
\t$(CXX) -c $(CXXFLAGS) $(DEFINES) $(INCPATH) -MMD -MP -MF test_kgstv.d -o $@ "$(KGSTV_TEST_SOURCE)"
'''
with tempfile.NamedTemporaryFile(mode='w', suffix='.mk') as temporary:
    temporary.write(recipe)
    temporary.flush()
    subprocess.run(['make', '-j2', '-f', temporary.name, 'verify-kgstv',
                    'KGSTV_TEST_SOURCE=' + str(source), 'KGSTV_CONTROLLER_TEST_SOURCE=' + str(source.with_name('test_kgstv_controller.cpp'))], cwd=build, check=True)
subprocess.run([str(build / 'test_kgstv')], cwd=build, check=True)
subprocess.run([str(build / 'test_kgstv_controller')], cwd=build, check=True)
subprocess.run([sys.executable, str(source.with_name('test_kgstv_offline.py')),
                str(build / 'kgstv-offline')], cwd=build, check=True)
