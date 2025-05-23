/*

 **************************************************************************
 **                        STMicroelectronics							 **
 **************************************************************************
 **                        marco.cali@st.com							 **
 **************************************************************************
 *                                                                        *
 *                     I2C/SPI Communication							  *
 *                                                                        *
 **************************************************************************
 **************************************************************************

 */

/*!
* \file ftsIO.c
* \brief Contains all the functions which handle with the I2C/SPI communication
*/

#include "ftsSoftware.h"

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/stdarg.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/of_gpio.h>

#ifdef I2C_INTERFACE
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
static u16 I2CSAD;
#include <linux/spi/spidev.h>
#endif

static void *client;

#include "ftsCore.h"
#include "ftsError.h"
#include "ftsHardware.h"
#include "ftsIO.h"
#include "../fts.h"
extern struct fts_ts_info *fts_info;
static struct mutex rw_lock;
static u8 *buf1;
static u8 *buf2;
/**
* Initialize the static client variable of the fts_lib library in order to allow any i2c/spi transaction in the driver. (Must be called in the probe)
* @param clt pointer to i2c_client or spi_device struct which identify the bus slave device
* @return OK
*/
int openChannel(void *clt)
{
	client = clt;
#ifdef I2C_INTERFACE
	I2CSAD = ((struct i2c_client *)clt)->addr;
	logError(0, "%s openChannel: SAD: %02X \n", tag, I2CSAD);
#else
	logError(1, "%s %s: spi_master: flags = %04X !\n", tag, __func__,
		 ((struct spi_device *)client)->controller->flags);
	logError(
		1,
		"%s %s: spi_device: max_speed = %d chip select = %02X bits_per_words = %d mode = %04X !\n",
		tag, __func__, ((struct spi_device *)client)->max_speed_hz,
		((struct spi_device *)client)->chip_select[0],
		((struct spi_device *)client)->bits_per_word,
		((struct spi_device *)client)->mode);
	logError(1, "%s openChannel: completed! \n", tag);
#endif
	mutex_init(&rw_lock);

	if (!buf1)
		buf1 = (u8 *)kzalloc(PAGE_SIZE, GFP_ATOMIC);
	if (!buf2)
		buf2 = (u8 *)kzalloc(PAGE_SIZE, GFP_ATOMIC);
	if (!buf1 || !buf2)
		return ERROR_ALLOC;

	return OK;
}

#ifdef I2C_INTERFACE
/**
* Change the I2C slave address which will be used during the transaction (For Debug Only)
* @param sad new slave address id
* @return OK
*/
int changeSAD(u8 sad)
{
	I2CSAD = sad;
	return OK;
}
#endif

/**
* Retrieve the pointer to the device struct of the IC
* @return a the device struct pointer if client was previously set or NULL in all the other cases
*/
struct device *getDev(void)
{
	if (client != NULL)
		return &(getClient()->dev);
	else
		return NULL;
}

#ifdef I2C_INTERFACE
/**
* Retrieve the pointer of the i2c_client struct representing the IC as i2c slave
* @return client if it was previously set or NULL in all the other cases
*/
struct i2c_client *getClient()
{
	if (client != NULL)
		return (struct i2c_client *)client;
	else
		return NULL;
}
#else
/**
* Retrieve the pointer of the spi_device struct representing the IC as spi slave
* @return client if it was previously set or NULL in all the other cases
*/
struct spi_device *getClient(void)
{
	if (client != NULL)
		return (struct spi_device *)client;
	else
		return NULL;
}
#endif

/****************** New I2C API *********************/

