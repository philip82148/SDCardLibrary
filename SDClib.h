/* 
 * File: SDClib.h
 * SDC/MMCのアクセスを行う関数ライブラリ
 * 
 * Fanctions -
 * ans = SDC_Send_Command(cmd, prm) :
 *  SDCへコマンドを送る処理
 * ans = SDC_Send_RDCMD(sector) :
 *  SDCへシングルリードコマンドを送る処理
 * ans = SDC_Get_DataToken() :
 *  データトークンを待つ処理
 * ans = SDC_Read_SBrock(sector, *buf) :
 *  SDCからシングルブロック読み込む処理
 * ans = SDC_Write_SBrock(sector, *buf) :
 *  SDCへシングルブロック書き込む処理
 * ans = SDC_Init_SDC(*fat_info) :
 *  SDCの初期化を行う処理
 * *ans = SDC_Remake_FileName(*pre_filename) :
 *  ファイルの名前を整形する処理
 * ans = SDC_Make_DirEntry(*filename, *fds, *buf) :
 *  ディレクトリエントリーをつくる処理
 * ans = SDC_Delete_DirEntry(index, *buf) :
 *  ディレクトリエントリーを消去する処理
 * ans = SDC_Search_EmptyDirEntry() :
 *  空のディレクトリエントリーを探す処理
 * ans = SDC_Search_EmptyFAT(offset) :
 *  空きのFATを探す処理
 * ans = SDC_Get_NextFATNo(fatno) :
 *  次のFAT番号を得る処理
 * ans = SDC_Set_NextFAT(pfatno, set_fatno, *buf) :
 *  次のFAT番号をセットする処理
 * 
 * ans = _SDC_Calculate_DirEntryP(index, fat_info) :
 *  ディレクトリエントリーのインデックスからセクタ位置を算出する
 * ans = _SDC_Calculate_DEPByte(index) :
 *  ディレクトリエントリーのインデックスからセクタ内のバイト位置を算出する
 * ans = _SDC_Calculate_DataAreaP(fatno, file_seek_p, fat_info) :
 *  FAT番号からデータエリアのポイントをセクタ位置で算出する
 * ans = _SDC_Calculate_FATAreaP(fatno, fat_info) :
 *  FAT番号からFATエリアのポイントをセクタ位置で算出する
 * ans = _SDC_Calculate_FAPByte(fatno, fat_info) :
 *  FAT番号からFATエリアのポイントのセクタ位置の中のバイト位置を算出する
 * 
 * Define -
 * ans = SDC_SPI_Transfer(dt) :
 *  SPI通信でSDC間のデータ送受信を行う関数
 * SDC_Select_CS(flag) :
 *  SDCのCS信号を出力する関数
 * ans = SDC_Get_SDC() :
 *  SDCの有り/無しを返す関数
 * SDC_Get_Time(*date, *time, *a_tenth_seconds) :
 *  現在の時間を返す関数を指定
 * 
 * memo -
 * ・これらの関数は標準またはmini、microのSD(FAT16)/SDHC(FAT32)
 * またはMMC(FAT12)で利用可能
 * 
 * ============================================================================
 * Version Date       TotalSize(Program/SRAM)(目安) Note
 * ============================================================================
 *    1.00 2015/09/27  15B4hbyte/0062byte(18F25K80) 実際はバッファ用にもう512byte
 *                                                  (読み込みのみの場合は不要)と
 *                                                  とFAT_INFO型変数がひとつ必要
 * ============================================================================
 * Microchip MPLAB XC8 C Compiler (Free Mode) V1.35
 * 
 */
#ifndef SDCLIB_H
#define	SDCLIB_H

////////////////////////////////////////////////////////////////////////////////
//ＳＤＣアクセス関連

#define SDC_CMD0   (0x00 | 0x40)               // カードへのリセットコマンド
#define SDC_CMD1   (0x01 | 0x40)               // MMCへの初期化コマンド
#define SDC_CMD8   (0x08 | 0x40)               // 動作電圧の確認とSDCのバージョンチェック
#define SDC_CMD12  (0x0C | 0x40)               // データ読込みを停止させるコマンド
#define SDC_CMD13  (0x0D | 0x40)               // 書込みの状態を問い合わせるコマンド
#define SDC_CMD16  (0x10 | 0x40)               // ブロックサイズの初期値設定コマンド
#define SDC_CMD17  (0x11 | 0x40)               // シングルブロック読込み要求コマンド
#define SDC_CMD18  (0x12 | 0x40)               // マルチブロック読込み要求コマンド
#define SDC_CMD24  (0x18 | 0x40)               // シングルブロック書込み要求コマンド
#define SDC_ACMD41 (0x29 | 0x40)               // SDCへの初期化コマンド
#define SDC_CMD55  (0x37 | 0x40)               // ACMD41/ACMD23とセットで使用するコマンド
#define SDC_CMD58  (0x3A | 0x40)               // OCRの読出しコマンド

