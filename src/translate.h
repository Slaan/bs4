#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <string.h>
#include <linux/fs.h> //this is the file structure, file open read close
#include <linux/cdev.h> // this is for character device, makes cdev avilable
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h> //this is for copy_user vice vers
#include <linux/semaphore.h> // this is for the semaphore
#include <linux/moduleparam.h>

MODULE_LICENCE("GPL2");

// Parameterlist for module TRANSLATE
static int translate_bufsize = 40;
moduleparam(translate_bufsize, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARAM_DESC(translate_bufsize, "Buffersize of device. Default is 40");

static int translate_shift = 3;
moduleparam(translate_shift, int,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARAM_DESC(translate_shift, "Ceaser shift number. Default is 3");

// Encryption alphabet
const char alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";
int alphabet_size;

void translate_init(void);
void translate_exit(void);

// open device, to write or read
static int device_open(struct inode *, struct file *);

// close device, to unlock it
static int device_close(struct inode *, struct file *);

// read from device, which has to be open
static ssize_t device_read(struct file *, char *, size_t, loff_t *);

// write on device, which has to be open
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

// 
static loff_t device_lseek(struct file *file, loff_t offset, int orig);


module_init(translate_init);
module_exit(translate_exit);

#endif // TRANSLATE_H