/**
* Perform a direct bus read
* @param outBuf pointer of a byte array which should contain the byte read from the IC
* @param byteToRead number of bytes to read
* @return OK if success or an error code which specify the type of error encountered
*/
int fts_read(u8 *outBuf, int byteToRead)
{
	int ret = -1;
	int retry = 0;

#ifdef I2C_INTERFACE
	struct i2c_msg I2CMsg[1];

	I2CMsg[0].addr = (__u16)I2CSAD;
	I2CMsg[0].flags = (__u16)I2C_M_RD;
	I2CMsg[0].len = (__u16)byteToRead;
	I2CMsg[0].buf = (__u8 *)outBuf;
#else
	struct spi_message msg;
	struct spi_transfer transfer[1] = { { 0 } };

	spi_message_init(&msg);

	transfer[0].len = byteToRead;
	transfer[0].delay.value = SPI_DELAY_CS;
	transfer[0].delay.unit = SPI_DELAY_UNIT_USECS;
	transfer[0].tx_buf = NULL;
	transfer[0].rx_buf = outBuf;
	spi_message_add_tail(&transfer[0], &msg);
#endif

	if (client == NULL)
		return ERROR_BUS_O;
	if (fts_info && fts_info->tp_pm_suspend) {
		logError(1, "%s %s system suspend,don't to transfer\n", tag,
			 __func__);
		return ERROR_BUS_O;
	}
	while (retry < I2C_RETRY && ret < OK) {
#ifdef I2C_INTERFACE
		ret = i2c_transfer(getClient()->adapter, I2CMsg, 1);
#else
		ret = spi_sync(getClient(), &msg);
#endif

		retry++;
		if (ret < OK)
			mdelay(I2C_WAIT_BEFORE_RETRY);
	}
	if (ret < 0) {
		logError(1, "%s %s: ERROR %08X\n", tag, __func__, ERROR_BUS_R);
		return ERROR_BUS_R;
	}
	return OK;
}

/**
* Perform a bus write followed by a bus read without a stop condition
* @param cmd byte array containing the command to write
* @param cmdLength size of cmd
* @param outBuf pointer of a byte array which should contain the bytes read from the IC
* @param byteToRead number of bytes to read
* @return OK if success or an error code which specify the type of error encountered
*/
int fts_writeRead(u8 *cmd, int cmdLength, u8 *outBuf, int byteToRead)
{
	int ret = -1;
	int retry = 0;

#ifdef I2C_INTERFACE
	struct i2c_msg I2CMsg[2];

	I2CMsg[0].addr = (__u16)I2CSAD;
	I2CMsg[0].flags = (__u16)0;
	I2CMsg[0].len = (__u16)cmdLength;
	I2CMsg[0].buf = (__u8 *)cmd;

	I2CMsg[1].addr = (__u16)I2CSAD;
	I2CMsg[1].flags = I2C_M_RD;
	I2CMsg[1].len = byteToRead;
	I2CMsg[1].buf = (__u8 *)outBuf;

#else
	struct spi_message msg;
	struct spi_transfer transfer[2] = { { 0 }, { 0 } };

	spi_message_init(&msg);

	transfer[0].len = cmdLength;
	transfer[0].tx_buf = cmd;
	transfer[0].rx_buf = NULL;
	spi_message_add_tail(&transfer[0], &msg);

	transfer[1].len = byteToRead;
	transfer[1].delay.value = SPI_DELAY_CS;
	transfer[1].delay.unit = SPI_DELAY_UNIT_USECS;
	transfer[1].tx_buf = NULL;
	transfer[1].rx_buf = outBuf;
	spi_message_add_tail(&transfer[1], &msg);

#endif

	if (client == NULL)
		return ERROR_BUS_O;

	if (fts_info && fts_info->tp_pm_suspend) {
		logError(1, "%s %s system suspend,don't to transfer\n", tag,
			 __func__);
		return ERROR_BUS_O;
	}

	while (retry < I2C_RETRY && ret < OK) {
#ifdef I2C_INTERFACE
		ret = i2c_transfer(getClient()->adapter, I2CMsg, 2);
#else
		ret = spi_sync(getClient(), &msg);
#endif

		retry++;
		if (ret < OK)
			mdelay(I2C_WAIT_BEFORE_RETRY);
	}
	if (ret < 0) {
		logError(1, "%s %s: ERROR %08X\n", tag, __func__, ERROR_BUS_WR);
		return ERROR_BUS_WR;
	}
	return OK;
}

