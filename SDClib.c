/*
 * File: SDClib.c
 * 
 */
#include <htc.h>
#include <string.h>
#include "SDClib.h"

static SDC_FAT_INFO *FAT;

/*
 * ans = SDC_SPI_Transfer(dt)
 * SPI�ʐM��SDC�Ԃ̃f�[�^���M�ƃf�[�^��M���s������
 */
extern char SDC_SPI_Transfer(char dt); // ���[�U�[����`����

/*
 * SDC_Select_CS(flag)
 * SDC��CS�M����LOW/HIGH���o�͂��鏈��
 */
extern void SDC_Select_CS(char flag); // ���[�U�[����`����

/*
 * ans = SDC_Get_SDC()
 * SDC�����邩�ǂ�����Ԃ�����
 */
extern bit SDC_Get_SDC(); // ���[�U�[����`����

/*
 * ans = SDC_Send_Command(cmd, prm)
 * SDC�փR�}���h�𑗂鏈��
 */
unsigned char SDC_Send_Command(unsigned char cmd, unsigned long prm)
{
	SDC_EX_LONG p;
	unsigned char x, ans;

	SDC_Select_CS(0);
	if(cmd != SDC_CMD12) {
		// �J�[�h��Busy��Ԃ��甲���o���܂ő҂�
		for(x = 0; x < 255; x++)
			if(SDC_Get_SDC() == 0 || SDC_SPI_Transfer(0xff) == 0xff) break;
		for(x = 0; x < 255; x++)
			if(SDC_Get_SDC() == 0 || SDC_SPI_Transfer(0xff) == 0xff) break;
		for(x = 0; x < 255; x++)
			if(SDC_Get_SDC() == 0 || SDC_SPI_Transfer(0xff) == 0xff) break;
		for(x = 0; x < 255; x++)
			if(SDC_Get_SDC() == 0 || SDC_SPI_Transfer(0xff) == 0xff) break;
		if(x >= 255) return 0xfe; // �J�[�h��busy
	}
	if(SDC_Get_SDC() == 0) return 0xff; // �J�[�h���Ȃ�
	// �R�}���h�̑��M
	SDC_SPI_Transfer(cmd);
	// �p�����[�^�̑��M
	p.l = prm;
	for(x = 4; x > 0; x--) SDC_SPI_Transfer(p.c[x - 1]);
	switch(cmd) {
		case SDC_CMD0:
			SDC_SPI_Transfer(0x95);
			break;
		case SDC_CMD8:
			SDC_SPI_Transfer(0x87);
			break;
		case SDC_CMD12:
			SDC_SPI_Transfer(0xff);
		default:
			SDC_SPI_Transfer(0xff);
			break;
	}
	// �ԓ���҂�
	for(x = 0; x < 8; x++) {
		if(SDC_Get_SDC() == 0) return 0xff; // �J�[�h���Ȃ�
		if(((ans = SDC_SPI_Transfer(0xff)) & 0x80) == 0) return ans;
	}
	return 0xfd; // ���X�|���X�^�C���A�E�g
}

/*
 * ans = SDC_Send_RDCMD(sector)
 * �w��̃Z�N�^�ʒu����}���`�u���b�N�Ǎ��ރR�}���h�𑗂鏈��
 */
bit SDC_Send_RDCMD(unsigned long sector)
{
	if(FAT->FATType == 2) sector <<= 9; // FAT12/16
	// �}���`�u���b�N���[�h�R�}���h���M
	if(SDC_Send_Command(SDC_CMD18, sector) == 0) return 0;
	SDC_Select_CS(1);
	return 1;
}

/*
 * ans = SDC_Get_DataToken()
 * �f�[�^�g�[�N����҂���
 */
bit SDC_Get_DataToken()
{
	unsigned int i;

	_delay(64);
	for(i = 0; i < 0x300; i++) {
		switch(SDC_SPI_Transfer(0xff)) {
			case 0xff:
				break;
			case 0xfe:
				return 0;
			default:
				i = 0x300;
				break;
		}
	}
	SDC_Select_CS(1);
	return 1;
}

