/* 
 * File: SDClib.h
 * SDC/MMC�̃A�N�Z�X���s���֐����C�u����
 * 
 * Fanctions -
 * ans = SDC_Send_Command(cmd, prm) :
 *  SDC�փR�}���h�𑗂鏈��
 * ans = SDC_Send_RDCMD(sector) :
 *  SDC�փV���O�����[�h�R�}���h�𑗂鏈��
 * ans = SDC_Get_DataToken() :
 *  �f�[�^�g�[�N����҂���
 * ans = SDC_Read_SBrock(sector, *buf) :
 *  SDC����V���O���u���b�N�ǂݍ��ޏ���
 * ans = SDC_Write_SBrock(sector, *buf) :
 *  SDC�փV���O���u���b�N�������ޏ���
 * ans = SDC_Init_SDC(*fat_info) :
 *  SDC�̏��������s������
 * *ans = SDC_Remake_FileName(*pre_filename) :
 *  �t�@�C���̖��O�𐮌`���鏈��
 * ans = SDC_Make_DirEntry(*filename, *fds, *buf) :
 *  �f�B���N�g���G���g���[�����鏈��
 * ans = SDC_Delete_DirEntry(index, *buf) :
 *  �f�B���N�g���G���g���[���������鏈��
 * ans = SDC_Search_EmptyDirEntry() :
 *  ��̃f�B���N�g���G���g���[��T������
 * ans = SDC_Search_EmptyFAT(offset) :
 *  �󂫂�FAT��T������
 * ans = SDC_Get_NextFATNo(fatno) :
 *  ����FAT�ԍ��𓾂鏈��
 * ans = SDC_Set_NextFAT(pfatno, set_fatno, *buf) :
 *  ����FAT�ԍ����Z�b�g���鏈��
 * 
 * ans = _SDC_Calculate_DirEntryP(index, fat_info) :
 *  �f�B���N�g���G���g���[�̃C���f�b�N�X����Z�N�^�ʒu���Z�o����
 * ans = _SDC_Calculate_DEPByte(index) :
 *  �f�B���N�g���G���g���[�̃C���f�b�N�X����Z�N�^���̃o�C�g�ʒu���Z�o����
 * ans = _SDC_Calculate_DataAreaP(fatno, file_seek_p, fat_info) :
 *  FAT�ԍ�����f�[�^�G���A�̃|�C���g���Z�N�^�ʒu�ŎZ�o����
 * ans = _SDC_Calculate_FATAreaP(fatno, fat_info) :
 *  FAT�ԍ�����FAT�G���A�̃|�C���g���Z�N�^�ʒu�ŎZ�o����
 * ans = _SDC_Calculate_FAPByte(fatno, fat_info) :
 *  FAT�ԍ�����FAT�G���A�̃|�C���g�̃Z�N�^�ʒu�̒��̃o�C�g�ʒu���Z�o����
 * 
 * Define -
 * ans = SDC_SPI_Transfer(dt) :
 *  SPI�ʐM��SDC�Ԃ̃f�[�^����M���s���֐�
 * SDC_Select_CS(flag) :
 *  SDC��CS�M�����o�͂���֐�
 * ans = SDC_Get_SDC() :
 *  SDC�̗L��/������Ԃ��֐�
 * SDC_Get_Time(*date, *time, *a_tenth_seconds) :
 *  ���݂̎��Ԃ�Ԃ��֐����w��
 * 
 * memo -
 * �E�����̊֐��͕W���܂���mini�Amicro��SD(FAT16)/SDHC(FAT32)
 * �܂���MMC(FAT12)�ŗ��p�\
 * 
 * ============================================================================
 * Version Date       TotalSize(Program/SRAM)(�ڈ�) Note
 * ============================================================================
 *    1.00 2015/09/27  15B4hbyte/0062byte(18F25K80) ���ۂ̓o�b�t�@�p�ɂ���512byte
 *                                                  (�ǂݍ��݂݂̂̏ꍇ�͕s�v)��
 *                                                  ��FAT_INFO�^�ϐ����ЂƂK�v
 * ============================================================================
 * Microchip MPLAB XC8 C Compiler (Free Mode) V1.35
 * 
 */
#ifndef SDCLIB_H
#define	SDCLIB_H

////////////////////////////////////////////////////////////////////////////////
//�r�c�b�A�N�Z�X�֘A