/**
* Perform a bus write
* @param cmd byte array containing the command to write
* @param cmdLength size of cmd
* @return OK if success or an error code which specify the type of error encountered
*/
int fts_write(u8 *cmd, int cmdLength)
{
	int ret = -1;
	int retry = 0;

#ifdef I2C_INTERFACE
	struct i2c_msg I2CMsg[1];

	I2CMsg[0].addr = (__u16)I2CSAD;
	I2CMsg[0].flags = (__u16)0;
	I2CMsg[0].len = (__u16)cmdLength;
	I2CMsg[0].buf = (__u8 *)cmd;
#else
	struct spi_message msg;
	struct spi_transfer transfer[1] = { { 0 } };

	spi_message_init(&msg);

	transfer[0].len = cmdLength;
	transfer[0].delay.value = SPI_DELAY_CS;
	transfer[0].delay.unit = SPI_DELAY_UNIT_USECS;
	transfer[0].tx_buf = cmd;
	transfer[0].rx_buf = NULL;
	spi_message_add_tail(&transfer[0], &msg);
#endif

	if (client == NULL)
		return ERROR_BUS_O;
	if (fts_info && fts_info->tp_pm_suspend) {
		logError(1, "%s %s system suspend,don't to transfer\n", tag,
			 __func__);
		return ERROR_BUS_O;
	}
	while (retry < I2C_RETRY && ret < OK) {
#ifdef I2C_INTERFACE
		ret = i2c_transfer(getClient()->adapter, I2CMsg, 1);
#else
		ret = spi_sync(getClient(), &msg);
#endif

		retry++;
		if (ret < OK)
			mdelay(I2C_WAIT_BEFORE_RETRY);
	}
	if (ret < 0) {
		logError(1, "%s %s: ERROR %08X\n", tag, __func__, ERROR_BUS_W);
		return ERROR_BUS_W;
	}
	return OK;
}
#ifdef CONFIG_I2C_BY_DMA
/**
 * same as above but can be used when enable DMA.
 */
int fts_read_dma_safe(u8 *outBuf, int byteToRead)
{
	int ret;
	struct fts_dma_buf *dma = fts_info->dma_buf;
	u8 *malcBuf = dma->rdBuf;
	u8 *tmpBuf = NULL;
	u8 *finalBuf;

	mutex_lock(&dma->dmaBufLock);
	/*use malloc buf*/
	if (byteToRead > 1) {
		/*extend malloc buf*/
		if (unlikely(byteToRead > PAGE_SIZE)) {
			tmpBuf = kzalloc(byteToRead, GFP_KERNEL);
			if (!tmpBuf) {
				logError(1, "%s %s:Error alloc mem failed!",
					 tag, __func__);
				mutex_unlock(&dma->dmaBufLock);
				return -ENOMEM;
			}
			finalBuf = tmpBuf;
		} else {
			finalBuf = malcBuf;
		}
	} else {
		finalBuf = outBuf;
	}

	ret = fts_read(finalBuf, byteToRead);
	if ((ret == OK) && (byteToRead > 1)) {
		memcpy(outBuf, finalBuf, byteToRead);
	}
	if (unlikely(tmpBuf))
		kfree(tmpBuf);
	mutex_unlock(&dma->dmaBufLock);

	return ret;
}