/*
 * ans = SDC_Read_SBrock(sector, *buf)
 * �w��̃Z�N�^�ʒu����1�u���b�N�Ǎ��ޏ���
 */
bit SDC_Read_SBrock(unsigned long sector, void *buf)
{
	register char *b;
	unsigned int i;

	b = buf;
	if(FAT->FATType == 2) sector <<= 9; // FAT12/16
	if(SDC_Send_Command(SDC_CMD17, sector) == 0) { // �V���O�����[�h�R�}���h���M
		// �f�[�^�g�[�N���̎�M
		if(SDC_Get_DataToken() == 0) {
			// �f�[�^��ǂݎ��
			for(i = 0; i < SDC_SECTOR_BYTES; i++) *b++ = SDC_SPI_Transfer(0xff);
			SDC_SPI_Transfer(0xff); // CRC�̕�������M
			SDC_SPI_Transfer(0xff);
			SDC_Select_CS(1);
			return 0;
		}
	}
	SDC_Select_CS(1);
	return 1;
}

/*
 * ans = SDC_Write_SBrock(sector, *buf)
 * �w��̃Z�N�^�ʒu�Ƀf�[�^��1�u���b�N�������ޏ���
 */
bit SDC_Write_SBrock(unsigned long sector, const void *buf)
{
	register const char *b;
	unsigned int i;

	b = buf;
	if(FAT->FATType == 2) sector <<= 9; // FAT32�łȂ�
	if(SDC_Send_Command(SDC_CMD24, sector) == 0) { // �V���O�����C�g�R�}���h���M
		SDC_SPI_Transfer(0xff);
		SDC_SPI_Transfer(0xfe); // �f�[�^�g�[�N��
		for(i = 0; i < SDC_SECTOR_BYTES; i++) SDC_SPI_Transfer(*b++);
		SDC_SPI_Transfer(0xff); // CRC�̕����𑗐M
		SDC_SPI_Transfer(0xff);
		// ����ɏ������߂����`�F�b�N����
		if((SDC_SPI_Transfer(0xff) & 0x1f) == 0x05) {
			if(SDC_Send_Command(SDC_CMD13, sector) == 0
					|| SDC_Send_Command(SDC_CMD13, sector) == 0) {
				if(SDC_SPI_Transfer(0xff) == 0) {
					SDC_Select_CS(1);
					return 0;
				}
			}
		}
	}
	SDC_Select_CS(1);
	return 1;
}

/*
 * ans = SDC_Init_SDC(*fat_info, *buf)
 * SDC(FAT12/16/32)/MMC(FAT12)�̏��������s������
 */