#define SDC_CMD0   (0x00 | 0x40)               // �J�[�h�ւ̃��Z�b�g�R�}���h
#define SDC_CMD1   (0x01 | 0x40)               // MMC�ւ̏������R�}���h
#define SDC_CMD8   (0x08 | 0x40)               // ����d���̊m�F��SDC�̃o�[�W�����`�F�b�N
#define SDC_CMD12  (0x0C | 0x40)               // �f�[�^�Ǎ��݂��~������R�}���h
#define SDC_CMD13  (0x0D | 0x40)               // �����݂̏�Ԃ�₢���킹��R�}���h
#define SDC_CMD16  (0x10 | 0x40)               // �u���b�N�T�C�Y�̏����l�ݒ�R�}���h
#define SDC_CMD17  (0x11 | 0x40)               // �V���O���u���b�N�Ǎ��ݗv���R�}���h
#define SDC_CMD18  (0x12 | 0x40)               // �}���`�u���b�N�Ǎ��ݗv���R�}���h
#define SDC_CMD24  (0x18 | 0x40)               // �V���O���u���b�N�����ݗv���R�}���h
#define SDC_ACMD41 (0x29 | 0x40)               // SDC�ւ̏������R�}���h
#define SDC_CMD55  (0x37 | 0x40)               // ACMD41/ACMD23�ƃZ�b�g�Ŏg�p����R�}���h
#define SDC_CMD58  (0x3A | 0x40)               // OCR�̓Ǐo���R�}���h

////////////////////////////////////////////////////////////////////////////////
// FAT�֘A

#define SDC_SECTOR_BYTES  512                  // 1�Z�N�^�̃o�C�g��

/* FAT�t�@�C���V�X�e���̃p�����[�^ */
typedef struct {
	unsigned char FATType;                     // FAT�̎��
	unsigned char SectorsPerClusterCT;         // 1�N���X�^������̃Z�N�^��
	unsigned long FATAreaStartP;               // FAT�̈�̊J�n�Z�N�^�ʒu
	unsigned long SectorsPerFATCT;             // 1�g��FAT�̈悪��߂�Z�N�^��
	unsigned long DirEntryStartP;              // �f�B���N�g���G���g���[�̊J�n�Z�N�^�ʒu
	unsigned int DirEntrySectorCT;             // �f�B���N�g���G���g���[�̃Z�N�^��
	unsigned long DataAreaStartP;              // �f�[�^�̈�̊J�n�Z�N�^�ʒu
} SDC_FAT_INFO;
#ifndef _FAT_INFO_
#define _FAT_INFO_
typedef SDC_FAT_INFO FAT_INFO;
#endif

/* FAT�t�@�C���V�X�e��(FAT12/16/32)�̃p�����[�^�\����(512�o�C�g) */
struct SDC_FAT_HEADER {
	unsigned char jump[3];                     // �u�[�g�p�̃W�����v�R�[�h
	unsigned char oemId[8];                    // �t�H�[�}�b�g���̃{�����[����
	unsigned int  BytesPerSector;              // 1�Z�N�^������̃o�C�g���A�ʏ��512�o�C�g
	unsigned char SectorsPerCluster;           // 1�N���X�^������̃Z�N�^��
	unsigned int  ReservedSectorCount;         // �u�[�g�Z�N�^�ȍ~�̗\��̈�̃Z�N�^��
	unsigned char FATCount;                    // FAT�̑g��(�o�b�N�A�b�vFAT��)�A�ʏ�͂Q�g
	unsigned int  RootDirEntryCount;           // �f�B���N�g���̍쐬�\���A�ʏ��512��
	unsigned int  TotalSectors16;              // �S�̈�̃Z�N�^�[����(FAT12/FAT16�p)
	unsigned char MediaType;                   // FAT�̈�̐擪�̒l�A�ʏ��0xF8
	unsigned int  SectorsPerFAT16;             // 1�g��FAT�̈悪��߂�Z�N�^��(FAT12/FAT16�p)
	unsigned int  SectorsPerTrack;             // 1�g���b�N������̃Z�N�^��
	unsigned int  HeadCount;                   // �w�b�h��
	unsigned long HidddenSectors;              // �B�����ꂽ�Z�N�^��
	unsigned long TotalSectors32;              // �S�̈�̃Z�N�^�[����(FAT32�p)
	unsigned long SectorsPerFAT32;             // 1�g��FAT�̈悪��߂�Z�N�^��(FAT32�p)
	unsigned int  FlagsFAT32;                  // FAT�̗L���������̏��t���O
	unsigned int  VersionFAT32;                //
	unsigned long RootDirClusterFAT32;         // �f�B���N�g���̃X�^�[�g�N���X�^(FAT32�p)
	unsigned char Dumy[6];                     //
	unsigned char FileSystemType[8];           // FAT�̎��("FAT12/16")(FAT32��20�o�C�g���ɗL��)
	unsigned char BootCode[448];               // �u�[�g�R�[�h�̈�
	unsigned char BootSectorSig0;              // 0x55
	unsigned char BootSectorSig1;              // 0xaa
} ;
#ifndef FAT_HEADER
#define FAT_HEADER  SDC_FAT_HEADER
#endif

