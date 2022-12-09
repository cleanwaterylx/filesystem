#define BLOCKSIZE 1024
#define SIZE 1024000
#define END 0xFFFF
#define FREE 0x0000
#define ROOTBLOCKNUM 2
#define MAXOPENFILE 10
#define MAX_SIZE 10000
#define FILENAME "filesys.txt"

typedef struct FCB
{
    char filename[8];   
    char exname[3];
    unsigned char attribute;
    unsigned short time;
    unsigned short date;
    unsigned short first;     //文件起始盘块号
    unsigned long length;   
    char free;                //表示目录项是否为空
} fcb;

typedef struct FAT
{
    unsigned short id;        //下一个块的块号
} fat;

typedef struct USEROPEN
{
    fcb filefcb;
    int dirno;              //打开文件的目录项在父目录文件中的盘块号
    int diroff;             //打开文件的目录项在父目录文件的dirno盘块中的目录项序号
    char dir[80];
    int file_ptr;           //读写位置
    char fcbstate;      
    char topenfile;         //打开表项是否为空
} useropen;

typedef struct BLOCK0
{
    char information[200];
    unsigned short root; //根目录文件的起始盘块号
    unsigned char *startblock;
} block0;

unsigned char *v_start_pos; //虚拟磁盘的起始位置
useropen openfilelist[MAXOPENFILE];
int curfd;
char curdir[80];
unsigned char *startp;       //虚拟磁盘数据区起始位置
unsigned char buffer[SIZE];       //缓冲区

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
int my_close(int fd);
int my_write(int fd);
int do_write(int fd, char *text, int len, char wstyle);
int my_read(int fd, int len);
int do_read(int fd, int len, char *text);
void my_exitsys();

int FindFatherDir();
int DistributeBlock(int *blockNum, fat *fatPtr);
int GetFreeOpenfile();
void CopyFcbToOpenfilelist(useropen *useropenPtr, fcb *fcbPtr);
void CopyOpenfilelistToFcb(useropen *useropenPtr, fcb *fcbPtr);

