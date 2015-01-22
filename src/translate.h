/*
 * translate.h
 *
 *  Created on: 18.12.2014
 *      Author: louisa
 */

#ifndef TRANSLATE_H_
#define TRANSLATE_H_

/* Includes */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>  	/* char device aufsetzten, device Nummer bekommen */
#include <linux/cdev.h>
#include <linux/slab.h> /* für kzalloc */
#include <linux/wait.h>
#include <linux/sched.h> /* für wait_event_interruptible */
#include <linux/mutex.h> /* für Semaphore */
#include <linux/sem.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>



//#include <pthread.h>

/* Defines */
#define DEVICE_NODE "trans"
#define FIRSTMINOR 0
#define COUNT 1
#define translate_bufsize 40
#define translate_shift 3
	//schreiben möglich, wenn weniger Elemente im Ringbuffer drin sind als translate_bufsize
#define WRITE_POSSIBLE (device->count < bufsize)
	//lesen möglich, wenn mehr Elemente als 0 im Ringbuffer drin sind
#define READ_POSSIBLE (device->count > 0)

/* Variables */
dev_t dev_num;

int bufsize = translate_bufsize;
int shiftsize = translate_shift;

struct translate_dev{
	struct cdev chardevice;
	char *buffer;
	char *p_in;		/* Zeiger auf Stelle in Buffer, wo reingeschrieben werden kann */
	char *p_out; 	/* Zeiger auf Stelle in Buffer, wo herausgelesen weerden kann */
	int count; 		/* Anzahl der elemente im buffer */
	int shiftcount;  /* Verschiebungsgrad: bei Trans0 -> +1, bei Trans1 -> -1 */
	int is_open_read; /* Prädikat ob Device bereits offen zum lesen ist */
	int is_open_write; /* Prädikat ob Device bereits offen zum schreiben ist */
	wait_queue_head_t waitqueue_read; /* Warteschlange wenn das Device nicht lesen kann, weil keine Elemente vorhanden */
	wait_queue_head_t waitqueue_write; /* Warteschlange wenn das Device nicht schreiben kann, weil Buffer voll */
	struct semaphore semaphore; /* Semaphore */
};

static int find_index_in_alphabet(char character);

/* Functions */
static int __init init_translate(void);

static void cleanup_translate(void);


/* open, wenn Programm auf Device zugreifen möchte */
static int open(struct inode *devicefile, struct file *instance);

/* close, wenn Programm Driver space verlässt */
static int close(struct inode *devicefile, struct file *instance);

/* read, wenn Programm lesen möchte */
ssize_t read(struct file *instance, char __user *output, size_t count, loff_t *offp);

/* write, wenn Programm schreiben möchte */
ssize_t write(struct file *instance, const char __user *input, size_t count, loff_t *offp);

/* Strukturen */
/* Strukt, welches zeigt welche Funktion welche ist */
struct file_operations fops = {
	.open = open,
	.release = close,
	.read = read,
	.write = write,
	.owner = THIS_MODULE,
};

#endif /* TRANSLATE_H_ */