/* �f�B���N�g���G���g���[�̍\����(32�o�C�g) */
typedef struct {
	         char FileName[11];                // �t�@�C����(8)+�g���q(3)
	unsigned char Attributes;                  // �t�@�C���̑���
	unsigned char ReservedNT;                  // Windows NT �p �\��̈�
	unsigned char CreationTimeTenths;          // �t�@�C���쐬���Ԃ�1/10�b�P�ʂ�����킷
	unsigned int  CreationTime;                // �t�@�C���̍쐬����(hhhhhmmmmmmsssss)
	unsigned int  CreationDate;                // �t�@�C���̍쐬��(yyyyyyymmmmddddd)
	unsigned int  LastAccessDate;              // �ŏI�̃A�N�Z�X��
	unsigned int  FirstClusterHigh;            // �f�[�^�i�[���FAT�ԍ���ʂQ�o�C�g
	unsigned int  LastWriteTime;               // �ŏI�̃t�@�C�������ݎ���
	unsigned int  LastWriteDate;               // �ŏI�̃t�@�C�������ݓ�
	unsigned int  FirstClusterLow;             // �f�[�^�i�[���FAT�ԍ����ʂQ�o�C�g
	unsigned long FileSize;                    // �t�@�C���̃T�C�Y
} SDC_DIR_ENTRY;
#ifndef _DIR_ENTRY_
#define _DIR_ENTRY_
typedef SDC_DIR_ENTRY DIR_ENTRY;
#endif

typedef union {
	unsigned long l;
	unsigned int  i[2];
	unsigned char c[4];
} SDC_EX_LONG;
#ifndef _SDC_EX_LONG_
#define _SDC_EX_LONG_
typedef SDC_EX_LONG EX_LONG;
#endif

/* �t�@�C����� */
typedef struct {
	unsigned int  DirEntryIndex;               // �f�B���N�g���G���g���[�̌��������ꏊ�̈ʒu
	unsigned long FileSize;                    // �t�@�C���̃T�C�Y
	SDC_EX_LONG   FirstFATNo;                  // �f�[�^�i�[���FAT�ԍ�(�Ȃ����0x0fffffff�ɂ���)
	unsigned char Attributes;                  // �t�@�C���̑���
} SDC_FDS;
#ifndef _FDS_
#define _FDS_
typedef SDC_FDS FDS;
#endif

// DIR_ENTRY��FDS��Attributes�p�̃t���O
#define SDC_READ_ONLY_FILE  0x01               // �������݋֎~
#define SDC_HIDDEN_FILE     0x02               // �B��
#define SDC_SYSTEM_FILE     0x04               // �V�X�e��
#define SDC_DIRECTORY_FILE  0x10               // �f�B���N�g��
#define SDC_ARCHIVE_FILE    0x20               // �A�[�J�C�u

////////////////////////////////////////////////////////////////////////////////

/*
 * ans = SDC_SPI_Transfer(dt)(���[�U�[��`)
 * SPI�ʐM��SDC�Ԃ̃f�[�^���M�ƃf�[�^��M���s������
 *  dt  : 8bit�̑��M����f�[�^
 *  ans : 8bit�̎�M�����f�[�^
 */
extern char SDC_SPI_Transfer(char dt); // ���[�U�[����`����
#ifndef SDC_Transfer(p1)
#define SDC_Transfer(p1)  SDC_SPI_Transfer(p1)
#endif

/*
 * SDC_Select_CS(flag)(���[�U�[��`)
 * SDC��CS�M����LOW/HIGH���o�͂��鏈��
 *  flag : CS�M�����w�肷��
 */
extern void SDC_Select_CS(char flag); // ���[�U�[����`����
#ifndef Select_SDCCS(p1)
#define Select_SDCCS(p1)  SDC_Select_CS(p1)
#endif

