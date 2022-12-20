#include "filesystem.h"
#include <stdio.h>
#include <string.h>

int main()
{
    char cmd[15][10] = {"mkdir", "rmdir", "ls", "cd", "create", "rm", "open", "close", "write", "read", "exit", "help"};
    char temp[30], command[30], *sp, *len, yesorno;
    int indexOfCmd, i;
    int length = 0;

    printf("\n\n************************ 文件系统 **************************************************\n");
    printf("************************************************************************************\n\n");
    my_startsys();
    printf("文件系统已开启.\n\n");
    printf("输入help来显示帮助页面.\n\n");

    while (1)
    {
        printf("%s>", openfilelist[curfd].dir);
        fgets(temp, 100, stdin);
        indexOfCmd = -1;

        for (int i = 0; i < strlen(temp) - 1; i++)
        {
            command[i] = temp[i]; // fgets存在一个bug，会把\n也读进去，所以要微调一下
            command[i + 1] = '\0';
        }

        if (strcmp(command, ""))
        {                              // 不是空命令
            sp = strtok(command, " "); // 把空格前的命令分离出来
            // printf("%s\n",sp);
            for (i = 0; i < 15; i++)
            {
                if (strcmp(sp, cmd[i]) == 0)
                {
                    indexOfCmd = i;
                    break;
                }
            }
            switch (indexOfCmd)
            {
            case 0: // mkdir
                sp = strtok(NULL, " ");
                //  printf("%s\n",sp);
                if (sp != NULL)
                    my_mkdir(sp);
                else
                    error("mkdir");
                break;
            case 1: // rmdir
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_rmdir(sp);
                else
                    error("rmdir");
                break;
            case 2: // ls
                my_ls();
                break;
            case 3: // cd
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_cd(sp);
                else
                    error("cd");
                break;
            case 4: // create
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_create(sp);
                else
                    error("create");
                break;
            case 5: // rm
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_rm(sp);
                else
                    error("rm");
                break;
            case 6: // open
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_open(sp);
                else
                    error("open");
                break;
            case 7: // close
                if (openfilelist[curfd].filefcb.attribute == 1)
                    my_close(curfd);
                else
                    printf("当前没有的打开的文件\n");
                break;
            case 8: // write
                if (openfilelist[curfd].filefcb.attribute == 1)
                    my_write(curfd);
                else
                    printf("请先打开文件,然后再使用write操作\n");
                break;
            case 9: // read
                sp = strtok(NULL, " ");
                length = 0;
                if (sp != NULL)
                {
                    for (int i = 0; i < strlen(sp); i++)
                        length = length * 10 + sp[i] - '0';
                }
                if (length == 0)
                    error("read");
                else if (openfilelist[curfd].filefcb.attribute == 1)
                    my_read(curfd, length);
                else
                    printf("请先打开文件,然后再使用read操作\n");
                break;

            case 10: // exit
                my_exitsys();
                printf("退出文件系统.\n");
                return 0;
                break;
            case 11: // help
                show_help();
                break;
            default:
                printf("没有 %s 这个命令\n", sp);
                break;
            }
        }
        else
            printf("\n");
    }
    return 0;
}

void error(char *command)
{
    printf("%s : 缺少参数\n", command);
    printf("输入 'help' 来查看命令提示.\n");
}

void show_help()
{
    printf("命令名\t\t命令参数\t\t命令功能\n\n");
    printf("cd\t\t目录名(路径名)\t\t切换当前目录到指定目录\n");
    printf("mkdir\t\t目录名\t\t\t在当前目录创建新目录\n");
    printf("rmdir\t\t目录名\t\t\t在当前目录删除指定目录\n");
    printf("ls\t\t无\t\t\t显示当前目录下的目录和文件\n");
    printf("create\t\t文件名\t\t\t在当前目录下创建指定文件\n");
    printf("rm\t\t文件名\t\t\t在当前目录下删除指定文件\n");
    printf("open\t\t文件名\t\t\t在当前目录下打开指定文件\n");
    printf("write\t\t无\t\t\t在打开文件状态下,写该文件\n");
    printf("read\t\t无\t\t\t在打开文件状态下,读取该文件\n");
    printf("close\t\t无\t\t\t在打开文件状态下,关闭该文件\n");
    printf("exit\t\t无\t\t\t退出系统\n\n");
}
