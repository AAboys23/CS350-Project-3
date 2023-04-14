#include "parser.h"

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

            break;


        default:
            PANIC("");
    
    }
    
    printcmd_exit:

    return;
}


int main(void)
{
    static char buf[1024];
    int fd;

    setbuf(stdout, NULL);

    // Read and run input commands.
    while(getcmd(buf, sizeof(buf)) >= 0)
    {
        struct cmd * command;
        command = parsecmd(buf);
        printcmd(command); // TODO: run the parsed command instead of printing it
    }

    PANIC("getcmd error!\n");
    return 0;
}