int fts_writeRead_dma_safe(u8 *cmd, int cmdLength, u8 *outBuf, int byteToRead)
{
	int ret;
	struct fts_dma_buf *dma = fts_info->dma_buf;
	u8 *malcRdBuf = dma->rdBuf;
	u8 *malcWrBuf = dma->wrBuf;
	u8 *rdBuf, *tmpRdBuf = NULL;
	u8 *wrBuf, *tmpWrBuf = NULL;

	mutex_lock(&dma->dmaBufLock);
	if (cmdLength > 1) {
		if (unlikely(cmdLength > PAGE_SIZE)) {
			tmpWrBuf = kzalloc(cmdLength, GFP_KERNEL);
			if (!tmpWrBuf) {
				logError(1, "%s %s:Error alloc mem failed!",
					 tag, __func__);
				mutex_unlock(&dma->dmaBufLock);
				return -ENOMEM;
			}
			wrBuf = tmpWrBuf;
		} else {
			wrBuf = malcWrBuf;
		}
		memcpy(wrBuf, cmd, cmdLength);
	} else {
		wrBuf = cmd;
	}

	if (byteToRead > 1) {
		if (unlikely(byteToRead > PAGE_SIZE)) {
			tmpRdBuf = kzalloc(byteToRead, GFP_KERNEL);
			if (!tmpRdBuf) {
				logError(1, "%s %s:Error alloc mem failed!",
					 tag, __func__);
				if (tmpWrBuf)
					kfree(tmpWrBuf);
				mutex_unlock(&dma->dmaBufLock);
				return -ENOMEM;
			}
			rdBuf = tmpRdBuf;
		} else {
			rdBuf = malcRdBuf;
		}
	} else {
		rdBuf = outBuf;
	}

	ret = fts_writeRead(wrBuf, cmdLength, rdBuf, byteToRead);
	if ((ret == OK) && (byteToRead > 1))
		memcpy(outBuf, rdBuf, byteToRead);

	if (unlikely(tmpRdBuf))
		kfree(tmpRdBuf);
	if (unlikely(tmpWrBuf))
		kfree(tmpWrBuf);
	mutex_unlock(&dma->dmaBufLock);

	return ret;
}

int fts_write_dma_safe(u8 *cmd, int cmdLength)
{
	int ret;
	struct fts_dma_buf *dma = fts_info->dma_buf;
	u8 *malcBuf = dma->wrBuf;
	u8 *tmpBuf = NULL;
	u8 *finalBuf;

	mutex_lock(&dma->dmaBufLock);
	/*use malloc buf*/
	if (cmdLength > 1) {
		/*extend malloc buf*/
		if (unlikely(cmdLength > PAGE_SIZE)) {
			tmpBuf = kzalloc(cmdLength, GFP_KERNEL);
			if (!tmpBuf) {
				logError(1, "%s %s:Error alloc mem failed!",
					 tag, __func__);
				mutex_unlock(&dma->dmaBufLock);
				return -ENOMEM;
			}
			finalBuf = tmpBuf;
		} else {
			finalBuf = malcBuf;
		}
		memcpy(finalBuf, cmd, cmdLength);
	} else {
		finalBuf = cmd;
	}

	ret = fts_write(finalBuf, cmdLength);

	if (unlikely(tmpBuf))
		kfree(tmpBuf);
	mutex_unlock(&dma->dmaBufLock);

	return ret;
}
#else
int fts_read_dma_safe(u8 *outBuf, int byteToRead)
{
	return fts_read(outBuf, byteToRead);
}
int fts_writeRead_dma_safe(u8 *cmd, int cmdLength, u8 *outBuf, int byteToRead)
{
	return fts_writeRead(cmd, cmdLength, outBuf, byteToRead);
}
int fts_write_dma_safe(u8 *cmd, int cmdLength)
{
	return fts_write(cmd, cmdLength);
}
#endif

