#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#define INPUT 0
#define OUTPUT 1
#define APPEND 2


void rm_whiteSpace(char* buf_c)
{
    if(buf_c[strlen(buf_c)-1]==' ' || buf_c[strlen(buf_c)-1]=='\n')
    buf_c[strlen(buf_c)-1]='\0';
    if(buf_c[0]==' ' || buf_c[0]=='\n') memmove(buf_c, buf_c+1, strlen(buf_c));
}


void tok_buffer(char** par_c,int *siz_c,char *buf_c,const char *c)
{
    char *tok;
    tok=strtok(buf_c,c);
    int i=-1;
    while(tok)
    {
        par_c[++i]=malloc(sizeof(tok)+1);
        strcpy(par_c[i],tok);
        rm_whiteSpace(par_c[i]);
        tok=strtok(NULL,c);
    }
    par_c[++i]=NULL;
    *siz_c=i;
}

void execbasic(char** argv)
{
    if(fork()>0)
    {
        wait(NULL);
    }
    else
    {
        execvp(argv[0],argv);
        perror("invalid input\n");
        exit(1);
    }
}

void execpipe(char** buf_c,int siz_c)
{
    if(siz_c>10) return;
    
    int f[10][2],i,p;
    char *argv[100];

    for(i=0;i<siz_c;i++)
    {
        tok_buffer(argv,&p,buf_c[i]," ");
        if(i!=siz_c-1)
        {
            if(pipe(f[i])<0)
            {
                perror("pipe not created successfully\n");
                return;
            }
        }
        if(fork()==0)
        {
            if(i!=siz_c-1)
            {
                dup2(f[i][1],1);
                close(f[i][0]);
                close(f[i][1]);
            }
            if(i!=0)
            {
                dup2(f[i-1][0],0);
                close(f[i-1][1]);
                close(f[i-1][0]);
            }
            execvp(argv[0],argv);
            perror("invalid input\n");
            exit(1);
        }
        if(i!=0)
        {
            close(f[i-1][0]);
            close(f[i-1][1]);
        }
        wait(NULL);
    }
}

void execfile(char** buf_c,int siz_c,int mode)
{
    int p,f;
    char *argv[100];
    rm_whiteSpace(buf_c[1]);
    tok_buffer(argv,&p,buf_c[0]," ");
    if(fork()==0)
    {
        switch(mode)
        {
            case INPUT:  f=open(buf_c[1],O_RDONLY); break;
            case OUTPUT: f=open(buf_c[1],O_WRONLY); break;
            case APPEND: f=open(buf_c[1],O_WRONLY | O_APPEND); break;
            default: return;
        }

        if(f<0)
        {
            perror("unable to open file\n");
            return;
        }

        switch(mode)
        {
            case INPUT:         dup2(f,0); break;
            case OUTPUT:        dup2(f,1); break;
            case APPEND:        dup2(f,1); break;
            default: return;
        }
        execvp(argv[0],argv);
        perror("invalid input\n");
        exit(1);
    }
    wait(NULL);
}

void Help()
{
    printf(   "\n----------Help--------\n");
    printf(   "\nNot all internal commands are supported.\n");
    printf(   "\nList of supported internal commands: 'cd', 'pwd', 'echo', 'exit' \n");
    printf(   "\nPiped together commands are supported\n");
    printf(   "\nMultiple commands are allowed seperated by ;\n");
    printf(   "Example case: ps -e ; date | wc -w\n");
    printf(   "\ncommand or processes or executable file can run in background using &. example case: 'ls &' or './myprogram &'\n");
    printf(   "\nOutput redirection on file is supported. example case: ls > fileOutput\n");
    printf(   "\nOutput redirection on file with append is supported. example case: ls >> fileOutput\n");
    printf(   "\nInput redirection from file is supported. example case: wc -c < fileInput\n");
}

void launchbg(char** args, int bg)
{     
    pid_t pid = fork();
    if(pid==-1)
    {
        printf("\nchild process not created\n");
    }
    if(pid==0)
    {
        signal(SIGINT, SIG_IGN);
        kill(getpid(),SIGTERM);
    }
    if (bg == 0)
    {
        waitpid(pid,NULL,0);
    }
    else
    {
        if(fork()>0)
        {
        wait(NULL);
        }
        else
        {
            printf("\nProcess created with PID: %d\n",pid);
            execvp(args[0],&args[1]);
            exit(1);
        }
    }  
    wait(NULL);  
}

