#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// done
void my_startsys()
{
    v_start_pos = (unsigned char)malloc(SIZE); //为文件系统分配空间

    printf("读取文件filesys.txt");
    FILE *file;
    if (file = fopen(FILENAME, "r") != NULL)
    {
        fread(buffer, sizeof(char), SIZE, file); // TODO: ?
        fclose(file);
    }
    else
    {
        printf("文件系统不存在，创建文件系统\n");
        my_format();
        memcpy(buffer, v_start_pos, SIZE);
    }

    fcb *root = (fcb *)(v_start_pos + 5 * BLOCKSIZE);
    // TODO: 封装成函数
    CopyFcbToOpenfilelist(&openfilelist[0], root);
    openfilelist[0].dirno = 5;
    openfilelist[0].diroff = 0;
    strcpy(openfilelist[0].dir, "\\root\\");
    openfilelist[0].file_ptr = 0;
    openfilelist[0].fcbstate = 0;
    openfilelist[0].topenfile = 1;

    startp = ((block0 *)v_start_pos)->startblock;
    curfd = 0;
}
// done
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
    fwrite(v_start_pos, sizeof(char), SIZE, file); // TODO: ？
    fclose(file);
}
// done
void my_ls()
{
    if (openfilelist[curfd].filefcb.attribute == 1)
    {
        printf("数据文件不能使用ls\n");
        return;
    }

    //读取目录文件
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);

    fcb *fcbPtr = (fcb *)buf;
    printf("name\t size\t type\t\t date\t\t time\n");
    for (int i = 0; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++)
    {
        if (fcbPtr->free == 1) // TODO:?
        {
            if (fcbPtr->attribute == 0)
            {
                printf("%s\t%dB\t<DIR>\t%d/%d/%d\t%02d:%02d:%02d\n",
                       fcbPtr->filename, fcbPtr->length,
                       (fcbPtr->date >> 9) + 1900,
                       (fcbPtr->date >> 5) & 0x000f,
                       (fcbPtr->date) & 0x001f,
                       (fcbPtr->time >> 11),
                       (fcbPtr->time >> 5) & 0x003f,
                       ((fcbPtr->time)) & 0x001f * 2);
            }
            else
            {
                unsigned int length = fcbPtr->length;
                if (length != 0)
                    length -= 2;
                printf("%s.%s\t%dB\t<File>\t%d/%d/%d\t%02d:%02d:%02d\n",
                       fcbPtr->filename,
                       fcbPtr->exname,
                       length,
                       (fcbPtr->date >> 9) + 1900,
                       (fcbPtr->date >> 5) & 0x000f,
                       (fcbPtr->date) & 0x001f,
                       (fcbPtr->time >> 11),
                       (fcbPtr->time >> 5) & 0x003f,
                       (fcbPtr->time) & 0x001f * 2);
            }
        }
        fcbPtr++;
    }
}

void my_cd(char *dirname)
{
    if (openfilelist[curfd].filefcb.attribute == 1)
    {
        printf("该文件是数据文件,不能使用cd\n");
        return;
    }
    else
    {
        char buf[MAX_SIZE];
        openfilelist[curfd].file_ptr = 0;
        do_read(curfd, openfilelist[curfd].filefcb.length, buf);
        //寻找目录
        int i = 0;
        fcb *fcbPtr = (fcb *)buf;
        for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++, fcbPtr++)
        {
            if (strcmp(fcbPtr->filename, dirname) == 0 && fcbPtr->attribute == 0)
            {
                break;
            }
        }
        if (strcmp(fcbPtr->attribute, "di") != 0)
        {
            printf("不允许cd非目录文件\n");
            return;
        }
        else
        {
            // cd .
            if (strcmp(fcbPtr->filename, ".") == 0)
            {
                return;
            }
            //
            else if (strcmp(fcbPtr->filename, "..") == 0)
            {
                if (curfd == 0)
                    return;
                else
                {
                }
            }
        }
    }
}
// done
int my_open(char *filename)
{
    //读取当前目录文件
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    char *fname = strtok(filename, ".");
    char *exname = strtok(NULL, ".");

    //寻找文件的fcb
    int i;
    fcb *fcbPtr = (fcb *)buf;
    for (i = 0; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++, fcbPtr++)
    {
        if (strcmp(fcbPtr->filename, fname) == 0 && strcmp(fcbPtr->exname, exname) && fcbPtr->attribute == 1)
        {
            break;
        }
    }
    if (i == (int)(openfilelist[curfd].filefcb.length))
    {
        printf("不存在此文件\n");
        return -1;
    }

    int fd = GetFreeOpenfile();
    if (fd == -1)
    {
        printf("用户打开文件表已满\n");
        return -1;
    }
    CopyFcbToOpenfilelist(&openfilelist[fd], fcbPtr);
    openfilelist[fd].dirno = openfilelist[curfd].filefcb.first;
    openfilelist[fd].diroff = i;
    strcpy(openfilelist[fd].dir, strcat(openfilelist[curfd].dir, filename));
    openfilelist[fd].file_ptr = 0;
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].topenfile = 1;

    curfd = fd;
    return 1;
}

