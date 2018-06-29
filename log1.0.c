#include <sys/types.h>
#include <sys/stat.h>
#include <sys/klog.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define	FILE_SIZE 10000000		//自定义文件大小
#define DATA_SIZE 50			//数据大小

int main()
{
	char temp[DATA_SIZE]={0};
	char over[DATA_SIZE];
	int i=1,fd_log,buf_size=0;
	fd_log = open("./loginfo.log", O_WRONLY | O_CREAT, 0640);	//打开需要写入的日志文件
	FILE *fd_set = fopen("./Offset.txt", "rt");		//打开存放偏移量的文件
	if(fd_log < 0)
	{
		perror("open loginfo.log fail\n");
	}
	if(fd_set == 0)		//文件不存在，重新创建文件，并写入0
	{
		printf("Create the Offset.txt\n");
		fd_set = fopen("./Offset.txt","wt");
		fprintf(fd_set,"%d",0);
	}
	
	fscanf(fd_set,"%d",&buf_size);		//获取偏移量
	lseek(fd_log,buf_size,SEEK_SET);	//指向上次结束时的位置
	
	while(1)
	{
		
		klogctl(4, temp, DATA_SIZE);				//获取日志文件内容
		printf("%d :\n%s\n", i,temp);		//打印内容
		
		if(FILE_SIZE-buf_size < strlen(temp))	//比较文件剩余大小和数据大小
		{
			write(fd_log,temp,FILE_SIZE-buf_size);	//将temp中等同文件剩余大小数据存入log文件中
		//	printf("11111\n");
			lseek(fd_log,0,SEEK_SET);				//文件指针指向文件头
			strcpy(over,temp+(FILE_SIZE-buf_size));
			write(fd_log,over,strlen(over));		//将temp剩余数据存入log文件中
			buf_size = strlen(over);
			fd_set=NULL;						
			fd_set = fopen("./Offset.txt", "w");
			fprintf(fd_set,"%d",buf_size);		//写入新的偏移量值
			fclose(fd_set);
			printf("new fd_size %d\n",buf_size);
			i++;
			usleep(100000);
			continue;
		}
		write(fd_log,temp,strlen(temp));	//将内容写入到log文件中
		buf_size +=strlen(temp);			//增加偏移量
		fd_set=NULL;						//清空.txt文件内容
		fd_set = fopen("./Offset.txt", "w");
		fprintf(fd_set,"%d",buf_size);		//写入新的偏移量值
		fclose(fd_set);
		printf("new fd_size %d\n",buf_size);
		i++;
		usleep(100000);	
	}
	close(fd_log);
	return 0;
} 