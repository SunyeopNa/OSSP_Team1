# Paint Chat Program
------------------
Paint Chat Program is chat program with painting program.
you can share your paint with others.


#### Main functions
- Change Bursh color, size
- Save image in Server.

# Dependency
------------------
Paint Chat Program is  based [GTK2.0]. here is the command line that install gtk2.0
```
sudo apt-get install gtk2.0
sudo apt-get install build-essential libgtk2.0-dev
```
You also need to install gcc, pkg-config. These are commonly pre-installed.

# Build
------------------
### Server (server.c)
```terminal
$ gcc server.c -pthread -o server.out
```

### Client (chat_cli.c)
```terminal
$ gcc chat_cli.c -o paint.out pkg-config --cflags --lib gtk+-2.0
```
- Server IP Setting :
#define SERVER_IP "127.0.0.1" //Edit this string 127.0.0.1 to your server IP.

# License
------------------

```
-* GNU Library or Lesser General Public License (LGPLv2)
-* This library is free software; you can redistribute it and/or
-* modify it under the terms of the GNU Library General Public
-* License as published by the Free Software Foundation; either
-* version 2 of the License, or (at your option) any later version.
-
-* This library is distributed in the hope that it will be useful,
-* but WITHOUT ANY WARRANTY; without even the implied warranty of
-* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
-* Library General Public License for more details.

```
