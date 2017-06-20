# 그림채팅 프로그램
------------------
그림채팅 프로그램은 여러 사람이 같이 그림을 그리면서 소통할 수 있는 도구입니다.

#### 주요 기능
- 브러시 색상 변경
- 브러시 크기 조절
- 그림채팅 종료 시 이미지 파일 텍스트 형식으로 저장
# Dependency
------------------
그림채팅 프로그램은 [GTK2.0]를 기반으로 하고 있습니다. GTK2.0은 다음의 명령어로 설치할 수 있습니다.
```
sudo apt-get install gtk2.0
sudo apt-get install build-essential libgtk2.0-dev
```
Project 빌드를 수행하기 위해서 GCC가 필요합니다.

# 빌드
------------------
### 서버(server.c)
```terminal
$ gcc server.c -o server.out
```

### 클라이언트(chat_cli.c)
```terminal
$ gcc paint.c -o paint.out pkg-config --cflags --lib gtk+-2.0
```
- 대상 서버변경 :
#define SERVER_IP "127.0.0.1"의 IP를 원하는 Server IP로 변경합니다.

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
-*/
```
