#include "kernel/buf.h"
#include "kernel/ide.h"
#include "kernel/iopic.h"
#include "kernel/trap.h"
#include "kernel/x86.h"
#include "stdio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define PORT_DATA(ide_chan)        ((ide_chan)->port_base + 0)
#define PORT_ERROR(ide_chan)       ((ide_chan)->port_base + 1)
#define PORT_SEC_CNT(ide_chan)     ((ide_chan)->port_base + 2)
#define PORT_LBA_L(ide_chan)       ((ide_chan)->port_base + 3)
#define PORT_LBA_M(ide_chan)       ((ide_chan)->port_base + 4)
#define PORT_LBA_H(ide_chan)       ((ide_chan)->port_base + 5)
#define PORT_DEV(ide_chan)         ((ide_chan)->port_base + 6)
#define PORT_STATUS(ide_chan)      ((ide_chan)->port_base + 7)
#define PORT_CMD(ide_chan)         (PORT_STATUS(ide_chan))
#define PORT_ALT_STATUS(ide_chan)  ((ide_chan)->port_base + 0x206)
#define PORT_CTL(ide_chan)         (PORT_ALT_STATUS(ide_chan))

#define IDE_BSY  0b10000000
#define IDE_DRDY 0b01000000
#define IDE_DF   0b00100000
#define IDE_ERR  0b00000001

#define CMD_READ  0x20
#define CMD_WRITE 0x30
#define CMD_RDMUL 0xC4
#define CMD_WRMUL 0xC5

#define GET_CMD_WRITE(secs) (((secs) == 1) ? CMD_WRITE : CMD_WRMUL)
#define GET_CMD_READ(secs)  (((secs) == 1) ? CMD_READ : CMD_RDMUL)

#define BIT_DEV_MBS   0B10100000
#define BIT_DEV_LBA   0B01000000
#define BIT_DEV_SLAVE 0B00010000

/**
 * Gets the number of hard disks from the 0x475(provided by BIOS).
 */
#define HARD_DISK_CNT ((uint8_t)(*((uint8_t *) 0x475)))

struct ide_channel ide_chans[2] = {
	{.ide_chan_id = 0, .port_base = 0x1f0, .irq_no = 0xe},
	{.ide_chan_id = 1, .port_base = 0x170, .irq_no = 0xf}
};

static struct spinlock idelock;
static struct buf *idequeue;

static struct disk *cur_disk = NULL;

/**
 * Setup the LBA disk address and the number of sectors to be operated.
 */ 
static inline void select_secs(struct disk *disk, uint32_t lba, uint32_t secs) {
	struct ide_channel *ide_chan = disk->ide_chan;
	outb(PORT_ALT_STATUS(disk->ide_chan), 0); // generate interrupt.
	outb(PORT_SEC_CNT(ide_chan), secs);
	outb(PORT_LBA_L(ide_chan), lba & 0xFF);
	outb(PORT_LBA_M(ide_chan), (lba >> 8) & 0xFF);
	outb(PORT_LBA_H(ide_chan), (lba >> 16) & 0xFF);
	outb(PORT_DEV(ide_chan), BIT_DEV_LBA | BIT_DEV_MBS |
								 ((disk->dev_no == 1) ? BIT_DEV_SLAVE : 0) |
								 ((lba >> 24) & 0xF));
}

static int idewait(struct ide_channel *ide_chan) {
	uint32_t r;

	while (((r = inb(PORT_STATUS(ide_chan))) & (IDE_BSY | IDE_DRDY)) != IDE_DRDY)
		/* Nothing to do */;

	if ((r & (IDE_DF | IDE_ERR)) != 0) {
		return -1;
	}
	
	return 0;
}

static void ide_write_to_disk(struct disk *disk, uint32_t lba, uint32_t secs,
							  void *buf) {
	bool int_save;
	spinlock_acquire(&idelock, &int_save);

	select_secs(disk, lba, secs);
	outb(PORT_CMD(disk->ide_chan), CMD_WRITE);
	if (idewait(disk->ide_chan) == 0) {
		outsl(PORT_DATA(disk->ide_chan), buf, SECTOR_SIZE * secs / 4);
		
		// Block self until the IDE interruption occurs(write to finish).
		sem_wait(&disk->ide_chan->sem);
	}
	
	spinlock_release(&idelock, &int_save);
}

static void ide_read_from_disk(struct disk *disk, uint32_t lba, uint32_t secs,
							   void *buf) {
	bool int_save;
	spinlock_acquire(&idelock, &int_save);
	
	select_secs(disk, lba, secs);
	outb(PORT_CMD(disk->ide_chan), CMD_READ);
	
	// Block self until the IDE interruption occurs(prepare).
	sem_wait(&disk->ide_chan->sem);
	if (idewait(disk->ide_chan) == 0) {
		insl(PORT_DATA(disk->ide_chan), buf, SECTOR_SIZE * secs / 4);
	}
	
	spinlock_release(&idelock, &int_save);
}

/**
 * Start to syncronize the buf.
 */
static void idestart(struct buf *buf) {
	ASSERT(buf != NULL);
	
	struct ide_channel *ide_chan = buf->disk->ide_chan;
	int sectors_per_block = BLOCK_SIZE / SECTOR_SIZE;
	int sector_lba = buf->block_no * sectors_per_block;
	
	ASSERT(sectors_per_block < 8);

	select_secs(buf->disk, sector_lba, sectors_per_block);
	if (buf->flags & BUF_FLAGS_DIRTY) {
		outb(PORT_CMD(ide_chan), GET_CMD_WRITE(sectors_per_block));
		outsl(PORT_DATA(ide_chan), buf->data, BLOCK_SIZE / 4);
	} else {
		outb(PORT_CMD(ide_chan), GET_CMD_READ(sectors_per_block));
	}
}

