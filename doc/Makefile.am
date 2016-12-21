# AM_MAKEINFOHTMLFLAGS = --no-headers --no-split
# info_TEXINFOS = uz80as.texi
dist_noinst_DATA = aenig4.h2m
dist_man_MANS = aenig4.1

aenig4.1: $(top_srcdir)/src/aenig4.c $(top_srcdir)/configure.ac $(top_srcdir)/doc/aenig4.h2m
	$(HELP2MAN) --no-info --name="Enigma M4 Cypher Machine Emulator" --include="$(top_srcdir)/doc/aenig4.h2m" --output=$@ $(top_builddir)/src/aenig4$(EXEEXT)