bit SDC_Init_SDC(SDC_FAT_INFO *fat_info)
{
	SDC_EX_LONG prm;
	unsigned int i;

	FAT = fat_info;
	// �J�[�h���R�}���h�҂���Ԃɂ���
	SDC_Select_CS(1);
	for(i = 0; i < 10; i++) SDC_SPI_Transfer(0xff);
	if(SDC_Send_Command(SDC_CMD0, 0) == 1) { // ���Z�b�g�R�}���h�𑗐M
		// �o�[�W�����`�F�b�N�R�}���h�𑗐M
		if((i = SDC_Send_Command(SDC_CMD8, 0x1AA)) == 1) {
			for(i = 0; i < 4; i++) {
				if(SDC_SPI_Transfer(0xff) == 0xAA && i == 3) {
					FAT->FATType = 4;
					prm.l = 0x40000000;
				} else FAT->FATType = 0;
			}
		} else if((i & 0x80) == 0) {
			if(i & 0x04) FAT->FATType = 2;
			else FAT->FATType = 1;
			prm.l = 0;
		} else FAT->FATType = 0;
		if(FAT->FATType) {
			// �������R�}���h�𑗐M
			for(i = 0; i < 0x300; i++) {
				if(FAT->FATType != 1) { // SDC
					SDC_Send_Command(SDC_CMD55, 0);
					if(SDC_Send_Command(SDC_ACMD41, prm.l) == 0) break;
				} else { // MMC
					if(SDC_Send_Command(SDC_CMD1, 0) != 1) break;
				}
				_delay(1000);
			}
			if(i < 0x300) {
				if(SDC_Get_SDC() == 0) FAT->FATType = 0;
				if(FAT->FATType == 4) {
					if(SDC_Send_Command(SDC_CMD58, 0) == 0) {
						for(i = 0; i < 4; i++) {
							if((SDC_SPI_Transfer(0xff) & 0x40) == 0 && i == 0)
								FAT->FATType = 2; // SDHC�łȂ�
						}
					} else FAT->FATType = 0;
				}
			} else FAT->FATType = 0;
		}
		// �u���b�N�T�C�Y�̐ݒ�
		if(FAT->FATType && SDC_Send_Command(SDC_CMD16, SDC_SECTOR_BYTES) != 0)
			FAT->FATType = 0;
	} else FAT->FATType = 0;
	SDC_Select_CS(1);
	if(FAT->FATType == 1) FAT->FATType = 2;

	// FAT�t�@�C���V�X�e���̃p�����[�^��ǂݍ���
	if(FAT->FATType) {
		// BPB(�u�[�g�Z�N�^)�̈ʒu�𒲂ׂ�
		if(SDC_Send_Command(SDC_CMD17, 0) || SDC_Get_DataToken()) FAT->FATType = 0;
		else {
			prm.l = 0xffffffff;
			for(i = 0; i < 14; i++) SDC_SPI_Transfer(0xff);
			for(i = 0; i < 2; i++) prm.c[i] = SDC_SPI_Transfer(0xff);
			// ReservedSectorCount��0�łȂ���
			if(prm.i[0]) {
				for(i = 0; i < 5; i++) SDC_SPI_Transfer(0xff);
				if((prm.c[2] = SDC_SPI_Transfer(0xff)) == 0xf0 || prm.c[2] >= 0xf8) {
					for(i = 0; i < 6; i++) SDC_SPI_Transfer(0xff);
					// HidddenSectors��0��
					if((SDC_SPI_Transfer(0xff) | SDC_SPI_Transfer(0xff)
							| SDC_SPI_Transfer(0xff) | SDC_SPI_Transfer(0xff)) == 0) {
						for(i = 0; i < 480; i++) SDC_SPI_Transfer(0xff);
						SDC_SPI_Transfer(0xff); // CRC
						SDC_SPI_Transfer(0xff);
						// FAT[0]��ǂݏo��
						if((SDC_Send_Command(SDC_CMD17,
								prm.i[0] << (FAT->FATType == 2 ? 9 : 0)) || SDC_Get_DataToken())
								&& (SDC_Send_Command(SDC_CMD17,
								prm.i[0] << (FAT->FATType == 2 ? 9 : 0)) || SDC_Get_DataToken()))
							FAT->FATType = 0;
						else {
							if(SDC_SPI_Transfer(0xff) == prm.c[2])
								prm.l = 0; // MBR��BPB��
							for(i = 0; i < 511; i++) SDC_SPI_Transfer(0xff);
						}
					} else {
						for(i = 0; i < 480; i++) SDC_SPI_Transfer(0xff);
					}
				} else {
					for(i = 0; i < 486; i++) SDC_SPI_Transfer(0xff);
				}
			} else {
				for(i = 0; i < 496; i++) SDC_SPI_Transfer(0xff);
			}
			SDC_SPI_Transfer(0xff); // CRC
			SDC_SPI_Transfer(0xff);
			SDC_Select_CS(1);
			if(prm.l != 0) { // MBR��BPB�łȂ�
				// BPB�܂ł̃I�t�Z�b�g�l�����o��
				if((SDC_Send_Command(SDC_CMD17, 0) || SDC_Get_DataToken())
						&& (SDC_Send_Command(SDC_CMD17, 0) || SDC_Get_DataToken()))
					FAT->FATType = 0;
				else {
					prm.l = 0;
					for(i = 0; i < 446; i++) SDC_SPI_Transfer(0xff);
					for(i = 0; (i & 0xff) < 4; i++) {
						if(prm.l == 0) {
							for(i &= 0xff; (i & 0xff00) < (4 << 8); i += 0x100)
								SDC_SPI_Transfer(0xff);
							switch(SDC_SPI_Transfer(0xff)) {
								case 0x01: // FAT12
								case 0x04: // FAT16
								case 0x06: // FAT12/16
								case 0x0e: // FAT12/16
								case 0x0b: // FAT32
								case 0x0c: // FAT32
									for(i &= 0xff; (i & 0xff00) < (3 << 8); i += 0x100)
										SDC_SPI_Transfer(0xff);
									for(i &= 0xff; (i & 0xff00) < (4 << 8); i += 0x100)
										prm.c[i >> 8] = SDC_SPI_Transfer(0xff);
									break;
								default:
									for(i &= 0xff; (i & 0xff00) < (7 << 8); i += 0x100)
										SDC_SPI_Transfer(0xff);
									break;
							}
							for(i &= 0xff; (i & 0xff00) < (4 << 8); i += 0x100)
								SDC_SPI_Transfer(0xff);
						} else {
							for(i &= 0xff; (i & 0xff00) < (16 << 8); i += 0x100)
								SDC_SPI_Transfer(0xff);
						}
					}
					SDC_SPI_Transfer(0xff); // CRC
					SDC_SPI_Transfer(0xff);
					if(prm.l == 0) FAT->FATType = 0; // ���o���Ɏ��s
				}
				SDC_Select_CS(1);
			}
			if(FAT->FATType) {
				// BPB��ǂݏo��
				if((SDC_Send_Command(SDC_CMD17,
						prm.l << (FAT->FATType == 2 ? 9 : 0)) || SDC_Get_DataToken())
						&& (SDC_Send_Command(SDC_CMD17,
						prm.l << (FAT->FATType == 2 ? 9 : 0)) || SDC_Get_DataToken()))
					FAT->FATType = 0;
				else {
					for(i = 0; i < 13; i++) SDC_SPI_Transfer(0xff);
					// 1�N���X�^�[������̃Z�N�^��(SectorsPerCluster)
					FAT->SectorsPerClusterCT = SDC_SPI_Transfer(0xff);
					// FAT�̈�̊J�n�Z�N�^�ʒu(prm.l + ReservedSectorCount)
					FAT->FATAreaStartP = prm.l;
					for(i = 0; i < 2; i++) prm.c[i] = SDC_SPI_Transfer(0xff);
					FAT->FATAreaStartP += prm.i[0];
					// FATCount��DirEntryStartP�ɕێ�
					FAT->DirEntryStartP = SDC_SPI_Transfer(0xff);
					// �f�B���N�g���G���g���̈�̊J�n�Z�N�^�ʒu
					// ((sizeof(SDC_DIR_ENTRY) * RootDirEntryCount
					// + SDC_SECTOR_BYTES - 1) / SDC_SECTOR_BYTES)
					for(i = 0; i < 2; i++) prm.c[i] = SDC_SPI_Transfer(0xff);
					FAT->DirEntrySectorCT = (sizeof (SDC_DIR_ENTRY) * prm.i[0]
							+ SDC_SECTOR_BYTES - 1) / SDC_SECTOR_BYTES;
					for(i = 0; i < 3; i++) SDC_SPI_Transfer(0xff);
					// ��g��FAT�̈悪���߂�Z�N�^��
					// (SectorsPerFAT16)
					for(i = 0; i < 2; i++) prm.c[i] = SDC_SPI_Transfer(0xff);
					FAT->SectorsPerFATCT = prm.i[0];
					for(i = 0; i < 12; i++) SDC_SPI_Transfer(0xff);
					// (SectorsPerFAT32)
					for(i = 0; i < 4; i++) prm.c[i] = SDC_SPI_Transfer(0xff);
					if(FAT->SectorsPerFATCT) FAT->FATType = 2; // FAT12/16
					else {
						FAT->SectorsPerFATCT = prm.l;
						FAT->FATType = 4; // FAT32
					}
					// �f�B���N�g���G���g���[�̈�̊J�n�Z�N�^�ʒu
					// (FATCount * SectorsPerFATCT + FATAreaStartP)
					FAT->DirEntryStartP *= FAT->SectorsPerFATCT;
					FAT->DirEntryStartP += FAT->FATAreaStartP;
					// �f�[�^�̈�̊J�n�Z�N�^�ʒu
					FAT->DataAreaStartP = FAT->DirEntryStartP + FAT->DirEntrySectorCT;
					if(FAT->FATType == 4)
						FAT->DirEntrySectorCT = FAT->SectorsPerClusterCT; // FAT32
					for(i = 0; i < 472; i++) SDC_SPI_Transfer(0xff);
					SDC_SPI_Transfer(0xff); // CRC
					SDC_SPI_Transfer(0xff);
				}
			}
		}
		SDC_Select_CS(1);
	}
	if(FAT->FATType) return 0;
	else return 1;
}

