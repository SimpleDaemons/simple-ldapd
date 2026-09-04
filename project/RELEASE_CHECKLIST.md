# Release checklist — simple-ldapd

Use before cutting a version tag.

- [ ] `VERSION`, `CMakeLists.txt` `project(... VERSION ...)`, and `include/simple-ldapd/version.hpp` match
- [ ] [CHANGELOG.md](../CHANGELOG.md) has a dated section for the release
- [ ] [project/PROGRESS_REPORT.md](PROGRESS_REPORT.md) reflects what actually ships
- [ ] Packaging units use installed binary + `/etc/simple-ldapd/simple-ldapd.conf` + `--foreground`
- [ ] `cmake -B build -DENABLE_TESTS=ON && cmake --build build && ctest --test-dir build`
- [ ] `./build/simple-ldapd --version` and `--test-config` succeed
- [ ] Tag annotated: `git tag -a vX.Y.Z -m "Release vX.Y.Z"`
- [ ] GitHub Release created from the tag
- [ ] Monorepo submodule pin updated when publishing into SimpleDaemons