////////////////////////////////////////////////////////////////////////////////
// FAT関連

#define SDC_SECTOR_BYTES  512                  // 1セクタのバイト数

/* FATファイルシステムのパラメータ */
typedef struct {
	unsigned char FATType;                     // FATの種類
	unsigned char SectorsPerClusterCT;         // 1クラスタあたりのセクタ数
	unsigned long FATAreaStartP;               // FAT領域の開始セクタ位置
	unsigned long SectorsPerFATCT;             // 1組のFAT領域が占めるセクタ数
	unsigned long DirEntryStartP;              // ディレクトリエントリーの開始セクタ位置
	unsigned int DirEntrySectorCT;             // ディレクトリエントリーのセクタ個数
	unsigned long DataAreaStartP;              // データ領域の開始セクタ位置
} SDC_FAT_INFO;
#ifndef _FAT_INFO_
#define _FAT_INFO_
typedef SDC_FAT_INFO FAT_INFO;
#endif

/* FATファイルシステム(FAT12/16/32)のパラメータ構造体(512バイト) */
struct SDC_FAT_HEADER {
	unsigned char jump[3];                     // ブート用のジャンプコード
	unsigned char oemId[8];                    // フォーマット時のボリューム名
	unsigned int  BytesPerSector;              // 1セクタあたりのバイト数、通常は512バイト
	unsigned char SectorsPerCluster;           // 1クラスタあたりのセクタ数
	unsigned int  ReservedSectorCount;         // ブートセクタ以降の予約領域のセクタ数
	unsigned char FATCount;                    // FATの組数(バックアップFAT数)、通常は２組
	unsigned int  RootDirEntryCount;           // ディレクトリの作成可能個数、通常は512個
	unsigned int  TotalSectors16;              // 全領域のセクター総数(FAT12/FAT16用)
	unsigned char MediaType;                   // FAT領域の先頭の値、通常は0xF8
	unsigned int  SectorsPerFAT16;             // 1組のFAT領域が占めるセクタ数(FAT12/FAT16用)
	unsigned int  SectorsPerTrack;             // 1トラックあたりのセクタ数
	unsigned int  HeadCount;                   // ヘッド数
	unsigned long HidddenSectors;              // 隠蔽されたセクタ数
	unsigned long TotalSectors32;              // 全領域のセクター総数(FAT32用)
	unsigned long SectorsPerFAT32;             // 1組のFAT領域が占めるセクタ数(FAT32用)
	unsigned int  FlagsFAT32;                  // FATの有効無効等の情報フラグ
	unsigned int  VersionFAT32;                //
	unsigned long RootDirClusterFAT32;         // ディレクトリのスタートクラスタ(FAT32用)
	unsigned char Dumy[6];                     //
	unsigned char FileSystemType[8];           // FATの種類("FAT12/16")(FAT32は20バイト下に有る)
	unsigned char BootCode[448];               // ブートコード領域
	unsigned char BootSectorSig0;              // 0x55
	unsigned char BootSectorSig1;              // 0xaa
} ;
#ifndef FAT_HEADER
#define FAT_HEADER  SDC_FAT_HEADER
#endif

