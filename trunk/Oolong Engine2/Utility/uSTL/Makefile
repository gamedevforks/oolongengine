include Config.mk

################ Source files ##########################################

SRCS	= $(wildcard *.cc)
INCS	= $(filter-out bsconf.%,$(wildcard *.h)) ustl.tbff
OBJS	= $(SRCS:.cc=.o)
DOCT	= ustldoc.in

################ Library link names ####################################

TOCLEAN	+= ${LIBSO} ${LIBA} ${LIBSOBLD}

ALLINST	= install-incs
ifdef BUILD_SHARED
ALLLIBS	+= ${LIBSOBLD}
ALLINST	+= install-shared
endif
ifdef BUILD_STATIC
ALLLIBS	+= ${LIBA}
ALLINST	+= install-static
endif

################ Compilation ###########################################

.PHONY:	all install uninstall install-incs uninstall-incs
.PHONY: install-static install-shared uninstall-static uninstall-shared
.PHONY:	clean depend dox html check dist distclean maintainer-clean

all:	${ALLLIBS}

%.o:	%.cc
	@echo "    Compiling $< ..."
	@${CXX} ${CXXFLAGS} -o $@ -c $<

%.s:	%.cc
	@echo "    Compiling $< to assembly ..."
	@${CXX} ${CXXFLAGS} -S -o $@ -c $<

${LIBA}:	${OBJS}
	@echo "Linking $@ ..."
	@${AR} r $@ $?
	@${RANLIB} $@

${LIBSOBLD}:	${OBJS}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} ${SHBLDFL} -o $@ $^ ${LIBS}

html:
dox:
	@${DOXYGEN} ${DOCT}

################ Installation ##########################################

install:	${ALLINST}
uninstall:	$(subst install,uninstall,${ALLINST})

install-shared: ${LIBSOBLD}
	@echo "Installing ${LIBSOBLD} to ${LIBDIR} ..."
	@${INSTALLDIR} ${LIBDIR}
	@${INSTALLLIB} ${LIBSOBLD} ${LIBDIR}
	@(cd ${LIBDIR}; \
	    rm -f ${LIBSO} ${LIBSOLNK}; \
	    ln -sf ${LIBSOBLD} ${LIBSO}; \
	    ln -sf ${LIBSOBLD} ${LIBSOLNK})

uninstall-shared:
	@echo "Removing ${LIBSOBLD} from ${LIBDIR} ..."
	@(cd ${LIBDIR}; \
	    rm -f ${LIBSO} ${LIBSOLNK} ${LIBSOBLD})

install-static: ${LIBA}
	@echo "Installing ${LIBA} to ${LIBDIR} ..."
	@${INSTALLDIR} ${LIBDIR}
	@${INSTALLLIB} ${LIBA} ${LIBDIR}

uninstall-static:
	@echo "Removing ${LIBA} from ${LIBDIR} ..."
	@rm -f ${LIBDIR}/${LIBA}

install-incs: ${INCS}
	@echo "Installing headers to ${INCDIR} ..."
	@${INSTALLDIR} ${INCDIR}/${LIBNAME}
	@for i in $(filter-out ${LIBNAME}.h,${INCS}); do	\
	    ${INSTALLDATA} $$i ${INCDIR}/${LIBNAME}/$$i;	\
	done;
	@${INSTALLDATA} ${LIBNAME}.h ${INCDIR}

uninstall-incs:
	@echo "Removing headers from ${INCDIR} ..."
	@rm -rf ${INCDIR}/${LIBNAME} ${INCDIR}/${LIBNAME}.h

################ Maintenance ###########################################

clean:
	@echo "Removing generated files ..."
	@rm -f ${OBJS} ${TOCLEAN} *.rpo
	@+${MAKE} -C bvt clean

depend: ${SRCS}
	@${CXX} ${CXXFLAGS} -M ${SRCS} > .depend;
	@+${MAKE} -C bvt depend

check:	install
	@echo "Compiling and running build verification tests ..."
	@+${MAKE} -C bvt run

TMPDIR	= /tmp
DISTDIR	= ${HOME}/stored
DISTNAM	= ${LIBNAME}-${MAJOR}.${MINOR}
DISTTAR	= ${DISTNAM}.${BUILD}.tar.bz2
DDOCTAR	= ${LIBNAME}-docs-${MAJOR}.${MINOR}.${BUILD}.tar.bz2

dist:
	mkdir ${TMPDIR}/${DISTNAM}
	cp -r . ${TMPDIR}/${DISTNAM}
	+${MAKE} -C ${TMPDIR}/${DISTNAM} dox distclean
	(cd ${TMPDIR}; tar jcf ${DISTDIR}/${DDOCTAR} ${DISTNAM}/docs/html)
	(cd ${TMPDIR}/${DISTNAM}; rm -rf `find . -name .svn` docs/html)
	(cd ${TMPDIR}; tar jcf ${DISTDIR}/${DISTTAR} ${DISTNAM}; rm -rf ${DISTNAM})

distclean:	clean
	@rm -f Config.mk config.h ${LIBNAME}.spec bsconf.o bsconf .depend bvt/.depend

maintainer-clean: distclean
	@rm -rf docs/html

-include .depend
 