/**
* Write a FW command to the IC and check automatically the echo event
* @param cmd byte array containing the command to send
* @param cmdLength size of cmd
* @return OK if success, or an error code which specify the type of error encountered
*/
int fts_writeFwCmd(u8 *cmd, int cmdLength)
{
	int ret = -1;
	int ret2 = -1;
	int retry = 0;
#ifdef I2C_INTERFACE
	struct i2c_msg I2CMsg[1];

	I2CMsg[0].addr = (__u16)I2CSAD;
	I2CMsg[0].flags = (__u16)0;
	I2CMsg[0].len = (__u16)cmdLength;
	I2CMsg[0].buf = (__u8 *)cmd;
#else
	struct spi_message msg;
	struct spi_transfer transfer[1] = { { 0 } };

	spi_message_init(&msg);

	transfer[0].len = cmdLength;
	transfer[0].delay.value = SPI_DELAY_CS;
	transfer[0].delay.unit = SPI_DELAY_UNIT_USECS;
	transfer[0].tx_buf = cmd;
	transfer[0].rx_buf = NULL;
	spi_message_add_tail(&transfer[0], &msg);
#endif

	if (client == NULL)
		return ERROR_BUS_O;
	if (fts_info && fts_info->tp_pm_suspend) {
		logError(1, "%s %s system suspend,don't to transfer\n", tag,
			 __func__);
		return ERROR_BUS_O;
	}
	resetErrorList();
	while (retry < I2C_RETRY && (ret < OK || ret2 < OK)) {
#ifdef I2C_INTERFACE
		ret = i2c_transfer(getClient()->adapter, I2CMsg, 1);
#else
		ret = spi_sync(getClient(), &msg);
#endif
		retry++;
		if (ret >= 0)
			ret2 = checkEcho(cmd, cmdLength);
		if (ret < OK || ret2 < OK)
			mdelay(I2C_WAIT_BEFORE_RETRY);
	}
	if (ret < 0) {
		logError(1, "%s fts_writeFwCmd: ERROR %08X\n", tag,
			 ERROR_BUS_W);
		return ERROR_BUS_W;
	}
	if (ret2 < OK) {
		logError(1, "%s fts_writeFwCmd: check echo ERROR %08X\n", tag,
			 ret2);
		return ret2;
	}
	return OK;
}

/**
* Perform two bus write and one bus read without any stop condition
* In case of FTI this function is not supported and the same sequence can be achieved calling fts_write followed by an fts_writeRead.
* @param writeCmd1 byte array containing the first command to write
* @param writeCmdLength size of writeCmd1
* @param readCmd1 byte array containing the second command to write
* @param readCmdLength size of readCmd1
* @param outBuf pointer of a byte array which should contain the bytes read from the IC
* @param byteToRead number of bytes to read
* @return OK if success or an error code which specify the type of error encountered
*/
int fts_writeThenWriteRead(u8 *writeCmd1, int writeCmdLength, u8 *readCmd1,
			   int readCmdLength, u8 *outBuf, int byteToRead)
{
	int ret = -1;
	int retry = 0;

#ifdef I2C_INTERFACE
	struct i2c_msg I2CMsg[3];

	I2CMsg[0].addr = (__u16)I2CSAD;
	I2CMsg[0].flags = (__u16)0;
	I2CMsg[0].len = (__u16)writeCmdLength;
	I2CMsg[0].buf = (__u8 *)writeCmd1;

	I2CMsg[1].addr = (__u16)I2CSAD;
	I2CMsg[1].flags = (__u16)0;
	I2CMsg[1].len = (__u16)readCmdLength;
	I2CMsg[1].buf = (__u8 *)readCmd1;

	I2CMsg[2].addr = (__u16)I2CSAD;
	I2CMsg[2].flags = I2C_M_RD;
	I2CMsg[2].len = byteToRead;
	I2CMsg[2].buf = (__u8 *)outBuf;
#else
	struct spi_message msg;
	struct spi_transfer transfer[3] = { { 0 }, { 0 }, { 0 } };

	spi_message_init(&msg);

	transfer[0].len = writeCmdLength;
	transfer[0].tx_buf = writeCmd1;
	transfer[0].rx_buf = NULL;
	spi_message_add_tail(&transfer[0], &msg);

	transfer[1].len = readCmdLength;
	transfer[1].tx_buf = readCmd1;
	transfer[1].rx_buf = NULL;
	spi_message_add_tail(&transfer[1], &msg);

	transfer[2].len = byteToRead;
	transfer[2].delay.value = SPI_DELAY_CS;
	transfer[2].delay.unit = SPI_DELAY_UNIT_USECS;
	transfer[2].tx_buf = NULL;
	transfer[2].rx_buf = outBuf;
	spi_message_add_tail(&transfer[2], &msg);
#endif

	if (client == NULL)
		return ERROR_BUS_O;
	if (fts_info && fts_info->tp_pm_suspend) {
		logError(1, "%s %s system suspend,don't to transfer\n", tag,
			 __func__);
		return ERROR_BUS_O;
	}
	while (retry < I2C_RETRY && ret < OK) {
#ifdef I2C_INTERFACE
		ret = i2c_transfer(getClient()->adapter, I2CMsg, 3);
#else
		ret = spi_sync(getClient(), &msg);
#endif
		retry++;
		if (ret < OK)
			mdelay(I2C_WAIT_BEFORE_RETRY);
	}

	if (ret < 0) {
		logError(1, "%s %s: ERROR %08X\n", tag, __func__, ERROR_BUS_WR);
		return ERROR_BUS_WR;
	}
	return OK;
}

