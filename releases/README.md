# SuperR7 releases

Public firmware downloads are distributed through versioned GitHub Releases.
The source repository retains only a small archive of early hardware-validation
packages whose hashes establish the beginning of the SuperR7 lineage.

See [`archive/`](archive/) for those historical packages and
[`docs/hardware-validation.md`](../docs/hardware-validation.md) for the complete
accepted lineage and current release gates.

The Luna release line uses tags such as `Luna1.2` and board-labelled firmware
assets such as `SuperR7-SD-Luna1.2.gba` and
`SuperR7-Chis-Luna1.2.gba`. Each release also includes a byte-identical `.fw`
copy for internal flashing, a checksum file, and build metadata. Chain-load
the `.gba` first and only use the `.fw` after that exact hash passes on the
matching cart. Internal phase names and
`*-hardware-test.gba` filenames are reserved for development history and
rollback records.
