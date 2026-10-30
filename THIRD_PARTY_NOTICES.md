# Licenses and sources

The project is distributed under GPL-2.0-or-later (`LICENSE`). The choice is
consistent with prior use of the slicer algorithm from rtl_433 in the model's
history. We do not claim rights to external authors' code.

- **rtl_433** — GPL-2.0-or-later; the `schraeder.c` decoder, Copyright
  Benjamin Larsson, Christian W. Zuckschwerdt and subsequent authors. Used as
  an independent decoding program for testing. Source and full history:
  <https://github.com/merbanan/rtl_433>. Test reference: commit
  `406f9a33550a2b6b3d0d8a8a486417ce5b3f13ab`.
- **rtl_433_tests** — the recordings and metadata belong to their authors;
  they are not copied into the firmware artifact or the release. The script
  fetches the pinned commit `1aa8c29d6b9383739d1aeed495dbc5069b704392`.
- **SmartRC-CC1101-Driver-Lib** — an external library fetched by PlatformIO;
  its original headers and license are preserved. Commit used:
  `de6757ef633b3e626dae5c7835cdb66ab53f9a1d`.
- **Arduino ESP32 / ESP-IDF / PlatformIO / Unity** — build/test dependencies
  under their own licenses; they are not covered by the attribution of this
  project.
- rtl_433 issue #2978 served only as a CC1101 integration pattern. The
  Renault protocol code is not EG53MA4 code and was not used as the protocol.

The full license texts of the dependencies are in their sources, fetched
during the build. This file does not narrow their rights or licenses.
