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
 * SPI通信でSDC間のデータ送信とデータ受信を行う処理
 */
extern char SDC_SPI_Transfer(char dt); // ユーザーが定義する

/*
 * SDC_Select_CS(flag)
 * SDCのCS信号のLOW/HIGHを出力する処理
 */
extern void SDC_Select_CS(char flag); // ユーザーが定義する

/*
 * ans = SDC_Get_SDC()
 * SDCがあるかどうかを返す処理
 */
extern bit SDC_Get_SDC(); // ユーザーが定義する

/*
 * ans = SDC_Send_Command(cmd, prm)
 * SDCへコマンドを送る処理
 */
unsigned char SDC_Send_Command(unsigned char cmd, unsigned long prm)
{
	SDC_EX_LONG p;
	unsigned char x, ans;

	SDC_Select_CS(0);
	if(cmd != SDC_CMD12) {
		// カードがBusy状態から抜け出すまで待つ
		for(x = 0; x < 255; x++)
			if(SDC_Get_SDC() == 0 || SDC_SPI_Transfer(0xff) == 0xff) break;
		for(x = 0; x < 255; x++)
			if(SDC_Get_SDC() == 0 || SDC_SPI_Transfer(0xff) == 0xff) break;
		for(x = 0; x < 255; x++)
			if(SDC_Get_SDC() == 0 || SDC_SPI_Transfer(0xff) == 0xff) break;
		for(x = 0; x < 255; x++)
			if(SDC_Get_SDC() == 0 || SDC_SPI_Transfer(0xff) == 0xff) break;
		if(x >= 255) return 0xfe; // カードはbusy
	}
	if(SDC_Get_SDC() == 0) return 0xff; // カードがない
	// コマンドの送信
	SDC_SPI_Transfer(cmd);
	// パラメータの送信
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
	// 返答を待つ
	for(x = 0; x < 8; x++) {
		if(SDC_Get_SDC() == 0) return 0xff; // カードがない
		if(((ans = SDC_SPI_Transfer(0xff)) & 0x80) == 0) return ans;
	}
	return 0xfd; // レスポンスタイムアウト
}

/*
 * ans = SDC_Send_RDCMD(sector)
 * 指定のセクタ位置からマルチブロック読込むコマンドを送る処理
 */
bit SDC_Send_RDCMD(unsigned long sector)
{
	if(FAT->FATType == 2) sector <<= 9; // FAT12/16
	// マルチブロックリードコマンド送信
	if(SDC_Send_Command(SDC_CMD18, sector) == 0) return 0;
	SDC_Select_CS(1);
	return 1;
}

/*
 * ans = SDC_Get_DataToken()
 * データトークンを待つ処理
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
 * 指定のセクタ位置から1ブロック読込む処理
 */
