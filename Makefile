#
# Copyright(c) 2020-2023. All rights reserved by Heekuck Oh.
# 이 파일은 한양대학교 ERICA 컴퓨터학부 학생을 위해 만들었다.
#
CC = gcc
CFLAGS = -Wall -O
#라이브러리는 딱히 없으니까 그냥 둠.
CLIBS =
#mac환경인지 리눅스 환경인지 컴파일 옵션이 달라질 수 있어서 둘중 무엇인지 검사
OS := $(shell uname -s)
ifeq ($(OS), Linux)
#	CLIBS += -lbsd
endif
ifeq ($(OS), Darwin)
#	CLIBS +=
endif
#

#make 하면 make all 실행 tsh.o 파일 필요함 / 밑으로감 /
all: tsh.o
	$(CC) -o tsh tsh.o $(CLIBS)

#tsh.o는 tsh.c가 필요해 그럼 tsh.c가 있으면 밑 실행 / 위에처럼 $(cc)  -> gcc /
#매크로 넣으면 하나하나 할 필요 없음. / 종속이라 tsh.c 변경하면 트리거됨 ! 그래서 자동으로 다시 컴파일 됨
tsh.o: tsh.c
	$(CC) $(CFLAGS) -c tsh.c

#make clean 들어오면 복제 + 실행파일 지움
clean:
	rm -rf *.o
	rm -rf tsh
	
