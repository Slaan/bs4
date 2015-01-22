/*
 * example.c
 *
 *  Created on: 18.12.2014
 *      Author: louisa
 */


#include "translate.h"

MODULE_LICENSE("GPL");

//pthread_mutex_t rb_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_lock(&rb_mutex);

struct translate_dev *ptranslate_devices;
char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";




static int find_index_in_alphabet(char character){
	printk(KERN_INFO "Sie befinden sich in Funktion find_index_in_alphabet");
	int index = 0;
	while (alphabet[index] != '\0'){
		if(alphabet[index] == character){
			#ifdef PDEBUG
				printk(KERN_DEBUG "Charakter in Alphabet gefunden");
			#endif
			return index;
		}
		index++;
	}
	return -1;
}

void setup_devices(struct translate_dev *dev, int index){
	printk(KERN_INFO "Sie befinden sich in Funktion setup");

	/* Funktionen werden an das eine Device der Devices gebunden
	 * Struct chardevice wird belegt */
	cdev_init(&dev->chardevice, &fops);
	dev->chardevice.owner = THIS_MODULE;
	int dev_no_of_curr_minor = MKDEV(MAJOR(dev_num), index);

	/*  Fehlerfall Device konnte nicht erstellt werden */
	if(cdev_add(&dev->chardevice, dev_no_of_curr_minor, 1)){
		printk(KERN_ALERT "unable to add Device: major %d minor %d", MAJOR(dev_num), index);
	}

	/* Initialiesierung des Ringbuffers
	 * Schreib und leseadresse sind anfangs identisch */
	dev->buffer = kzalloc(bufsize * sizeof(char), GFP_KERNEL);
	dev->p_in = dev->buffer;
	dev->p_out = dev->buffer;
	dev->count = 0;
	dev->shiftcount = (index ? -shiftsize : shiftsize); //index >= 1 -> true, == 0 -> false

	#ifdef PDEBUG
		printk(KERN_DEBUG "Ringpuffer initialisiert");
	#endif

	init_waitqueue_head(&dev->waitqueue_read);
	init_waitqueue_head(&dev->waitqueue_write);

	#ifdef PDEBUG
		printk(KERN_DEBUG "Wait-Queues initialisiert");
	#endif

	sema_init(&(dev->semaphore),1);

	#ifdef PDEBUG
		printk(KERN_DEBUG "Semaphore initialisiert");
	#endif

}

static int __init init_translate(void){
	printk(KERN_INFO "Sie befinden sich in Funktion init");

	/*int alloc_chrdev_region(dev_t *dev, unsigned int firstminor, unsigned int count, char *name);
	 * return value if success then 0, else negative value */
	if (alloc_chrdev_region(&dev_num, FIRSTMINOR, COUNT, DEVICE_NODE) != 0){
		printk(KERN_ALERT "init failed");
		return -1;
	}

	/* Mit kmalloc wird der Speicherplatz der Devices reserviert
	 * kzalloc belegt den Speicher mit 0 vor */
	ptranslate_devices = kzalloc(sizeof(struct translate_dev) * 2, GFP_KERNEL);
	if(!ptranslate_devices){
		printk(KERN_ALERT "unable to allocate memory");
		cleanup_translate();
		return -ENOMEM;
	}

	int i;
	for(i = 0; i<2; i++){
		setup_devices(&ptranslate_devices[i], i);  /* chardevice wird belegt, Device meldet sich im Kernel an*/
	}


	#ifdef PDEBUG
		printk(KERN_DEBUG "debugging test");
	#endif

	//major = MAJOR(dev);
	return 0;
}






static void cleanup_translate(void){
	printk(KERN_INFO "Sie befinden sich in Funktion cleanup");

	/* Nur wenn Translate devices gesetzt waren */
	if(ptranslate_devices){

		/* Devices entfernen */
		int i;
		for(i=0; i<2; i++){
			kfree(&ptranslate_devices[i].buffer);
			cdev_del(&ptranslate_devices[i].chardevice);

		}

		/* Speicher befreien */
		kfree(ptranslate_devices);
	}

	/* unregestrieren der Devices */
	unregister_chrdev_region(dev_num, COUNT);


}





