#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include "file.h"
#define PERMS 0666 //RW for owner, group, others

long int length;



file *myfopen(char *name, char *mode){
	int fd;
	file *fp;
	
	for(fp = iob; fp < iob + OPEN_MAX; fp++)
		if((fp -> flag & (READ | WRITE)) == 0)
			break; // found free slots
		if(fp >= iob + OPEN_MAX)        //no free slots
			return NULL;
			
		if((strcmp(mode, "w")) == 0){
			fd = creat(name, PERMS);
			fp -> flag = WRITE;
			fp -> mode = 'w';
		}
		
		else if((strcmp(mode, "a")) == 0){
			if((fd = open(name, O_WRONLY, 0)) == -1)
				fd = creat(name, PERMS);
			lseek(fd, 0L, 2);
			fp -> flag = WRITE;
			fp -> mode = 'a';
		}
		
		else if((strcmp(mode, "r")) == 0){
			fd = open(name, O_RDONLY, 0);
			lseek(fp -> fd, 0, 0);
			fp -> flag = READ;
			fp -> mode = 'r';
		}
		
		else if((strcmp(mode, "w+")) == 0){
			fd = creat(name, PERMS);
			fd = open(name, O_RDWR, S_IRUSR, S_IWUSR);
			fp -> flag = READ | WRITE;
			fp -> mode = 'w';
		}
		
		else if((strcmp(mode, "a+")) == 0){
			fd = open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
			fp -> flag = READ | WRITE;
			lseek(fp -> fd, 0L, 2);
			fp -> mode = 'r';
		}
		
		else if((strcmp(mode, "r+")) == 0){
			fd = open(name, O_RDWR, S_IRUSR, S_IWUSR);
			fp -> flag = READ | WRITE;
			lseek(fp -> fd, 0, 0);
			fp -> mode = 'r';
			
		}
		
		else 
			return NULL;
		if(fd == -1)
			return NULL;
		fp -> fd = fd;
		fp -> cnt = 0;
		fp -> base = NULL;
		fp -> count = 0;
		return fp;
	
}

int myfseek(file *fp, long offset, int origin){
	int rc;
	rc = lseek(fp -> fd, offset, origin);
	return rc;
}

int myfillbuf(file *fp){
	int bufsize, size;
	if(((fp -> flag) & (READ | EoF | ERR)) != READ && ((fp -> flag) & (READ | EoF | ERR)) != READ | WRITE)
		return EOF;
	size = fp -> cnt;
	bufsize = (size < BUFSIZE) ? size : BUFSIZE;
	if(fp -> base == NULL){
		if((fp -> base = (char*)malloc(bufsize)) == NULL) //allocating buffer 
			fp -> flag |= UNBUF;
	}
	fp -> ptr = fp -> base;
	fp -> cnt = read(fp -> fd, fp -> ptr, bufsize);
	if(fp -> cnt -1 < 0){
		if(fp -> cnt == -1)
			fp -> flag = EoF;
		else
			fp -> flag = ERR;
		fp -> cnt = 0;
		return EOF;	
	}
	return (unsigned char) *fp -> ptr;
}

size_t myfread(void *ptr, size_t size, size_t nobj, file *fp){
	int i = 0, j = 0, bytes_read = 0;
	char *temp;
	temp = ptr;
	int bytes_to_read = size * nobj;
	fp -> cnt = nobj;
	myfillbuf(fp);
	if(fp -> flag & UNBUF){
		// unbuffered reading
		bytes_read = read(fp -> fd, temp, bytes_to_read);
		return bytes_read;
	}
	for(i = 0; i < bytes_to_read; i++){
		temp[i] = fp -> ptr[j++];
		fp -> cnt--;
		if(fp -> cnt == 0){
			fp -> base = NULL;
			fp -> cnt = nobj - BUFSIZE;
			j = 0;
			myfillbuf(fp); 
		}
		bytes_read++;
	}
	length = bytes_read;
	return bytes_read;
}

