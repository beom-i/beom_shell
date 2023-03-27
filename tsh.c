/*
 * Copyright(c) 2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 * 2023.03.20-21 / 컴퓨터학부 / 2019096699 / 이예범 / 리다이렉션 기능 추가
 * 2023.03.23    / 컴퓨터학부 / 2019096699 / 이예범 / 파이프, 다중파이프 기능 추가
 * 2023.03.24    / 컴퓨터학부 / 2019096699 / 이예범 / 리다이렉션 기능 수정, 조합 기능 추가
 * 2023.03.27    / 컴퓨터학부 / 2019096699 / 이예범 / 최종 주석 추가
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>


#define MAX_LINE 80             /* 명령어의 최대 길이 */

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다.
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */
static void cmdexec(char *cmd)
{
    char *argv[MAX_LINE/2+1];   /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;               /* 인자의 개수 */
    char *p, *q;                /* 명령어를 파싱하기 위한 변수 */
    char *input_file = NULL;    /* 표준 입력 리다이렉션에 사용하는 변수 ( 표준 입력 -> input_file )*/
    char *output_file = NULL;   /* 표준 출력 리다이렉션에 사용하는 변수 ( 표준 출력 -> output_file )*/
    /*
     * 먼저, cmdexec은 뉴 기본기능 (기본기능 + 리다이렉션) 과 파이프기능 두 개로 나뉘어져 있음
     * Chap 1 : 명령어 앞부분 공백문자를 제거하고 명령어에 '|' 문자가 존재여부 확인
     * '|' 문자가 존재하지 않을 경우 기본 기능 ( + 리다이렉션 기능) 수행
     * '|' 문자가 존재할 경우 else if(*q == '|')으로 이동 후 파이프 기능 수행
     */
    p = cmd; p += strspn(p, " \t");
    /*
     p에 '|'문자 존재여부 확인 ->  존재하면 q에 해당포인터 할당 / 존재하지 않으면 Null값 배정
     * strpbrk 함수 설명 : p가 str2에 해당하는 값이 없으면 q에 null배정_예범
     * 출처 : https://www.ibm.com/docs/ko/i/7.3?topic=functions-strpbrk-find-characters-in-string
     */
    q = strpbrk(p,"|");
    /* '|' 문자가 존재하지 않을 경우 기본 기능 (+ 리다이렉션 기능) 수행*/
    if(q == NULL){
        do {
            /*
             * 공백문자, 큰 따옴표, 작은 따옴표, <> 문자가 있는지 검사한다.
             */
            q = strpbrk(p, " \t\'\"<>");
            /*
             * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
             */
            if (q == NULL || *q == ' ' || *q == '\t') {
                q = strsep(&p, " \t");
                if (*q) argv[argc++] = q;
            }
            /*
             * < 문자가 있는 경우
             * 입력을 표준 장치인 키보드에서 읽어 오지 않고 파일에서 읽어 온다.
             * < 구분자로 p를 나눈 다음에 다음 공백까지 문자열을 p에 할당.
             * 1) < 이후에 또 다른 명령어가 존재한다면 공백기준으로 자른다.
             * 이전 문자열은 file이 되고 배열에 담긴다. 이후 while문 작동
             * 2) < 이후 명령어 존재하지 않고 마지막일경우 p가 그대로 리다이렉션 file이 되고
             * 이후 While문 안에서 argv 배열에 담긴다.
             */
            else if (*q == '<'){
                q = strsep(&p, "<");
                if (*q) argv[argc++] = q;
                p += strspn(p, " \t");
                /*
                 * 남은 문자열(p)의 *끝*에 공백이 있는지 없는지 for문으로 판별
                 * isspace함수 : 공백이면 0이 아닌 수를 반환 / 출처 : https://blockdmask.tistory.com/449
                 */
                int i;
                for(i = 0 ; i < strlen(p) ; i++){
                    if(isspace(p[i])!=0) {
                        i=-1;                      // i로 공백인지 아닌지 판별
                        break;                     // 공백이면 for문 종료
                    }
                }
                if(i < 0){                         // 공백이 있음
                    q = strsep(&p, " ");           // 공백문자로 분할
                    input_file = q;                // 나뉘어진 왼쪽 문자열 input_file에 할당
                    if (*q) argv[argc++] = q;      // q 존재할경우 argv 배열에 담음
                }
                else{                              // 공백 아무것도 없을 경우
                    input_file = p;                // p input_file에 할당
                    break;
                }
            }
            /*
             * > 문자가 있는 경우
             * 출력을 표준 장치로 출력하지 않고 파일로 출력한다.
             * > 구분자로 p를 나눈 다음에 다음 공백까지 문자열을 p에 할당.
             * 1) > 이후에 또 다른 명령어가 존재한다면 공백기준으로 자른다.
             * 이전 문자열은 file이 되고 배열에 담긴다. 이후 while문 작동
             * 2) < 이후 명령어 존재하지 않고 마지막일경우 p가 그대로 리다이렉션 file이 되고
             * 이후 While문 안에서 argv 배열에 담긴다.
             */
            else if (*q == '>'){
                q = strsep(&p, ">");
                if (*q) argv[argc++] = q;
                p += strspn(p, " \t");

                /*
                 * 남은 문자열(p)의 *끝*에 공백이 있는지 없는지 for문으로 판별
                 * isspace함수 : 공백이면 0이 아닌 수를 반환 / 출처 : https://blockdmask.tistory.com/449
                 */
                int i;
                for(i = 0 ; i < strlen(p) ; i++){
                    if(isspace(p[i])!=0) {
                        i=-1;                      // i로 공백인지 아닌지 판별
                        break;                     // 공백이면 for문 종료
                    }
                }
                if(i<0){                           // 공백이 더 있음
                    q = strsep(&p, " ");           // 공백문자로 분할
                    output_file = q;               // 나뉘어진 왼쪽 문자열 output_file에 할당
                    if (*q) argv[argc++] = q;      // q 존재할경우 argv 배열에 담음
                }
                else{                              //아무것도 없을 경우
                    output_file = p;               // p output_file에 할당
                    break;
                }
            }
            /*
             * 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고,
             * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
             * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
             */
            else if (*q == '\'') {
                q = strsep(&p, "\'");
                if (*q) argv[argc++] = q;
                q = strsep(&p, "\'");
                if (*q) argv[argc++] = q;
            }
            /*
             * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고,
             * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
             * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
             */
            else {
                q = strsep(&p, "\"");
                if (*q) argv[argc++] = q;
                q = strsep(&p, "\"");
                if (*q) argv[argc++] = q;
            }
        } while (p); // p가 null일 경우에 종료_예범
        
        /*
         * 리다이렉션 할 파일이 존재한다면
         * 입력 파일을 표준 입력으로 리다이렉션함
         */
        
        if (input_file != NULL) {
            int input_fd = open(input_file, O_RDONLY);  // 입력 파일 오픈
            if (input_fd == -1) {                       // 파일을 오픈하는데 실패함
                perror("input_open_error");
                exit(EXIT_FAILURE);
            }
            if(dup2(input_fd,STDIN_FILENO) == 1){       // 표준 출력 리다이렉션 실패
                perror("input_dup2_error");
                exit(EXIT_FAILURE);
            }
            close(input_fd);
        }
        /*
         * 리다이렉션 할 파일이 존재한다면
         * 출력 파일을 표준 출력으로 리다이렉션함
         */
        if (output_file != NULL) {
            int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);  // 출력 파일 오픈
            if (output_fd == -1) {                                                  // 출력 파일을 오픈하는데 실패함
                perror("output_open_error");
                exit(EXIT_FAILURE);
            }
            if(dup2(output_fd,STDOUT_FILENO) == -1){                                // 표준 출력 리다이렉션 실패
                perror("output_dup2_error");
                exit(EXIT_FAILURE);
            };
            close(output_fd);
        }
        
        /* 명령(argv)의 종료지점 설정 */
        argv[argc] = NULL;
        
        /* 명령 실행 */
        if (argc > 0){
            execvp(argv[0], argv);
        }
    }
    /*
     * '|' 문자가 존재할 경우 파이프 기능 수행
     * 1) '|'을 기준으로 q, p로 나눔
     * 2) fork을 통해 자식프로세스를 생성하여 q를 실행,
     * 3) 부모는 자식프로세스가 종료될 때까지 wait한 후 p를 실행
     * 4) p에 '|'가 없을 때 까지 재귀적으로 cmdexec(p)가 실행
     */
    else if(*q == '|'){
        q = strsep(&p, "|");

        int fd[2];
        pid_t pid;

        if(pipe(fd) == -1 ){                // 파이프 실패될 경우
            fprintf(stderr,"Pipe failed");
            exit(EXIT_FAILURE);
        }
                    
        if((pid=fork()) < 0){
            fprintf(stderr, "Fork Failed"); // 포크 실패될 경우
            exit(EXIT_FAILURE);
        }
        else if(pid == 0){                  // 자식 프로세스
            close(fd[0]);                   // 자식 프로세스는 보내야하므로 read을 닫음(입력을 닫음)
            dup2(fd[1],STDOUT_FILENO);      // 표준 출력을 파이프 연결로 바꿈
            close(fd[1]);                   // 출력 파이프 닫음
            cmdexec(q);                     // 파이프 전 명령들 실행  /  부모프로세스와 연결된 파이프로 보냄
        }
        else if(pid > 0){                   // 부모 프로세스
            close(fd[1]);                   // 표준 입력을 파이프 연결로 바꿈
            dup2(fd[0],STDIN_FILENO);       // 부모프로세스는 받아야하므로 write을 닫음(출력을 닫음)
            close(fd[0]);                   // 입력 파이프 닫음
            wait(NULL);                     // 자식 먼저 명령 실행해야함. 그래서 부모 기다림
            memset(argv, 0, sizeof(argv));  // 부모프로세스 실행하기 전에 자식프로세스에서 실행한만큼 배열 초기화 중복 실행 위험 요소 제거
            argc = 0;                       // 배열 개수 초기화
            cmdexec(p);                     // 파이프 문자 이후 명령들 실행 / 자식과 연결된 상태이므로 출력값 이어받아서 실행

        }
    }

}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void)
{
    char cmd[MAX_LINE+1];       /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    int background;             /* 백그라운드 실행 유무 */
    
    /*
     * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
     */
    while (true) {
        /*
         * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
        /*
         * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
         */
        printf("tsh> "); fflush(stdout);
        /*
         * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
         * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
         * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
        /*
         * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
        /*
         * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
         */
        char *p = strchr(cmd, '&');
        if (p != NULL) {
            background = 1;
            *p = '\0';
        }
        else
            background = 0;
        /*
         * 자식 프로세스를 생성하여 입력된 명령어를 실행하게 한다.
         */
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        /*
         * 자식 프로세스는 명령어를 실행하고 종료한다.
         */
        else if (pid == 0) {
            cmdexec(cmd);
            exit(EXIT_SUCCESS);
        }
        /*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        else if (!background)
            waitpid(pid, NULL, 0);
    }
    return 0;
}