/**
* Perform a chunked write with one byte op code and 1 to 8 bytes address
* @param cmd byte containing the op code to write
* @param addrSize address size in byte
* @param address the starting address
* @param data pointer of a byte array which contain the bytes to write
* @param dataSize size of data
* @return OK if success or an error code which specify the type of error encountered
*/
/* this function works only if the address is max 8 bytes */
int fts_writeU8UX(u8 cmd, AddrSize addrSize, u64 address, u8 *data,
		  int dataSize)
{
	u8 *finalCmd = buf1;
	int remaining = dataSize;
	int toWrite = 0, i = 0;

	mutex_lock(&rw_lock);
	if (addrSize <= sizeof(u64)) {
		while (remaining > 0) {
			if (remaining >= WRITE_CHUNK) {
				toWrite = WRITE_CHUNK;
				remaining -= WRITE_CHUNK;
			} else {
				toWrite = remaining;
				remaining = 0;
			}

			finalCmd[0] = cmd;
			logError(0, "%s %s: addrSize = %d \n", tag, __func__,
				 addrSize);
			for (i = 0; i < addrSize; i++) {
				finalCmd[i + 1] =
					(u8)((address >>
					      ((addrSize - 1 - i) * 8)) &
					     0xFF);
				logError(1, "%s %s: cmd[%d] = %02X \n", tag,
					 __func__, i + 1, finalCmd[i + 1]);
			}

			memcpy(&finalCmd[addrSize + 1], data, toWrite);

			if (fts_write(finalCmd, 1 + addrSize + toWrite) < OK) {
				logError(1, "%s %s: ERROR %08X \n", tag,
					 __func__, ERROR_BUS_W);
				mutex_unlock(&rw_lock);
				return ERROR_BUS_W;
			}

			address += toWrite;

			data += toWrite;
		}
	} else {
		logError(
			1,
			"%s %s: address size bigger than max allowed %d... ERROR %08X \n",
			tag, __func__, sizeof(u64), ERROR_OP_NOT_ALLOW);
	}
	mutex_unlock(&rw_lock);

	return OK;
}

