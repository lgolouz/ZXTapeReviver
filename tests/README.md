# Save-file regression tests

Build out of tree with the same Qt kit and FFTW dependency as the application:

```powershell
mkdir build/savefiles-tests
cd build/savefiles-tests
qmake ../../savefiles-tests.pro
mingw32-make -j8
./release/savefiles-tests.exe -o results.txt,txt
```

The Qt, compiler runtime and FFTW DLL directories must be on `PATH` on Windows.
For headless execution, set `QT_QPA_PLATFORM=offscreen`.

Coverage:

- TAP little-endian lengths, replacement, all/selected blocks.
- Maximum TAP length (65535), overflow (65536), empty/null blocks.
- Failed validation and failed replacement preserve existing files.
- WFM chunked sample serialization, suspicious points and round-trip loading.
- WFM errors propagate through FileWorkerModel; enum values are accessible in QML.
- Windows destination locks exercise failed commit for both formats.

Disk-full/short-write fault injection is not part of this suite. The Windows
sharing-lock test is skipped on other platforms. Tests use temporary files only.

# Undo/redo regression tests

Build `actions-tests.pro` in a separate build directory using the same commands
and environment as above, then run `./release/actions-tests.exe -o results.txt,txt`.

Coverage:

- Undo/redo and new edits in the same document, including separate channels.
- WAV/TAP/WFM loading and reopening the same file clear undo and redo histories.
- Model reset and action-state notifications, checked with QAbstractItemModelTester.
- Failed file opens preserve history; retained actions cannot modify a new document.
- Invalid channels and sample indices, including undo after a channel shrinks.

# WAV decoding regression tests

Build `wavreader-tests.pro` in a separate build directory and run
`./release/wavreader-tests.exe -o results.txt,txt` with the same environment.

- Matching internal amplitudes for PCM8/16/24/32 and float32, mono and stereo.
- Signed extrema, zero and fractional low-level PCM24/32 samples.
- Float headroom is preserved; NaN, infinity and scale overflow are rejected.
- Invalid float bit depths and incomplete interleaved frames are rejected.
- WFM round trips preserve decoded internal amplitudes without scaling twice.

Internal samples use PCM16 amplitude units stored as floats. Existing WFM files
remain raw sample snapshots and are not automatically rescaled: old files may
already contain edited data in the former format-dependent scale. Reimport the
original WAV to obtain corrected decoding for those files.
