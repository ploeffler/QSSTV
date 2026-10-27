"""Optional regression for the user-supplied KG-STV image or text MP3.
Usage: python3 tests/run_kgstv_reference_test.py BUILD RECORDING_MP3 OUTPUT_FILE
Requires ffmpeg; recording is not distributed or downloaded. No devices are opened.
"""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile

build, recording, output = (Path(p).resolve() for p in sys.argv[1:])
fixtures = {
    '6f65c15ba9065df2e0c781bbe9b3c19d890079cdc77985b96140e58b445613e7': 'image',
    'aa79e18f41f1172d88648f863e759c8fba530772f921a562fb314ace57f492f7': 'text',
}
kind = fixtures.get(hashlib.sha256(recording.read_bytes()).hexdigest())
if kind is None:
    raise SystemExit('Unexpected reference recording; this test is fixture-specific.')
if output.exists() or output.is_symlink():
    raise SystemExit('Output already exists.')
subprocess.run(['make', '-j2'], cwd=build, check=True)
source = Path(__file__).with_name('test_kgstv_reference.cpp').resolve()
recipe = '''include Makefile
-include test_kgstv_reference.d
.PHONY: verify-reference
verify-reference: $(filter-out main.o,$(OBJECTS)) test_kgstv_reference.o
\t$(LINK) $(LFLAGS) -fno-lto -o test_kgstv_reference test_kgstv_reference.o $(filter-out main.o,$(OBJECTS)) $(OBJCOMP) $(LIBS)
test_kgstv_reference.o: $(KGSTV_REFERENCE_SOURCE) Makefile
\t$(CXX) -c $(CXXFLAGS) $(DEFINES) $(INCPATH) -MMD -MP -MF test_kgstv_reference.d -o $@ "$(KGSTV_REFERENCE_SOURCE)"
'''
with tempfile.TemporaryDirectory(prefix='kgstv-reference-') as temp:
    temp = Path(temp)
    (temp / 'test.mk').write_text(recipe)
    subprocess.run(['make', '-j2', '-f', str(temp / 'test.mk'), 'verify-reference',
                    'KGSTV_REFERENCE_SOURCE=' + str(source)], cwd=build, check=True)
    wav = temp / 'reference.wav'
    subprocess.run(['ffmpeg', '-nostdin', '-v', 'error', '-i', str(recording),
                    '-ac', '1', '-ar', '12000', '-c:a', 'pcm_s16le', str(wav)], check=True)
    subprocess.run([str(build / 'test_kgstv_reference'), str(wav), str(output), kind], check=True)