/* open, wenn Programm auf Device zugreifen möchte */
static int open(struct inode *devicefile, struct file *instance){
	printk(KERN_INFO "Sie befinden sich in Funktion open");

	/* zeiger auf die Device Struktur */
	struct translate_dev *device;

	device = container_of(devicefile->i_cdev, struct translate_dev, chardevice);
	instance->private_data = device;

	/* Semaphore für Device Prozess blockieren */
	if(down_interruptible(&device->semaphore)){
		return -ERESTARTSYS;
	}

	/* kritischer Abschnitt */
	/* wenn Device bereits geöffnet ist, darf Prozess nicht darauf zugreifen */
	if((instance->f_flags & O_ACCMODE) == O_RDONLY && !device->is_open_read){
		device->is_open_read = 1;
	}else if((instance->f_flags & O_ACCMODE) == O_WRONLY && !device->is_open_write){
		device->is_open_write = 1;
	}else if((instance->f_flags & O_ACCMODE) == O_RDWR && !device->is_open_read && !device->is_open_write){
		device->is_open_read = 1;
		device->is_open_write = 1;
	}else{
		printk(KERN_ALERT "already open");
		return -EBUSY; /* Device ist beschäftigt */
	}

	/* kritischen Bereich verlassen und Semaphore freigeben */
	up(&device->semaphore);


	if(iminor(devicefile)==0){
		printk(KERN_INFO "Minor Number 0");
	}else if(iminor(devicefile)==1) {
		printk(KERN_INFO "Minor Number 1");
	}else{
		printk(KERN_INFO "Minor Number is higher than 1");
	}


	try_module_get(THIS_MODULE);
	return 0;
}







/* close, wenn Programm Driver space verlässt */
static int close(struct inode *devicefile, struct file *instance){

	/* zeiger auf die Device Struktur */
	struct translate_dev *device;

	device = container_of(devicefile->i_cdev, struct translate_dev, chardevice);
	instance->private_data = device;

	if((instance->f_flags & O_ACCMODE) == O_RDONLY && device->is_open_read){  /* Device durfte Lesen */
		device->is_open_read = 0;
	}else if((instance->f_flags & O_ACCMODE) == O_WRONLY && device->is_open_write){ /* Device durfte Schreiben */
		device->is_open_write = 0;
	}else if((instance->f_flags & O_ACCMODE) == O_RDWR && device->is_open_read && device->is_open_write){  /* Device durfte Lesen und Schreiben */
		device->is_open_read = 0;
		device->is_open_write = 0;
	}

	module_put(THIS_MODULE);

	return 0;
}






/* read, wenn Programm lesen möchte
 * filp: zugreifenden Treiberinstanz (wird über Minornumber identifisiert) mit Zugriffsmodus (blockieren, nicht blockierend)
 * buff: enthält die Adresse des Speicherbereichs im Userspace, aus dem die Daten stammen
 * count: Anzahl der Bytes, die gelesen werden sollen
 * offp:*/
ssize_t read(struct file *instance, char __user *output, size_t count, loff_t *offp){
	printk(KERN_INFO "Sie befinden sich in Funktion read");

	//minor Device das Device zugeordnet, welches gerade hier aktiv ist
		struct translate_dev *device;
		device = instance->private_data;

		#ifdef PDEBUG
			printk(KERN_DEBUG "Minor Device dem Device zugeordnet");
		#endif
		int bytes_read = 0;
		int num_read_chars;
		while(count) {
			char chari;
			int index_in_alpha;
			int shifted_index;
			char character;
			int alphabet_length;
			alphabet_length = strlen(alphabet);

			#ifdef PDEBUG
				printk(KERN_DEBUG "Variablen in Write() initialisiert");
			#endif


			if(!READ_POSSIBLE){
				if(instance->f_flags & O_NONBLOCK){
					return -EAGAIN;
				}

				if(wait_event_interruptible(device->waitqueue_read, READ_POSSIBLE)){
					return -ERESTARTSYS;
				}
			}


			/* schreibt einen Wert aus der Adresse input in die Variable chari
			 * get_user(variable, source)
			 * source: Adresse von dem ein Wert in die Variable geschrieben werden soll
			 * variable: der Wert der 1,2,4 oder 8 Byte lang ist, je nach Datentyps*/
			if(put_user(*(device->p_out), output++) != 0){
				printk(KERN_ALERT "Couldn't write char to output (user)");
			}

			#ifdef PDEBUG
				printk(KERN_DEBUG "put_user in read() ausgeführt: %c", *(device->p_out));
			#endif

			/* Zeiger auf Stelle in Buffer, wo reingeschrieben werden kann erhöhen */
			device->p_out++;
			/* Anzahl Elemente im Ringbuffer erhöhen. */
			device->count--;

			/* Wenn die Zeiger p_in über die Größe des Ringbuffers rüberschießt,
			 * dann setzte den Zeiger wieder an den Anfang des Ringbufers */
			if(device->p_out - device->buffer >= translate_bufsize){
				device->p_out = device->buffer;
			}

			/* Konnte das Device nicht schreiben, weil Ringbuffer voll war, kann
			 * es jetzt wieder schreiben. Ein Element wurde gelesen und es ist Platz
			 * frei geworden */
			wake_up_interruptible(&device->waitqueue_write);
			count--;
			bytes_read++;		
}


	return bytes_read;
}






