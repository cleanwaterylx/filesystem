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
    unsigned char attribute;   //1:data, 0:dir
    unsigned short time;
    unsigned short date;
    unsigned short first;     //�ļ���ʼ�̿��
    unsigned long length;   
    char free;                //��ʾĿ¼���Ƿ�Ϊ��
} fcb;

typedef struct FAT
{
    unsigned short id;        //��һ����Ŀ��
} fat;

typedef struct USEROPEN
{
    fcb filefcb;
    int dirno;              //���ļ���Ŀ¼���ڸ�Ŀ¼�ļ��е��̿��
    int diroff;             //���ļ���Ŀ¼���ڸ�Ŀ¼�ļ���dirno�̿��е�Ŀ¼�����
    char dir[80];
    int file_ptr;           //��дλ��
    char fcbstate;      
    char topenfile;         //�򿪱����Ƿ�Ϊ��
} useropen;

typedef struct BLOCK0
{
    char information[200];
    unsigned short root; //��Ŀ¼�ļ�����ʼ�̿��
    unsigned char *startblock;
} block0;

unsigned char *v_start_pos; //������̵���ʼλ��
useropen openfilelist[MAXOPENFILE];
int curfd;
char curdir[80];
unsigned char *startp;       //���������������ʼλ��
unsigned char buffer[SIZE];       //������

int main();
void error(char * command);
void show_help();
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

unsigned short GetFreeBlock();
int FindFatherDir(int fd);
int DistributeBlock(int *blockNum, fat *fatPtr);
int GetFreeOpenfile();
void CopyFcbToOpenfilelist(useropen *useropenPtr, fcb *fcbPtr);
void CopyOpenfilelistToFcb(useropen *useropenPtr, fcb *fcbPtr);

