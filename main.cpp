/**************************************************************************

PNG：
段组成：
名称    占用字节      内容
长度       4        数据长度
段名       4       如：'IHDR'
数据       N       数据内容
CRC        4  CRC32值(段名及数据的CRC)

添加文本信息：
长度： strlen(text)
段名：'tEXt'
数据： text(内容自定)
CRC： crc32('tEXt'+text)

放置位置：位于IHDR后面
PNG头8字节，IHDR段25字节(内容13字节，其它12字节)，共33字节，
故放置33偏移量处后面。

JPEG：
组成：
名称   占用字节                  内容
段名      2       如：FF D8表示JPEG图片，FF FE表示注释段
长度      2          数据段长度+2(即这里的2字节)
数据      N                  数据内容

添加文件信息：
段名： FF FE
长度： strlen(text_len) + 2
数据： info(text)

放置位置：位于FF E0或FF E1段后面
读0x04、0x05处长度(即FFE0或FFE1的长度)，须转换成小端模式

PNG、JPEG图片为大端模式，以字节保存没问题，但数据长度必须转换成大端模式

写新文件方法：
先保存原来的开始部分，再插入文本信息，再保存剩下部分数据
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "crc.h"

const char *text = ">AUTHOR: Late Lee from http://www.latelee.org.\n";

#define little2big32(x) \
	( (x&0xff000000) >> 24 | (x&0xff0000) >> 8 | (x&0xff00) << 8 | (x&0xff) << 24 )

#define little2big16(x) \
	( (x&0xff00) >> 8 | (x&0xff) << 8 )

#define HEAD_SIZE 33
int add_text_png(const char *file, const char *new_file)
{
	FILE *in;
	FILE *out;
	int len = 0;
	size_t ret = 0;
	char *buffer;
	char *text_buf;
	size_t text_len = 0;
	size_t text_len_1 = 0;
	char text_type[4] = {'t', 'E', 'X', 't'};
	int text_crc = 0;

	in = fopen(file, "rb");
	if (in == NULL)
	{
		printf("Can not open file %s\n", file);
		return -1;
	}

	fseek(in, 0, SEEK_END);
	len = ftell(in);
	fseek(in, 0, SEEK_SET);

	printf("file size: %d\n", len);

	buffer = (char *)malloc(sizeof(char) * len);
	if (buffer == NULL)
	{
		return -1;
	}

	// read all to buffer
	ret = fread(buffer, 1, len, in);
	if (ret != len)
	{
		printf("read file failed.\n");
		return -1;
	}

	out = fopen(new_file, "wb");
	if (out == NULL)
	{
		printf("open new file failed.\n");
		return -1;
	}

	// write 'head'
	ret = fwrite(buffer, 1, HEAD_SIZE, out);
	if (ret != HEAD_SIZE)
	{
		printf("write head failed.\n");
		return -1;
	}

	/////////////////////////
	text_len = strlen(text);
	text_buf = (char *)malloc(sizeof(char) * (text_len + 12));
	text_len_1 = little2big32(text_len);
	memcpy(text_buf, &text_len_1, 4);	// big endian
	memcpy(text_buf + 4, text_type, 4);
	memcpy(text_buf + 4 + 4, text, text_len);
	text_crc = crc32((unsigned char *)(text_buf + 4), (u32)text_len + 4);
	memcpy(text_buf + 4 + 4 + text_len, &text_crc, 4);

	printf("text len: %d\n", text_len);

	// write information
	ret = fwrite(text_buf, 1, text_len + 12, out);
	if (ret != text_len + 12)
	{
		printf("write head failed.\n");
		return -1;
	}

	// write 'left'
	ret = fwrite(buffer + HEAD_SIZE, 1, len - HEAD_SIZE, out);
	if (ret != len - HEAD_SIZE)
	{
		printf("write head failed.\n");
		return -1;
	}

	fclose(in);
	fclose(out);

	free(buffer);
	free(text_buf);

	printf("good job.\n");
	
	return 0;
}

int add_text_jpeg(const char *file, const char *new_file)
{
	FILE *in;
	FILE *out;
	int len = 0;
	size_t ret = 0;
	unsigned char *buffer;	// must be unsigned char
	char *text_buf;
	size_t text_len = 0;
	size_t text_len_1 = 0;
	size_t tmp_len = 0;
	unsigned char text_type[2] = {0xFF, 0xFE};

	in = fopen(file, "rb");
	if (in == NULL)
	{
		printf("Can not open file %s\n", file);
		return -1;
	}

	fseek(in, 0, SEEK_END);
	len = ftell(in);
	fseek(in, 0, SEEK_SET);

	printf("file size: %d\n", len);

	buffer = (unsigned char *)malloc(sizeof(char) * len);
	if (buffer == NULL)
	{
		return -1;
	}

	// read all to buffer
	ret = fread(buffer, 1, len, in);
	if (ret != len)
	{
		printf("read file failed.\n");
		return -1;
	}

	// read tmp_len
	tmp_len = *(buffer + 4) << 8 | *(buffer + 5);
	printf("tmp len: %x\n", tmp_len);

	out = fopen(new_file, "wb");
	if (out == NULL)
	{
		printf("open new file failed.\n");
		return -1;
	}

	// write 'head'
	ret = fwrite(buffer, 1, tmp_len + 4, out);
	if (ret != tmp_len + 4)
	{
		printf("write head failed.\n");
		return -1;
	}

	/////////////////////////
	text_len = strlen(text);
	text_buf = (char *)malloc(sizeof(char) * (text_len + 4));
	text_len_1 = little2big16(text_len + 2);
	memcpy(text_buf, text_type, 2);	//  FF FE
	memcpy(text_buf + 2, &text_len_1, 2);	// len
	memcpy(text_buf + 2 + 2, text, text_len);	// text

	printf("text len: %d\n", text_len);

	// write information
	ret = fwrite(text_buf, 1, text_len + 4, out);
	if (ret != text_len + 4)
	{
		printf("write head failed.\n");
		return -1;
	}

	// write 'left'
	ret = fwrite(buffer + tmp_len + 4, 1, len - (tmp_len + 4), out);
	if (ret != len - (tmp_len + 4))
	{
		printf("write head failed.\n");
		return -1;
	}

	fclose(in);
	fclose(out);

	free(buffer);
	free(text_buf);

	printf("good job.\n");
	
	return 0;
}

int my_strnicmp(const char *s1, const char *s2, size_t len)
{
	unsigned char c1, c2;

	const char *tmp1 = s1 + strlen(s1) - 1;
	const char *tmp2 = s2 + strlen(s2) - 1;
	c1 = 0;	c2 = 0;
	if (len) {
		do {
			c1 = *tmp1; c2 = *tmp2;
			tmp1--; tmp2--;
			if (!c1)
				break;
			if (!c2)
				break;
			if (c1 == c2)
				continue;
			c1 = tolower(c1);
			c2 = tolower(c2);
			if (c1 != c2)
				break;
		} while (--len);
	}
	return (int)c1 - (int)c2;
}

// a.out foo.jpg bar.jpg
// 其中的图片格式需要一致，由用户控制
int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("usage: %s old.jpg|png new.jpg|png.\n", argv[0]);
		return -1;
	}

	if (!my_strnicmp(argv[1], "jpg", 3))
		add_text_jpeg(argv[1], argv[2]);

	else if (!my_strnicmp(argv[1], "png", 3))
		add_text_png(argv[1], argv[2]);
	else
	{
		printf("not support.\n");
		return -1;
	}
	return 0;
}