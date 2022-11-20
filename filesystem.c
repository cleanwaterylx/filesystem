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
        fread(buf, sizeof(char), SIZE, file);          //TODO: ?
        fclose(file);
    }
    else
    {
        printf("文件系统不存在，创建文件系统\n");
        my_format();
        memcpy(buf, v_start_pos, SIZE);
    }

    fcb *root = (fcb *)(v_start_pos + 5 * BLOCKSIZE);
    //TODO: 封装成函数
    strcpy(openfilelist[0].filefcb.filename, root->filename);
    strcpy(openfilelist[0].filefcb.exname, root->exname);
    openfilelist[0].filefcb.attribute = root->attribute;
    openfilelist[0].filefcb.time = root->time;
    openfilelist[0].filefcb.date = root->date;
    openfilelist[0].filefcb.first = root->first;
    openfilelist[0].filefcb.length = root->length;
    openfilelist[0].filefcb.free = root->free;
    openfilelist[0].dirno = 5;
    openfilelist[0].diroff = 0;
    strcpy(openfilelist[0].dir, "\\root\\");
    openfilelist[0].file_ptr = 0;
    openfilelist[0].fcbstate = 0;
    openfilelist[0].topenfile = 1;

    startp = ((block0 *)v_start_pos)->startblock;
    curdirfd = 0;
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
    root->date = time->tm_year << 9 + (time->tm_mon + 1) << 5 + time->tm_mday;  
    root->first = 5;
    root->free = 1;
    root->length = 2 * sizeof(fcb);

    fcb *root2 = root + 1;
    memcpy(root2, root, sizeof(fcb));
    strcpy(root2->filename, "..");
    for (int i = 2; i < (int)(BLOCKSIZE / sizeof(fcb)); i++)
    {
        root2++;
        strcpy(root2->filename, "");
        root->free = 0;
    }

    //写入文件
    FILE *file = fopen(FILENAME, "w");
    fwrite(v_start_pos, sizeof(char), SIZE, file);  //TODO: ？
    fclose(file);
}
