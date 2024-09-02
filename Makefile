PREFIX=/usr
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

SECURITY_BUILD_OPTIONS = -fPIC -fstack-protector-all -z relro -z noexecstack -z now -pie
export GOPATH = /usr/share/gocode

all: build

prepare:
	@mkdir -p ${GOBUILD_DIR}/src/$(dir ${GOPKG_PREFIX})
	@ln -snf ../../../.. ${GOBUILD_DIR}/src/${GOPKG_PREFIX}
	@mkdir -p out/bin

out/bin/%: prepare
	env GOPATH="${GOPATH}:${CURDIR}/${GOBUILD_DIR}" ${GOBUILD} -o $@ ${GOBUILD_OPTIONS} ${GOPKG_PREFIX}/service

out/${LIBRARIES}:
	gcc lib/*.c ${SECURITY_BUILD_OPTIONS} -shared -DIN_CRACKLIB -W -Wall -Wl,-soname,libdeepin_pw_check.so.1 -o $@ $^ -lcrypt -lcrack -liniparser
	cd out; ln -s ${LIBRARIES} ${LINK_LIBRARIES}

lib/%:
	gcc $(addsuffix .c, $@) -c ${SECURITY_BUILD_OPTIONS} -DIN_CRACKLIB -W -Wall -o $(addsuffix .o, $@)

link: $(addprefix lib/, ${LIBSRCS_C})
	# cd lib ;ar x /usr/lib/$(DEB_HOST_MULTIARCH)/libiniparser.a
	# cd lib ;ar x /usr/lib/$(DEB_HOST_MULTIARCH)/libcrack.a
	# cd lib ;ar x /usr/lib/$(DEB_HOST_MULTIARCH)/libcrypt.a
	ar rcs out/libdeepin_pw_check.a lib/*.o

static_lib: link

out/${PAM_MODULE}: out/${LIBRARIES}
	gcc pam/*.c -fPIC -W -Wall -shared -lpam -L./out/ -ldeepin_pw_check -o $@ $^

build_tool: prepare
	gcc tool/*.c -liniparser -W -Wall ${SECURITY_BUILD_OPTIONS} -o out/${TOOL_BINARAY}

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
	mkdir -p ${DESTDIR}${PREFIX}/lib/deepin-pw-check/
	cp out/bin/deepin-pw-check ${DESTDIR}${PREFIX}/lib/deepin-pw-check/
	mkdir -p ${DESTDIR}${PREFIX}/share/polkit-1/actions
	cp -r misc/polkit-action/*.policy ${DESTDIR}${PREFIX}/share/polkit-1/actions
	mkdir -p ${DESTDIR}/lib/systemd/system
	cp  misc/systemd-service/deepin-passwd-conf.service ${DESTDIR}/lib/systemd/system/

test: $(addprefix unit_test/, $(SRCS_C)) clean_test

unit_test/%:
	gcc $(addsuffix .c, $@) ./lib/*.c -lcrypt -lcrack -liniparser -DIN_CRACKLIB -W -Wall -DPASSWD_CONF_FILE_GRUB2=\"unit_test/testdata/grub2_edit_auth.conf\" -z noexecstack -o $@
	@chmod +x $@
	@./$@

clean:
	rm -rf out
	rm -rf lib/*.o

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
