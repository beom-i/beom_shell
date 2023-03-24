/*
 * Copyright(c) 2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
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
    char *p, *q,*r;                /* 명령어를 파싱하기 위한 변수 */
    char *input_file = NULL;
    char *output_file = NULL;
    
    /*
     * 명령어 앞부분 공백문자를 제거하고 인자를 하나씩 꺼내서 argv에 차례로 저장한다.
     * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
     */
    p = cmd; p += strspn(p, " \t");
    q = strpbrk(p,"|");
    if(q == NULL){
        do {
            /*
             * 공백문자, 큰 따옴표, 작은 따옴표가 있는지 검사한다.
             */
            q = strpbrk(p, " \t\'\"<>|");//p가 str2에 해당하는 값이 없으면 q에 null배정_예범 https://www.ibm.com/docs/ko/i/7.3?topic=functions-strpbrk-find-characters-in-string
            /*
             * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
             */
            if (q == NULL || *q == ' ' || *q == '\t') {
                q = strsep(&p, " \t");
                if (*q) argv[argc++] = q;
            }
            /*
             < 문자가 있는 경우
             입력을 표준 장치인 키보드에서 읽어 오지 않고 파일에서 읽어 온다.
             < 구분자로 p를 나눈다음에 그 전 문자열을 argv배열에 담고, p이름을 가진 파일을
             open에 넣어서 파일에서 읽어옴 _ 예범
             2023.03.19 파일 이름이 argv에 담기는 오류 발생.
             2023.03.21 이 경우에 표준 입출력을 담겼을 때 argc을 기다려서 해당 인덱싱 null 초기화 or T/F값으로 중간에 break하는걸로
             */
            else if (*q == '<'){
                int is_only_alpha = 1;

                q = strsep(&p, "<");

                if (*q) argv[argc++] = q;
                p += strspn(p, " \t");
                q=p;
                while (*q != '\0') {
                  if (!isalpha(*q)) {
                    is_only_alpha = 0;
                    break;
                  }
                  q++;
                }
                if(!(is_only_alpha)){ //공백이 더 있음
                    q = strsep(&p, " ");
                    input_file = q;
                    if (*q) argv[argc++] = q;
                }
                else if(is_only_alpha){ //알파벳만 이루어져있음 = 공백 없음
                    input_file = p;
                    break;
                }
            }
            /*
             > 문자가 있는 경우
             출력을 표준장치인 화면으로 보내지 않고 파일에 저장한다
             > 구분자로 p를 나눈다음에 그 전 문자열을 argv배열에 담고, p이름을 가진 파일을 open에 넣어서 파일에서 읽어옴
             
             2023.02.18 p앞에 공백이 존재하는 경우 파일이 공백과 함께 저장된다.
             2023.03.19 (해결) p += strspn(p, " \t");으로 앞 공백을 없애고 p포인터를 옮겨준다._ 예범
             */
            else if (*q == '>'){
                int is_only_alpha = 1;
                q = strsep(&p, ">");
                if (*q) argv[argc++] = q;
                p += strspn(p, " \t");
                q=p;
                while (*q != '\0') {
                  if (!isalpha(*q)) {
                    is_only_alpha = 0;
                    break;
                  }
                  q++;
                }
                if(!(is_only_alpha)){ //공백이 더 있음
                    q = strsep(&p, " ");
                    output_file = q;
                    if (*q) argv[argc++] = q;
                }
                else if(is_only_alpha){ //알파벳만 이루어져있음 = 공백 없음
                    fprintf(stderr,"%s",p);
                    output_file = p;
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
        } while (p); //p가 null일 경우에 종료_예범
        
        if (input_file != NULL) {
            int input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1) {       // 파일을 오픈하는데 실패함
                perror("open");
                exit(EXIT_FAILURE);
            }
            if(dup2(input_fd,STDIN_FILENO) == 1){
                perror("input_dup2_error");
                exit(EXIT_FAILURE);
            }
            close(input_fd);
        }
        /*
            출력파일을 표준 출력으로 리다이렉션함
        */
        if (output_file != NULL) {
            int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);  // 출력 파일 오픈
            if (output_fd == -1) {     // 출력 파일을 오픈하는데 실패함
                perror("open");
                exit(EXIT_FAILURE);
            }
            if(dup2(output_fd,STDOUT_FILENO) == -1){
                perror("output_dup2_error");
                exit(EXIT_FAILURE);
            };
            close(output_fd);
        }
        
        argv[argc] = NULL;
        for(int i=0;i<argc;i++){
            fprintf(stderr,"%s  %d\n",argv[i],argc);
        }
        if (argc > 0){
            execvp(argv[0], argv);
        }
    }
    else if(*q == '|'){
        q = strsep(&p, "|");

        int fd[2];
        pid_t pid;

        if(pipe(fd) == -1 ){
            fprintf(stderr,"Pipe failed");
            exit(EXIT_FAILURE);
        }
                    
        if((pid=fork()) < 0){
            fprintf(stderr, "Fork Failed");
            exit(EXIT_FAILURE);
        }
        else if(pid == 0){ // 자식 프로세스
            /* 자식 프로세스는 보내야하므로 read을 닫음(입력을 닫음)*/
            close(fd[0]);
            /* 표준 출력을 파이프 연결로 바꿈*/
            dup2(fd[1],STDOUT_FILENO);
            close(fd[1]);
            /* 파이프 전 명령들 실행 -> 파이프 입력쪽으로 보냄 *********/
            cmdexec(q);
            //break;
            //exit(EXIT_SUCCESS);
        }
        else if(pid > 0){ // 부모 프로세스
            close(fd[1]);
            /* 표준 입력을 파이프 연결로 바꿈*/
            dup2(fd[0],STDIN_FILENO);
            /* 부모프로세스는 받아야하므로 write을 닫음(출력을 닫음)*/
            close(fd[0]);
            wait(NULL);
            /* 자식 먼저 명령 실행해야함. 그래서 부모 기다림*/
            /* 배열로 자식프로세스에서 실행한만큼 초기화 */
            memset(argv, 0, sizeof(argv));
            argc = 0;
            cmdexec(p);

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
