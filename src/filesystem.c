#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// done
void my_startsys()
{
    v_start_pos = (unsigned char *)malloc(SIZE); // Ϊ�ļ�ϵͳ����ռ�

    printf("��ȡ�ļ�filesys.txt");
    FILE *file;
    if ((file = fopen(FILENAME, "r")) != NULL)
    {
        fread(buffer, SIZE, 1, file); // TODO: ?
        fclose(file);
        memcpy(v_start_pos, buffer, SIZE);
    }
    else
    {
        printf("�ļ�ϵͳ�����ڣ������ļ�ϵͳ\n");
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
    // ����������
    block0 *boot = (block0 *)v_start_pos;
    strcpy(boot->information, "�ļ�ϵͳ,�����䷽ʽ:FAT,���̿ռ����:�����FAT��λʾͼ,Ŀ¼�ṹ:���û��༶Ŀ¼�ṹ.");
    boot->root = 5;
    boot->startblock = v_start_pos + BLOCKSIZE * 5;

    // ����FAT��
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

    // ��Ŀ¼��
    fat1[5].id = fat2[5].id = END;
    fcb *root = (fcb *)(v_start_pos + BLOCKSIZE * 5);
    strcpy(root->filename, ".");
    strcpy(root->exname, "di");
    root->attribute = 0;

    time_t rawtime = time(NULL);
    struct tm *time = localtime(&rawtime);
    root->time = (time->tm_hour << 11) + (time->tm_min << 5) + (time->tm_sec >> 1);
    root->date = (time->tm_year << 9) + (time->tm_mon + 1 << 5) + time->tm_mday;
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
        root2->free = 0;
    }

    // д���ļ�
    FILE *file = fopen(FILENAME, "w");
    fwrite(v_start_pos, SIZE, 1, file); // TODO: ��
    fclose(file);
}
// done
void my_ls()
{
    if (openfilelist[curfd].filefcb.attribute == 1)
    {
        printf("�����ļ�����ʹ��ls\n");
        return;
    }

    // ��ȡĿ¼�ļ�
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
                printf("%s\t%dB\t<DIR>\t\t%d/%d/%d\t%02d:%02d:%02d\n",
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
                printf("%s.%s\t%dB\t<File>\t\t%d/%d/%d\t%02d:%02d:%02d\n",
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
        printf("�������ļ���\n");
        return -1;
    }
    if (!exname)
    {
        printf("�������׺��\n");
        return -1;
    }
    if (openfilelist[curfd].filefcb.attribute == 1)
    {
        printf("�����ļ�������create\n");
        return -1;
    }

    // ��ȡĿ¼�ļ�
    openfilelist[curfd].file_ptr = 0;
    char buf[MAX_SIZE];
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    int i;
    fcb *fcbPtr = (fcb *)buf;
    for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++)
    {
        if (strcmp(fcbPtr[i].filename, fname) == 0 && strcmp(fcbPtr[i].exname, exname))
        {
            printf("����ͬ���ļ�\n");
            return -1;
        }
    }

    // Ѱ�ҿյ�fcb
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
    fcbtmp->time = (time->tm_hour << 11) + (time->tm_min << 5) + (time->tm_sec >> 1);
    fcbtmp->date = (time->tm_year << 9) + (time->tm_mon + 1 << 5) + time->tm_mday;
    strcpy(fcbtmp->filename, fname);
    strcpy(fcbtmp->exname, exname);
    fcbtmp->first = blockNum;
    fcbtmp->length = 0;
    fcbtmp->free = 1;
    do_write(curfd, (char *)fcbtmp, sizeof(fcb), 1);
    // ���µ�ǰĿ¼��fcb
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
        printf("�������׺��\n");
        return;
    }
    if (strcmp(exname, "di") == 0)
    {
        printf("����ɾ��Ŀ¼�ļ�\n");
        return;
    }

    // ��ȡĿ¼�ļ�
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    int i = 0;
    fcb *fcbPtr = (fcb *)buf;
    for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++, fcbPtr++)
    {
        if (strcmp(fcbPtr->filename, fname) == 0 && strcmp(fcbPtr->exname, exname) == 0)
        {
            break;
        }
    }
    if (i == (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)))
    {
        printf("û������ļ�\n");
        return;
    }

    // ���fat
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

    // ���fcb
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
    // ���µ�ǰĿ¼��fcb
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
        printf("���ļ��������ļ�,����ʹ��cd\n");
        return;
    }
    else
    {
        char buf[MAX_SIZE];
        openfilelist[curfd].file_ptr = 0;
        do_read(curfd, openfilelist[curfd].filefcb.length, buf);
        // Ѱ��Ŀ¼ fcbPtr
        int i = 0;
        fcb *fcbPtr = (fcb *)buf;
        // TODO: ���Ŀ¼��Ҫ�ݹ飿
        for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++, fcbPtr++)
        {
            if (strcmp(fcbPtr->filename, dirname) == 0 && fcbPtr->attribute == 0)
            {
                break;
            }
        }
        if (strcmp(fcbPtr->exname, "di") != 0)
        {
            printf("������cd��Ŀ¼�ļ�\n");
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
                    char tmp1[10];
                    strcpy(tmp1, openfilelist[curfd].dir);
                    char *tmp2 = "\\";
                    strcpy(openfilelist[fd].dir, strcat(strcat(tmp1, dirname), tmp2));
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
        printf("�����������׺��\n");
        return;
    }

    // ��ȡ��ǰĿ¼�ļ�
    char text[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    int fileLen = do_read(curfd, openfilelist[curfd].filefcb.length, text);
    fcb *fcbPtr = (fcb *)(text);
    for (int i = 0; i < (int)(fileLen / sizeof(fcb)); i++)
    {
        if (strcmp(dirname, fcbPtr[i].filename) == 0 && fcbPtr[i].attribute == 0)
        {
            printf("��Ŀ¼�Ѿ�����\n");
            return;
        }
    }

    int fd = GetFreeOpenfile();
    if (fd == -1)
    {
        printf("���ļ�������\n");
        return;
    }

    unsigned short blockNum = GetFreeBlock();
    if (blockNum == END)
    {
        printf("�̿鲻��\n");
        openfilelist[fd].topenfile = 0;
        return;
    }
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    fat *fat2 = (fat *)(v_start_pos + BLOCKSIZE * 3);
    fat1[blockNum].id = END;
    fat2[blockNum].id = END;

    // �ҵ�δ�����Ŀ¼��
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
    fcbtmp->time = (time->tm_hour << 11) + (time->tm_min << 5) + (time->tm_sec >> 1);
    fcbtmp->date = (time->tm_year << 9) + (time->tm_mon + 1 << 5) + time->tm_mday;
    strcpy(fcbtmp->filename, dirname);
    strcpy(fcbtmp->exname, "di");
    fcbtmp->first = blockNum;
    fcbtmp->length = 2 * sizeof(fcb); // . & .. ��fcb
    fcbtmp->free = 1;
    do_write(curfd, (char *)fcbtmp, sizeof(fcb), 1);

    CopyFcbToOpenfilelist(&openfilelist[fd], fcbtmp);
    openfilelist[fd].dirno = openfilelist[curfd].filefcb.first;
    openfilelist[fd].diroff = i;
    char tmp1[10];
    strcpy(tmp1, openfilelist[curfd].dir);
    char *tmp2 = "\\";
    strcpy(openfilelist[fd].dir, strcat(strcat(tmp1, dirname), tmp2));
    openfilelist[fd].file_ptr = 0;
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].topenfile = 1;

    // ���.��..
    strcpy(fcbtmp->filename, ".");
    do_write(fd, (char *)fcbtmp, sizeof(fcb), 1);
    strcpy(fcbtmp->filename, "..");
    fcbtmp->first = openfilelist[curfd].filefcb.first;
    fcbtmp->length = openfilelist[curfd].filefcb.length;
    fcbtmp->date = openfilelist[curfd].filefcb.date;
    fcbtmp->time = openfilelist[curfd].filefcb.time;
    do_write(fd, (char *)fcbtmp, sizeof(fcb), 1);

    my_close(fd);
    // ����currfdĿ¼�ļ���fcb
    fcbPtr->length = openfilelist[curfd].filefcb.length;
    openfilelist[curfd].file_ptr = 0;
    do_write(curfd, (char *)fcbPtr, sizeof(fcb), 1);
    openfilelist[curfd].fcbstate = 1;
    free(fcbtmp);
}
// done
void my_rmdir(char *dirname)
{
    char *fname = strtok(dirname, ".");
    char *exname = strtok(NULL, ".");
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)
    {
        printf("�޷�ɾ��\n");
        return;
    }
    if (exname)
    {
        printf("����Ҫ�����׺��\n");
        return;
    }

    // ��ȡcurfd
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    int i = 0;
    fcb *fcbPtr = (fcb *)buf;
    for (; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++, fcbPtr++)
    {
        if (strcmp(fcbPtr->filename, fname) == 0 && fcbPtr->attribute == 0)
        {
            break;
        }
    }
    if (i == (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)))
    {
        printf("û������ļ�\n");
        return;
    }

    // TODO ɾ��Ŀ¼�µ��ļ����ݹ�ɾ��
    if (fcbPtr->length > 2 * sizeof(fcb))
    {
        printf("����������Ŀ¼�µ������ļ�,��ɾ��Ŀ¼�ļ�\n");
        return;
    }
    // ���fat
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

    // ���fcb
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
    // ���µ�ǰĿ¼��fcb
    fcbPtr = (fcb *)buf;
    fcbPtr->length = openfilelist[curfd].filefcb.length;
    openfilelist[curfd].file_ptr = 0;
    do_write(curfd, (char *)fcbPtr, sizeof(fcb), 1);
    openfilelist[curfd].fcbstate = 1;
}
// done
int my_open(char *filename)
{
    // ��ȡ��ǰĿ¼�ļ�
    char buf[MAX_SIZE];
    openfilelist[curfd].file_ptr = 0;
    do_read(curfd, openfilelist[curfd].filefcb.length, buf);
    char *fname = strtok(filename, ".");
    char *exname = strtok(NULL, ".");
    if (!exname)
    {
        printf("�������׺��\n");
        return -1;
    }

    // Ѱ���ļ���fcb
    int i;
    fcb *fcbPtr = (fcb *)buf;
    for (i = 0; i < (int)(openfilelist[curfd].filefcb.length / sizeof(fcb)); i++, fcbPtr++)
    {
        if (strcmp(fcbPtr->filename, fname) == 0 && strcmp(fcbPtr->exname, exname) == 0 && fcbPtr->attribute == 1)
        {
            break;
        }
    }
    if (i == (int)(openfilelist[curfd].filefcb.length))
    {
        printf("�����ڴ��ļ�\n");
        return -1;
    }

    int fd = GetFreeOpenfile();
    if (fd == -1)
    {
        printf("�û����ļ�������\n");
        return -1;
    }
    CopyFcbToOpenfilelist(&openfilelist[fd], fcbPtr);
    openfilelist[fd].dirno = openfilelist[curfd].filefcb.first;
    openfilelist[fd].diroff = i;
    char tmp1[10];
    strcpy(tmp1, openfilelist[curfd].dir);
    strcpy(openfilelist[fd].dir, strcat(tmp1, filename));
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
        printf("����������ļ�\n");
        return -1;
    }
    else
    {
        int fatherfd = FindFatherDir(fd);
        if (fatherfd == -1)
        {
            printf("��Ŀ¼������\n");
            return -1;
        }
        // д��fcb
        if (openfilelist[fd].fcbstate == 1)
        {
            char buf[MAX_SIZE];
            do_read(fatherfd, openfilelist[fatherfd].filefcb.length, buf);
            fcb *fcbPtr = (fcb *)(buf + sizeof(fcb) * openfilelist[fd].diroff);
            CopyOpenfilelistToFcb(&openfilelist[fd], fcbPtr);
            openfilelist[fatherfd].file_ptr = openfilelist[fd].diroff * sizeof(fcb);
            do_write(fatherfd, (char *)fcbPtr, sizeof(fcb), 1);
        }
        // ���openfilelist[fd]
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
        printf("do_read�����ڴ�ʧ��\n");
        return -1;
    }

    // �ҵ�Ҫ���ĵ�һ���̿���̿��
    int offset = openfilelist[fd].file_ptr; // ��дλ��
    int blockNum = openfilelist[fd].filefcb.first;
    fat *fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum; // ��ǰ��fat
    while (offset >= BLOCKSIZE)
    {
        offset -= BLOCKSIZE;
        blockNum = fatPtr->id; // ��һ���̿�
        if (blockNum == END)
        {
            printf("do_readѰ�ҵĿ鲻����\n");
            return -1;
        }
        fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum;
    }

    unsigned char *blockPtr = v_start_pos + BLOCKSIZE * blockNum;
    memcpy(buf, blockPtr, BLOCKSIZE);
    char *textPtr = text; // ά��textָ��

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
                printf("len̫��\n");
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
        printf("�ļ�������\n");
        return -1;
    }
    openfilelist[fd].file_ptr = 0;
    char text[MAX_SIZE];
    do_read(fd, len, text);
    printf("��ȡ�Ľ���ǣ�%s\n", text);
    return 1;
}
// done
int do_write(int fd, char *text, int len, char wstyle)
{
    int blockNum = openfilelist[fd].filefcb.first;
    fat *fatPtr = (fat *)(v_start_pos + BLOCKSIZE) + blockNum;
    if (wstyle == 0)
    {
        // �ض�д
        openfilelist[fd].file_ptr = 0;
        openfilelist[fd].filefcb.length = 0;
    }
    else if (wstyle == 1)
    {
        // ����д
        if (openfilelist[fd].filefcb.attribute == 1 && openfilelist[fd].filefcb.length != 0)
        {
            openfilelist[fd].file_ptr -= 1;
        }
    }
    else if (wstyle == 2)
    {
        // ׷��д
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
    // ��off > BLOCKSIZE �ҵ��Ǹ��̿�
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
        printf("�����ڴ�ʧ��\n");
        return -1;
    }

    unsigned char *blockPtr = (unsigned char *)(v_start_pos + BLOCKSIZE * blockNum);
    int lenTmp = 0;
    char *textTmp = text;
    // д
    while (len > lenTmp)
    {
        memcpy(buf, blockPtr, BLOCKSIZE); // ���̿��ȡ��buf��
        for (; off < BLOCKSIZE; off++)
        {
            *(buf + off) = *textTmp;
            textTmp++;
            lenTmp++;
            if (len == lenTmp)
                break;
        }
        memcpy(blockPtr, buf, BLOCKSIZE); // ��buf�������̿���
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
    // �ͷſ��е��̿�, �޸�fat��
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
    // ͬ��fat2
    memcpy((fat *)(v_start_pos + BLOCKSIZE * 3), (fat *)(v_start_pos + BLOCKSIZE), BLOCKSIZE * 2);
    return len;
}
// down
int my_write(int fd)
{
    if (fd < 0 || fd >= MAXOPENFILE)
    {
        printf("�ļ�������\n");
        return -1;
    }
    int wstyle;
    printf("����: 0=�ض�д, 1=����д, 2=׷��д\n");
    scanf("%d", &wstyle);
    getchar();
    char text[MAX_SIZE];
    fgets(text, MAX_SIZE, stdin);
    do_write(fd, text, strlen(text) + 1, wstyle);
    openfilelist[fd].fcbstate = 1;
    return 1;
}

void my_exitsys()
{
    while (curfd)
    {
        my_close(curfd);
    }
    FILE *fp = fopen(FILENAME, "w");
    fwrite(v_start_pos, SIZE, 1, fp);
    fclose(fp);
    free(v_start_pos);
}

unsigned short GetFreeBlock()
{
    fat *fat1 = (fat *)(v_start_pos + BLOCKSIZE);
    for (int i = 0; i < (int)(SIZE / BLOCKSIZE); i++)
    {
        if (fat1[i].id == FREE)
        {
            return i;
        }
    }
    return -1;
}

// �����̿�
int DistributeBlock(int *blockNum, fat *fatPtr)
{
    *blockNum = GetFreeBlock();
    if (*blockNum == END)
    {
        printf("�̿鲻��\n");
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

// �õ����еĴ��ļ���
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

// �õ���Ŀ¼
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

// ����fcb��openfilelist
void CopyFcbToOpenfilelist(useropen *useropenPtr, fcb *fcbPtr)
{
    memcpy(&useropenPtr->filefcb, fcbPtr, sizeof(fcb));
}

// ����openfilelist��fcb
void CopyOpenfilelistToFcb(useropen *useropenPtr, fcb *fcbPtr)
{
    memcpy(fcbPtr, &useropenPtr->filefcb, sizeof(fcb));
}