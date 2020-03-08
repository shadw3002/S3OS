#include "hd_driver.h"

#include "assert.h"
#include "proc.h"
#include "stdio.h"
#include "interruption.h"
#include "port.h"
#include "clock.h"
#include "string.h"

using namespace proc;

static void	init_hd			();
static void	hd_cmd_out		(struct hd_cmd* cmd);
static int  waitfor			(int mask, int val, int timeout);
static void	interrupt_wait		();
static void hd_identify		(int drive);
static void	print_identify_info	(u16* hdinfo);

static	u8	hd_status;
static	u8	hdbuf[SECTOR_SIZE * 2];

static int waitfor(int mask, int val, int timeout)
{
	int t = clock::get_ticks();

	while(((clock::get_ticks() - t) * 1000 / HZ) < timeout)
		if ((in_byte(REG_STATUS) & mask) == val)
			return 1;

	return 0;
}

static void print_identify_info(u16* hdinfo)
{
	int i, k;
	char s[64];

	struct iden_info_ascii {
		int idx;
		int len;
		char * desc;
	} iinfo[] = {{10, 20, "HD SN"}, /* Serial number in ASCII */
		     {27, 40, "HD Model"} /* Model number in ASCII */ };

	for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++) {
		char * p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len/2; i++) {
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}
		s[i*2] = 0;
		printf("%s: %s\n", iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	printf("LBA supported: %s\n",
	       (capabilities & 0x0200) ? "Yes" : "No");

	int cmd_set_supported = hdinfo[83];
	printf("LBA48 supported: %s\n",
	       (cmd_set_supported & 0x0400) ? "Yes" : "No");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	printf("HD size: %dMB\n", sectors * 512 / 1000000);
}

static void hd_cmd_out(struct hd_cmd* cmd)
{
	/**
	 * For all commands, the host must first check if BSY=1,
	 * and should proceed no further unless and until BSY=0
	 */
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
		panic("hd error.");

	/* Activate the Interrupt Enable (nIEN) bit */
	out_byte(REG_DEV_CTRL, 0);
	/* Load required parameters in the Command Block Registers */
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR,  cmd->count);
	out_byte(REG_LBA_LOW,  cmd->lba_low);
	out_byte(REG_LBA_MID,  cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE,   cmd->device);
	/* Write the command code to the Command Register */
	out_byte(REG_CMD,     cmd->command);
}

/*****************************************************************************
 *                                interrupt_wait
 *****************************************************************************/
/**
 * <Ring 1> Wait until a disk interrupt occurs.
 *
 *****************************************************************************/
static void interrupt_wait()
{
	proc::ipc::Message msg;
	send_recv(proc::ipc::Type::RECEIVE, INTERRUPT, &msg);
}

static void hd_identify(int drive)
{
	hd_cmd cmd;
	cmd.device  = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	interrupt_wait();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	print_identify_info((u16*)hdbuf);
}

static void hd_read(proc::ipc::Message* p)
{
    hd_cmd cmd;

    u32 sect_nr = (u32)p->POSITION >> 9;

	cmd.features = 0;
	cmd.count	 = (p->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lba_low	 = sect_nr & 0xFF;
	cmd.lba_mid	 = (sect_nr >>  8) & 0xFF;
	cmd.lba_high = (sect_nr >> 16) & 0xFF;
	cmd.device	 = MAKE_DEVICE_REG(1, 0, (sect_nr >> 24) & 0xF);
	cmd.command	 = ATA_READ;

	hd_cmd_out(&cmd);

	int bytes_left = p->CNT;
	char* la = (char*)va2la(p->PROC_NR, p->BUF);

	while (bytes_left > 0) {
		int bytes = SECTOR_SIZE < bytes_left ? SECTOR_SIZE : bytes_left;

		interrupt_wait();
		port_read(REG_DATA, hdbuf, SECTOR_SIZE);
		memcpy(la, (void*)va2la(2, hdbuf), bytes);

		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
    printf("[hd]end\n");
}

void task_hd()
{
	proc::ipc::Message msg;

	init_hd();

	while (1) {
		send_recv(proc::ipc::Type::RECEIVE, ANY, &msg);

		int src = msg.source;

		switch (msg.type) {
		case proc::ipc::DEV_OPEN:
			hd_identify(0);
			break;
        case proc::ipc::DEV_READ:
            hd_read(&msg);
            break;
		default:
			assert("HD driver::unknown msg");
			break;
		}

		send_recv(proc::ipc::Type::SEND, src, &msg);
	}
}

static void init_hd()
{
	/* Get the number of drives from the BIOS data area */
	u8 * pNrDrives = (u8*)(0x475);
	printf("NrDrives:%d.\n", *pNrDrives);
	assert(*pNrDrives);

	put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);
}

void hd_handler(int irq)
{
	/*
	 * Interrupts are cleared when the host
	 *   - reads the Status Register,
	 *   - issues a reset, or
	 *   - writes to the Command Register.
	 */
	hd_status = in_byte(REG_STATUS);

	proc::ipc::inform_int(2);
}
