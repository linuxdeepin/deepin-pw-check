
PREFIX=/usr
GOPKG_PREFIX = service
GOBUILD = go build $(GO_BUILD_FLAGS)
ifeq (${PAM_MODULE_DIR},)
PAM_MODULE_DIR := /etc/pam.d
endif
BINARIES = deepin-pw-check
LIBRARIES = libdeepin_pw_check.so
LANGUAGES = $(basename $(notdir $(wildcard misc/po/*.po)))

all: build

prepare:
	@mkdir -p out/bin


out/bin/%: prepare
	env GOPATH="${GOPATH}" ${GOBUILD} -o $@  ${GOPKG_PREFIX}/*.go

out/libdeepin_pw_check.so: 
	gcc lib/*.c -fPIC -shared $(shell pkg-config --libs libsystemd) -lcrypt -lcrack -DIN_CRACKLIB -lpam -o $@ $^
	chmod -x $@


build: $(addprefix out/bin/, ${BINARIES}) out/${LIBRARIES}

install:
	mkdir -p ${DESTDIR}${PREFIX}/lib/deepin-pw-check/
	cp out/bin/deepin-pw-check ${DESTDIR}${PREFIX}/lib/deepin-pw-check/
	mkdir -p ${DESTDIR}${PREFIX}/lib
	cp -f out/${LIBRARIES} ${DESTDIR}${PREFIX}/lib
	mkdir -p ${DESTDIR}${PREFIX}/include
	cp lib/deepin_pw_check.h ${DESTDIR}${PREFIX}/include/
	mkdir -p ${DESTDIR}${PREFIX}/share/dbus-1/system.d
	cp misc/conf/*.conf ${DESTDIR}${PREFIX}/share/dbus-1/system.d/
	mkdir -pv ${DESTDIR}${PREFIX}/share/dbus-1
	cp -r misc/system-services ${DESTDIR}${PREFIX}/share/dbus-1/
	mkdir -pv ${DESTDIR}/usr/share/pam-configs
	cp misc/pam-configs/deepin-pw-check ${DESTDIR}/usr/share/pam-configs/deepin-pw-check


clean:
	rm -rf out

pot:
	xgettext --from-code utf-8 lib/* -o misc/po/deepin-pw-check.pot

out/locale/%/LC_MESSAGES/deepin-pw-check.mo: misc/po/%.po
	mkdir -p $(@D)
	msgfmt -o $@ $<

translate: $(addsuffix /LC_MESSAGES/deepin-pw-check.mo, $(addprefix out/locale/, ${LANGUAGES}))
