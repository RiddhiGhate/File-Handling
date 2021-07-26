
#define BUFSIZE 1024
#define OPEN_MAX 20 /*max #files open at once*/
typedef struct post{
	int position;
}post;
typedef struct iobuf{
	int cnt;        /*characters left*/
	char *ptr;      /* next character position */
	char *base;   	/* location of buffer */
	int flag;	    /* mode of file access */
	int fd;  		/* file descriptor */
	int count; 		/*to maintain count of bytes read,written or seeked*/
	char mode;   
}file;
file iob[OPEN_MAX];

#define mystdin       (&iob[0])
#define mystdout     (&iob[1])
#define mystderr      (&iob[2])

enum flags{
	READ   =   01   ,     /*file open for reading*/
	WRITE  =   02    ,    /*file open for writing*/
	UNBUF  =   04     ,   /*file is unbuffered*/
	EoF    =   010    ,  /*EOF has occurred on this file*/
	ERR    =   020     /*error occurred on this file*/
};

file iob[OPEN_MAX] = {    /* stdin, stdout, stderr: */
    {0, (char *) 0, (char *) 0, READ, 0, 0, 0},
    {0, (char *) 0, (char *) 0, WRITE, 1, 0, 0},
    {0, (char *) 0, (char *) 0, WRITE | UNBUF, 2, 0, 0},
};
#define myfeof(p) (((p)-> flag & EoF) != 0)
file *myfopen(char *name, char *mode);
size_t myfread(void *ptr, size_t size, size_t nobj, file *fp);
int myflushbuf(int c, file *fp);
int myfillbuf(file *fp);
int myfwrite(void *ptr, size_t size, size_t nobj, file *fp);
int myfseek(file *fp, long offset, int origin);
long int myftell(file *fp);
int myfsetpos(file *fp, post *pos);
int mygsetpos(file *fp, post *pos);
int myfclose(file *fp);
