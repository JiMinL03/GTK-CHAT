# 채팅 프로그램

이 프로젝트는 간단한 채팅 서버와 클라이언트를 구현한 것입니다. 클라이언트는 GTK+3.0을 사용하여 GUI를 구성하고, 서버는 멀티스레딩을 통해 여러 클라이언트의 연결을 처리합니다.

## 컴파일 방법

### 1. 컴파일
 파일을 컴파일하려면 다음 명령어를 사용하세요:
```bash
gcc -o server server.c -pthread
gcc -o client client.c -pthread `pkg-config --cflags --libs gtk+-3.0`
./server &
./client