/**
* Perform a chunked write read with one byte op code and 1 to 8 bytes address and dummy byte support.
* @param cmd byte containing the op code to write
* @param addrSize address size in byte
* @param address the starting address
* @param outBuf pointer of a byte array which contain the bytes to read
* @param byteToRead number of bytes to read
* @param hasDummyByte  if the first byte of each reading is dummy (must be skipped) set to 1, otherwise if it is valid set to 0 (or any other value)
* @return OK if success or an error code which specify the type of error encountered
*/
int fts_writeReadU8UX(u8 cmd, AddrSize addrSize, u64 address, u8 *outBuf,
		      int byteToRead, int hasDummyByte)
{
	u8 *finalCmd = buf1;
	u8 *buff = buf2;
	int remaining = byteToRead;
	int toRead = 0, i = 0;

	mutex_lock(&rw_lock);
	while (remaining > 0) {
		if (remaining >= READ_CHUNK) {
			toRead = READ_CHUNK;
			remaining -= READ_CHUNK;
		} else {
			toRead = remaining;
			remaining = 0;
		}

		finalCmd[0] = cmd;
		for (i = 0; i < addrSize; i++) {
			finalCmd[i + 1] =
				(u8)((address >> ((addrSize - 1 - i) * 8)) &
				     0xFF);
		}

		if (hasDummyByte == 1) {
			if (fts_writeRead(finalCmd, 1 + addrSize, buff,
					  toRead + 1) < OK) {
				logError(1,
					 "%s %s: read error... ERROR %08X \n",
					 tag, __func__, ERROR_BUS_WR);
				mutex_unlock(&rw_lock);
				return ERROR_BUS_WR;
			}
			memcpy(outBuf, buff + 1, toRead);
		} else {
			if (fts_writeRead(finalCmd, 1 + addrSize, buff,
					  toRead) < OK) {
				logError(1,
					 "%s %s: read error... ERROR %08X \n",
					 tag, __func__, ERROR_BUS_WR);
				mutex_unlock(&rw_lock);
				return ERROR_BUS_WR;
			}
			memcpy(outBuf, buff, toRead);
		}

		address += toRead;

		outBuf += toRead;
	}

	mutex_unlock(&rw_lock);

	return OK;
}

/**
* Perform a chunked write followed by a second write with one byte op code  for each write and 1 to 8 bytes address (the sum of the 2 address size of the two writes can not exceed 8 bytes)
* @param cmd1 byte containing the op code of first write
* @param addrSize1 address size in byte of first write
* @param cmd2 byte containing the op code of second write
* @param addrSize2 address size in byte of second write
* @param address the starting address
* @param data pointer of a byte array which contain the bytes to write
* @param dataSize size of data
* @return OK if success or an error code which specify the type of error encountered
*/
int fts_writeU8UXthenWriteU8UX(u8 cmd1, AddrSize addrSize1, u8 cmd2,
			       AddrSize addrSize2, u64 address, u8 *data,
			       int dataSize)
{
	u8 *finalCmd1 = NULL;
	u8 *finalCmd2 = buf1;
	int remaining = dataSize;
	int toWrite = 0, i = 0, ret = OK;

	mutex_lock(&rw_lock);
	finalCmd1 = (u8 *)kzalloc(sizeof(u8) * 10, GFP_KERNEL);
	if (!finalCmd1) {
		ret = -ENOMEM;
		goto end;
	}

	while (remaining > 0) {
		if (remaining >= WRITE_CHUNK) {
			toWrite = WRITE_CHUNK;
			remaining -= WRITE_CHUNK;
		} else {
			toWrite = remaining;
			remaining = 0;
		}

		finalCmd1[0] = cmd1;
		for (i = 0; i < addrSize1; i++) {
			finalCmd1[i + 1] =
				(u8)((address >>
				      ((addrSize1 + addrSize2 - 1 - i) * 8)) &
				     0xFF);
		}

		finalCmd2[0] = cmd2;
		for (i = addrSize1; i < addrSize1 + addrSize2; i++) {
			finalCmd2[i - addrSize1 + 1] =
				(u8)((address >>
				      ((addrSize1 + addrSize2 - 1 - i) * 8)) &
				     0xFF);
		}

		memcpy(&finalCmd2[addrSize2 + 1], data, toWrite);

		if (fts_write(finalCmd1, 1 + addrSize1) < OK) {
			logError(1, "%s %s: first write error... ERROR %08X \n",
				 tag, __func__, ERROR_BUS_W);
			if (finalCmd1)
				kfree(finalCmd1);
			mutex_unlock(&rw_lock);
			return ERROR_BUS_W;
		}

		if (fts_write(finalCmd2, 1 + addrSize2 + toWrite) < OK) {
			logError(1,
				 "%s %s: second write error... ERROR %08X \n",
				 tag, __func__, ERROR_BUS_W);
			if (finalCmd1)
				kfree(finalCmd1);
			mutex_unlock(&rw_lock);
			return ERROR_BUS_W;
		}

		address += toWrite;

		data += toWrite;
	}

end:
	if (finalCmd1)
		kfree(finalCmd1);
	mutex_unlock(&rw_lock);

	return ret;
}