/* ディレクトリエントリーの構造体(32バイト) */
typedef struct {
	         char FileName[11];                // ファイル名(8)+拡張子(3)
	unsigned char Attributes;                  // ファイルの属性
	unsigned char ReservedNT;                  // Windows NT 用 予約領域
	unsigned char CreationTimeTenths;          // ファイル作成時間の1/10秒単位をあらわす
	unsigned int  CreationTime;                // ファイルの作成時間(hhhhhmmmmmmsssss)
	unsigned int  CreationDate;                // ファイルの作成日(yyyyyyymmmmddddd)
	unsigned int  LastAccessDate;              // 最終のアクセス日
	unsigned int  FirstClusterHigh;            // データ格納先のFAT番号上位２バイト
	unsigned int  LastWriteTime;               // 最終のファイル書込み時間
	unsigned int  LastWriteDate;               // 最終のファイル書込み日
	unsigned int  FirstClusterLow;             // データ格納先のFAT番号下位２バイト
	unsigned long FileSize;                    // ファイルのサイズ
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

/* ファイル情報 */
typedef struct {
	unsigned int  DirEntryIndex;               // ディレクトリエントリーの検索した場所の位置
	unsigned long FileSize;                    // ファイルのサイズ
	SDC_EX_LONG   FirstFATNo;                  // データ格納先のFAT番号(なければ0x0fffffffにする)
	unsigned char Attributes;                  // ファイルの属性
} SDC_FDS;
#ifndef _FDS_
#define _FDS_
typedef SDC_FDS FDS;
#endif

// DIR_ENTRYとFDSのAttributes用のフラグ
#define SDC_READ_ONLY_FILE  0x01               // 書き込み禁止
#define SDC_HIDDEN_FILE     0x02               // 隠し
#define SDC_SYSTEM_FILE     0x04               // システム
#define SDC_DIRECTORY_FILE  0x10               // ディレクトリ
#define SDC_ARCHIVE_FILE    0x20               // アーカイブ

////////////////////////////////////////////////////////////////////////////////

/*
 * ans = SDC_SPI_Transfer(dt)(ユーザー定義)
 * SPI通信でSDC間のデータ送信とデータ受信を行う処理
 *  dt  : 8bitの送信するデータ
 *  ans : 8bitの受信したデータ
 */
extern char SDC_SPI_Transfer(char dt); // ユーザーが定義する
#ifndef SDC_Transfer(p1)
#define SDC_Transfer(p1)  SDC_SPI_Transfer(p1)
#endif

/*
 * SDC_Select_CS(flag)(ユーザー定義)
 * SDCのCS信号のLOW/HIGHを出力する処理
 *  flag : CS信号を指定する
 */
extern void SDC_Select_CS(char flag); // ユーザーが定義する
#ifndef Select_SDCCS(p1)
#define Select_SDCCS(p1)  SDC_Select_CS(p1)
#endif

/*
 * ans = SDC_Get_SDC()(ユーザー定義)
 * SDCがあるかどうかを返す処理
 *  ans : SDCがあれば1なければ0を返す
 */
extern bit SDC_Get_SDC(); // ユーザーが定義する
#ifndef Get_SDC()
#define Get_SDC()  SDC_Get_SDC()
#endif

/*
 * ans = SDC_Send_Command(cmd, prm)
 * SDCへコマンドを送る処理
 *  cmd : 送るコマンドを指定
 *  prm : コマンドのパラメータを指定
 *  ans : SDCからの返答データ
 *   0xff - カードがない
 *   0xfe - カードはbusy
 *   0xfd - レスポンスタイムアウト
 */
unsigned char SDC_Send_Command(unsigned char cmd, unsigned long prm);
#ifndef Send_Command(p1, p2)
#define Send_Command(p1, p2)  SDC_Send_Command(p1, p2)
#endif

/*
 * ans = SDC_Send_RDCMD(sector)
 * 指定のセクタ位置からマルチブロック読込むコマンドを送る処理
 *  sector : セクタ位置を指定
 *  ans    : 成功なら0、失敗なら1
 */
bit SDC_Send_RDCMD(unsigned long sector);
#ifndef Send_RDCMD(p1)
#define Send_RDCMD(p1)  SDC_Send_RDCMD(p1)
#endif

/*
 * ans = SDC_Get_DataToken()
 * データトークンを待つ処理
 *  ans : 成功なら0、失敗なら1
 */
bit SDC_Get_DataToken();
#ifndef Get_DataToken()
#define Get_DataToken()  SDC_Get_DataToken()
#endif

/*
 * ans = SDC_Read_SBrock(sector, *buf)
 * 指定のセクタ位置から1ブロック読込む処理
 *  sector : セクタ位置を指定
 *  *buf   : 読み込んだデータを格納する変数の先頭アドレス
 *  ans    : 読み込み成功なら0、失敗なら1
 */
bit SDC_Read_SBrock(unsigned long sector, void *buf);
#ifndef Read_SBrock(p1, p2)
#define Read_SBrock(p1, p2)  SDC_Read_SBrock(p1, p2)
#endif

/*
 * ans = SDC_Write_SBrock(sector, *buf)
 * 指定のセクタ位置にデータを1ブロック書き込む処理
 *  sector : セクタ位置を指定
 *  *buf   : 書き込むデータを格納した変数の先頭アドレス
 *  ans    : 書き込み成功なら0、失敗なら1
 */
bit SDC_Write_SBrock(unsigned long sector, const void *buf);
#ifndef Write_SBrock(p1, p2)
#define Write_SBrock(p1, p2)  SDC_Write_SBrock(p1, p2)
#endif

/*
 * ans = SDC_Init_SDC(*fat_info, *buf)
 * SDC(FAT16/32)/MMC(FAT12)の初期化を行う処理
 *  *buf : 内部で使う一時バッファ(512byte)の先頭アドレス
 *  ans  : 成功なら0、失敗なら1
 */
bit SDC_Init_SDC(SDC_FAT_INFO *fat_info);
#ifndef Init_SDC(p1)
#define Init_SDC(p1)  SDC_Init_SDC(p1)
#endif

/*
 * SDC_Get_Time(*date, *time, *a_tenth_seconds)(ユーザー定義)
 * 現在の時間を返す処理
 *  date            : 現在の日付(yyyyyyymmmmddddd)を格納する変数のアドレスを指定
 *  time            : 現在時刻(hhhhhmmmmmmsssss)を格納する変数のアドレスを指定
 *  a_tenth_seconds : 現在時刻の1/10秒を格納する変数のアドレスを指定
 */
extern void SDC_Get_Time(unsigned int *date, unsigned int *time,
		unsigned char *a_tenth_seconds); // ユーザーが定義する
#ifndef Get_Time(p1, p2, p3)
#define Get_Time(p1, p2, p3)  SDC_Get_Time(p1, p2, p3)
#endif

/*
 * *ans = SDC_Remake_FileName(*pre_filename)
 * ファイルの名前を成形する処理
 *  *pre_filename  : 元のファイルの名前を格納した変数の先頭アドレス
 *  *ans           : 成形したファイルの名前を格納した変数(11byte)の先頭アドレス
 */
char *SDC_Remake_FileName(const char *pre_filename);
#ifndef Remake_FileName(p1)
#define Remake_FileName(p1)  SDC_Remake_FileName(p1)
#endif

/*
 * ans = SDC_Make_DirEntry(*filename, *fds, *buf)
 * FDS型変数の情報を基にディレクトリエントリーを作る処理
 *  *filename : ファイルの名前(Remake_FileNameで成型済み)を格納した変数の先頭アドレス
 *  *fds      : 基となるFDS型変数のアドレス
 *  *buf      : 内部で使う一時バッファ(512byte)の先頭アドレス
 *  ans       : 成功なら0、失敗なら1
 * memo -
 * ・*buf以外で、かけている情報があればその情報は更新しない(FDS変数の中のものでも)
 * (例) filename = "" の場合、filenameは更新しない(元の情報のまま)
 *      (元の情報に何もなくてもエラーは起きない)
 */
bit SDC_Make_DirEntry(const char *filename, const SDC_FDS *fds, void *buf);
#ifndef Make_DirEntry(p1, p2, p3)
#define Make_DirEntry(p1, p2, p3)  SDC_Make_DirEntry(p1, p2, p3)
#endif

/*
 * ans = SDC_Delete_DirEntry(index, *buf)
 * ディレクトリエントリーを消去する処理
 *  index : 消去するディレクトリエントリー
 *  *buf  : 内部で使う一時バッファ(512byte)の先頭アドレス
 *  ans   : 成功なら0、失敗なら1
 */
bit SDC_Delete_DirEntry(unsigned int index, void *buf);
#ifndef Delete_DirEntry(p1, p2)
#define Delete_DirEntry(p1, p2)  SDC_Delete_DirEntry(p1, p2)
#endif

/*
 * ans = SDC_Search_EmptyDirEntry()
 * 空きのディレクトリエントリーを探す処理
 *  ans : 空きのディレクトリエントリーのインデックス、無い場合、もしくはエラーの場合は0xffff
 */
unsigned int SDC_Search_EmptyDirEntry();
#ifndef Search_EmptyDirEntry()
#define Search_EmptyDirEntry()  SDC_Search_EmptyDirEntry()
#endif

/*
 * ans = SDC_Search_EmptyFAT(offset)
 * 空きのFATを探す処理
 *  offset : offset以降のFAT番号から検索
 *  ans    : 空きのFAT番号、無ければ0x0fffffff、エラーは0
 */
unsigned long SDC_Search_EmptyFAT(unsigned long offset);
#ifndef Search_EmptyFAT(p1)
#define Search_EmptyFAT(p1)  SDC_Search_EmptyFAT(p1)
#endif

/*
 * ans = SDC_Get_NextFATNo(fatno)
 * 次のFAT番号を取得する処理
 *  fatno : 現在のFAT番号
 *  ans   : 次のFAT番号、無ければ0x0fffffff、エラーは0
 */
unsigned long SDC_Get_NextFATNo(unsigned long fatno);
#ifndef Get_NextFATNo(p1)
#define Get_NextFATNo(p1)  SDC_Get_NextFATNo(p1)
#endif

/*
 * ans = SDC_Set_NextFAT(pfatno, set_fatno, buf)
 * 次のFAT番号をセットする処理
 *  pfatno    : セットする場所のFAT番号
 *  set_fatno : pfatnoにセットするFAT番号
 *  ans       : 成功なら0、失敗なら1
 */
bit SDC_Set_NextFAT(unsigned long pfatno, unsigned long set_fatno, void *buf);
#ifndef Set_NextFAT(p1, p2, p3)
#define Set_NextFAT(p1, p2, p3)  SDC_Set_NextFAT(p1, p2, p3)
#endif

/*
 * ans = _SDC_Calculate_DirEntryP(index, fat_info)
 * ディレクトリエントリーのインデックスからセクタ位置を算出する
 *  index    : ディレクトリエントリーのインデックス
 *  fat_info : 計算に使うFAT_INFO
 */
#define _SDC_Calculate_DirEntryP(index, fat_info)  \
		((fat_info).DirEntryStartP \
		+ ((index) / (SDC_SECTOR_BYTES / sizeof(SDC_DIR_ENTRY))))
#ifndef _Calculate_DirEntryP(p1, p2)
#define _Calculate_DirEntryP(p1, p2)  _SDC_Calculate_DirEntryP(p1, p2)
#endif

/*
 * ans = _SDC_Calculate_DEPByte(index)
 * ディレクトリエントリーのインデックスからセクタ内のバイト位置を算出する
 *  index : ディレクトリエントリーのインデックス
 */
#define _SDC_Calculate_DEPByte(index)  \
		((index) \
		% (SDC_SECTOR_BYTES / sizeof(SDC_DIR_ENTRY)) * sizeof(SDC_DIR_ENTRY))
#ifndef _Calculate_DEPByte(p1)
#define _Calculate_DEPByte(p1)  _SDC_Calculate_DEPByte(p1)
#endif

/*
 * ans = _SDC_Calculate_DataAreaP(fatno, file_seek_p, fat_info)
 * FAT番号からデータエリアのポイントをセクタ位置で算出する
 *  fatno       : 計算に使うFAT番号
 *  file_seek_p : ファイルのシーク位置
 *  fat_info    : 計算に使うFAT_INFO
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
 * FAT番号からFATエリアのポイントをセクタ位置で算出する
 *  fatno  　: 計算に使うFAT番号
 *  fat_info : 計算に使うFAT_INFO
 */
#define _SDC_Calculate_FATAreaP(fatno, fat_info)  \
		((fat_info).FATAreaStartP \
		+ ((fatno) / (SDC_SECTOR_BYTES / (fat_info).FATType)))
#ifndef _Calculate_FATAreaP(p1, p2)
#define _Calculate_FATAreaP(p1, p2)  _SDC_Calculate_FATAreaP(p1, p2)
#endif

/*
 * ans = _SDC_Calculate_FAPByte(fatno, fat_info) :
 * FAT番号からFATエリアのポイントのセクタ位置の中のバイト位置を算出する
 *  fatno  　: 計算に使うFAT番号
 *  fat_info : 計算に使うFAT_INFO
 */
#define _SDC_Calculate_FAPByte(fatno, fat_info)  \
		(((fatno) % (SDC_SECTOR_BYTES / (fat_info).FATType)) \
		* (fat_info).FATType)
#ifndef _Calculate_FAPByte(p1, p2)
#define _Calculate_FAPByte(p1, p2)  _SDC_Calculate_FAPByte(p1, p2)
#endif

#endif	/* SDCLIB_H */
