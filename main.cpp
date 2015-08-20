/**************************************************************************

PNG��
����ɣ�
����    ռ���ֽ�      ����
����       4        ���ݳ���
����       4       �磺'IHDR'
����       N       ��������
CRC        4  CRC32ֵ(���������ݵ�CRC)

����ı���Ϣ��
���ȣ� strlen(text)
������'tEXt'
���ݣ� text(�����Զ�)
CRC�� crc32('tEXt'+text)

����λ�ã�λ��IHDR����
PNGͷ8�ֽڣ�IHDR��25�ֽ�(����13�ֽڣ�����12�ֽ�)����33�ֽڣ�
�ʷ���33ƫ���������档

JPEG��
��ɣ�
����   ռ���ֽ�                  ����
����      2       �磺FF D8��ʾJPEGͼƬ��FF FE��ʾע�Ͷ�
����      2          ���ݶγ���+2(�������2�ֽ�)
����      N                  ��������

����ļ���Ϣ��
������ FF FE
���ȣ� strlen(text_len) + 2
���ݣ� info(text)

����λ�ã�λ��FF E0��FF E1�κ���
��0x04��0x05������(��FFE0��FFE1�ĳ���)����ת����С��ģʽ

PNG��JPEGͼƬΪ���ģʽ�����ֽڱ���û���⣬�����ݳ��ȱ���ת���ɴ��ģʽ

д���ļ�������
�ȱ���ԭ���Ŀ�ʼ���֣��ٲ����ı���Ϣ���ٱ���ʣ�²�������
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
// ���е�ͼƬ��ʽ��Ҫһ�£����û�����
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