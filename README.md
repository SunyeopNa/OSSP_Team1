# 그림채팅 프로그램
------------------
그림채팅 프로그램은 여러 사람이 같이 그림을 그리면서 소통할 수 있는 도구입니다.

#### 주요 기능
- 브러시 색상 변경
- 브러시 크기 조절
- 그림채팅 종료 시 이미지 파일 저장
# Dependency
------------------
그림채팅 프로그램은 [GTK]를 기반으로 하고 있습니다.

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
- 대상 서버변경
#define SERVER_IP "127.0.0.1"의 IP를 원하는 Server IP로 변경합니다.
