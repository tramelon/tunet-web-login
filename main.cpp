/*
 * main.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: zebra
 */

#include <openssl/md5.h>
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <ctime>
#define MAX_BUF	65536
using namespace std;

char wr_buf[MAX_BUF + 1];
int wr_index;
char serverURL[] = "http://net.tsinghua.edu.cn/cgi-bin/do_login";
/*
 * Write data callback function (called within the context of
 * curl_easy_perform.
 */
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	int segsize = size * nmemb;

	/* Check to see if this data exceeds the size of our buffer. If so,
	 * set the user-defined context value and return 0 to indicate a
	 * problem to curl.
	 */
	if (wr_index + segsize > MAX_BUF) {
		*(int *) userp = 1;
		return 0;
	}

	/* Copy the data from the curl buffer into our buffer */
	memcpy((void *) &wr_buf[wr_index], buffer, (size_t) segsize);

	/* Update the write index */
	wr_index += segsize;

	/* Null terminate the buffer */
	wr_buf[wr_index] = 0;

	/* Return the number of bytes received, indicating to curl that all is okay */
	wr_index = 0; //prepare for next call
	return segsize;
}

/*
 * Simple curl application to read the index.html file from a Web site.
 */
int postData(char* urlstr, char * data) {
	CURL *curl;
	CURLcode ret;
	int wr_error;

	wr_error = 0;
	wr_index = 0;

	/* First step, init curl */
	curl = curl_easy_init();
	if (!curl) {
		printf("couldn't init curl\n");
		fflush(stdout);
		return 0;
	}

	/* Tell curl the URL of the file we're going to retrieve */
	curl_easy_setopt( curl, CURLOPT_URL, urlstr);
	/* Tell curl that we'll receive data to the function write_data, and
	 * also provide it with a context pointer for our error return.
	 */
	curl_easy_setopt( curl, CURLOPT_POSTFIELDS, data);
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&wr_error);
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_data);

	/* Allow curl to perform the action */
	ret = curl_easy_perform(curl);

//  printf( "ret = %d (write_error = %d)\n", ret, wr_error );

	/* Emit the page if curl indicates that no errors occurred */

	curl_easy_cleanup(curl);
	return ret;
}

void format_flux(char* result, long size) {
	if (size <= 0) {
		result[0] = 0;
		return;
	}
	char units[5][3] = { "B", "KB", "MB", "GB", "TB" };
	int digitGroups = (int) (log10(size) / log10(1000));
	sprintf(result, "%.2f%s", size / pow(1000, digitGroups),
			units[digitGroups]);
	return;
}
void format_time(char* result, int sec) {
	int h = (int) floor(sec / 3600);
	int m = (int) floor((sec % 3600) / 60);
	int s = sec % 3600 % 60;
	sprintf(result, "%02d:%02d:%02d", h, m, s);
	return;
}
void do_logout() {
	postData("http://net.tsinghua.edu.cn/cgi-bin/do_logout", "");
	if (!strcmp("logout_ok", wr_buf))
		printf("连接已断开\n");
	else
		printf("操作失败\n");
	fflush(stdout);
}

char *str2md5(const char *str, int length) {
	int n;
	MD5_CTX c;
	unsigned char digest[16];
	char *out = (char*) malloc(33);

	MD5_Init(&c);

	while (length > 0) {
		if (length > 512) {
			MD5_Update(&c, str, 512);
		} else {
			MD5_Update(&c, str, length);
		}
		length -= 512;
		str += 512;
	}

	MD5_Final(digest, &c);

	for (n = 0; n < 16; ++n) {
		snprintf(&(out[n * 2]), 16 * 2, "%02x", (unsigned int) digest[n]);
	}

	return out;
}

void printEstimate(long flux) {
	time_t t = time(0);
	char temp[10];
	struct tm * now = localtime(&t);
	int month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	long estimatedFlux = flux / now->tm_mday * month[now->tm_mon];
	format_flux(temp, estimatedFlux);
	printf("估计本月流量%s ", temp);
	if (estimatedFlux>20*pow(1000,3))
	{
		format_flux(temp,estimatedFlux-20*pow(1000,3));
		printf("超出限额%s\n",temp);
		fflush(stdout);
	}
	else
		printf("\n");
}
int main(void) {
	char isonline[100];
	char* pch;
	char arr[5][20];
	char flux[10];
	char time[10];
	int i = 0;
	char key;
	postData(serverURL, "action=check_online");
	strncpy(isonline, wr_buf, 100);
	if (isonline[0] != 0) {
		cout << "您已经在线，下面是连接信息：" << endl;
		pch = strtok(isonline, ",");
		while (pch != NULL) {
			strncpy(arr[i], pch, 20);
			pch = strtok(NULL, ",");
			i++;
		}
		format_flux(flux, atol(arr[2]));
		format_time(time, atoi(arr[4]));
		printf("用户名%s\n已用流量%s\n在线时长%s\n", arr[1], flux, time);
		printEstimate(atol(arr[2]));
		printf("是否退出[y/N]？\n");
		fflush(stdout);
		key = getchar();
		if (key == 'y') {
			do_logout();
		}
		exit(0);
	}

	const char fileName[] = "MyLogin_Info";
	char uname[10];
	char pass_md5[50];
	bool isFromFile = false;
	FILE * fp;
	fp = fopen(fileName, "r");
	if (fp) { //exists
		fscanf(fp, "%s", uname);
		fscanf(fp, "%s", pass_md5);
		fclose(fp);
		isFromFile = true;
	} else {
		printf("输入用户名: ");
		fflush(stdout);
		scanf("%s", uname);
		printf("输入密码: ");
		fflush(stdout);
		char password[20];
		scanf("%s", password);
		strcpy(pass_md5, str2md5(password, strlen(password)));
		isFromFile = false;
	}
	char data[200];
	sprintf(data, "username=%s&password=%s&drop=0&type=1&n=100", uname,
			pass_md5);
	postData(serverURL, data);
	if (strchr(wr_buf, ',')) {
		strncpy(isonline, wr_buf, 100);
		cout << "登陆成功！" << endl;
		pch = strtok(isonline, ",");
		while (pch != NULL) {
			strncpy(arr[i], pch, 20);
			pch = strtok(NULL, ",");
			i++;
		}
		format_flux(flux, atol(arr[2]));
		printf("已用流量%s\n", flux);
		printEstimate(atol(arr[2]));
		fflush(stdout);
		if (!isFromFile) {
			fp = fopen(fileName, "w");
			if (fp) {
				fprintf(fp, "%s\n%s\n", uname, pass_md5);
				fclose(fp);
				cout << "用户" << uname << "信息已保存" << endl;
			} else
				cout << "无法保存用户信息" << endl;
		}
	} else {
		printf("%s\n", wr_buf);
		fflush(stdout);
	}
	return 0;
}
