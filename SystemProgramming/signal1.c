#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

int
main(void) {

	int cnt;
	sigset_t chkset;

	// your codes here
    // ctrl+c: SIGINT
    // 시그널 집합 생성
    sigset_t set;
    // 시그널 추가
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, NULL);

	// do not modify
	cnt = 0;
	for(;;)
	{
		printf("hello\n");
		fflush(stdout);
		sleep(1);	
		cnt++;
		
		if(!(cnt % 3)){
			raise(SIGINT);
			
			sigprocmask(0, NULL, &chkset);
			// check for other signals are blocked
			// NSIG is defined in signal.h
			// don't worry about the red underline
			for(int i=1; i<NSIG; i++)
			{
				if(i == SIGINT) continue;
				if(sigismember(&set, i))
				{
					printf("Block just SIGINT not SIG%s(%d)\n", strsignal(i), i);	
					exit(EXIT_FAILURE);
				}
			}
			
			// take pended masks
			if(sigpending(&chkset))
			{
				perror("pending\n");
				exit(EXIT_FAILURE);
			}
			
			// check SIGINT is successfully blocked
			if(sigismember(&chkset, SIGINT))
			{
				printf("Success\n");
				exit(EXIT_SUCCESS);
			
			} 
			// fail to blocking SIGINT
			else 
			{
				printf("Failed\n");
				exit(EXIT_FAILURE);
			}
		}
	}	
	return 0;
}
