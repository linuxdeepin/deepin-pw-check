# Keep `make` building the package; test rules below must not become the default.
.DEFAULT_GOAL := all

PREFIX=/usr
exec_prefix = $(PREFIX)
libexecdir = $(exec_prefix)/libexec
GOPKG_PREFIX = github.com/linuxdeepin/deepin-pw-check
GOBUILD = go build $(GO_BUILD_FLAGS)
GOBUILD_DIR = gobuild
ifeq (${PAM_MODULE_DIR},)
PAM_MODULE_DIR := /etc/pam.d
endif
ifeq (${PKG_FILE_DIR},)
PKG_FILE_DIR := /usr/local/lib/pkgconfig
endif
BINARIES = deepin-pw-check
LIBRARIES = libdeepin_pw_check.so.1.1
LINK_LIBRARIES = libdeepin_pw_check.so
PAM_MODULE = pam_deepin_pw_check.so
LANGUAGES = $(basename $(notdir $(wildcard misc/po/*.po)))
SRCS_C = $(basename $(shell cd unit_test; ls *.c))
LIBSRCS_C = $(basename $(shell cd lib; ls *.c))
TOOL_BINARAY = pwd-conf-update

# ---- GTest unit tests ----
TEST_NAMES = test_pw_check_core test_md5_crypt test_passwd_compare \
             test_bigcrypt test_debug
TEST_OBJDIR ?= tests/obj
TEST_BINDIR ?= tests
TEST_BINS = $(addprefix $(TEST_BINDIR)/,$(TEST_NAMES))
# Coverage builds use their own dirs so they never mix with normal objects.
COV_OBJDIR = tests/obj-cov
COV_BINDIR = tests/bin-cov
COV_BINS = $(addprefix $(COV_BINDIR)/,$(TEST_NAMES))
TEST_C_SRCS = lib/deepin_pw_check.c lib/debug.c lib/md5.c lib/md5_crypt.c \
              lib/passwd_compare.c lib/bigcrypt.c
TEST_CXX_SRCS = $(wildcard tests/test_*.cpp)
TEST_OBJS = $(patsubst lib/%.c,$(TEST_OBJDIR)/%.o,$(TEST_C_SRCS)) \
            $(patsubst tests/%.cpp,$(TEST_OBJDIR)/%.o,$(TEST_CXX_SRCS))
GTEST_CFLAGS = -DGTEST_HAS_PTHREAD=1
GTEST_LIBS = -lgtest_main -lgtest -pthread
# `make coverage` re-runs the build with COV=1; normal builds stay uninstrumented.
ifeq ($(COV),1)
COV_CFLAGS = --coverage -O0
endif
# The library API takes char * for some string parameters; tests pass literals.
TEST_CXXFLAGS = -std=c++17 -g -W -Wall -Wno-write-strings -Ilib -MMD -MP $(COV_CFLAGS)
TEST_CFLAGS = -g -DIN_CRACKLIB -Dstatic= -W -Wall -MMD -MP $(COV_CFLAGS) \
              -DPASSWD_CONF_FILE_GRUB2=\"$(CURDIR)/tests/testdata/grub2_edit_auth.conf\"
TEST_LDFLAGS = $(COV_CFLAGS)
-include $(TEST_OBJS:.o=.d)
# Keep make from trying to rebuild missing .d files through the lib/% rule.
$(TEST_OBJS:.o=.d): ;

PIC_FLAG = -fPIC
ifdef CFLAGS
	CFLAGS += $(PIC_FLAG)
else
	CFLAGS = $(PIC_FLAG)
endif
export GOPATH = /usr/share/gocode

all: build

prepare:
	@mkdir -p ${GOBUILD_DIR}/src/$(dir ${GOPKG_PREFIX})
	@ln -snf ../../../.. ${GOBUILD_DIR}/src/${GOPKG_PREFIX}
	@mkdir -p out/bin

out/bin/%: prepare
	env GOPATH="${GOPATH}:${CURDIR}/${GOBUILD_DIR}" ${GOBUILD} -o $@ ${GOBUILD_OPTIONS} ${GOPKG_PREFIX}/service

out/${LIBRARIES}:
	gcc lib/*.c ${CFLAGS} -shared -DIN_CRACKLIB -W -Wall -Wl,-soname,libdeepin_pw_check.so.1 -o $@ $^ -lcrypt -lcrack -liniparser
	cd out; ln -s ${LIBRARIES} ${LINK_LIBRARIES}

lib/%:
	gcc $(addsuffix .c, $@) -c ${CFLAGS} -DIN_CRACKLIB -W -Wall -o $(addsuffix .o, $@)

link: $(addprefix lib/, ${LIBSRCS_C})
	# cd lib ;ar x /usr/lib/$(DEB_HOST_MULTIARCH)/libiniparser.a
	# cd lib ;ar x /usr/lib/$(DEB_HOST_MULTIARCH)/libcrack.a
	# cd lib ;ar x /usr/lib/$(DEB_HOST_MULTIARCH)/libcrypt.a
	ar rcs out/libdeepin_pw_check.a lib/*.o

static_lib: link

out/${PAM_MODULE}: out/${LIBRARIES}
	gcc pam/*.c ${CFLAGS} -W -Wall -shared -lpam -L./out/ -ldeepin_pw_check -o $@ $^

build_tool: prepare
	gcc tool/*.c -liniparser -W -Wall ${CFLAGS} -o out/${TOOL_BINARAY}

build: prepare $(addprefix out/bin/, ${BINARIES}) out/${LIBRARIES} static_lib out/${PAM_MODULE} build_tool ts_to_policy

install: translate
	mkdir -pv ${DESTDIR}${PREFIX}/share/locale
	- cp -rf out/locale/* ${DESTDIR}${PREFIX}/share/locale
	mkdir -p ${DESTDIR}${PREFIX}/lib
	cp -f out/lib* ${DESTDIR}${PREFIX}/lib
	mkdir -p ${DESTDIR}${PREFIX}/include
	cp lib/deepin_pw_check.h ${DESTDIR}${PREFIX}/include/
	mkdir -pv ${DESTDIR}/${PKG_FILE_DIR}
	cp misc/pkgconfig/libdeepin_pw_check.pc ${DESTDIR}/${PKG_FILE_DIR}
	mkdir -pv ${DESTDIR}/${PAM_MODULE_DIR}
	cp out/${PAM_MODULE} ${DESTDIR}/${PAM_MODULE_DIR}
	mkdir -pv ${DESTDIR}${PREFIX}/bin/
	cp out/${TOOL_BINARAY} ${DESTDIR}${PREFIX}/bin/${TOOL_BINARAY}
	mkdir -p ${DESTDIR}${PREFIX}/share/dbus-1/system.d
	cp misc/conf/*.conf ${DESTDIR}${PREFIX}/share/dbus-1/system.d/
	mkdir -pv ${DESTDIR}${PREFIX}/share/dbus-1
	cp -r misc/system-services ${DESTDIR}${PREFIX}/share/dbus-1/
	mkdir -p ${DESTDIR}${libexecdir}/deepin-pw-check/
	cp out/bin/deepin-pw-check ${DESTDIR}${libexecdir}/deepin-pw-check/
	mkdir -p ${DESTDIR}${PREFIX}/share/polkit-1/actions
	cp -r misc/polkit-action/*.policy ${DESTDIR}${PREFIX}/share/polkit-1/actions
	mkdir -p ${DESTDIR}${PREFIX}/lib/systemd/system
	cp misc/systemd-service/deepin-passwd-conf.service ${DESTDIR}${PREFIX}/lib/systemd/system/

$(TEST_OBJDIR)/%.o: lib/%.c
	@mkdir -p $(@D)
	gcc ${CFLAGS} $(TEST_CFLAGS) -c $< -o $@

$(TEST_OBJDIR)/%.o: tests/%.cpp
	@mkdir -p $(@D)
	g++ $(TEST_CXXFLAGS) $(GTEST_CFLAGS) -DTESTDATA_DIR=\"$(CURDIR)/tests/testdata\" -c $< -o $@

$(TEST_BINDIR)/test_pw_check_core: $(TEST_OBJDIR)/test_pw_check_core.o \
                          $(TEST_OBJDIR)/deepin_pw_check.o $(TEST_OBJDIR)/debug.o
	@mkdir -p $(@D)
	g++ $^ $(GTEST_LIBS) $(TEST_LDFLAGS) -liniparser -lcrypt -o $@

$(TEST_BINDIR)/test_md5_crypt: $(TEST_OBJDIR)/test_md5_crypt.o \
                      $(TEST_OBJDIR)/md5.o $(TEST_OBJDIR)/md5_crypt.o
	@mkdir -p $(@D)
	g++ $^ $(GTEST_LIBS) $(TEST_LDFLAGS) -lcrypt -o $@

$(TEST_BINDIR)/test_passwd_compare: $(TEST_OBJDIR)/test_passwd_compare.o \
                           $(TEST_OBJDIR)/passwd_compare.o $(TEST_OBJDIR)/md5.o \
                           $(TEST_OBJDIR)/md5_crypt.o $(TEST_OBJDIR)/bigcrypt.o
	@mkdir -p $(@D)
	g++ $^ $(GTEST_LIBS) $(TEST_LDFLAGS) -lcrypt -o $@

$(TEST_BINDIR)/test_bigcrypt: $(TEST_OBJDIR)/test_bigcrypt.o $(TEST_OBJDIR)/bigcrypt.o
	@mkdir -p $(@D)
	g++ $^ $(GTEST_LIBS) $(TEST_LDFLAGS) -lcrypt -o $@

$(TEST_BINDIR)/test_debug: $(TEST_OBJDIR)/test_debug.o $(TEST_OBJDIR)/debug.o
	@mkdir -p $(@D)
	g++ $^ $(GTEST_LIBS) $(TEST_LDFLAGS) -o $@

test: $(addprefix unit_test/, $(SRCS_C)) clean_test $(TEST_BINS)
	@set -e; for t in $(TEST_BINS); do echo "== $$t"; ./$$t; done

unit_test/%:
	gcc $(addsuffix .c, $@) ./lib/*.c ${CFLAGS} -lcrypt -lcrack -liniparser -DIN_CRACKLIB -W -Wall -DPASSWD_CONF_FILE_GRUB2=\"unit_test/testdata/grub2_edit_auth.conf\" -o $@
	@chmod +x $@
	@./$@

.PHONY: test coverage clean

# lcov 1.x and 2.x use different names for the branch-coverage config key.
LCOV_MAJOR := $(shell lcov --version 2>/dev/null | grep -oE '[0-9]+' | head -1)
ifneq ($(shell [ -n "$(LCOV_MAJOR)" ] && [ "$(LCOV_MAJOR)" -ge 2 ] && echo yes),)
LCOV_BRANCH = --rc branch_coverage=1
else
LCOV_BRANCH = --rc lcov_branch_coverage=1
endif

# Build the GTest suite with gcov instrumentation, run it and render an
# lcov report. Uses a dedicated object/binary dir so normal builds and
# coverage builds can never be mixed.
coverage:
	@command -v lcov >/dev/null || { echo "lcov not found: install lcov"; exit 1; }
	rm -rf $(COV_OBJDIR) $(COV_BINDIR)
	$(MAKE) TEST_OBJDIR=$(COV_OBJDIR) TEST_BINDIR=$(COV_BINDIR) COV=1 $(COV_BINS)
	@set -e; for t in $(COV_BINS); do echo "== $$t"; ./$$t; done
	lcov --capture --directory $(COV_OBJDIR) \
	     $(LCOV_BRANCH) --output-file coverage.info
	lcov --extract coverage.info "$(CURDIR)/lib/*" \
	     $(LCOV_BRANCH) --output-file coverage_filtered.info
	@if command -v genhtml >/dev/null; then \
	    genhtml coverage_filtered.info --branch-coverage \
	        --output-directory coverage_report; \
	else \
	    echo "genhtml not found: skipping HTML report"; \
	fi
	lcov --summary coverage_filtered.info $(LCOV_BRANCH)

clean:
	rm -rf out
	rm -rf lib/*.o
	rm -rf $(TEST_OBJDIR) $(TEST_BINS) $(COV_OBJDIR) $(COV_BINDIR)
	rm -rf coverage.info coverage_filtered.info coverage_report

clean_test: $(addprefix unit_test/, $(SRCS_C))
	rm -f $^

pot:
	xgettext --from-code utf-8 lib/*.c pam/*.c -o misc/po/deepin-pw-check.pot

POLICY_NAME = org.deepin.dde.passwdconf

ts:
	deepin-policy-ts-convert policy2ts misc/polkit-action/$(POLICY_NAME).policy.in misc/ts/$(POLICY_NAME).policy

ts_to_policy:
	deepin-policy-ts-convert ts2policy misc/polkit-action/$(POLICY_NAME).policy.in misc/ts/$(POLICY_NAME).policy misc/polkit-action/$(POLICY_NAME).policy

out/locale/%/LC_MESSAGES/deepin-pw-check.mo: misc/po/%.po
	mkdir -p $(@D)
	msgfmt -o $@ $<

translate: $(addsuffix /LC_MESSAGES/deepin-pw-check.mo, $(addprefix out/locale/, ${LANGUAGES}))