int myflushbuf(int x, file *fp){
	int bytes_written;
	if((fp -> flag & (WRITE | EoF | ERR)) != WRITE && ((fp -> flag) & (READ | EoF | ERR)) != READ | WRITE)
		return EOF; //check whether file is opened for writing
	if(fp -> base == NULL && ((fp -> flag & UNBUF) == 0)){
		if((fp -> base = (char*)malloc(BUFSIZE)) == NULL) //allocating buffer 
			fp -> flag |= UNBUF;
		else
			fp -> ptr = fp -> base;
			fp -> cnt = BUFSIZE;
			fp -> cnt = fp -> cnt - 1;
	
	}
	if(x == BUFSIZE){
		bytes_written = write(fp -> fd, fp -> base, BUFSIZE);
	}
	else{
		bytes_written = write(fp -> fd, fp -> base, x);
	}
	return bytes_written;
}

int myfwrite(void *ptr, size_t size, size_t nobj, file *fp){
	int i = 0, j = 0, obj = nobj, c, len;
	char *temp;
	temp = ptr;
	int bytes_to_write;
	bytes_to_write = size * nobj;
	fp -> base = (char*)malloc(BUFSIZE);
	fp -> cnt = (obj < BUFSIZE) ? obj : BUFSIZE;
	c = fp -> cnt;
	if(fp -> mode == 'a'){
		len = lseek(fp -> fd, 0, 2);
	}
	for(i = 0; i < bytes_to_write; i = i + size){
		fp -> base[j++] = temp[i];
		fp -> cnt--;
		if(fp -> cnt == 0){
			myflushbuf(c, fp);
			j = 0;
			obj = obj - BUFSIZE;
			fp -> cnt = (obj < BUFSIZE) ? obj : BUFSIZE;
			c = fp -> cnt;
		}
		fp -> count++;
	
	}
	if(fp -> mode == 'w')
		length = lseek(fp -> fd, fp -> count, 0);
	else if(fp -> mode == 'a')
		length = len + lseek(fp -> fd, fp -> count, 0);
	return fp -> count;
}

long int myftell(file *fp){
	return length;
}

int myfsetpos(file *fp, post *pos){
	return myfseek(fp, pos -> position, 0);
}

int myfgetpos(file *fp, post *pos){
	pos -> position = fp -> count;
	return 0;	
}


int myfclose(file *fp){
	int cf;
	if(fp == NULL)
		return;
	if(fp -> base)
		free(fp -> base);
	cf = close(fp ->fd);
	fp -> fd = -1;
	fp -> flag = 0;
	fp -> ptr = NULL;
	fp -> base = NULL;
	fp -> cnt = 0;
	fp -> count = 0;
	return  cf;
}


