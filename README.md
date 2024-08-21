This is a light fork of Sourcetrail, intended only to fix issues related to building and running on Arch Linux. The result is packaged at [`aur/sourcetrail`](https://aur.archlinux.org/packages/sourcetrail).

### Organization

This project is organized as follows:

**Tags**

* Tags tend to follow a date-based format:
  * `YYYY.C` (year.count)
  * `YYYY.M.C` (year.month.count)
* Old tags that don't follow the above format have been deleted.
* New tags may not match upstream tags.  They are created in the `petermost` branch

**Branches**

* `main` - Contains this document and license (GPL-3.0-only).
* `petermost` – Synced with `PeterMost/SourceTrail/master`.
* `arch-xyz` – Based on tag `xyz` with rebased patches to build on Arch Linux.
* `wip` – Staging for new `arch-xyz` branches or other testing.  Frequently rebased and force pushed.  Not for general use.

### Issues

Open issues here only for problems that are specific to Arch-Linux or AUR packaging.  All other problems are out of scope and should be directed upstream.

* [PeterMost/SourceTrail](https://github.com/PeterMost/SourceTrail).  Created in 2023 to fix issues with building on newer toolchains.  The scope of the fork expanded in 2024.  However, development makes assumptions that do not hold for all Linux distros.  Before opening new issues, confirm the problem exists in release binaries.

* [CoatiSoftware/SourceTrail](https://github.com/CoatiSoftware/SourceTrail).  The original project.  Discontinued Dec 2021.  Do not open new issues.
