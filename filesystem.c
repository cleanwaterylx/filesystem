#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void my_startsys()
{
    v_start_pos = (unsigned char)malloc(SIZE); //为文件系统分配空间

    printf("读取文件filesys.txt");
    FILE *file;
    if (file = fopen(FILENAME, "r") != NULL)
    {
    }
}

void my_format()
{
    //设置引导块
    block0 *boot = (block0 *)v_start_pos;
    strcpy(boot->information, "文件系统,外存分配方式:FAT,磁盘空间管理:结合于FAT的位示图,目录结构:单用户多级目录结构.");
    boot->root = 5;
    boot->startblock = v_start_pos + BLOCKSIZE * 5;

    //设置FAT表
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    for (int i = 0; i < 5; i++)
    {
        fat1[i].id = END;
    }
    for (int i = 5; i < 1000; i++)
    {
        fat1[i].id = FREE;
    }
    fat *fat2 = (fat *)(v_start_pos + BLOCKSIZE * 3);
    memcpy(fat2, fat1, BLOCKSIZE);

    //根目录区
    fat1[5].id = fat2[5].id = END;
    fcb *root = (fcb *)(v_start_pos + BLOCKSIZE * 5);
    strcpy(root->filename, ".");
    strcpy(root->exname, "di");
    root->attribute = 0;

    time_t rawtime = time(NULL);
    struct tm *time = localtime(&rawtime);
    root->time = time->tm_hour << 11 + time->tm_min << 5 + time->tm_sec >> 1;
    
}