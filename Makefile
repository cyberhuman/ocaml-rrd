OCAMLMAKEFILE = OCamlMakefile

SOURCES = rrd_c.c rrd.ml
RESULT  = rrd
THREADS = yes
CLIBS = rrd_th

LIBINSTALL_FILES = \
	rrd.cmi \
	rrd.cmx \
	rrd.cma \
	rrd.a \
	rrd.cmxa \
	librrd_stubs.a \
	dllrrd_stubs.so

OCAMLFLAGS=-g -w Aez -warn-error Aez
OCAMLLDFLAGS=-g
CFLAGS = -Wall -Werror -g -ggdb -O2 $(RPM_OPT_FLAGS)

# XXX: This is in order to install the lib into the correct /tmp dir when
# rpmbuild-ing. This might be broken now, test and fix(?)
ifdef DISTLIBDIR
	OCAMLDISTLIBDIR=$(DISTLIBDIR)/ocaml
    OCAMLFIND_INSTFLAGS = -destdir $(OCAMLDISTLIBDIR)
endif

all: byte-code-library native-code-library

cleanlibs:
	ocamlfind remove $(RESULT)
	if [ "$(OCAMLDISTLIBDIR)" != "" ]; then mkdir -p $(OCAMLDISTLIBDIR); fi

install: cleanlibs libinstall

include $(OCAMLMAKEFILE)
