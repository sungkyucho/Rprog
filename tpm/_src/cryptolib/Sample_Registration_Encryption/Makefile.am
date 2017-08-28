AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
CLEANFILES = newsecret.key.blob secret.key.blob
CLEANFILES += cscope.in.out cscope.out cscope.po.out cscope.files
DISTCLEANFILES = tests/privkey.pem tests/pubkey.pem
SUBDIRS = GMMP_lib src tests samples

if HAVE_DOXYGEN

doxyfile.stamp:
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

CLEANFILES += doxyfile.stamp

all-local: doxyfile.stamp
html-local: doxyfile.stamp
pdf-local: doxyfile.stamp
	sed -i -e "s/hfont/kotexutf/" docs/latex/refman.tex
	make -C docs/latex pdf
clean-local:
	rm -rf $(top_srcdir)/docs/html
	rm -rf $(top_srcdir)/docs/latex
endif
