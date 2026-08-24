# BSD make (bmake) compatibility shim — full build rules live in GNUmakefile.
# FreeBSD: pkg install gmake  (then: gmake all, or make all via this shim)
#
# GNU make reads GNUmakefile first when present; bmake uses this file only.

GMAKE ?= gmake

# Default goal when invoked as plain "make"
all:
	@command -v $(GMAKE) >/dev/null 2>&1 || { \
		echo "ERROR: GNU Make ($(GMAKE)) is required." >&2; \
		echo "  FreeBSD: pkg install gmake && gmake all" >&2; \
		exit 1; \
	}
	@$(GMAKE) -f GNUmakefile all

# bmake: delegate any other target (build, test, package, …) to GNUmakefile
.DEFAULT:
	@command -v $(GMAKE) >/dev/null 2>&1 || { \
		echo "ERROR: GNU Make ($(GMAKE)) is required." >&2; \
		echo "  FreeBSD: pkg install gmake && gmake ${.TARGET}" >&2; \
		exit 1; \
	}
	@$(GMAKE) -f GNUmakefile ${.TARGET}
