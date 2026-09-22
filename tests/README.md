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
