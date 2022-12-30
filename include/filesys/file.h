#ifndef FILESYS_FILE_H
#define FILESYS_FILE_H

#include "filesys/off_t.h"

/************ P2 syscall: 헤더 추가 - 시작 ************/
#include "lib/stdbool.h" // file 구조체 이 파일로 옮기고 bool 타입을 위해 추가함

/************ P2 syscall: 헤더 추가 - 끝 ************/

// struct file; // 원래 코드 주석처리 
struct inode;

/****** P2 syscall: file 구조체 정의 filesys/file.c에서 옮겨옴 ******/
/* An open file. */
struct file
{
	struct inode *inode; /* File's inode. */
	off_t pos;			 /* Current position. */
	bool deny_write;	 /* Has file_deny_write() been called? */
};
/****** P2 syscall: file 구조체 정의 filesys/file.c에서 옮겨옴 - 끝 ******/


/* Opening and closing files. */
struct file *file_open(struct inode *);
struct file *file_reopen(struct file *);
struct file *file_duplicate(struct file *file);
void file_close(struct file *);
struct inode *file_get_inode(struct file *);

/* Reading and writing. */
off_t file_read(struct file *, void *, off_t);
off_t file_read_at(struct file *, void *, off_t size, off_t start);
off_t file_write(struct file *, const void *, off_t);
off_t file_write_at(struct file *, const void *, off_t size, off_t start);

/* Preventing writes. */
void file_deny_write(struct file *);
void file_allow_write(struct file *);

/* File position. */
void file_seek(struct file *, off_t);
off_t file_tell(struct file *);
off_t file_length(struct file *);

#endif /* filesys/file.h */
