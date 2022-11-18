#define BLOCKSIZE 1024
#define SIZE 1024000
#define END 0xFFFF
#define FREE 0x0000
#define ROOTBLOCKNUM 2
#define MAXOPENFILE 10
#define FILENAME "filesys.txt"

typedef struct FCB
{
    char filename[8];   
    char exname[3];
    unsigned char attribute;
    unsigned short time;
    unsigned short date;
    unsigned short first;
    unsigned long length;
    char free;
} fcb;

typedef struct FAT
{
    unsigned short id;
} fat;

typedef struct USEROPEN
{
    fcb filefcb;
    char dir[80];
    int pos; //读写位置
    char fcbstate;
    char topenfile;
} useropen;

typedef struct BLOCK0
{
    char information[200];
    unsigned short root; //根目录文件的起始盘块号
    unsigned char *startblock;
} block0;

unsigned char *v_start_pos; //虚拟磁盘的起始位置
useropen openfilelist[MAXOPENFILE];
int curdirfd;
char curdir[80];
unsigned char *startp;

int main();
void my_startsys();
void my_format();
void my_cd(char *dirname);
void my_mkdir(char *dirname);
void my_rmdir(char *dirname);
void my_ls();
int my_create(char *filename);
void my_rm(char *filename);
int my_open(char *filename);
void my_close(int fd);
int my_write(int fd);
int do_write(int fd, char *text, int len, char wstyle);
int my_read(int fd, int len);
int do_read(int fd, int len, char *text);
void my_exitsys();