/*
 * SDC_Get_Time(*date, *time, *a_tenth_seconds)
 * ���݂̎��Ԃ�Ԃ�����
 */
extern void SDC_Get_Time(unsigned int *date, unsigned int *time,
		unsigned char *a_tenth_seconds); // ���[�U�[����`����

/*
 * *ans = SDC_Remake_FileName(*pre_filename)
 * �t�@�C���̖��O�𐮌`���鏈��
 */
char *SDC_Remake_FileName(const char *pre_filename)
{
	static char filename[11];
	unsigned char c;

	// �t�@�C���̖��O�̕����̐��`('.'�̑O�܂ŏ����ʂ��A�c���0x20�Ŗ��߂�)
	for(c = 0; c < 8; c++) {
		if(*pre_filename == '.' || *pre_filename == '\0') filename[c] = 0x20;
		else filename[c] = *pre_filename++;
	}
	// �t�@�C���̊g���q�ɃJ�[�\�������킹��
	while(*pre_filename != '.' && *pre_filename != '\0') pre_filename++;
	if(*pre_filename == '.') pre_filename++;
	// �g���q�̐��`(�g���q�������ʂ��A�c���0x20�Ŗ��߂�)
	for(c = 8; c < 11; c++) {
		if(*pre_filename == '\0') filename[c] = 0x20;
		else filename[c] = *pre_filename++;
	}
	return filename;
}

