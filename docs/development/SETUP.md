# Setup

1. Clone the repository (or use the SimpleDaemons submodule at `projects/simple-ldapd`).
2. Install dependencies from [BUILD_GUIDE.md](BUILD_GUIDE.md).
3. `cmake -B build -DENABLE_TESTS=ON && cmake --build build`
4. Validate a template: `./build/simple-ldapd --test-config -c config/templates/development.conf`
5. Optional: copy `config/templates/development.conf` and point `schema_dir` at `schemas/`.

Default lab ports are **3389** (LDAP) and **6636** (LDAPS) so the daemon can bind without root. Production templates use 389/636.
