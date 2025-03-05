# Makefile for Sphinx documentation
#
# Copyright 2022 Stéphane Caron

# You can also set these variables from the command line.
DOXYGEN = doxygen
OUTPUTDIR = html

.PHONY: html
html:  ## build the documentation
	(cd .. && $(DOXYGEN) $(CURDIR)/Doxyfile)

.PHONY: clean
clean:  ## remove all generated files
	rm -rf $(OUTPUTDIR)
	rm -f $(wildcard images/*.eps) $(wildcard images/*.png)

.PHONY: open
open:  ## open the locally built documentation in Firefox
	xdg-open $(OUTPUTDIR)/index.html

# Help snippet from:
# http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help
help:
	@grep -P '^[a-zA-Z0-9_-]+:.*? ## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-24s\033[0m %s\n", $$1, $$2}'