/*
 * ans = SDC_Make_DirEntry(*filename, *fds, *buf)
 * FDS�^�ϐ��̏�����Ƀf�B���N�g���G���g���[����鏈��
 */
bit SDC_Make_DirEntry(const char *filename, const SDC_FDS *fds, void *buf)
{
	SDC_DIR_ENTRY *inf;
	unsigned long p;
	unsigned int date, time;
	unsigned char atensec;

	SDC_Get_Time(&date, &time, &atensec);
	if(SDC_Read_SBrock(p =
			_SDC_Calculate_DirEntryP(fds->DirEntryIndex, *FAT), buf)) return 1;
	inf = (SDC_DIR_ENTRY *)&((char *)buf)[_SDC_Calculate_DEPByte(fds->DirEntryIndex)];
	// �t�@�C������ݒ肷��(filename�ɉ��������Ă��Ȃ���Ή������Ȃ�)
	if(filename[0]) memcpy(inf->FileName, filename, 11);
	// �t�@�C���̃T�C�Y��ݒ肷��
	inf->FileSize = fds->FileSize;
	//�f�[�^�i�[���FAT�ԍ���ݒ�(��Ȃ�Ή������Ȃ�)
	if(fds->FirstFATNo.l) {
		if(FAT->FATType == 2) {
			// FAT12/16
			if(fds->FirstFATNo.i[0] == 0xffff) inf->FirstClusterLow = 0;
			else inf->FirstClusterLow = fds->FirstFATNo.i[0];
			inf->FirstClusterHigh = 0;
		} else {
			// FAT32
			if(fds->FirstFATNo.l == 0x0fffffff) {
				inf->FirstClusterLow = 0;
				inf->FirstClusterHigh = 0;
			} else {
				inf->FirstClusterLow = fds->FirstFATNo.i[0];
				inf->FirstClusterHigh = fds->FirstFATNo.i[1];
			}
		}
	}
	// �t�@�C���̑�����ݒ肷��
	inf->Attributes = fds->Attributes;
	// �t�@�C���̍쐬������ݒ肷��(filename�ɉ��������Ă��Ȃ���Ή������Ȃ�)
	if(filename[0]) {
		inf->CreationTimeTenths = atensec;
		inf->CreationTime = time;
		inf->CreationDate = date;
	}
	// �ŏI�������ݎ��Ԃ�ݒ肷��
	inf->LastWriteTime = time;
	// �t�@�C���̍쐬����ݒ肷��
	// �ŏI�������ݓ���ݒ肷��
	inf->LastWriteDate = date;
	// �A�N�Z�X����ݒ肷��
	inf->LastAccessDate = date;

	inf->ReservedNT = 0;

	if(SDC_Write_SBrock(p, buf) && SDC_Write_SBrock(p, buf)) return 1;
	return 0;
}