/* write, wenn Programm schreiben möchte */
ssize_t write(struct file *instance, const char __user *input, size_t count, loff_t *offp){
	printk(KERN_INFO "Sie befinden sich in Funktion write");

	//minor Device das Device zugeordnet, welches gerade hier aktiv ist
	struct translate_dev *device;
	device = instance->private_data;

	#ifdef PDEBUG
		printk(KERN_DEBUG "Minor Device dem Device zugeordnet");
	#endif


	int num_written_chars;
	for (num_written_chars = 0; num_written_chars < count; num_written_chars++){

		char chari;
		int index_in_alpha;
		int shifted_index;
		char character;
		int alphabet_length;
		alphabet_length = strlen(alphabet);

		#ifdef PDEBUG
			printk(KERN_DEBUG "Variablen in Write() initialisiert");
		#endif


		/* Blockiere wenn der Ringbuffer voll ist */
		if(!WRITE_POSSIBLE){
			if(instance->f_flags & O_NONBLOCK){
				return -EAGAIN;
			}

			if(wait_event_interruptible(device->waitqueue_write, WRITE_POSSIBLE)){
				return -ERESTARTSYS;
			}
		}


		/* schreibt einen Wert aus der Adresse input in die Variable chari
		 * get_user(variable, source)
		 * source: Adresse von dem ein Wert in die Variable geschrieben werden soll
		 * variable: der Wert der 1,2,4 oder 8 Byte lang ist, je nach Datentyps*/
		if(get_user(chari, input++) != 0){
			printk(KERN_ALERT "Couldn't read char from input");
		}

		#ifdef PDEBUG
			printk(KERN_DEBUG "get_user in write() ausgeführt: %c", chari);
		#endif

		index_in_alpha = find_index_in_alphabet(chari);

		#ifdef PDEBUG
			printk(KERN_DEBUG "find_index_in_alphabet() ausgeführt");
		#endif

		if(index_in_alpha != -1){

			shifted_index = (index_in_alpha + device->shiftcount) % alphabet_length;
			if(shifted_index < 0){
				shifted_index += alphabet_length;
			}

			/* den gewünschten Chracter (geshiftet) aus Alphabet holen */
			character = alphabet[shifted_index];

		}else{
			character = chari;
		}


		#ifdef PDEBUG
			printk(KERN_DEBUG "Caesar-Verschiebung durchgeführt");
		#endif
		/* geshifteter Charakter in den Ringbuffer schreiben. */
		*device->p_in = character;

		#ifdef PDEBUG
			printk(KERN_DEBUG "Caesar verschobenener Charakter in Ringbuffer hereingeschrieben");
		#endif

		/* Zeiger auf Stelle in Buffer, wo reingeschrieben werden kann erhöhen */
		device->p_in++;
		/* Anzahl Elemente im Ringbuffer erhöhen. */
		device->count++;

		/* Wenn die Zeiger p_in über die Größe des Ringbuffers rüberschießt,
		 * dann setzte den Zeiger wieder an den Anfang des Ringbufers */
		if(device->p_in - device->buffer >= translate_bufsize){
			device->p_in = device->buffer;
		}

		/* Konnte das Device nicht lesen, weil Ringbuffer keine Elemente beeinhaltet hat, kann
		 * es jetzt wieder lesen. Ein Element wurde in den Ringbuffer geschrieben */
		wake_up_interruptible(&device->waitqueue_read);

	}

	return num_written_chars;
}






module_init(init_translate);
module_exit(cleanup_translate);
module_param(bufsize, int, 0644);
MODULE_PARM_DESC(bufsize, "RingBuffer Grösse");




/* Fpr cleanup funktion
 * void unregister_chrdev_region(dev_t first, unsigned int count);*/
