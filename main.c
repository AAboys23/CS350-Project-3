#include "parser.h"

int backgroundPIDs[1000];

int backPIDIndex = 0;

void handle_sigint(int sig)
{
    printf("\nCtrl-C catched. But currently there is no foreground process running.\n");
}

void printcmd(struct cmd *cmd)
{
    struct backcmd *bcmd = NULL;
    struct execcmd *ecmd = NULL;
    struct listcmd *lcmd = NULL;
    struct pipecmd *pcmd = NULL;
    struct redircmd *rcmd = NULL;

    int i = 0;
    
    if(cmd == NULL)
    {
        PANIC("NULL addr!");
        return;
    }
    

    switch(cmd->type){
        case EXEC:
            ecmd = (struct execcmd*)cmd;
            if(ecmd->argv[0] == 0)
            {
                goto printcmd_exit;
            }

            int pid = fork();

            if(pid == 0){
                execvp(ecmd->argv[0], ecmd->argv);
                exit(0);
            }else{
                wait(NULL);
            }


            // MSG("COMMAND: %s", ecmd->argv[0]);
            // for (i = 1; i < MAXARGS; i++)
            // {            
            //     if (ecmd->argv[i] != NULL)
            //     {
            //         MSG(", arg-%d: %s", i, ecmd->argv[i]);
            //     }
            // }
            // MSG("\n");

            break;

        case REDIR:
            rcmd = (struct redircmd*)cmd;

            printcmd(rcmd->cmd);

            if (0 == rcmd->fd_to_close)
            {
                MSG("... input of the above command will be redirected from file \"%s\". \n", rcmd->file);
            }
            else if (1 == rcmd->fd_to_close)
            {
                MSG("... output of the above command will be redirected to file \"%s\". \n", rcmd->file);
            }
            else
            {
                PANIC("");
            }

            break;

        case LIST:
            lcmd = (struct listcmd*)cmd;

            printcmd(lcmd->left);
            MSG("\n\n");
            printcmd(lcmd->right);
            
            break;

        case PIPE:
            pcmd = (struct pipecmd*)cmd;

            // printcmd(pcmd->left);
            // MSG("... output of the above command will be redrecited to serve as the input of the following command ...\n");            
            // printcmd(pcmd->right);

            int fd[2];
            pipe(fd);
            int c1 = fork();
            int c2;

            if(c1==0){
                dup2(fd[1],STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                printcmd(pcmd->left);
                exit(0);
            }else{
                wait(NULL);
                c2 = fork();
                if(c2==0){
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                    printcmd(pcmd->right);
                    exit(0);
                }else{
                    close(fd[0]);
                    close(fd[1]);
                    wait(NULL);
                }
            }
            setbuf(stdout, NULL);


            break;

        case BACK:
            bcmd = (struct backcmd*)cmd;


            printcmd(bcmd->cmd);
            MSG("... the above command will be executed in background. \n");    

            //int pid = fork();

            /*
            if(pid == 0){   //Child (background process)
                //IDEA: Store background process pid's somewhere (may need to store more than one if many background processes)
                //Then in the loop in main, call waitpid for all the background pids and check if they finished. If they did then
                //reap them.
                //execvp(bcmd->cmd->argv[0], bcmd->cmd->argv);
            }else{  //PARENT 
            
            }
            */

            break;


        default:
            PANIC("");
    
    }
    
    printcmd_exit:

    return;
}



//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
void execcmd(struct cmd *cmd) {
        struct backcmd *bcmd = NULL;
        struct execcmd *ecmd = NULL;
        struct listcmd *lcmd = NULL;
        struct pipecmd *pcmd = NULL;
        struct redircmd *rcmd = NULL;


        switch(cmd->type){
            case EXEC:  //Work
                ecmd = (struct execcmd*)cmd;
                if(ecmd->argv[0] == 0)
                {
                    //break;
                    goto exec_exit;
                }
                int pid = fork();

                if(pid == 0){
                    execvp(ecmd->argv[0], ecmd->argv);
                    exit(0);
                }else{
                    int exitstatus;
                    wait(&exitstatus);
                    if (WEXITSTATUS(exitstatus) != 0) {
                        printf("Non-zero exit code (%d) detected\n", WEXITSTATUS(exitstatus));
                    }
                }
                break;
            case REDIR: //Not sure if it works getting problems
                rcmd = (struct redircmd*)cmd;

                // printcmd(rcmd->cmd); 

                if (0 == rcmd->fd_to_close)
                {
                    // MSG("... input of the above command will be redirected from file \"%s\". \n", rcmd->file);
                    int pid = fork();
                    if(pid == 0){
                        int fdin = open(rcmd->file, O_RDONLY,00700);
                        dup2(fdin, STDIN_FILENO);

                        execcmd(rcmd->cmd);
                        exit(0);
                    }else{
                        wait(NULL);
                        dup2(STDIN_FILENO, 1);
                    }
                }
                else if (1 == rcmd->fd_to_close)
                {
                    // MSG("... output of the above command will be redirected to file \"%s\". \n", rcmd->file);
                    int pid = fork();
                    if(pid == 0){
                        int fdout = open(rcmd->file, O_RDWR|O_CREAT|O_TRUNC, 00700);
                        dup2(fdout, STDOUT_FILENO);
                        
                        execcmd(rcmd->cmd);
                        exit(0);
                    }else{
                        wait(NULL);
                        dup2(STDOUT_FILENO, 1);
                    }

                }
                else
                {
                    PANIC("");
                }
                break;

            case LIST:  //Works
                lcmd = (struct listcmd*)cmd;

                execcmd(lcmd->left);

                execcmd(lcmd->right);
                
                break;

            case PIPE:
                pcmd = (struct pipecmd*)cmd;

                int fd[2];
                pipe(fd);
                int c1 = fork();
                int c2;

                if(c1==0){
                    dup2(fd[1],STDOUT_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                    execcmd(pcmd->left);
                    exit(0);
                }else{
                    wait(NULL);
                    c2 = fork();
                    if(c2==0){
                        dup2(fd[0], STDIN_FILENO);
                        close(fd[0]);
                        close(fd[1]);
                        execcmd(pcmd->right);
                        exit(0);
                    }else{
                        close(fd[0]);
                        close(fd[1]);
                        wait(NULL);
                    }
                }
                setbuf(stdout, NULL);

                break;

            case BACK:
                bcmd = (struct backcmd*)cmd;

                //IDEA: Store background process pid's somewhere (may need to store more than one if many background processes)
                //Then in the loop in main, call waitpid for all the background pids and check if they finished. If they did then
                //reap them.
                //printcmd(bcmd->cmd);
                //MSG("... the above command will be executed in background. \n");    

                int bpid = fork();

                if (bpid == 0){     //CHILD
                    backgroundPIDs[backPIDIndex] = getpid();    //first add it to the array
                    backPIDIndex++; //incrememt it by 1
                    execcmd(bcmd->cmd); //run it
                    exit(0);
                }
                else {
                    //Parent should execute and not wait for the child to finish
                }

                break;


            default:
                PANIC("");
                
        }
        exec_exit:
        return;
}



int main(void)
{
    signal(SIGINT, handle_sigint);
    static char buf[1024];
    int fd;

    setbuf(stdout, NULL);

    // Read and run input commands.
    while(getcmd(buf, sizeof(buf)) >= 0)
    {
        for (int i = 0; i < backPIDIndex; i++) {
            //printf("TEST");
            waitpid(backgroundPIDs[i], NULL, WNOHANG);   //WNOHANG
        }

        struct cmd * command;
        command = parsecmd(buf);

        //printcmd(command); // TODO: run the parsed command instead of printing it
        execcmd(command);
    }

    PANIC("getcmd error!\n");
    return 0;
}