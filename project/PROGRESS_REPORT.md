# Progress report

v0.1.0 stands up a SimpleDaemons-shaped tree so protocol work can land without fighting packaging.

What works today: `--help` / `--version` / `--test-config`, in-memory backend CRUD in process, schema name loading from `.schema` files, and `ctest`.

What does not work: no client can bind or search on the wire. CLI tools other than help/version exit 2.