/**
 * IDE Interrupt handler.
 */
static void ide_intr_handler(struct trap_frame *tf) {
	ASSERT(tf->intr_nr == IRQ_START_VEC_NR + 0xe ||
		   tf->intr_nr == IRQ_START_VEC_NR + 0xf);
	bool int_save;
	spinlock_acquire(&idelock, &int_save);

	uint8_t ide_chan_idx = tf->intr_nr - (IRQ_START_VEC_NR + 0xe);
	struct ide_channel *ide_chan = &ide_chans[ide_chan_idx];
		
	struct buf *buf = idequeue;
	if (buf == NULL) {
		// Wakeup the task wating for reading/writing.
		// see ide_write_to_disk/ide_read_from_disk.
		sem_signal(&ide_chan->sem);
		spinlock_release(&idelock, &int_save);
		return;
	}
	
	if ((buf->flags & BUF_FLAGS_DIRTY) == 0 && idewait(ide_chan) == 0) {
		// The buffer is invalid. read from disk.
		insl(PORT_DATA(ide_chan), buf->data, BLOCK_SIZE / 4);
	}
	buf->flags |= BUF_FLAGS_VALID;
	buf->flags &= ~BUF_FLAGS_DIRTY;
	
	// Wakeup the task wating for this buffer.
	sem_signal(&ide_chan->sem);
	
	idequeue = buf->qnext;
	if (idequeue != NULL) {
		idestart(idequeue);
	}
	
	spinlock_release(&idelock, &int_save);
}

static void init_ide_chan(struct ide_channel *ide_chan) {
	sprintf(ide_chan->name, "ide_%d", ide_chan->ide_chan_id);
	sem_init(&ide_chan->sem, 0, ide_chan->name);

	setup_irq_handler(ide_chan->irq_no, ide_intr_handler);
	enable_irq(ide_chan->irq_no);
	
	for (int dev_no = 0; dev_no < 2; dev_no++) {
		struct disk *hd = &ide_chan->devices[dev_no];
		sprintf(hd->name, "hd%c", 'a' + dev_no);
		hd->dev_no = dev_no;
		hd->ide_chan = ide_chan;	

		// Initialize sb and log in 'fs.c'(only initialize cur_disk).
		hd->sb = NULL;
		hd->log = NULL;
		
		// Don't use the main hard disk as a fs.
		if (dev_no != 0 || ide_chan->ide_chan_id != 0) {
			cur_disk = hd;
		}
	}
}

struct disk* get_current_disk() {
	return cur_disk;
}

uint8_t get_ide_channel_cnt() {
	const uint8_t hd_cnt = HARD_DISK_CNT;
	// Each IDE channel has two hard disks.
	uint8_t ide_chan_cnt = ROUND_UP(hd_cnt, 2);
	if (ide_chan_cnt > 2) {
		ide_chan_cnt = 2;
	}
	return ide_chan_cnt;
}

void ide_init() {
	printk("ide_init start...\n");
	spinlock_init(&idelock);
	idequeue = NULL;
	
	uint8_t ide_chan_cnt = get_ide_channel_cnt();
	for (uint8_t nr = 0; nr < ide_chan_cnt; nr++) {
		init_ide_chan(&ide_chans[nr]);
	}

	if (cur_disk == NULL) {
		PANIC("No disk found.");
	}
	printk("ide_init done...\n");
}

struct ide_channel *get_ide_channel(uint8_t nr) {
	ASSERT(nr < sizeof(ide_chans) / sizeof(*ide_chans));
	return &ide_chans[nr];
}

void ide_write(struct disk *disk, uint32_t lba, uint32_t secs, void *buf) {
	ASSERT(secs != 0);
	uint32_t sec_per;
	uint32_t sec_done = 0;

	while (sec_done < secs) {
		sec_per = 256;
		if (secs - sec_done < 256) {
			sec_per = secs - sec_done;
		}
		ide_write_to_disk(disk, lba, sec_per,
						  (uint8_t *) buf + (sec_done * SECTOR_SIZE));
		sec_done += sec_per;
	}
}

void ide_read(struct disk *disk, uint32_t lba, uint32_t secs, void *buf) {
	ASSERT(secs != 0);
	uint32_t sec_per;
	uint32_t sec_done = 0;
	
	while (sec_done < secs) {
		sec_per = 256;
		if (secs - sec_done < 256) {
			sec_per = secs - sec_done;
		}
		ide_read_from_disk(disk, lba, sec_per,
						   (uint8_t *) buf + (sec_done * SECTOR_SIZE));
		sec_done += sec_per;
	}
}

void iderw(struct buf *buf) {
	ASSERT(sem_holding(&buf->sem));
	ASSERT((buf->flags & (BUF_FLAGS_VALID | BUF_FLAGS_DIRTY)) !=
		   BUF_FLAGS_VALID);
	bool int_save;
	spinlock_acquire(&idelock, &int_save);
	
	struct buf **p;
	for (p = &idequeue; *p != NULL; p = &(*p)->qnext)
		/* Nothing to do */;
	*p = buf;
	if (idequeue == buf) {
		// idequeue start from this buffer.
		idestart(buf);
	}

	// Block self. Wait for request to finish.
	while ((buf->flags & (BUF_FLAGS_VALID | BUF_FLAGS_DIRTY)) !=
		   BUF_FLAGS_VALID) {
		sem_wait(&buf->disk->ide_chan->sem);
	}
		
	spinlock_release(&idelock, &int_save);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
