/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/* Modified                                                              */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h" /* Declarations FatFs MAI */
#include "drivers/sd.h"
#include "ff.h" /* Basic definitions of FatFs */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {

	if (pdrv != 0) {
		return STA_NOINIT;
	}
	return sd_is_ready() ? 0 : STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

// NOTE: startup initialization already does the sd_init() so just return status
DSTATUS disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
	if (pdrv != 0) {
		return STA_NOINIT;
	}
	return sd_is_ready() ? 0 : STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, /* Physical drive nmuber to identify the drive */
	BYTE* buff,	     /* Data buffer to store read data */
	LBA_t sector,	     /* Start sector in LBA */
	UINT count	     /* Number of sectors to read */
) {

	if (pdrv != 0 || buff == NULL || count == 0) {
		return RES_PARERR;
	}

	if (!sd_is_ready()) {
		return RES_NOTRDY;
	}

	uint64_t block_count = sd_get_block_count();

	if (sector > UINT32_MAX || sector >= block_count || count > block_count - sector) {
		return RES_PARERR;
	}

	for (UINT i = 0; i < count; i++) {

		uint32_t block = (uint32_t)(sector + i);
		BYTE* destination = buff + ((size_t)i * SD_BLOCK_LEN);
		if (sd_read_block(block, destination) != SD_OK) {
			return RES_ERROR;
		}
	}

	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
	const BYTE* buff,     /* Data to be written */
	LBA_t sector,	      /* Start sector in LBA */
	UINT count	      /* Number of sectors to write */
) {

	(void)pdrv;
	(void)buff;
	(void)sector;
	(void)count;

	return RES_WRPRT;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
	if (pdrv != 0) {
		return RES_PARERR;
	}

	if (!sd_is_ready()) {
		return RES_NOTRDY;
	}

	switch (cmd) {
	case CTRL_SYNC:
		return RES_OK; // read only

	case GET_SECTOR_COUNT:
		if (buff == NULL) {
			return RES_PARERR;
		}

		*(LBA_t*)buff = (LBA_t)sd_get_block_count();
		return RES_OK;

	case GET_SECTOR_SIZE:
		if (buff == NULL) {
			return RES_PARERR;
		}

		*(WORD*)buff = SD_BLOCK_LEN;
		return RES_OK;

	case GET_BLOCK_SIZE:
		if (buff == NULL) {
			return RES_PARERR;
		}

		*(DWORD*)buff = 1;
		return RES_OK;

	default:
		return RES_PARERR;
	}
}
