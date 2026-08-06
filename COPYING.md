# Copying

escsafelib is free software licensed under the **GNU General Public License,
version 3 or later**.

    Copyright (C) 2022-2026 Engin Subasi

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program. If not, see <https://www.gnu.org/licenses/>.

## Every file carries it

This library is consumed by copying a header and source pair into another
project. A licence named only in a repository's documentation does not
travel with the pair, so each file states it:

```c
/* SPDX-License-Identifier: GPL-3.0-or-later */     /* in every header */
```

```
  * @par License                                     /* in every source banner */
  * SPDX-License-Identifier: GPL-3.0-or-later
```

`tools/doxcheck.py` checks that both are present, so a new module cannot
ship without one.

## The full text

**The verbatim licence text belongs in a file named `LICENSE` at the root
of this repository, and it is not here yet.**

That gap is deliberate rather than forgotten. The GPL is a legal document
and a copy of it is only the GPL if it is exact; a text reproduced from
memory that differs by a clause is a different licence with the same name.
It has to be the official copy, taken from:

    https://www.gnu.org/licenses/gpl-3.0.txt

Save it verbatim as `LICENSE`. Nothing else in this repository needs to
change: the SPDX identifiers already name the licence unambiguously, and
they are what a licence scanner and a downstream integrator read.

## What GPLv3 means for someone integrating this

Worth being direct, because this is a library aimed at products rather than
at other free software.

Copying a module into a product and shipping that product distributes a
work based on this library, and the GPL asks for the source of that work to
be available to whoever receives it. For a lot of embedded products that is
a decision to take deliberately and early rather than to discover at the
end.

If that does not suit the project, ask the copyright holder. A different
licence for a particular use is the copyright holder's to give and this
document cannot give it.
