#Customisable stuff here
CFLAGS = -g -Wall
SYSTEM_CFLAGS = -I/usr/include -I/usr/local/include
DEB_PACKAGE_DIR = ./debdir
SRCDIR=./src
FCHAT_SOURCES = \
	${SRCDIR}/actions.c \
	${SRCDIR}/attention.c \
	${SRCDIR}/buddy.c \
	${SRCDIR}/chat.c \
	${SRCDIR}/connection.c \
	${SRCDIR}/fchat.c \
	${SRCDIR}/fchat.h \
	${SRCDIR}/info.c \
	${SRCDIR}/process_cmds.c \
	${SRCDIR}/send_cmds.c \
	${SRCDIR}/status.c \

#Standard stuff here
.PHONY:	all clean install uninstall getpot locale dist

#all:	clean libfchat.so dist sourcepackage
all:	clean libfchat.so locale

install:	libfchat.so locale
#	cp libfchat.so ~/.purple/plugins/
#	mkdir --mode=664 --parents ${DESTDIR}/usr/lib/purple-2/
	mkdir --parents ${DESTDIR}/usr/lib/purple-2/
	cp libfchat.so ${DESTDIR}/usr/lib/purple-2/
	chmod 664 ${DESTDIR}/usr/lib/purple-2/libfchat.so
	chown root:root ${DESTDIR}/usr/lib/purple-2/libfchat.so
#	fchat16.png
	mkdir --parents ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/
	cp ./share/pixmaps/pidgin/protocols/48/fchat.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/fchat.png
	chmod 664 ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/fchat.png
	chown root:root ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/fchat.png
#	fchat22.png
	mkdir --parents ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/
	cp ./share/pixmaps/pidgin/protocols/22/fchat.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/fchat.png
	chmod 664 ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/fchat.png
	chown root:root ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/fchat.png
#	fchat48.png
	mkdir --parents ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/
	cp ./share/pixmaps/pidgin/protocols/48/fchat.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/fchat.png
	chmod 664 ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/fchat.png
	chown root:root ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/fchat.png
#	fchat.mo
	mkdir --parents ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/
	cp ./share/locale/ru/LC_MESSAGES/fchat.mo ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/fchat.mo
	chmod 664 ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/fchat.mo
	chown root:root ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/fchat.mo

uninstall:
#	rm -f ~/.purple/plugins/libfchat.so
	rm -f ${DESTDIR}/usr/lib/purple-2/libfchat.so
	rm -f ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/fchat.mo
	rm -f ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/fchat.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/fchat.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/fchat.png

libfchat.so:	${FCHAT_SOURCES}
	${CC} ${SYSTEM_CFLAGS} $$(pkg-config --libs --cflags glib-2.0 purple) ${FCHAT_SOURCES} -o libfchat.so -shared -fPIC -DPIC

clean:
	rm -f libfchat.so prpl-fchat-source.tar.bz2
	rm -rf prpl-fchat

pot:
	xgettext --package-name=fchat -k_ -o ./i18n/fchat.pot ${FCHAT_SOURCES}

locale:
	mkdir -p ./share/locale/ru/LC_MESSAGES/
	msgfmt -o ./share/locale/ru/LC_MESSAGES/fchat.mo ./i18n/ru.po