bit SDC_Read_SBrock(unsigned long sector, void *buf)
{
	register char *b;
	unsigned int i;

	b = buf;
	if(FAT->FATType == 2) sector <<= 9; // FAT12/16
	if(SDC_Send_Command(SDC_CMD17, sector) == 0) { // シングルリードコマンド送信
		// データトークンの受信
		if(SDC_Get_DataToken() == 0) {
			// データを読み取る
			for(i = 0; i < SDC_SECTOR_BYTES; i++) *b++ = SDC_SPI_Transfer(0xff);
			SDC_SPI_Transfer(0xff); // CRCの部分を受信
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
 * 指定のセクタ位置にデータを1ブロック書き込む処理
 */
bit SDC_Write_SBrock(unsigned long sector, const void *buf)
{
	register const char *b;
	unsigned int i;

	b = buf;
	if(FAT->FATType == 2) sector <<= 9; // FAT32でない
	if(SDC_Send_Command(SDC_CMD24, sector) == 0) { // シングルライトコマンド送信
		SDC_SPI_Transfer(0xff);
		SDC_SPI_Transfer(0xfe); // データトークン
		for(i = 0; i < SDC_SECTOR_BYTES; i++) SDC_SPI_Transfer(*b++);
		SDC_SPI_Transfer(0xff); // CRCの部分を送信
		SDC_SPI_Transfer(0xff);
		// 正常に書き込めたかチェックする
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
 * SDC(FAT12/16/32)/MMC(FAT12)の初期化を行う処理
 */
bit SDC_Init_SDC(SDC_FAT_INFO *fat_info)
{
	SDC_EX_LONG prm;
	unsigned int i;

	FAT = fat_info;
	// カードをコマンド待ち状態にする
	SDC_Select_CS(1);
	for(i = 0; i < 10; i++) SDC_SPI_Transfer(0xff);
	if(SDC_Send_Command(SDC_CMD0, 0) == 1) { // リセットコマンドを送信
		// バージョンチェックコマンドを送信
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
			// 初期化コマンドを送信
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
								FAT->FATType = 2; // SDHCでない
						}
					} else FAT->FATType = 0;
				}
			} else FAT->FATType = 0;
		}
		// ブロックサイズの設定
		if(FAT->FATType && SDC_Send_Command(SDC_CMD16, SDC_SECTOR_BYTES) != 0)
			FAT->FATType = 0;
	} else FAT->FATType = 0;
	SDC_Select_CS(1);
	if(FAT->FATType == 1) FAT->FATType = 2;

	// FATファイルシステムのパラメータを読み込む
	if(FAT->FATType) {
		// BPB(ブートセクタ)の位置を調べる
		if(SDC_Send_Command(SDC_CMD17, 0) || SDC_Get_DataToken()) FAT->FATType = 0;
		else {
			prm.l = 0xffffffff;
			for(i = 0; i < 14; i++) SDC_SPI_Transfer(0xff);
			for(i = 0; i < 2; i++) prm.c[i] = SDC_SPI_Transfer(0xff);
			// ReservedSectorCountは0でないか
			if(prm.i[0]) {
				for(i = 0; i < 5; i++) SDC_SPI_Transfer(0xff);
				if((prm.c[2] = SDC_SPI_Transfer(0xff)) == 0xf0 || prm.c[2] >= 0xf8) {
					for(i = 0; i < 6; i++) SDC_SPI_Transfer(0xff);
					// HidddenSectorsは0か
					if((SDC_SPI_Transfer(0xff) | SDC_SPI_Transfer(0xff)
							| SDC_SPI_Transfer(0xff) | SDC_SPI_Transfer(0xff)) == 0) {
						for(i = 0; i < 480; i++) SDC_SPI_Transfer(0xff);
						SDC_SPI_Transfer(0xff); // CRC
						SDC_SPI_Transfer(0xff);
						// FAT[0]を読み出す
						if((SDC_Send_Command(SDC_CMD17,
								prm.i[0] << (FAT->FATType == 2 ? 9 : 0)) || SDC_Get_DataToken())
								&& (SDC_Send_Command(SDC_CMD17,
								prm.i[0] << (FAT->FATType == 2 ? 9 : 0)) || SDC_Get_DataToken()))
							FAT->FATType = 0;
						else {
							if(SDC_SPI_Transfer(0xff) == prm.c[2])
								prm.l = 0; // MBRがBPBだ
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
			if(prm.l != 0) { // MBRはBPBでない
				// BPBまでのオフセット値を取り出す
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
					if(prm.l == 0) FAT->FATType = 0; // 取り出しに失敗
				}
				SDC_Select_CS(1);
			}
			if(FAT->FATType) {
				// BPBを読み出す
				if((SDC_Send_Command(SDC_CMD17,
						prm.l << (FAT->FATType == 2 ? 9 : 0)) || SDC_Get_DataToken())
						&& (SDC_Send_Command(SDC_CMD17,
						prm.l << (FAT->FATType == 2 ? 9 : 0)) || SDC_Get_DataToken()))
					FAT->FATType = 0;
				else {
					for(i = 0; i < 13; i++) SDC_SPI_Transfer(0xff);
					// 1クラスターあたりのセクタ数(SectorsPerCluster)
					FAT->SectorsPerClusterCT = SDC_SPI_Transfer(0xff);
					// FAT領域の開始セクタ位置(prm.l + ReservedSectorCount)
					FAT->FATAreaStartP = prm.l;
					for(i = 0; i < 2; i++) prm.c[i] = SDC_SPI_Transfer(0xff);
					FAT->FATAreaStartP += prm.i[0];
					// FATCountをDirEntryStartPに保持
					FAT->DirEntryStartP = SDC_SPI_Transfer(0xff);
					// ディレクトリエントリ領域の開始セクタ位置
					// ((sizeof(SDC_DIR_ENTRY) * RootDirEntryCount
					// + SDC_SECTOR_BYTES - 1) / SDC_SECTOR_BYTES)
					for(i = 0; i < 2; i++) prm.c[i] = SDC_SPI_Transfer(0xff);
					FAT->DirEntrySectorCT = (sizeof (SDC_DIR_ENTRY) * prm.i[0]
							+ SDC_SECTOR_BYTES - 1) / SDC_SECTOR_BYTES;
					for(i = 0; i < 3; i++) SDC_SPI_Transfer(0xff);
					// 一組のFAT領域がしめるセクタ数
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
					// ディレクトリエントリー領域の開始セクタ位置
					// (FATCount * SectorsPerFATCT + FATAreaStartP)
					FAT->DirEntryStartP *= FAT->SectorsPerFATCT;
					FAT->DirEntryStartP += FAT->FATAreaStartP;
					// データ領域の開始セクタ位置
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
 * 現在の時間を返す処理
 */
extern void SDC_Get_Time(unsigned int *date, unsigned int *time,
		unsigned char *a_tenth_seconds); // ユーザーが定義する

/*
 * *ans = SDC_Remake_FileName(*pre_filename)
 * ファイルの名前を整形する処理
 */
char *SDC_Remake_FileName(const char *pre_filename)
{
	static char filename[11];
	unsigned char c;

	// ファイルの名前の部分の整形('.'の前まで書き写し、残りは0x20で埋める)
	for(c = 0; c < 8; c++) {
		if(*pre_filename == '.' || *pre_filename == '\0') filename[c] = 0x20;
		else filename[c] = *pre_filename++;
	}
	// ファイルの拡張子にカーソルを合わせる
	while(*pre_filename != '.' && *pre_filename != '\0') pre_filename++;
	if(*pre_filename == '.') pre_filename++;
	// 拡張子の整形(拡張子を書き写し、残りは0x20で埋める)
	for(c = 8; c < 11; c++) {
		if(*pre_filename == '\0') filename[c] = 0x20;
		else filename[c] = *pre_filename++;
	}
	return filename;
}

/*
 * ans = SDC_Make_DirEntry(*filename, *fds, *buf)
 * FDS型変数の情報を基にディレクトリエントリーを作る処理
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
	// ファイル名を設定する(filenameに何も入っていなければ何もしない)
	if(filename[0]) memcpy(inf->FileName, filename, 11);
	// ファイルのサイズを設定する
	inf->FileSize = fds->FileSize;
	//データ格納先のFAT番号を設定(空ならば何もしない)
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
	// ファイルの属性を設定する
	inf->Attributes = fds->Attributes;
	// ファイルの作成日時を設定する(filenameに何も入っていなければ何もしない)
	if(filename[0]) {
		inf->CreationTimeTenths = atensec;
		inf->CreationTime = time;
		inf->CreationDate = date;
	}
	// 最終書き込み時間を設定する
	inf->LastWriteTime = time;
	// ファイルの作成日を設定する
	// 最終書き込み日を設定する
	inf->LastWriteDate = date;
	// アクセス日を設定する
	inf->LastAccessDate = date;

	inf->ReservedNT = 0;

	if(SDC_Write_SBrock(p, buf) && SDC_Write_SBrock(p, buf)) return 1;
	return 0;
}

/*
 * ans = SDC_Delete_DirEntry(index, *buf)
 * ディレクトリエントリーを消去する処理
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
 * 空きのディレクトリエントリーを探す処理
 */
unsigned int SDC_Search_EmptyDirEntry()
{
	unsigned int i;
	unsigned char c, x;

	// リード開始
	if(SDC_Send_RDCMD(FAT->DirEntryStartP)) return 0xffff; // エラー
	for(i = 0; i < FAT->DirEntrySectorCT; i++) {
		if(SDC_Get_SDC() == 0) return 0xffff; // SDCがない
		if(SDC_Get_DataToken()) {
			if(SDC_Send_RDCMD(FAT->DirEntryStartP + i)
					&& SDC_Send_RDCMD(FAT->DirEntryStartP + i)) return 0xffff; // エラー
		}
		for(c = 0; c < (SDC_SECTOR_BYTES / sizeof (SDC_DIR_ENTRY)); c++) {
			x = SDC_SPI_Transfer(0xff);
			// 消去されたエントリ、または空のエントリなら、その位置を記録する
			if(x == 0xe5 || x == 0x00) {
				SDC_Send_Command(SDC_CMD12, 0);
				SDC_Select_CS(1);
				return (i * (SDC_SECTOR_BYTES / sizeof (SDC_DIR_ENTRY))) +c;
			}
			for(x = 0; x < sizeof (SDC_DIR_ENTRY) - 1; x++) SDC_SPI_Transfer(0xff);
		}
		SDC_SPI_Transfer(0xff); // CRCの部分を受信
		SDC_SPI_Transfer(0xff);
	}
	SDC_Send_Command(SDC_CMD12, 0);
	SDC_Select_CS(1);
	return 0xffff; // 無かった
}

/*
 * ans = SDC_Search_EmptyFAT(offset)
 * 空きのFATを探す処理
 */
unsigned long SDC_Search_EmptyFAT(unsigned long offset)
{
	unsigned long l;
	unsigned int i;

	// 検索場所をoffset以降に設定する(offsetが2未満のときは最初から検索、
	// それ以外のときは検索場所にoffsetを含まない)
	if(offset < 2) offset = 2;
	else offset++;
	l = offset / (SDC_SECTOR_BYTES / FAT->FATType); // セクタ位置(_Calculate...はFATAreaStartPを足すので使えない)
	offset = _SDC_Calculate_FAPByte(offset, *FAT); // セクタの中のバイト位置
	// リード開始
	if(SDC_Send_RDCMD(FAT->FATAreaStartP + l)) return 0;
	for(; l < FAT->SectorsPerFATCT; l++) {
		if(SDC_Get_SDC() == 0) return 0; // SDCがない
		if(SDC_Get_DataToken()) {
			if(SDC_Send_RDCMD(FAT->FATAreaStartP + l)
					&& SDC_Send_RDCMD(FAT->FATAreaStartP + l)) return 0; // エラー
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
				return (l * SDC_SECTOR_BYTES + offset) / FAT->FATType; // FAT番号を算出
			}
		}
		SDC_SPI_Transfer(0xff); // CRCの部分を受信
		SDC_SPI_Transfer(0xff);
		offset = 0;
	}
	SDC_Send_Command(SDC_CMD12, 0);
	SDC_Select_CS(1);
	return 0x0fffffff; // 無かった
}

/*
 * ans = SDC_Get_NextFATNo(fatno)
 * 次のFAT番号を取得する処理
 */
unsigned long SDC_Get_NextFATNo(unsigned long fatno)
{
	SDC_EX_LONG ans;
	unsigned int i;

	// リード開始
	if(SDC_Send_RDCMD(_SDC_Calculate_FATAreaP(fatno, *FAT))
			&& SDC_Get_DataToken()) return 0; // エラー
	else if(SDC_Send_RDCMD(_SDC_Calculate_FATAreaP(fatno, *FAT))
			&& SDC_Get_DataToken()) return 0; // エラー
	fatno = _SDC_Calculate_FAPByte(fatno, *FAT);
	for(i = 0; i < fatno; i++) SDC_SPI_Transfer(0xff);
	ans.l = 0;
	// FAT番号を記録
	for(i = 0; i < FAT->FATType; i++) ans.c[i] = SDC_SPI_Transfer(0xff);
	SDC_Send_Command(SDC_CMD12, 0);
	SDC_Select_CS(1);
	if(FAT->FATType == 2 && ans.i[0] == 0xffff) ans.l = 0x0fffffff;
	if(FAT->FATType == 4) ans.c[3] &= 0x0f;
	return ans.l;
}

/*
 * ans = SDC_Set_NextFAT(pfatno, set_fatno, *buf)
 * 次のFAT番号をセットする処理
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