//Test suite begins here//
int main(){
	file *fp1, *fp2, *fp3, *fp4, *fp5, *fp6;
	FILE *f1, *f2, *f3, *f4, *f5, *f6;
	int i;
	char read1[32] = "12345"; 
	char read2[32] = "67890";
	char read3[32] = "smart";
	char read4[32] = "computer";
	char samp1[32], samp2[32];
	char samp3[32], samp4[32];
	post ptr, ptr1, ptr2, ptr3, ptr4;
	fpos_t fptr, fptr1, fptr2, fptr3, fptr4;
	//Writing on stdout
	printf("Writing on stdout\n");
	printf("Using myfwrite\n");
	myfwrite(read1, 1, 4, mystdout);
	printf("\n");
	printf("Using fwrite\n");
	fwrite(read1, 1, 4, stdout);
	printf("\n");
	
	//Reading from stdin
	printf("Reading from stdin and writing to a file opened in w+ mode\n");
	printf("Using myfread\n");
	printf("Enter four characters\n");
	myfread(samp1, 1, 4, mystdin);
	fp1 = myfopen("mystdin.txt", "w+");
	myfwrite(samp1, 1, 4, fp1);
	myfread(samp2, 1, 4, fp1);
	printf("Data in mystdin.txt\n");
	myfwrite(samp2, 1, 4, mystdout);
	printf("\n");
	myfclose(fp1);
	
	printf("Reading from stdin and writing to a file opened in w+ mode\n");
	printf("Using fread\n");
	printf("Enter four characters\n");
	fread(samp3, 1, 4, stdin);
	f1 = fopen("stdin.txt", "w+");
	fwrite(samp3, 1, 4, f1);
	fread(samp4, 1, 4, f1);
	printf("Data in stdin.txt\n");
	fwrite(samp4, 1, 4, stdout);
	printf("\n");
	fclose(f1);
	
	//Checking the working of fwrite
	printf("Opening a file in w mode and checking for fwrite\n");
	fp1 = myfopen("w.txt", "w");
	myfwrite(read1, 1, 4, fp1);
	myfwrite(read3, 1, 5, fp1);
	myfclose(fp1);
	printf("Reopening file in read mode to check fwrite\n");
	fp1 = myfopen("w.txt", "r");
	myfread(samp1, 1, 9, fp1);
	myfclose(fp1);
	printf("Using myfwrite\n");
	myfwrite(samp1, 1, 9, mystdout);
	printf("\n");
	printf("\n");
	
		
	f1 = fopen("fw.txt", "w");
	fwrite(read1, 1, 4, f1);
	fwrite(read3, 1, 5, f1);
	fclose(f1);
	printf("Reopening file in read mode to check fwrite\n");
	f1 = fopen("fw.txt", "r");
	fread(samp2, 1, 9, f1);
	fclose(f1);
	printf("Using fwrite\n");
	fwrite(samp2, 1, 9, stdout);
	printf("\n");
	printf("\n");
	
	
	printf("Opening a file in a mode and checking for fwrite\n");
	fp2 = myfopen("w.txt", "a");
	myfwrite(read2, 1, 4, fp2);
	myfclose(fp2);
	printf("Reopening file in read mode to check fwrite\n");
	fp2 = myfopen("w.txt", "r");
	myfread(samp3, 1, 13, fp2);
	myfclose(fp2);
	printf("Using myfwrite\n");
	myfwrite(samp3, 1, 13, mystdout);
	printf("\n");
	printf("\n");
	
	
	f2 = fopen("fw.txt", "a");
	fwrite(read2, 1, 4, f2);
	fclose(f2);
	printf("Reopening file in read mode to check fwrite\n");
	f2 = fopen("fw.txt", "r");
	fread(samp4, 1, 13, f2);
	fclose(f2);
	printf("Using fwrite\n");
	fwrite(samp4, 1, 13, stdout);
	printf("\n");
	printf("\n");
	
	printf("Opening a file in w+ mode and checking for fwrite\n");
	fp4 = myfopen("w+.txt", "w+");
	myfwrite(read1, 1, 4, fp4);
	myfseek(fp4, 0, 0);
	myfread(samp1, 1, 4, fp4);
	myfclose(fp4);
	printf("Using myfwrite\n");
	myfwrite(samp1, 1, 4, mystdout);
	printf("\n");
	printf("\n");
	
	f4 = fopen("fw+.txt", "w+");
	fwrite(read1, 1, 4, f4);
	printf("Using fwrite\n");
	fclose(f4);
	f4 = fopen("fw+.txt", "w+");
	fread(samp2, 1, 4, f4);
	fclose(f4);
	fwrite(samp2, 1, 4, stdout);
	printf("\n");
	printf("\n");
	
	
	printf("Opening a file in a+ mode and checking for fwrite\n");
	fp5 = myfopen("app.txt","a+");
	myfwrite(read4, 1, 8, fp5);
	myfseek(fp5, 0, 0);
	myfread(samp2, 1, 8, fp5);
	myfclose(fp5);
	printf("Using myfwrite\n");
	myfwrite(samp2, 1, 8, mystdout);
	printf("\n");
	printf("\n");
	
	
	f5 = fopen("append.txt", "a+");
	fwrite(read4, 1, 8, f5);
	fseek(f5, 0, 0);
	fread(samp3, 1, 8, f5);
	fclose(f5);
	printf("Using fwrite\n");
	
	fwrite(samp3, 1, 8, stdout);
	printf("\n");
	printf("\n");
	
	
	printf("Opening a file in r+ mode and checking for fwrite\n");
	fp6 = myfopen("filer.txt", "r+");
	myfwrite(read2, 1,  5, fp6);
	myfseek(fp6, 0, 0);
	myfread(samp1, 1, 5, fp6);
	printf("Using myfwrite\n");
	myfclose(fp6);
	myfwrite(samp1, 1, 5, mystdout);
	printf("\n");
	printf("\n");
	
	f6 = fopen("filr.txt", "r+");
	fwrite(read2, 1,  5, f6);
	fseek(f6, 0, 0);
	fread(samp2, 1, 5, f6);
	printf("Using fwrite\n");
	fclose(f6);
	fwrite(samp2, 1, 5, stdout);
	printf("\n");
	printf("\n");
	
	
	//Checking the working of ftell
	printf("Opening file in read mode and checking for ftell\n");
	fp2 = myfopen("tell.txt", "r");
	myfread(samp1, 1, 4, fp2);
	long int x = myftell(fp2);
	printf("Using myftell\n");
	printf("length = %ld\n", x);
	myfclose(fp2);
	printf("\n");
	
	f2 = fopen("tellf.txt", "r");
	fread(samp2, 1, 4, f2);
	long int y = ftell(f2);
	printf("Using ftell\n");
	printf("length = %ld\n", y);
	fclose(f2);
	printf("\n");
	
	printf("Opening file in write mode and checking for ftell\n");
	fp3 = myfopen("wtell.txt", "w");
	myfwrite(read1, 1, 4, fp3);
	long int p = myftell(fp3);
	printf("Using myftell\n");
	printf("length = %ld\n", p);
	myfclose(fp3);
	printf("\n");
	
	f3 = fopen("te.txt", "w");
	fwrite(read1, 1, 4, f3);
	long int q = ftell(f3);
	printf("Using ftell\n");
	printf("length = %ld\n", q);
	fclose(f3);
	printf("\n");
	
	printf("Opening file in append mode and checking for ftell\n");
	fp4 = myfopen("wtell.txt", "a");
	myfwrite(read1, 1, 4, fp4);
	long int r = myftell(fp4);
	printf("Using myftell\n");
	printf("length = %ld\n", r);
	myfclose(fp4);
	printf("\n");
	
	f4 = fopen("te.txt", "a");
	fwrite(read1, 1, 4, f4);
	long int s = ftell(f4);
	printf("Using ftell\n");
	printf("length = %ld\n", s);
	fclose(f4);
	printf("\n");
	
	//Checking for fgetpos and fsetpos functions
	printf("Checking fgetpos and fsetpos for file opened in w mode\n");
	fp1 = myfopen("getpos.txt","w");
	myfwrite(read1, 1, 4, fp1);
	myfwrite(read2, 1, 4, fp1);
	myfwrite(read3, 1, 4, fp1);
	myfwrite(read4, 1, 4, fp1);
	myfgetpos(fp1, &ptr);
	myfwrite(read1, 1, 4, fp1);
	myfwrite(read2, 1, 4, fp1);
	myfsetpos(fp1, &ptr);
	myfwrite(read3, 1, 4, fp1);
	myfclose(fp1);
	fp1 = myfopen("getpos.txt","r");
	myfread(samp2, 1, 24, fp1);
	myfclose(fp1);
	printf("Using mygetpos and myfsetpos text in file is \n");
	myfwrite(samp2, 1, 24, mystdout);	
	printf("\n");
	printf("\n");
	
	
	f1 = fopen("getposf.txt","w");
	fwrite(read1, 1, 4, f1);
	fwrite(read2, 1, 4, f1);
	fwrite(read3, 1, 4, f1);
	fwrite(read4, 1, 4, f1);
	fgetpos(f1, &fptr);
	fwrite(read1, 1, 4, f1);
	fwrite(read2, 1, 4, f1);
	fsetpos(f1, &fptr);
	fwrite(read3, 1, 4, f1);
	fclose(f1);
	f1 = fopen("getposf.txt","r");
	fread(samp3, 1, 24, f1);
	fclose(f1);
	printf("Using getpos and fsetpos text in file is \n");
	fwrite(samp3, 1, 24, stdout);	
	printf("\n");
	printf("\n");
	
	
	printf("Checking fgetpos and fsetpos for file opened in w+ mode\n");
	fp2 = myfopen("getpos1.txt","w+");
	myfwrite(read1, 1, 4, fp2);
	myfwrite(read2, 1, 4, fp2);
	myfwrite(read3, 1, 4, fp2);
	myfwrite(read4, 1, 4, fp2);
	myfgetpos(fp1, &ptr1);
	myfwrite(read1, 1, 4, fp2);
	myfwrite(read2, 1, 4, fp2);
	myfsetpos(fp1, &ptr1);
	myfwrite(read3, 1, 4, fp1);
	myfseek(fp2, 0, 0);
	myfread(samp2, 1, 24, fp1);
	myfclose(fp1);
	printf("Using mygetpos and myfsetpos text in file is \n");
	myfwrite(samp2, 1, 24, mystdout);	
	printf("\n");
	printf("\n");
	
	
	f2 = fopen("getposf1.txt","w+");
	fwrite(read1, 1, 4, f2);
	fwrite(read2, 1, 4, f2);
	fwrite(read3, 1, 4, f2);
	fwrite(read4, 1, 4, f2);
	fgetpos(f2, &fptr1);
	fwrite(read1, 1, 4, f2);
	fwrite(read2, 1, 4, f2);
	fsetpos(f2, &fptr1);
	fwrite(read3, 1, 4, f2);
	fseek(f2, 0, 0);
	fread(samp1, 1, 24, f2);
	fclose(f2);
	printf("Using getpos and fsetpos text in file is \n");
	fwrite(samp1, 1, 24, stdout);	
	printf("\n");
	printf("\n");
	
	
	printf("Checking fgetpos and fsetpos for file opened in a mode\n");
	fp3 = myfopen("getpos2.txt","a");
	myfwrite(read1, 1, 4, fp3);
	myfwrite(read2, 1, 4, fp3);
	myfwrite(read3, 1, 4, fp3);
	myfwrite(read4, 1, 4, fp3);
	myfgetpos(fp3, &ptr2);
	myfwrite(read1, 1, 4, fp3);
	myfwrite(read2, 1, 4, fp3);
	myfsetpos(fp3, &ptr2);
	myfwrite(read3, 1, 4, fp3);
	myfclose(fp3);
	fp3 = myfopen("getpos2.txt","r");
	myfread(samp4, 1, 24, fp3);
	myfclose(fp3);
	printf("Using mygetpos and myfsetpos text in file is \n");
	myfwrite(samp4, 1, 24, mystdout);	
	printf("\n");
	printf("\n");
	
	
	f3 = fopen("getposf2.txt","a");
	fwrite(read1, 1, 4, f3);
	fwrite(read2, 1, 4, f3);
	fwrite(read3, 1, 4, f3);
	fwrite(read4, 1, 4, f3);
	fgetpos(f3, &fptr2);
	fwrite(read1, 1, 4, f3);
	fwrite(read2, 1, 4, f3);
	fsetpos(f3, &fptr2);
	fwrite(read3, 1, 4, f3);
	fclose(f3);
	f3 = fopen("getposf2.txt","r");
	fread(samp3, 1, 24, f3);
	fclose(f3);
	printf("Using getpos and fsetpos text in file is \n");
	fwrite(samp3, 1, 24, stdout);	
	printf("\n");
	printf("\n");
	
	printf("Checking fgetpos and fsetpos for file opened in a+ mode\n");
	fp4 = myfopen("getpos3.txt","a+");
	myfwrite(read1, 1, 4, fp4);
	myfwrite(read2, 1, 4, fp4);
	myfwrite(read3, 1, 4, fp4);
	myfwrite(read4, 1, 4, fp4);
	myfgetpos(fp4, &ptr3);
	myfwrite(read1, 1, 4, fp4);
	myfwrite(read2, 1, 4, fp4);
	myfsetpos(fp4, &ptr3);
	myfwrite(read3, 1, 4, fp4);
	myfseek(fp4, 0, 0);
	myfread(samp2, 1, 24, fp4);
	myfclose(fp4);
	printf("Using mygetpos and myfsetpos text in file is \n");
	myfwrite(samp2, 1, 24, mystdout);	
	printf("\n");
	printf("\n");
	
	f4 = fopen("getposf3.txt","a+");
	fwrite(read1, 1, 4, f4);
	fwrite(read2, 1, 4, f4);
	fwrite(read3, 1, 4, f4);
	fwrite(read4, 1, 4, f4);
	fgetpos(f4, &fptr3);
	fwrite(read1, 1, 4, f4);
	fwrite(read2, 1, 4, f4);
	fsetpos(f4, &fptr3);
	fwrite(read3, 1, 4, f4);
	fseek(f4, 0, 0);
	fread(samp1, 1, 24, f4);
	fclose(f4);
	printf("Using getpos and fsetpos text in file is \n");
	fwrite(samp1, 1, 24, stdout);	
	printf("\n");
	printf("\n");
	
	
	printf("Checking fgetpos and fsetpos for file opened in r+ mode\n");
	fp5 = myfopen("getpos1.txt","r+");
	myfwrite(read1, 1, 4, fp5);
	myfwrite(read2, 1, 4, fp5);
	myfwrite(read3, 1, 4, fp5);
	myfwrite(read4, 1, 4, fp5);
	myfgetpos(fp5, &ptr4);
	myfwrite(read1, 1, 4, fp5);
	myfwrite(read2, 1, 4, fp5);
	myfsetpos(fp5, &ptr4);
	myfwrite(read3, 1, 4, fp5);
	myfseek(fp5, 0, 0);
	myfread(samp2, 1, 24, fp5);
	myfclose(fp5);
	printf("Using mygetpos and myfsetpos text in file is \n");
	myfwrite(samp2, 1, 24, mystdout);	
	printf("\n");
	printf("\n");
	
	
	f5 = fopen("getposf1.txt","r+");
	fwrite(read1, 1, 4, f5);
	fwrite(read2, 1, 4, f5);
	fwrite(read3, 1, 4, f5);
	fwrite(read4, 1, 4, f5);
	fgetpos(f5, &fptr4);
	fwrite(read1, 1, 4, f5);
	fwrite(read2, 1, 4, f5);
	fsetpos(f5, &fptr4);
	fwrite(read3, 1, 4, f5);
	fseek(f2, 0, 0);
	fread(samp1, 1, 24, f5);
	fclose(f5);
	printf("Using getpos and fsetpos text in file is \n");
	fwrite(samp1, 1, 24, stdout);	
	printf("\n");
	printf("\n");
	
	//Checking for fseek function
	printf("Checking fseek function on file opened in w mode\n");
	fp1 = myfopen("seek1.txt", "w");
	myfwrite(read1, 1, 4, fp1);
	myfseek(fp1, 2, 0);
	myfwrite(read2, 1, 4, fp1);
	myfclose(fp1);
	fp1 = myfopen("seek1.txt", "r");
	myfread(samp1, 1, 6, fp1);
	myfclose(fp1);
	printf("Data in file using myfseek is\n");
	myfwrite(samp1, 1, 6, mystdout);
	printf("\n");
	printf("\n");
	

	f1 = fopen("seekf1.txt", "w");
	fwrite(read1, 1, 4, f1);
	fseek(f1, 2, 0);
	fwrite(read2, 1, 4, f1);
	fclose(f1);
	f1 = fopen("seekf1.txt", "r");
	fread(samp2, 1, 6, f1);
	fclose(f1);
	printf("Data in file using fseek is\n");
	fwrite(samp2, 1, 6, stdout);
	printf("\n");
	printf("\n");
	
	
	printf("Checking fseek function on file opened in a mode\n");
	fp2 = myfopen("seek2.txt", "a");
	myfwrite(read1, 1, 4, fp2);
	myfseek(fp2, 2, 0);
	myfseek(fp2, 2, 1);
	myfwrite(read2, 1, 4, fp2);
	myfclose(fp2);
	fp2 = myfopen("seek2.txt", "r");
	myfread(samp1, 1, 8, fp2);
	myfclose(fp2);
	printf("Data in file using myfseek is\n");
	myfwrite(samp1, 1, 8, mystdout);
	printf("\n");
	printf("\n");
	

	f2 = fopen("seekf2.txt", "a");
	fwrite(read1, 1, 4, f2);
	fseek(f2, 2, 0);
	fseek(f2, 2, 1);
	fwrite(read2, 1, 4, f2);
	fclose(f2);
	f2 = fopen("seekf2.txt", "r");
	fread(samp2, 1, 8, f2);
	fclose(f2);
	printf("Data in file using fseek is\n");
	fwrite(samp2, 1, 8, stdout);
	printf("\n");
	printf("\n");
	
	
	printf("Checking fseek function on file opened in r mode\n");
	fp3 = myfopen("seek3.txt", "r");
	myfseek(fp3, -2, 2);
	myfread(samp2, 1, 2, fp3);
	myfclose(fp3);
	printf("Data in file using myfseek is\n");
	myfwrite(samp2, 1, 2, mystdout);
	printf("\n");
	printf("\n");
	

	f3 = fopen("seekf3.txt", "r");
	fseek(f3, -2, 2);
	fread(samp3, 1, 2, f3);
	fclose(f3);
	printf("Data in file using myfseek is\n");
	fwrite(samp3, 1, 2, stdout);
	printf("\n");
	printf("\n");
	
	
	printf("Checking fseek function on file opened in w+ mode\n");
	fp4 = myfopen("seek4.txt", "w+");
	myfwrite(read1, 1, 4, fp4);
	myfseek(fp1, 2, 0);
	myfwrite(read2, 1, 4, fp4);
	myfseek(fp4, 0, 0);
	myfread(samp1, 1, 6, fp4);
	myfclose(fp4);
	printf("Data in file using myfseek is\n");
	myfwrite(samp1, 1, 6, mystdout);
	printf("\n");
	printf("\n");
	

	f4 = fopen("seekf4.txt", "w+");
	fwrite(read1, 1, 4, f4);
	fseek(f4, 2, 0);
	fwrite(read2, 1, 4, f4);
	fseek(f4, 0, 0);
	fread(samp2, 1, 6, f4);
	fclose(f1);
	printf("Data in file using fseek is\n");
	fwrite(samp2, 1, 6, stdout);
	printf("\n");
	printf("\n");
	
	
	printf("Checking fseek function on file opened in a+ mode\n");
	fp5 = myfopen("seek5.txt", "a+");
	myfwrite(read1, 1, 4, fp5);
	myfseek(fp5, 2, 0);
	myfseek(fp5, 2, 1);
	myfwrite(read2, 1, 4, fp5);
	myfseek(fp5, 0, 0);
	myfread(samp1, 1, 8, fp5);
	myfclose(fp5);
	printf("Data in file using myfseek is\n");
	myfwrite(samp1, 1, 8, mystdout);
	printf("\n");
	printf("\n");
	

	f5 = fopen("seekf5.txt", "a+");
	fwrite(read1, 1, 4, f5);
	fseek(f5, 2, 0);
	fseek(f5, 2, 1);
	fwrite(read2, 1, 4, f5);
	fseek(f5, 0, 0);
	fread(samp2, 1, 8, f5);
	fclose(f5);
	printf("Data in file using fseek is\n");
	fwrite(samp2, 1, 8, stdout);
	printf("\n");
	printf("\n");
	
	
	printf("Checking fseek function on file opened in r+ mode\n");
	fp6 = myfopen("seek6.txt", "r+");
	myfseek(fp6, -2, 2);
	myfwrite(read2, 1, 4, fp6);
	myfseek(fp6, 0, 0);
	myfread(samp2, 1, 6, fp6);
	myfclose(fp6);
	printf("Data in file using myfseek is\n");
	myfwrite(samp2, 1, 6, mystdout);
	printf("\n");
	printf("\n");
	

	f6 = fopen("seekf6.txt", "r+");
	fseek(f6, -2, 2);
	fwrite(read2, 1, 4, f6);
	fseek(f6, 0, 0);
	fread(samp3, 1, 6, f6);
	fclose(f6);
	printf("Data in file using myfseek is\n");
	fwrite(samp3, 1, 6, stdout);
	printf("\n");
	printf("\n");
	i = 0;
	//Checking for feof function
	printf("Checking for feof function in r+ mode\n");
	fp1 = myfopen("feof1.txt", "r+");
	myfwrite(read1, 1, 4, fp1);
	myfseek(fp1, 0, 0);
	while(!myfeof(fp1)){
		myfread(samp1, 1, 1, fp1);
		samp4[i] = samp3[0];
		i++;
	}
	samp4[--i] = '\0';
	printf("Using myfeof data in file is\n");
	printf("%s", samp4);
	myfclose(fp1);
	
	f1 = fopen("feof2.txt", "r+");
	fwrite(read1, 1, 4, f1);
	fseek(f1, 0, 0);
	while(!feof(f1)){
		fread(samp1, 1, 1, f1);
		samp2[i++] = samp1[0];
	}
	samp2[--i] = '\0';
	printf("Using feof data in file is\n");
	printf("%s", samp2);
	fclose(f1);

	return 0;
}