/*
 * ans = SDC_Get_SDC()(���[�U�[��`)
 * SDC�����邩�ǂ�����Ԃ�����
 *  ans : SDC�������1�Ȃ����0��Ԃ�
 */
extern bit SDC_Get_SDC(); // ���[�U�[����`����
#ifndef Get_SDC()
#define Get_SDC()  SDC_Get_SDC()
#endif

/*
 * ans = SDC_Send_Command(cmd, prm)
 * SDC�փR�}���h�𑗂鏈��
 *  cmd : ����R�}���h���w��
 *  prm : �R�}���h�̃p�����[�^���w��
 *  ans : SDC����̕ԓ��f�[�^
 *   0xff - �J�[�h���Ȃ�
 *   0xfe - �J�[�h��busy
 *   0xfd - ���X�|���X�^�C���A�E�g
 */
unsigned char SDC_Send_Command(unsigned char cmd, unsigned long prm);
#ifndef Send_Command(p1, p2)
#define Send_Command(p1, p2)  SDC_Send_Command(p1, p2)
#endif

/*
 * ans = SDC_Send_RDCMD(sector)
 * �w��̃Z�N�^�ʒu����}���`�u���b�N�Ǎ��ރR�}���h�𑗂鏈��
 *  sector : �Z�N�^�ʒu���w��
 *  ans    : �����Ȃ�0�A���s�Ȃ�1
 */
bit SDC_Send_RDCMD(unsigned long sector);
#ifndef Send_RDCMD(p1)
#define Send_RDCMD(p1)  SDC_Send_RDCMD(p1)
#endif

/*
 * ans = SDC_Get_DataToken()
 * �f�[�^�g�[�N����҂���
 *  ans : �����Ȃ�0�A���s�Ȃ�1
 */
bit SDC_Get_DataToken();
#ifndef Get_DataToken()
#define Get_DataToken()  SDC_Get_DataToken()
#endif

/*
 * ans = SDC_Read_SBrock(sector, *buf)
 * �w��̃Z�N�^�ʒu����1�u���b�N�Ǎ��ޏ���
 *  sector : �Z�N�^�ʒu���w��
 *  *buf   : �ǂݍ��񂾃f�[�^���i�[����ϐ��̐擪�A�h���X
 *  ans    : �ǂݍ��ݐ����Ȃ�0�A���s�Ȃ�1
 */
bit SDC_Read_SBrock(unsigned long sector, void *buf);
#ifndef Read_SBrock(p1, p2)
#define Read_SBrock(p1, p2)  SDC_Read_SBrock(p1, p2)
#endif

/*
 * ans = SDC_Write_SBrock(sector, *buf)
 * �w��̃Z�N�^�ʒu�Ƀf�[�^��1�u���b�N�������ޏ���
 *  sector : �Z�N�^�ʒu���w��
 *  *buf   : �������ރf�[�^���i�[�����ϐ��̐擪�A�h���X
 *  ans    : �������ݐ����Ȃ�0�A���s�Ȃ�1
 */
bit SDC_Write_SBrock(unsigned long sector, const void *buf);
#ifndef Write_SBrock(p1, p2)
#define Write_SBrock(p1, p2)  SDC_Write_SBrock(p1, p2)
#endif

/*
 * ans = SDC_Init_SDC(*fat_info, *buf)
 * SDC(FAT16/32)/MMC(FAT12)�̏��������s������
 *  *buf : �����Ŏg���ꎞ�o�b�t�@(512byte)�̐擪�A�h���X
 *  ans  : �����Ȃ�0�A���s�Ȃ�1
 */
bit SDC_Init_SDC(SDC_FAT_INFO *fat_info);
#ifndef Init_SDC(p1)
#define Init_SDC(p1)  SDC_Init_SDC(p1)
#endif

/*
 * SDC_Get_Time(*date, *time, *a_tenth_seconds)(���[�U�[��`)
 * ���݂̎��Ԃ�Ԃ�����
 *  date            : ���݂̓��t(yyyyyyymmmmddddd)���i�[����ϐ��̃A�h���X���w��
 *  time            : ���ݎ���(hhhhhmmmmmmsssss)���i�[����ϐ��̃A�h���X���w��
 *  a_tenth_seconds : ���ݎ�����1/10�b���i�[����ϐ��̃A�h���X���w��
 */
extern void SDC_Get_Time(unsigned int *date, unsigned int *time,
		unsigned char *a_tenth_seconds); // ���[�U�[����`����
