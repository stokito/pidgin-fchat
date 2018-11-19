# FChat protocol plugin for libpurple (Pidgin)
![](/share/pixmaps/pidgin/protocols/48/fchat.png)
![](http://pidgin.im/shared/img/logo.text.jpg)

This project is an a reanimation of [FChat](http://www.kilievich.com/fchat/) - one of the famous p2p chat programs for local networks. Original FChat was with support of IRC protocol and itself FChat protocol based on UDP datagrams.
The most important feature is that FChat not require dedicated server and it's fully p2p!


[libpurple](https://developer.pidgin.im/wiki/WhatIsLibpurple) is a library intended to be used by programmers seeking to write an IM client that connects to many IM networks.
libpurple is used by several messengers: [Pidgin](https://pidgin.im/), Finch, Adium, Instantbird and Telepathy with haze.
This package add FChat protocol support for libpurple.
FChat is p2p chat program for local networks based on UDP protocol.
Your IP will be used as ID. If you have several network interfaces you can use different IP for them.
If you want use FChat for all networks enter IP 0.0.0.0 .

Official releases for Ubuntu are available from my [PPA](https://launchpad.net/~stokito/+archive)

This repository is mirror of [Launchpad repo](https://launchpad.net/fchat)

## Build and install

    sudo apt install libpurple-dev
    make
    sudo make install
