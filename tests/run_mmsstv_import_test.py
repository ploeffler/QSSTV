"""Run the native import/save/reload/render integration test in a qmake build.
Usage: python3 tests/run_mmsstv_import_test.py /path/to/build [--reference /path/to/mmsstv]
The build must contain a generated QSSTV Makefile and its dependencies.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('build', type=Path)
p.add_argument('--reference', type=Path)
args = p.parse_args()
build = args.build.resolve()
source = Path(__file__).with_name('test_mmsstv_import.cpp').resolve()
if (build / 'test_mmsstv_import.o').exists() and not (build / 'test_mmsstv_import.d').exists():
    (build / 'test_mmsstv_import.o').unlink()
recipe = '''include Makefile
-include test_mmsstv_import.d
.PHONY: verify-mmsstv-import
verify-mmsstv-import: $(filter-out main.o,$(OBJECTS)) test_mmsstv_import.o
\t$(LINK) $(LFLAGS) -fno-lto -o test_mmsstv_import test_mmsstv_import.o $(filter-out main.o,$(OBJECTS)) $(OBJCOMP) $(LIBS)
test_mmsstv_import.o: $(IMPORT_TEST_SOURCE) Makefile
\t$(CXX) -c $(CXXFLAGS) $(DEFINES) $(INCPATH) -MMD -MP -MF test_mmsstv_import.d -o $@ "$(IMPORT_TEST_SOURCE)"
'''
with tempfile.NamedTemporaryFile(mode='w', suffix='.mk') as f:
    f.write(recipe)
    f.flush()
    subprocess.run(['make', '-j2', '-f', f.name, 'verify-mmsstv-import',
                    'IMPORT_TEST_SOURCE=' + str(source)], cwd=build, check=True)
env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
command = [str(build / 'test_mmsstv_import')]
if args.reference:
    command.append(str(args.reference.resolve()))
subprocess.run(command, cwd=build, env=env, check=True)
