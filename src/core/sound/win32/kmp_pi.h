#ifndef KMP_PI_H
#define KMP_PI_H

/*

     KbMedia Player Plugin ヘッダファイル（テスト版）

*/

#define KMPMODULE_VERSION 100 //KMPMODULE のバージョン
#define KMP_GETMODULE     kmp_GetTestModule  //まだテスト版...
#define SZ_KMP_GETMODULE "kmp_GetTestModule" //まだテスト版...

typedef void* HKMP;//'K'b'M'edia 'P'layer Plugin の Handle

typedef struct
{//オープンしたサウンドデータの情報
    DWORD dwSamplesPerSec;//サンプリング周波数(44100, 22050 など)
    DWORD dwChannels;     //チャンネル数( mono = 1, stereo = 2)
    DWORD dwBitsPerSample;//量子化ビット数( 8 or 16)
    DWORD dwLength;       //曲の長さ（SPC のように計算不可能な場合は 0xFFFFFFFF）
    DWORD dwSeekable;     //シークをサポートしている場合は 1、しない場合は 0
                          //（大雑把にシークできても、シーク後の再生位置が
                          // 不正確な場合は 0 とする）
    DWORD dwUnitRender;   //Render 関数の第３引数はこの値の倍数が渡される（どんな値でも良い場合は 0）
    DWORD dwReserved1;    //常に 0
    DWORD dwReserved2;    //常に 0
}SOUNDINFO;


typedef struct
{
    DWORD dwVersion;
    //モジュールのバージョン。DLL のバージョンではない。必ず KMPMODULE_VERSION(=100) にすること。
    //この値が KbMedia Player が期待する値と一致しない場合は、KbMedia Player
    //によって直ちに FreeLibrary が呼ばれる。
    //その場合、InitDll() も DeinitDll() も呼ばれないことに注意。

    DWORD dwPluginVersion;
    //プラグインのバージョン
    //同じファイル名のプラグインが見つかった場合は、数字が大きいものを優先的に使う


    const char  *pszCopyright;
    //著作権
    //バージョン情報でこの部分の文字列を表示する予定
    //NULL にしてもよい


    const char  *pszDescription;
    //説明
    //バージョン情報でこの部分の文字列を表示する予定
    //NULL にしてもよい


    const char  **ppszSupportExts;
    //サポートする拡張子の文字列の配列(ピリオド含む)
    //NULL で終わるようにする
    //例：ppszSupportExts = {".mp1", ".mp2", ".mp3", "rmp", NULL};
    //
    //拡張子が１つもない場合は、KbMedia Player によって直ちに FreeLibrary が呼
    //ばれる。その場合は InitDll() も DeinitDll() も呼ばれないことに注意。


    DWORD dwReentrant;
    //プラグインが複数同時再生が可能な仕様な場合は 1, 不可能な場合は 0
    //複数の拡張子をサポートしていて、特定の拡張子は複数同時再生が可能であって
    //も、１つでも複数同時再生が不可能なものがある場合は 0 にすること。
    //この値が 0 であっても、Open 関数で戻ったハンドルが有効な間に Open を呼び
    //出しても、現在演奏中の結果に影響を及ぼさないようにすること。
    //将来的にクロスフェードに対応したときにこの値を参照する予定


    void  (WINAPI *Init)(void);
    //DLL初期化。Open 等を呼び出す前に KbMedia Player によって一度だけ呼ばれる。
    //必要ない場合は NULL にしても良い。
    //出来るだけ DllMain の PROCESS_ATTACH よりこちらで初期化する（起動の高速化のため）


    void  (WINAPI *Deinit)(void);
    //DLLの後始末。FreeLibrary の直前に一度だけ呼ばれる。
    //必要ない場合は NULL にしても良い。
    //InitDll() を一度も呼ばずに DeinitDll() を呼ぶ可能性もあることに注意


    HKMP (WINAPI *Open)(const char *cszFileName, SOUNDINFO *pInfo);
    //ファイルを開く。必ず実装すること。
    //エラーの場合は NULL を返す。
    //エラーでない場合は pInfo に適切な情報を入れること。適切な情報が入って
    //いない場合（dwBits が 0 など）は KbMedia Player によって直ちに Close
    //が呼ばれる。
    //pInfo が NULL の場合は何もしないで NULL を返すこと


    HKMP (WINAPI *OpenFromBuffer)(const BYTE *Buffer, DWORD dwSize, SOUNDINFO *pInfo);
    //メモリから開く。サポートしない場合は NULL にすること。
    //サポートしないからといって、常に NULL を返すように実装してはならない。
    //複数の拡張子に対応していても、１つでもサポートしない拡張子がある場合は
    //NULL にしなければならない。
    //pInfo が NULL の場合は何もしないで NULL を返すこと
    //今のところ、KbMedia Player 本体がこれに対応してません。(^_^;

    void   (WINAPI *Close)(HKMP hKMP);
    //ハンドルを閉じる。必ず実装すること。
    //hKMP に NULL を渡しても大丈夫なようにすること。


    DWORD  (WINAPI *Render)(HKMP hKMP, BYTE* Buffer, DWORD dwSize);
    //Buffer に PCM を入れる。必ず実装すること。
    //dwSize はバッファのサイズのバイト数。（サンプル数ではない）
    //戻り値は Buffer に書き込んだバイト数。（サンプル数ではない）
    //dwSize より小さい値を返したら演奏終了。
    //dwSize は SOUNDINFO::dwUnitRender の倍数が渡される。
    //（dwUnitRender の値そのものではなく、倍数の可能性もあることに注意）
    //
    //Buffer が NULL で dwSize が 0 でない場合は、dwSize 分だけ演奏位置
    //を進めること。強制終了してはいけない。
    //シークをサポートしない場合に、ダミーの Render を繰り返し実行して
    //強引にシークする処理を高速化するため。といっても、まだ本体の方で
    //Buffer に NULL を渡すことはないけど。(^^;
    //
    //hKMP に NULL を渡した場合は何もしないで 0 を返すようにすること。


    DWORD  (WINAPI *SetPosition)(HKMP hKMP, DWORD dwPos);
    //シーク。必ず実装すること。
    //dwPos はシーク先の再生位置。戻り値はシーク後の再生位置。単位はミリ秒。
    //dwPos と戻り値は完全に一致する必要はない。戻り値と本当の再生位置の
    //誤差が大きくなる（歌詞との同期再生時に支障をきたす）場合は Open 時に
    //SOUNDINFO の dwSeekable を 0 にしておくこと。誤差がないか、あっても
    //非常に小さい場合は dwSeekable を 1 にしておくこと。戻り値が正確なら
    //ば、dwPos と戻り値の差が大きくても dwSeekable=1 として良い。
    //今のところ、KbMeida Player 本体は dwSeekable の値を見てません。(^_^;
    //dwSeekable の値によって、歌詞データと同期再生するときとしないときで
    //シーク処理を変える予定。
    //
    //シークを全くサポートしない場合は、先頭位置に戻して 0 を返すこと。
    //hKMP に NULL を渡した場合は 0 を返すようにすること。


}KMPMODULE;

typedef KMPMODULE* (WINAPI *pfnGetKMPModule)(void);

#endif