#ifndef Get_Time(p1, p2, p3)
#define Get_Time(p1, p2, p3)  SDC_Get_Time(p1, p2, p3)
#endif

/*
 * *ans = SDC_Remake_FileName(*pre_filename)
 * �t�@�C���̖��O�𐬌`���鏈��
 *  *pre_filename  : ���̃t�@�C���̖��O���i�[�����ϐ��̐擪�A�h���X
 *  *ans           : ���`�����t�@�C���̖��O���i�[�����ϐ�(11byte)�̐擪�A�h���X
 */
char *SDC_Remake_FileName(const char *pre_filename);
#ifndef Remake_FileName(p1)
#define Remake_FileName(p1)  SDC_Remake_FileName(p1)
#endif

/*
 * ans = SDC_Make_DirEntry(*filename, *fds, *buf)
 * FDS�^�ϐ��̏�����Ƀf�B���N�g���G���g���[����鏈��
 *  *filename : �t�@�C���̖��O(Remake_FileName�Ő��^�ς�)���i�[�����ϐ��̐擪�A�h���X
 *  *fds      : ��ƂȂ�FDS�^�ϐ��̃A�h���X
 *  *buf      : �����Ŏg���ꎞ�o�b�t�@(512byte)�̐擪�A�h���X
 *  ans       : �����Ȃ�0�A���s�Ȃ�1
 * memo -
 * �E*buf�ȊO�ŁA�����Ă����񂪂���΂��̏��͍X�V���Ȃ�(FDS�ϐ��̒��̂��̂ł�)
 * (��) filename = "" �̏ꍇ�Afilename�͍X�V���Ȃ�(���̏��̂܂�)
 *      (���̏��ɉ����Ȃ��Ă��G���[�͋N���Ȃ�)
 */
bit SDC_Make_DirEntry(const char *filename, const SDC_FDS *fds, void *buf);
#ifndef Make_DirEntry(p1, p2, p3)
#define Make_DirEntry(p1, p2, p3)  SDC_Make_DirEntry(p1, p2, p3)
#endif

/*
 * ans = SDC_Delete_DirEntry(index, *buf)
 * �f�B���N�g���G���g���[���������鏈��
 *  index : ��������f�B���N�g���G���g���[
 *  *buf  : �����Ŏg���ꎞ�o�b�t�@(512byte)�̐擪�A�h���X
 *  ans   : �����Ȃ�0�A���s�Ȃ�1
 */
bit SDC_Delete_DirEntry(unsigned int index, void *buf);
#ifndef Delete_DirEntry(p1, p2)
#define Delete_DirEntry(p1, p2)  SDC_Delete_DirEntry(p1, p2)
#endif

/*
 * ans = SDC_Search_EmptyDirEntry()
 * �󂫂̃f�B���N�g���G���g���[��T������
 *  ans : �󂫂̃f�B���N�g���G���g���[�̃C���f�b�N�X�A�����ꍇ�A�������̓G���[�̏ꍇ��0xffff
 */
unsigned int SDC_Search_EmptyDirEntry();
#ifndef Search_EmptyDirEntry()
#define Search_EmptyDirEntry()  SDC_Search_EmptyDirEntry()
#endif

/*
 * ans = SDC_Search_EmptyFAT(offset)
 * �󂫂�FAT��T������
 *  offset : offset�ȍ~��FAT�ԍ����猟��
 *  ans    : �󂫂�FAT�ԍ��A�������0x0fffffff�A�G���[��0
 */
unsigned long SDC_Search_EmptyFAT(unsigned long offset);
#ifndef Search_EmptyFAT(p1)
#define Search_EmptyFAT(p1)  SDC_Search_EmptyFAT(p1)
#endif

/*
 * ans = SDC_Get_NextFATNo(fatno)
 * ����FAT�ԍ����擾���鏈��
 *  fatno : ���݂�FAT�ԍ�
 *  ans   : ����FAT�ԍ��A�������0x0fffffff�A�G���[��0
 */
unsigned long SDC_Get_NextFATNo(unsigned long fatno);
#ifndef Get_NextFATNo(p1)
#define Get_NextFATNo(p1)  SDC_Get_NextFATNo(p1)
#endif

/*
 * ans = SDC_Set_NextFAT(pfatno, set_fatno, buf)
 * ����FAT�ԍ����Z�b�g���鏈��
 *  pfatno    : �Z�b�g����ꏊ��FAT�ԍ�
 *  set_fatno : pfatno�ɃZ�b�g����FAT�ԍ�
 *  ans       : �����Ȃ�0�A���s�Ȃ�1
 */
