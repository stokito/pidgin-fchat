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
all:	clean libfchat.so

install:	libfchat.so locale
#	cp libfchat.so ~/.purple/plugins/
#	mkdir --mode=664 --parents ${DESTDIR}/usr/lib/purple-2/
	mkdir --parents ${DESTDIR}/usr/lib/purple-2/
	cp libfchat.so ${DESTDIR}/usr/lib/purple-2/
	chmod 664 ${DESTDIR}/usr/lib/purple-2/libfchat.so
	chown root:root ${DESTDIR}/usr/lib/purple-2/libfchat.so
#	fchat16.png
	mkdir --parents ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/
	cp ./share/pixmaps/pidgin/protocols/48/fchat16.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/fchat.png
	chmod 664 ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/fchat.png
	chown root:root ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/fchat.png
#	fchat22.png
	mkdir --parents ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/
	cp ./share/pixmaps/pidgin/protocols/22/fchat22.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/fchat.png
	chmod 664 ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/fchat.png
	chown root:root ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/fchat.png
#	fchat48.png
	mkdir --parents ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/
	cp ./share/pixmaps/pidgin/protocols/48/fchat48.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/fchat.png
	chmod 664 ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/fchat.png
	chown root:root ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/fchat.png
#	fchat.mo
	mkdir --parents ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/
	cp ./locale/ru/LC_MESSAGES/fchat.mo ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/fchat.mo
	chmod 664 ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/fchat.mo
	chown root:root ${DESTDIR}/usr/share/locale/ru/LC_MESSAGES/fchat.mo

uninstall:
#	rm -f ~/.purple/plugins/libfchat.so
	rm -f ${DESTDIR}/usr/lib/purple-2/libfchat.so
	rm -f ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/16/fchat.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/22/fchat.png ${DESTDIR}/usr/share/pixmaps/pidgin/protocols/48/fchat.png

libfchat.so:	${FCHAT_SOURCES}
	${CC} ${SYSTEM_CFLAGS} $$(pkg-config --libs --cflags glib-2.0 purple) ${FCHAT_SOURCES} -o libfchat.so -shared -fPIC -DPIC

clean:
	rm -f libfchat.so prpl-fchat-source.tar.bz2
	rm -rf prpl-fchat

pot:
	xgettext --package-name=fchat -k_ -o ./i18n/fchat.pot ${FCHAT_SOURCES}

locale:
	mkdir -p ./locale/ru/LC_MESSAGES/
	msgfmt -o ./locale/ru/LC_MESSAGES/fchat.mo ./i18n/ru.po

dist:	${FCHAT_SOURCES} Makefile ../ico/fchat16.png ../ico/fchat22.png ../ico/fchat48.png
	tar -cf tmp.tar $^
	mkdir prpl-fchat
	mv tmp.tar prpl-fchat
	tar xvf prpl-fchat/tmp.tar -C prpl-fchat
	rm prpl-fchat/tmp.tar
	tar --bzip2 -cf prpl-fchat-source.tar.bz2 prpl-fchat
	rm -rf prpl-fchat

#fchat.deb:	libfacebook.so libfacebookarm.so libfacebook64.so libfacebookppc.so
#	echo "Dont forget to update version number"
#	cp libfacebook.so ${DEB_PACKAGE_DIR}/usr/lib/purple-2/
#	cp libfacebookppc.so ${DEB_PACKAGE_DIR}/usr/lib/purple-2/
#	cp libfacebook64.so ${DEB_PACKAGE_DIR}/usr/lib64/purple-2/
#	cp libfacebookarm.so ${DEB_PACKAGE_DIR}/usr/lib/pidgin/
#	cp facebook16.png ${DEB_PACKAGE_DIR}/usr/share/pixmaps/pidgin/protocols/16/facebook.png
#	cp facebook22.png ${DEB_PACKAGE_DIR}/usr/share/pixmaps/pidgin/protocols/22/facebook.png
#	cp facebook48.png ${DEB_PACKAGE_DIR}/usr/share/pixmaps/pidgin/protocols/48/facebook.png
#	chown -R root:root ${DEB_PACKAGE_DIR}
#	chmod -R 755 ${DEB_PACKAGE_DIR}
#	dpkg-deb --build ${DEB_PACKAGE_DIR} $@ > /dev/null


# basic GTK+ app makefile
#SOURCES = myprg.c foo.c bar.c
#OBJS    = ${SOURCES:.c=.o}
#CFLAGS  = `pkg-config gtk+-2.0 --cflags`
#LDADD   = `pkg-config gtk+-2.0 --libs`
#CC      = gcc
#PACKAGE = myprg

#all : ${OBJS}
#  ${CC} -o ${PACKAGE} ${OBJS} ${LDADD}

#.c.o:
#  ${CC} ${CFLAGS} -c $<

# end of file
