NAME = report
BIBTEXSRCS = report.bib
#TEXSRCS = plots/*.tex
OTHER = TCP.pdf UDP.pdf Channel.pdf

USE_PDFLATEX = 1
#PDFLATEX_FLAGS += -interaction=nonstopmode --synctex=1

BIBTEX = biber

VIEWPDF=xdg-open
VIEWPDF_FLAGS=
VIEWPDF_LANDSCAPE_FLAGS=

LATEX_MK_DIR ?= /usr/share/latex-mk
include $(LATEX_MK_DIR)/latex.gmk