bit SDC_Set_NextFAT(unsigned long pfatno, unsigned long set_fatno, void *buf);
#ifndef Set_NextFAT(p1, p2, p3)
#define Set_NextFAT(p1, p2, p3)  SDC_Set_NextFAT(p1, p2, p3)
#endif

/*
 * ans = _SDC_Calculate_DirEntryP(index, fat_info)
 * �f�B���N�g���G���g���[�̃C���f�b�N�X����Z�N�^�ʒu���Z�o����
 *  index    : �f�B���N�g���G���g���[�̃C���f�b�N�X
 *  fat_info : �v�Z�Ɏg��FAT_INFO
 */
#define _SDC_Calculate_DirEntryP(index, fat_info)  \
		((fat_info).DirEntryStartP \
		+ ((index) / (SDC_SECTOR_BYTES / sizeof(SDC_DIR_ENTRY))))
#ifndef _Calculate_DirEntryP(p1, p2)
#define _Calculate_DirEntryP(p1, p2)  _SDC_Calculate_DirEntryP(p1, p2)
#endif

/*
 * ans = _SDC_Calculate_DEPByte(index)
 * �f�B���N�g���G���g���[�̃C���f�b�N�X����Z�N�^���̃o�C�g�ʒu���Z�o����
 *  index : �f�B���N�g���G���g���[�̃C���f�b�N�X
 */
#define _SDC_Calculate_DEPByte(index)  \
		((index) \
		% (SDC_SECTOR_BYTES / sizeof(SDC_DIR_ENTRY)) * sizeof(SDC_DIR_ENTRY))
#ifndef _Calculate_DEPByte(p1)
#define _Calculate_DEPByte(p1)  _SDC_Calculate_DEPByte(p1)
#endif

/*
 * ans = _SDC_Calculate_DataAreaP(fatno, file_seek_p, fat_info)
 * FAT�ԍ�����f�[�^�G���A�̃|�C���g���Z�N�^�ʒu�ŎZ�o����
 *  fatno       : �v�Z�Ɏg��FAT�ԍ�
 *  file_seek_p : �t�@�C���̃V�[�N�ʒu
 *  fat_info    : �v�Z�Ɏg��FAT_INFO
 */
#define _SDC_Calculate_DataAreaP(fatno, file_seek_p, fat_info)  \
		((fat_info).DataAreaStartP \
			+ (((fatno) - 2) * (fat_info).SectorsPerClusterCT) \
			+ (((file_seek_p) / SDC_SECTOR_BYTES) \
			% (fat_info).SectorsPerClusterCT))
#ifndef _Calculate_DataAreaP(p1, p2, p3)
#define _Calculate_DataAreaP(p1, p2, p3)  _SDC_Calculate_DataAreaP(p1, p2, p3)
#endif

/*
 * ans = _SDC_Calculate_FATAreaP(fatno, fat_info)
 * FAT�ԍ�����FAT�G���A�̃|�C���g���Z�N�^�ʒu�ŎZ�o����
 *  fatno  �@: �v�Z�Ɏg��FAT�ԍ�
 *  fat_info : �v�Z�Ɏg��FAT_INFO
 */
#define _SDC_Calculate_FATAreaP(fatno, fat_info)  \
		((fat_info).FATAreaStartP \
		+ ((fatno) / (SDC_SECTOR_BYTES / (fat_info).FATType)))
#ifndef _Calculate_FATAreaP(p1, p2)
#define _Calculate_FATAreaP(p1, p2)  _SDC_Calculate_FATAreaP(p1, p2)
#endif

/*
 * ans = _SDC_Calculate_FAPByte(fatno, fat_info) :
 * FAT�ԍ�����FAT�G���A�̃|�C���g�̃Z�N�^�ʒu�̒��̃o�C�g�ʒu���Z�o����
 *  fatno  �@: �v�Z�Ɏg��FAT�ԍ�
 *  fat_info : �v�Z�Ɏg��FAT_INFO
 */
#define _SDC_Calculate_FAPByte(fatno, fat_info)  \
		(((fatno) % (SDC_SECTOR_BYTES / (fat_info).FATType)) \
		* (fat_info).FATType)
#ifndef _Calculate_FAPByte(p1, p2)
#define _Calculate_FAPByte(p1, p2)  _SDC_Calculate_FAPByte(p1, p2)
#endif

#endif	/* SDCLIB_H */