void my_close(int fd)
{
    if (fd > MAXOPENFILE || fd < 0)
    {
        printf("不存在这个文件\n");
        return;
    }
}
// done
int do_read(int fd, int len, char *text)
{
    int lenTmp = len;
    unsigned char *buf = (unsigned char *)malloc(1024);
    if (buf == NULL)
    {
        printf("do_read申请内存失败\n");
        return -1;
    }

    //找到要读的第一个盘块的盘块号
    int offset = openfilelist[fd].file_ptr; //读写位置
    int blockNum = openfilelist[fd].filefcb.first;
    fat *fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum; //当前的fat
    while (offset >= BLOCKSIZE)
    {
        offset -= BLOCKSIZE;
        blockNum = fatPtr->id; //下一个盘块
        if (blockNum == END)
        {
            printf("do_read寻找的块不存在\n");
            return -1;
        }
        fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum;
    }

    unsigned char *blockPtr = v_start_pos + BLOCKSIZE * blockNum;
    memcpy(buf, blockPtr, BLOCKSIZE);
    char *textPtr = text; //维护text指针

    while (len > 0)
    {
        if (BLOCKSIZE - offset > len)
        {
            memcpy(textPtr, buf + offset, len);
            textPtr += len;
            offset += len;
            openfilelist[fd].file_ptr += len;
            len = 0;
        }
        else
        {
            memcpy(text, buf + offset, BLOCKSIZE - offset);
            textPtr += BLOCKSIZE - offset;
            offset = 0;
            len -= BLOCKSIZE - offset;

            blockNum = fatPtr->id;
            if (blockNum == END)
            {
                printf("len太长\n");
                break;
            }
            fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum;
            blockPtr = v_start_pos + BLOCKSIZE * blockNum;
            memcpy(buf, blockPtr, BLOCKSIZE);
        }
    }
    *textPtr = '\0';
    free(buf);
    return lenTmp - len;
}
// done
int my_read(int fd, int len)
{
    if (fd >= MAXOPENFILE || fd < 0)
    {
        printf("文件不存在\n");
        return -1;
    }
    openfilelist[fd].file_ptr = 0;
    char text[MAX_SIZE];
    do_read(fd, len, text);
    printf("读取的结果是：%s\n", text);
    return 1;
}

int GetFreeOpenfile()
{
    for (int i = 0; i < MAXOPENFILE; i++)
    {
        if (openfilelist[i].topenfile == 0)
        {
            openfilelist[i].topenfile = 1;
            return i;
        }
    }
    return -1;
}

int FindFatherDir(int fd)
{
    for (int i = 0; i < MAXOPENFILE; i++)
    {
        if(openfilelist[i].filefcb.first == openfilelist[fd].dirno)
        {
            return i;
        }
    }
    return -1;
}

//复制fcb到openfilelist
void CopyFcbToOpenfilelist(useropen *useropenPtr, fcb *fcbPtr)
{
    memcpy(&useropenPtr->filefcb, fcbPtr, sizeof(fcb));
}