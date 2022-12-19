#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// done
void my_startsys()
{
    v_start_pos = (unsigned char *)malloc(SIZE); // 为文件系统分配空间

    printf("读取文件filesys.txt");
    FILE *file;
    if ((file = fopen(FILENAME, "r")) != NULL)
    {
        fread(buffer, SIZE, 1, file); // TODO: ?
        fclose(file);
        memcpy(v_start_pos, buffer, SIZE);
    }
    else
    {
        printf("文件系统不存在，创建文件系统\n");
        my_format();
        memcpy(buffer, v_start_pos, SIZE);
    }

    fcb *root = (fcb *)(v_start_pos + 5 * BLOCKSIZE);
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
    // 设置引导块
    block0 *boot = (block0 *)v_start_pos;
    strcpy(boot->information, "文件系统,外存分配方式:FAT,磁盘空间管理:结合于FAT的位示图,目录结构:单用户多级目录结构.");
    boot->root = 5;
    boot->startblock = v_start_pos + BLOCKSIZE * 5;

    // 设置FAT表
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

    // 根目录区
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

    // 写入文件
    FILE *file = fopen(FILENAME, "w");
    fwrite(v_start_pos, SIZE, 1, file); // TODO: ？
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

    // 读取目录文件
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);

    fcb *fcbPtr = (fcb *)buf;
    printf("name\t size\t type\t\t date\t\t time\n");
    for (int i = 0; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++)
    {
        if (fcbPtr->free == 1) 
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
// done
int my_create(char *filename)
{
    char *fname = strtok(filename, ".");
    char *exname = strtok(NULL, ".");
    if (strcmp(fname, "") == 0)
    {
        printf("请输入文件名\n");
        return -1;
    }
    if (!exname)
    {
        printf("请输入后缀名\n");
        return -1;
    }
    if (openfilelist[curfd].filefcb.attribute == 1)
    {
        printf("数据文件不允许create\n");
        return -1;
    }

    // 读取目录文件
    openfilelist[curfd].file_ptr = 0;
    char buf[MAX_SIZE];
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    int i;
    fcb *fcbPtr = (fcb *)buf;
    for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++)
    {
        if (strcmp(fcbPtr[i].filename, fname) == 0 && strcmp(fcbPtr[i].exname, exname))
        {
            printf("已有同名文件\n");
            return -1;
        }
    }

    // 寻找空的fcb
    for (i = 0; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++)
    {
        if (fcbPtr[i].free == 0)
            break;
    }
    openfilelist[curfd].file_ptr = i * sizeof(fcb);
    openfilelist[curfd].fcbstate = 1;

    int blockNum = GetFreeBlock();
    if (blockNum == END)
    {
        return -1;
    }
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    fat *fat2 = (fat *)(v_start_pos + BLOCKSIZE * 3);
    fat1[blockNum].id = END;
    fat2[blockNum].id = END;

    fcb *fcbtmp = (fcb *)malloc(sizeof(fcb));
    fcbtmp->attribute = 1;
    time_t rawtime = time(NULL);
    struct tm *time = localtime(&rawtime);
    fcbtmp->time = time->tm_hour << 11 + time->tm_min << 5 + time->tm_sec >> 1;
    fcbtmp->date = time->tm_year << 9 + (time->tm_mon + 1) << 5 + time->tm_mday;
    strcpy(fcbtmp->filename, fname);
    strcpy(fcbtmp->exname, exname);
    fcbtmp->first = blockNum;
    fcbtmp->length = 0;
    fcbtmp->free = 1;
    do_write(curfd, (char *)fcbtmp, sizeof(fcb), 1);
    // 更新当前目录的fcb
    fcbPtr = (fcb *)buf;
    fcbPtr->length = openfilelist[curfd].filefcb.length;
    openfilelist[curfd].file_ptr = 0;
    do_write(curfd, (char *)fcbPtr, sizeof(fcb), 1);

    return 0;
}
// done
void my_rm(char *filename)
{
    char *fname = strtok(filename, ".");
    char *exname = strtok(NULL, ".");
    if (!exname)
    {
        printf("请输入后缀名\n");
        return;
    }
    if (strcmp(exname, "di") == 0)
    {
        printf("不能删除目录文件\n");
        return;
    }

    // 读取目录文件
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    int i;
    fcb *fcbPtr = (fcb *)buf;
    for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++)
    {
        if (strcmp(fcbPtr->filename, fname) == 0 && strcmp(fcbPtr->exname, exname) == 0)
        {
            break;
        }
    }
    if (i == (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)))
    {
        printf("没有这个文件\n");
        return;
    }

    // 清空fat
    int blockNum = fcbPtr->first;
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    fat *fat2 = (fat *)(v_start_pos + BLOCKSIZE * 3);
    int next = 0;
    while (1)
    {
        next = fat1[blockNum].id;
        fat1[blockNum].id = FREE;
        if (next != END)
        {
            blockNum = next;
        }
        else
            break;
    }
    memcpy(fat2, fat1, sizeof(fat));

    // 清空fcb
    fcbPtr->free = 0;
    fcbPtr->time = 0;
    fcbPtr->date = 0;
    fcbPtr->exname[0] = '\0';
    fcbPtr->filename[0] = '\0';
    fcbPtr->first = 0;
    fcbPtr->length = 0;
    openfilelist[curfd].file_ptr = i * sizeof(fcb);
    do_write(curfd, (char *)fcbPtr, sizeof(fcb), 1);
    openfilelist[curfd].filefcb.length -= sizeof(fcb);
    // 更新当前目录的fcb
    fcbPtr = (fcb *)buf;
    fcbPtr->length = openfilelist[curfd].filefcb.length;
    openfilelist[curfd].file_ptr = 0;
    do_write(curfd, (char *)fcbPtr, sizeof(fcb), 1);
    openfilelist[curfd].fcbstate = 1;
}
// done
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
        // 寻找目录 fcbPtr
        int i = 0;
        fcb *fcbPtr = (fcb *)buf;
        // TODO: 多层目录需要递归？
        for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++, fcbPtr++)
        {
            if (strcmp(fcbPtr->filename, dirname) == 0 && fcbPtr->attribute == 0)
            {
                break;
            }
        }
        if (strcmp(fcbPtr->exname, "di") != 0)
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
            // cd ..
            else if (strcmp(fcbPtr->filename, "..") == 0)
            {
                if (curfd == 0)
                    return; // root
                else
                {
                    curfd = my_close(curfd);
                    return;
                }
            }
            else
            {
                int fd = GetFreeOpenfile();
                if (fd == -1)
                    return;
                else
                {
                    CopyFcbToOpenfilelist(&openfilelist[fd], fcbPtr);
                    openfilelist[fd].fcbstate = 0;
                    openfilelist[fd].file_ptr = 0;
                    openfilelist[fd].topenfile = 1;
                    openfilelist[fd].dirno = openfilelist[curfd].filefcb.first;
                    openfilelist[fd].diroff = i;
                    char *tmp = "\\";
                    strcpy(openfilelist[fd].dir, strcat(strcat(openfilelist[curfd].dir, dirname), tmp));
                    curfd = fd;
                }
            }
        }
    }
}
// done
void my_mkdir(char *dirname)
{
    char *fname = strtok(dirname, ".");
    char *exname = strtok(NULL, ".");
    if (exname)
    {
        printf("不允许输入后缀名\n");
        return;
    }

    // 读取当前目录文件
    char text[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    int fileLen = do_read(curfd, openfilelist[curfd].filefcb.length, text);
    fcb *fcbPtr = (fcb *)(text);
    for (int i = 0; i < (int)(fileLen / sizeof(fcb)); i++)
    {
        if (strcmp(dirname, fcbPtr[i].filename) == 0 && fcbPtr[i].attribute == 0)
        {
            printf("该目录已经存在\n");
            return;
        }
    }

    int fd = GetFreeOpenfile();
    if (fd == -1)
    {
        printf("打开文件表已满\n");
        return;
    }

    unsigned short int blockNum = GetFreeBlock();
    if (blockNum == END)
    {
        printf("盘块不足\n");
        openfilelist[fd].topenfile = 0;
        return;
    }
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    fat *fat2 = (fat *)(v_start_pos + BLOCKSIZE * 3);
    fat1[blockNum].id = END;
    fat2[blockNum].id = END;

    // 找到未分配的目录项
    int i = 0;
    for (; i < (int)(fileLen / sizeof(fcb)); i++)
    {
        if (fcbPtr[i].free == 0)
            break;
    }
    openfilelist[curfd].file_ptr = i * sizeof(fcb);
    openfilelist[curfd].fcbstate = 1;

    fcb *fcbtmp = (fcb *)malloc(sizeof(fcb));
    fcbtmp->attribute = 0;
    time_t rawtime = time(NULL);
    struct tm *time = localtime(&rawtime);
    fcbtmp->time = time->tm_hour << 11 + time->tm_min << 5 + time->tm_sec >> 1;
    fcbtmp->date = time->tm_year << 9 + (time->tm_mon + 1) << 5 + time->tm_mday;
    strcpy(fcbtmp->filename, dirname);
    strcpy(fcbtmp->exname, "di");
    fcbtmp->first = blockNum;
    fcbtmp->length = 2 * sizeof(fcb); // . & .. 的fcb
    fcbtmp->free = 1;
    do_write(curfd, (char *)fcbtmp, sizeof(fcb), 1);

    CopyFcbToOpenfilelist(&openfilelist[fd], fcbtmp);
    openfilelist[fd].dirno = openfilelist[curfd].filefcb.first;
    openfilelist[fd].diroff = i;
    char *tmp = "\\";
    strcpy(openfilelist[fd].dir, strcat(strcat(openfilelist[curfd].dir, dirname), tmp));
    openfilelist[fd].file_ptr = 0;
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].topenfile = 1;

    // 添加.和..
    strcpy(fcbtmp->filename, ".");
    do_write(fd, (char *)fcbtmp, sizeof(fcb), 1);
    strcpy(fcbtmp->filename, "..");
    fcbtmp->first = openfilelist[curfd].filefcb.first;
    fcbtmp->length = openfilelist[curfd].filefcb.length;
    fcbtmp->date = openfilelist[curfd].filefcb.date;
    fcbtmp->time = openfilelist[curfd].filefcb.time;
    do_write(fd, (char *)fcbtmp, sizeof(fcb), 1);

    my_close(fd);
    // 更新currfd目录文件的fcb
    fcbPtr->length = openfilelist[curfd].filefcb.length;
    openfilelist[curfd].file_ptr = 0;
    do_write(curfd, (char *)fcbPtr, sizeof(fcb), 1);
    openfilelist[curfd].fcbstate = 1;
    free(fcbtmp);
}
//done
void my_rmdir(char *dirname)
{
    char *fname = strtok(dirname, ".");
    char *exname = strtok(NULL, ".");
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)
    {
        printf("无法删除\n");
        return;
    }
    if (exname)
    {
        printf("不需要输入后缀名\n");
        return;
    }

    // 读取curfd
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    int i;
    fcb *fcbPtr = (fcb *)buf;
    for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++)
    {
        if (strcmp(fcbPtr->filename, fname) == 0 && strcmp(fcbPtr->exname, exname) == 0)
        {
            break;
        }
    }
    if (i == (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)))
    {
        printf("没有这个文件\n");
        return;
    }

    // TODO 删除目录下的文件及递归删除

    if (fcbPtr->length > 2 * sizeof(fcb))
    {
        printf("请先清空这个目录下的所有文件,再删除目录文件\n");
        return;
    }
    //清空fat
    int blockNum = fcbPtr->first;
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    fat *fat2 = (fat *)(v_start_pos + BLOCKSIZE * 3);
    int next = 0;
    while (1)
    {
        next = fat1[blockNum].id;
        fat1[blockNum].id = END;
        if (next != END)
        {
            blockNum = next;
        }
        else
            break;
    }
    memcpy(fat2, fat1, sizeof(fat));

    // 清空fcb
    fcbPtr->free = 0;
    fcbPtr->time = 0;
    fcbPtr->date = 0;
    fcbPtr->exname[0] = '\0';
    fcbPtr->filename[0] = '\0';
    fcbPtr->first = 0;
    fcbPtr->length = 0;
    openfilelist[curfd].file_ptr = i * sizeof(fcb);
    do_write(curfd, (char *)fcbPtr, sizeof(fcb), 1);
    openfilelist[curfd].filefcb.length -= sizeof(fcb);
    // 更新当前目录的fcb
    fcbPtr = (fcb *)buf;
    fcbPtr->length = openfilelist[curfd].filefcb.length;
    openfilelist[curfd].file_ptr = 0;
    do_write(curfd, (char *)fcbPtr, sizeof(fcb), 1);
    openfilelist[curfd].fcbstate = 1;

}
// done
int my_open(char *filename)
{
    // 读取当前目录文件
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    char *fname = strtok(filename, ".");
    char *exname = strtok(NULL, ".");

    // 寻找文件的fcb
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
// done
int my_close(int fd)
{
    if (fd > MAXOPENFILE || fd < 0)
    {
        printf("不存在这个文件\n");
        return -1;
    }
    else
    {
        int fatherfd = FindFatherDir(fd);
        if (fatherfd == -1)
        {
            printf("父目录不存在\n");
            return -1;
        }
        // 写回fcb
        if (openfilelist[fd].fcbstate == 1)
        {
            char buf[MAX_SIZE];
            do_read(fatherfd, openfilelist[fatherfd].filefcb.length, buf);
            fcb *fcbPtr = (fcb *)(buf + sizeof(fcb) * openfilelist[fd].diroff);
            CopyOpenfilelistToFcb(&openfilelist[fd], fcbPtr);
            openfilelist[fatherfd].file_ptr = openfilelist[fd].diroff * sizeof(fcb);
            do_write(fatherfd, (char *)fcbPtr, sizeof(fcb), 1);
        }
        // 清空openfilelist[fd]
        memset(&openfilelist[fd], 0, sizeof(useropen));
        curfd = fatherfd;
        return fatherfd;
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

    // 找到要读的第一个盘块的盘块号
    int offset = openfilelist[fd].file_ptr; // 读写位置
    int blockNum = openfilelist[fd].filefcb.first;
    fat *fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum; // 当前的fat
    while (offset >= BLOCKSIZE)
    {
        offset -= BLOCKSIZE;
        blockNum = fatPtr->id; // 下一个盘块
        if (blockNum == END)
        {
            printf("do_read寻找的块不存在\n");
            return -1;
        }
        fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum;
    }

    unsigned char *blockPtr = v_start_pos + BLOCKSIZE * blockNum;
    memcpy(buf, blockPtr, BLOCKSIZE);
    char *textPtr = text; // 维护text指针

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
// done
int do_write(int fd, char *text, int len, char wstyle)
{
    int blockNum = openfilelist[fd].filefcb.first;
    fat *fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum;
    if (wstyle == 0)
    {
        // 截断写
        openfilelist[fd].file_ptr = 0;
        openfilelist[fd].filefcb.length = 0;
    }
    else if (wstyle == 1)
    {
        // 覆盖写
        if (openfilelist[fd].filefcb.attribute == 1 && openfilelist[fd].filefcb.length != 0)
        {
            openfilelist[fd].file_ptr -= 1;
        }
    }
    else if (wstyle == 2)
    {
        // 追加写
        if (openfilelist[fd].filefcb.attribute == 0)
        {
            openfilelist[fd].file_ptr = openfilelist[fd].filefcb.length;
        }
        else if (openfilelist[fd].filefcb.attribute == 1 && openfilelist[fd].filefcb.length != 0)
        {
            openfilelist[fd].file_ptr = openfilelist[fd].filefcb.length - 1;
        }
    }

    int off = openfilelist[fd].file_ptr;
    // 若off > BLOCKSIZE 找到那个盘块
    while (off > BLOCKSIZE)
    {
        blockNum = fatPtr->id;
        if (blockNum == END)
        {
            if (DistributeBlock(&blockNum, fatPtr) == -1)
                return -1;
        }
        fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum;
        off -= BLOCKSIZE;
    }

    unsigned char *buf = (unsigned char *)malloc(BLOCKSIZE * sizeof(unsigned char));
    if (buf == NULL)
    {
        printf("申请内存失败\n");
        return -1;
    }

    unsigned char *blockPtr = (unsigned char *)(v_start_pos + BLOCKSIZE * blockNum);
    int lenTmp = 0;
    char *textTmp = text;
    // 写
    while (len > lenTmp)
    {
        memcpy(buf, blockPtr, BLOCKSIZE); // 将盘块读取到buf中
        for (; off < BLOCKSIZE; off++)
        {
            *(buf + off) = *textTmp;
            textTmp++;
            lenTmp++;
            if (len == lenTmp)
                break;
        }
        memcpy(blockPtr, buf, BLOCKSIZE); // 将buf拷贝到盘块中
        if (off == BLOCKSIZE && len != lenTmp)
        {
            off = 0;
            blockNum = fatPtr->id;
            if (blockNum == END)
            {
                if (DistributeBlock(&blockNum, fatPtr) == -1)
                    return -1;
                blockPtr = (unsigned char *)(v_start_pos + BLOCKSIZE * blockNum);
            }
            else
            {
                blockPtr = (unsigned char *)(v_start_pos + BLOCKSIZE * blockNum);
                fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum;
            }
        }
    }

    openfilelist[fd].file_ptr += len;
    if (openfilelist[fd].file_ptr > openfilelist[fd].filefcb.length)
        openfilelist[fd].filefcb.length = openfilelist[fd].file_ptr;
    free(buf);
    // 释放空闲的盘块, 修改fat表
    int i = blockNum;
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    while (1)
    {
        if (fat1[i].id != END)
        {
            int next = fat1[i].id;
            fat1[i].id = FREE;
            i = next;
        }
        else
            break;
    }
    fat1[blockNum].id = END;
    // 同步fat2
    memcpy((fat *)(v_start_pos + BLOCKSIZE * 3), (fat *)(v_start_pos + BLOCKSIZE), BLOCKSIZE * 2);
    return len;
}
// down
int my_write(int fd)
{
    if (fd < 0 || fd >= MAXOPENFILE)
    {
        printf("文件不存在\n");
        return -1;
    }
    int wstyle;
    printf("输入: 0=截断写, 1=覆盖写, 2=追加写\n");
    scanf("%d", &wstyle);
    char text[MAX_SIZE] = "\0";
    int i = 0;
    while (text[i++] = getchar() != EOF)
    {
        if (i >= MAX_SIZE)
            break;
    }
    text[i] = '\0';

    do_write(fd, text, strlen(text) + 1, wstyle);
    return 1;
}

void my_exitsys()
{
    while(curfd)
    {
        my_close(curfd);
    }
    FILE *fp = fopen(FILENAME, "w");
    fwrite(v_start_pos, SIZE, 1, fp);
    fclose(fp);
    free(v_start_pos);
}

unsigned short int GetFreeBlock()
{
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    for (int i = 0; i < (int)(SIZE / BLOCKSIZE); i++)
    {
        if (fat1->id == FREE)
        {
            return i;
        }
    }
    return -1;
}

// 分配盘块
int DistributeBlock(int *blockNum, fat *fatPtr)
{
    *blockNum = GetFreeBlock();
    if (*blockNum == END)
    {
        printf("盘块不足\n");
        return -1;
    }
    else
    {
        fatPtr->id = *blockNum;
        fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + *blockNum;
        fatPtr->id = END;
        return 1;
    }
}

// 得到空闲的打开文件表
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

// 得到父目录
int FindFatherDir(int fd)
{
    for (int i = 0; i < MAXOPENFILE; i++)
    {
        if (openfilelist[i].filefcb.first == openfilelist[fd].dirno)
        {
            return i;
        }
    }
    return -1;
}

// 复制fcb到openfilelist
void CopyFcbToOpenfilelist(useropen *useropenPtr, fcb *fcbPtr)
{
    memcpy(&useropenPtr->filefcb, fcbPtr, sizeof(fcb));
}

// 复制openfilelist到fcb
void CopyOpenfilelistToFcb(useropen *useropenPtr, fcb *fcbPtr)
{
    memcpy(fcbPtr, &useropenPtr->filefcb, sizeof(fcb));
}