/**
* Perform a chunked write  followed by a write read with one byte op code and 1 to 8 bytes address for each write and dummy byte support.
* @param cmd1 byte containing the op code of first write
* @param addrSize1 address size in byte of first write
* @param cmd2 byte containing the op code of second write read
* @param addrSize2 address size in byte of second write	read
* @param address the starting address
* @param outBuf pointer of a byte array which contain the bytes to read
* @param byteToRead number of bytes to read
* @param hasDummyByte  if the first byte of each reading is dummy (must be skipped) set to 1, otherwise if it is valid set to 0 (or any other value)
* @return OK if success or an error code which specify the type of error encountered
*/
int fts_writeU8UXthenWriteReadU8UX(u8 cmd1, AddrSize addrSize1, u8 cmd2,
				   AddrSize addrSize2, u64 address, u8 *outBuf,
				   int byteToRead, int hasDummyByte)
{
	u8 *finalCmd1 = NULL;
	u8 *finalCmd2 = NULL;
	u8 *buff = buf1;
	int remaining = byteToRead;
	int toRead = 0, i = 0, ret = OK;

	mutex_lock(&rw_lock);
	finalCmd1 = (u8 *)kzalloc(sizeof(u8) * 10, GFP_KERNEL);
	if (!finalCmd1) {
		ret = -ENOMEM;
		goto end;
	}
	finalCmd2 = (u8 *)kzalloc(sizeof(u8) * 10, GFP_KERNEL);
	if (!finalCmd2) {
		ret = -ENOMEM;
		goto end;
	}

	while (remaining > 0) {
		if (remaining >= READ_CHUNK) {
			toRead = READ_CHUNK;
			remaining -= READ_CHUNK;
		} else {
			toRead = remaining;
			remaining = 0;
		}

		finalCmd1[0] = cmd1;
		for (i = 0; i < addrSize1; i++) {
			finalCmd1[i + 1] =
				(u8)((address >>
				      ((addrSize1 + addrSize2 - 1 - i) * 8)) &
				     0xFF);
		}

		finalCmd2[0] = cmd2;
		for (i = addrSize1; i < addrSize1 + addrSize2; i++) {
			finalCmd2[i - addrSize1 + 1] =
				(u8)((address >>
				      ((addrSize1 + addrSize2 - 1 - i) * 8)) &
				     0xFF);
		}

		if (fts_write(finalCmd1, 1 + addrSize1) < OK) {
			logError(1, "%s %s: first write error... ERROR %08X \n",
				 tag, __func__, ERROR_BUS_W);
			ret = ERROR_BUS_W;
			goto end;
		}

		if (hasDummyByte == 1) {
			if (fts_writeRead(finalCmd2, 1 + addrSize2, buff,
					  toRead + 1) < OK) {
				logError(1,
					 "%s %s: read error... ERROR %08X \n",
					 tag, __func__, ERROR_BUS_WR);
				ret = ERROR_BUS_WR;
				goto end;
			}
			memcpy(outBuf, buff + 1, toRead);
		} else {
			if (fts_writeRead(finalCmd2, 1 + addrSize2, buff,
					  toRead) < OK) {
				logError(1,
					 "%s %s: read error... ERROR %08X \n",
					 tag, __func__, ERROR_BUS_WR);
				ret = ERROR_BUS_WR;
				goto end;
			}
			memcpy(outBuf, buff, toRead);
		}

		address += toRead;

		outBuf += toRead;
	}

end:
	if (finalCmd1)
		kfree(finalCmd1);
	if (finalCmd2)
		kfree(finalCmd2);
	mutex_unlock(&rw_lock);
	return ret;
}