/*
 * ans = SDC_Delete_DirEntry(index, *buf)
 * �f�B���N�g���G���g���[���������鏈��
 */
bit SDC_Delete_DirEntry(unsigned int index, void *buf)
{
	unsigned long p;

	if(SDC_Read_SBrock(p = _SDC_Calculate_DirEntryP(index, *FAT), buf)) return 1;
	((char *)buf)[_SDC_Calculate_DEPByte(index)] = 0xe5;
	if(SDC_Write_SBrock(p, buf) && SDC_Write_SBrock(p, buf)) return 1;
	return 0;
}

/*
 * ans = SDC_Search_EmptyDirEntry()
 * �󂫂̃f�B���N�g���G���g���[��T������
 */
unsigned int SDC_Search_EmptyDirEntry()
{
	unsigned int i;
	unsigned char c, x;

	// ���[�h�J�n
	if(SDC_Send_RDCMD(FAT->DirEntryStartP)) return 0xffff; // �G���[
	for(i = 0; i < FAT->DirEntrySectorCT; i++) {
		if(SDC_Get_SDC() == 0) return 0xffff; // SDC���Ȃ�
		if(SDC_Get_DataToken()) {
			if(SDC_Send_RDCMD(FAT->DirEntryStartP + i)
					&& SDC_Send_RDCMD(FAT->DirEntryStartP + i)) return 0xffff; // �G���[
		}
		for(c = 0; c < (SDC_SECTOR_BYTES / sizeof (SDC_DIR_ENTRY)); c++) {
			x = SDC_SPI_Transfer(0xff);
			// �������ꂽ�G���g���A�܂��͋�̃G���g���Ȃ�A���̈ʒu���L�^����
			if(x == 0xe5 || x == 0x00) {
				SDC_Send_Command(SDC_CMD12, 0);
				SDC_Select_CS(1);
				return (i * (SDC_SECTOR_BYTES / sizeof (SDC_DIR_ENTRY))) +c;
			}
			for(x = 0; x < sizeof (SDC_DIR_ENTRY) - 1; x++) SDC_SPI_Transfer(0xff);
		}
		SDC_SPI_Transfer(0xff); // CRC�̕�������M
		SDC_SPI_Transfer(0xff);
	}
	SDC_Send_Command(SDC_CMD12, 0);
	SDC_Select_CS(1);
	return 0xffff; // ��������
}

/*
 * ans = SDC_Search_EmptyFAT(offset)
 * �󂫂�FAT��T������
 */
