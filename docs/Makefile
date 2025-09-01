# Makefile for Sphinx documentation
#
# Copyright 2022 Stéphane Caron

DOXYGEN = doxygen
OUTPUTDIR = html

.PHONY: html
html:  ## build the documentation
	(cd .. && $(DOXYGEN) $(CURDIR)/Doxyfile)

.PHONY: clean
clean:  ## remove all generated files
	rm -rf $(OUTPUTDIR)

.PHONY: open
open:  ## open the locally built documentation in Firefox
	xdg-open $(OUTPUTDIR)/index.html