int execbg(char * args[], int siz_c)
{
    
    int bg = 1;
    launchbg(args,bg);
    return 1;
}


void check_mul(char* buf_c)
{
    char *buffer[100],buffer2[500],buffer3[500], *par1[100],*par2[100],*token,cwd[1024];
    int siz_c=0;
    if(strchr(buf_c,'|'))
    {
        tok_buffer(buffer,&siz_c,buf_c,"|");
        execpipe(buffer,siz_c);
    }
    else if(strchr(buf_c,'&'))
    {
        tok_buffer(buffer,&siz_c,buf_c,"&");
        execbg(buffer, siz_c);
    }
    else if(strstr(buf_c,">>"))
    {
        tok_buffer(buffer,&siz_c,buf_c,">>");
        if(siz_c==2)execfile(buffer,siz_c,APPEND);
        else printf("\nIncorrect append output!\n");
    }
    else if(strchr(buf_c,'>'))
    {
       tok_buffer(buffer,&siz_c,buf_c,">");
       if(siz_c==2)execfile(buffer,siz_c, OUTPUT);
       else printf("\nIncorrect output redirection!\n");
    }
    else if(strchr(buf_c,'<'))
    {
        tok_buffer(buffer,&siz_c,buf_c,"<");
        if(siz_c==2)execfile(buffer,siz_c, INPUT);
        else printf("\nIncorrect input redirection!\n");
    }
    else
    {
        tok_buffer(par1,&siz_c,buf_c," ");
        if(strstr(par1[0],"cd"))
        {
            chdir(par1[1]);
        }
        else if(strstr(par1[0],"help"))
        {
            Help();
        }
        else if(strstr(par1[0],"exit"))
        {
            exit(0);
        }
        else execbasic(par1);
    }
}


void execsemi(char** buf_c,int siz_c)
{
    int i,p;
    char *argv[100];
    for(i=0;i<siz_c;i++)
    {
        check_mul(buf_c[i]);
    }
    for(i=0;i<siz_c;i++)
    {
        wait(NULL);
    }

}

int main(int argc, char** argv)
{
    char inp[500],*buffer[100],buffer2[500],buffer3[500], *par1[100],*par2[100],*token,cwd[1024];
    int siz=0;
    printf("\n*****************************************************************\n");
    printf("\n****************************MY SHELL*****************************\n");
    printf("\n*****************************************************************\n");

    while(1){
         
        printf ("\n\033[1m Input any command or type 'exit' to exit:\n\033[0m");

        if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("\nminiBash>");
        else    perror("getcwd failed\n");

        //user input
        fgets(inp, 500, stdin); 

        //condition for multiple commands
        if(strchr(inp,';'))
        {
            tok_buffer(buffer,&siz,inp,";");
            execsemi(buffer,siz);
        }
        //condition for pipe commands
        else if(strchr(inp,'|'))
        {
            tok_buffer(buffer,&siz,inp,"|");
            execpipe(buffer,siz);
        }
        // condition for background command
        else if(strchr(inp,'&'))
        {
            tok_buffer(buffer,&siz,inp,"&");
            execbg(buffer, siz);
        }
        //condition for append operation in command
        else if(strstr(inp,">>"))
        {
            tok_buffer(buffer,&siz,inp,">>");
            if(siz==2)execfile(buffer,siz,APPEND);
            else printf("\nIncorrect output!\n");
        }
        //condition for redirection of output of file
        else if(strchr(inp,'>'))
        {
            tok_buffer(buffer,&siz,inp,">");
            if(siz==2)execfile(buffer,siz, OUTPUT);
            else printf("\nIncorrect output redirection!\n");
        }
        //condition for redirection of file to input
        else if(strchr(inp,'<'))
        {
            tok_buffer(buffer,&siz,inp,"<");
            if(siz==2)execfile(buffer,siz, INPUT);
            else printf("\nIncorrect input redirection!\n");
        }
        //condition for single command including internal ones
        else{
            tok_buffer(par1,&siz,inp," ");
            //cd command
            if(strstr(par1[0],"cd"))
            {
                chdir(par1[1]);
            }
            //help command
            else if(strstr(par1[0],"help"))
            {
                Help();
            }
            //exit command
            else if(strstr(par1[0],"exit"))
            {
                exit(0);
            }
            else execbasic(par1);
        }
    }

    return 0;
}