unsigned long SDC_Search_EmptyFAT(unsigned long offset)
{
	unsigned long l;
	unsigned int i;

	// �����ꏊ��offset�ȍ~�ɐݒ肷��(offset��2�����̂Ƃ��͍ŏ����猟���A
	// ����ȊO�̂Ƃ��͌����ꏊ��offset���܂܂Ȃ�)
	if(offset < 2) offset = 2;
	else offset++;
	l = offset / (SDC_SECTOR_BYTES / FAT->FATType); // �Z�N�^�ʒu(_Calculate...��FATAreaStartP�𑫂��̂Ŏg���Ȃ�)
	offset = _SDC_Calculate_FAPByte(offset, *FAT); // �Z�N�^�̒��̃o�C�g�ʒu
	// ���[�h�J�n
	if(SDC_Send_RDCMD(FAT->FATAreaStartP + l)) return 0;
	for(; l < FAT->SectorsPerFATCT; l++) {
		if(SDC_Get_SDC() == 0) return 0; // SDC���Ȃ�
		if(SDC_Get_DataToken()) {
			if(SDC_Send_RDCMD(FAT->FATAreaStartP + l)
					&& SDC_Send_RDCMD(FAT->FATAreaStartP + l)) return 0; // �G���[
		}
		for(i = 0; i < offset; i++) SDC_SPI_Transfer(0xff);
		for(; offset < SDC_SECTOR_BYTES; offset += FAT->FATType) {
			i = SDC_SPI_Transfer(0xff);
			i |= SDC_SPI_Transfer(0xff);
			if(FAT->FATType == 4) { // FAT32
				i |= SDC_SPI_Transfer(0xff);
				i |= SDC_SPI_Transfer(0xff) & 0x0f;
			}
			if(i == 0) {
				SDC_Send_Command(SDC_CMD12, 0);
				SDC_Select_CS(1);
				return (l * SDC_SECTOR_BYTES + offset) / FAT->FATType; // FAT�ԍ����Z�o
			}
		}
		SDC_SPI_Transfer(0xff); // CRC�̕�������M
		SDC_SPI_Transfer(0xff);
		offset = 0;
	}
	SDC_Send_Command(SDC_CMD12, 0);
	SDC_Select_CS(1);
	return 0x0fffffff; // ��������
}

/*
 * ans = SDC_Get_NextFATNo(fatno)
 * ����FAT�ԍ����擾���鏈��
 */
unsigned long SDC_Get_NextFATNo(unsigned long fatno)
{
	SDC_EX_LONG ans;
	unsigned int i;

	// ���[�h�J�n
	if(SDC_Send_RDCMD(_SDC_Calculate_FATAreaP(fatno, *FAT))
			&& SDC_Get_DataToken()) return 0; // �G���[
	else if(SDC_Send_RDCMD(_SDC_Calculate_FATAreaP(fatno, *FAT))
			&& SDC_Get_DataToken()) return 0; // �G���[
	fatno = _SDC_Calculate_FAPByte(fatno, *FAT);
	for(i = 0; i < fatno; i++) SDC_SPI_Transfer(0xff);
	ans.l = 0;
	// FAT�ԍ����L�^
	for(i = 0; i < FAT->FATType; i++) ans.c[i] = SDC_SPI_Transfer(0xff);
	SDC_Send_Command(SDC_CMD12, 0);
	SDC_Select_CS(1);
	if(FAT->FATType == 2 && ans.i[0] == 0xffff) ans.l = 0x0fffffff;
	if(FAT->FATType == 4) ans.c[3] &= 0x0f;
	return ans.l;
}

/*
 * ans = SDC_Set_NextFAT(pfatno, set_fatno, *buf)
 * ����FAT�ԍ����Z�b�g���鏈��
 */
bit SDC_Set_NextFAT(unsigned long pfatno, unsigned long set_fatno, void *buf)
{
	SDC_EX_LONG fatno;

	fatno.l = set_fatno;
	if(SDC_Read_SBrock(set_fatno = _SDC_Calculate_FATAreaP(pfatno, *FAT), buf)) return 1;
	pfatno = _SDC_Calculate_FAPByte(pfatno, *FAT);
	((char *)buf)[pfatno] = fatno.c[0];
	((char *)buf)[pfatno + 1] = fatno.c[1];
	if(FAT->FATType == 4) {
		((char *)buf)[pfatno + 2] = fatno.c[2];
		((char *)buf)[pfatno + 3] &= 0xf0;
		((char *)buf)[pfatno + 3] |= fatno.c[3];
	}
	if(SDC_Write_SBrock(set_fatno, buf) && SDC_Write_SBrock(set_fatno, buf)) return 1;
	if(SDC_Write_SBrock(set_fatno + FAT->SectorsPerFATCT, buf)
			&& SDC_Write_SBrock(set_fatno + FAT->SectorsPerFATCT, buf)) return 1;
	return 0;
}
