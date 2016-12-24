//---------------------------------------------------------------------------
/*
	Risa [りさ]      alias 吉里吉里3 [kirikiri-3]
	 stands for "Risa Is a Stagecraft Architecture"
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//! @file
//! @brief Phase Vocoder の実装
//---------------------------------------------------------------------------
#ifndef RisaPhaseVocoderH
#define RisaPhaseVocoderH

#include "RingBuffer.h"

//---------------------------------------------------------------------------
//! @brief Phase Vocoder DSP クラス
//---------------------------------------------------------------------------
class tRisaPhaseVocoderDSP
{
protected:
	float ** AnalWork; //!< 解析(Analyze)用バッファ(FrameSize個) 名前で笑わないように
	float ** SynthWork; //!< 合成用作業バッファ(FrameSize)
	float ** LastAnalPhase; //!< 前回解析時の各フィルタバンドの位相 (各チャンネルごとにFrameSize/2個)
	float ** LastSynthPhase; //!< 前回合成時の各フィルタバンドの位相 (各チャンネルごとにFrameSize/2個)

	int * FFTWorkIp; //!< rdft に渡す ip パラメータ
	float * FFTWorkW; //!< rdft に渡す w パラメータ
	float * InputWindow; //!< 入力用窓関数
	float * OutputWindow; //!< 出力用窓関数

	unsigned int FrameSize; //!< FFTサイズ
	unsigned int OverSampling; //!< オーバー・サンプリング係数
	unsigned int Frequency; //!< PCM サンプリング周波数
	unsigned int Channels; //!< PCM チャンネル数
	unsigned int InputHopSize; //!< FrameSize/OverSampling
	unsigned int OutputHopSize; //!< InputHopSize * TimeScale (SetTimeScale時に再計算される)

	float	TimeScale; //!< 時間軸方向のスケール(出力/入力)
	float	FrequencyScale; //!< 周波数方向のスケール(出力/入力)

	// 以下、RebuildParams が真の時に再構築されるパラメータ
	// ここにあるメンバ以外では、InputWindow と OutputWindow も再構築される
	float OverSamplingRadian; //!< (2.0*M_PI)/OverSampling
	float OverSamplingRadianRecp; //!< OverSamplingRadian の逆数
	float FrequencyPerFilterBand; //!< Frequency/FrameSize
	float FrequencyPerFilterBandRecp; //!< FrequencyPerFilterBand の逆数
	float ExactTimeScale; //!< 厳密なTimeScale = OutputHopSize / InputHopSize
	// 再構築されるパラメータ、ここまで

	tRisaRingBuffer<float> InputBuffer; //!< 入力用リングバッファ
	tRisaRingBuffer<float> OutputBuffer; //!< 出力用リングバッファ

	bool	RebuildParams; //!< 内部的なパラメータなどを再構築しなければならないときに真

	unsigned long LastSynthPhaseAdjustCounter; //!< LastSynthPhase を補正する周期をはかるためのカウンタ
	const static unsigned long LastSynthPhaseAdjustIncrement = 0x03e8a444; //!< LastSynthPhaseAdjustCounterに加算する値
	const static unsigned long LastSynthPhaseAdjustInterval  = 0xfa2911fe; //!< LastSynthPhase を補正する周期


public:
	//! @brief Process が返すステータス
	enum tStatus
	{
		psNoError, //!< 問題なし
		psInputNotEnough, //!< 入力がもうない (GetInputBufferで得たポインタに書いてから再試行せよ)
		psOutputFull //!< 出力バッファがいっぱい (GetOutputBufferで得たポインタから読み出してから再試行せよ)
	};

public:
	//! @brief		コンストラクタ
	//! @param		framesize		フレームサイズ(2の累乗, 16～)
	//! @param		frequency		入力PCMのサンプリングレート
	//! @param		channels		入力PCMのチャンネル数
	//! @note		音楽用ではframesize=4096,oversamp=16ぐらいがよく、
	//! @note		ボイス用ではframesize=256,oversamp=8ぐらいがよい。
	tRisaPhaseVocoderDSP(unsigned int framesize,
					unsigned int frequency, unsigned int channels);

	//! @brief		デストラクタ
	~tRisaPhaseVocoderDSP();

	float GetTimeScale() const { return TimeScale; } //!< 時間軸方向のスケールを得る

	//! @brief		時間軸方向のスケールを設定する
	//! @param		v     スケール
	void SetTimeScale(float v);

	float GetFrequencyScale() const { return FrequencyScale; } //!< 周波数軸方向のスケールを得る

	//! @brief		周波数軸方向のスケールを設定する
	//! @param		v     スケール
	void SetFrequencyScale(float v);

	//! @brief		オーバーサンプリング係数を取得する
	//! @return		オーバーサンプリング係数
	unsigned int GetOverSampling() const { return OverSampling; }

	//! @brief		オーバーサンプリング係数を設定する
	//! @param		v		係数 ( 0 = 時間軸方向のスケールに従って自動的に設定 )
	void SetOverSampling(unsigned int v);

	unsigned int GetInputHopSize() const { return InputHopSize; } //!< InputHopSizeを得る
	unsigned int GetOutputHopSize() const { return OutputHopSize; } //!< OutputHopSize を得る

private:
	//! @brief		クリア
	void Clear();

public:
	//! @brief		入力バッファの空きサンプルグラニュール数を得る
	//! @return		入力バッファの空きサンプルグラニュール数
	size_t GetInputFreeSize();

	//! @brief		入力バッファの書き込みポインタを得る
	//! @param		numsamplegranules 書き込みたいサンプルグラニュール数
	//! @param		p1		ブロック1の先頭へのポインタを格納するための変数
	//! @param		p1size	p1の表すブロックのサンプルグラニュール数
	//! @param		p2		ブロック2の先頭へのポインタを格納するための変数(NULLがあり得る)
	//! @param		p2size	p2の表すブロックのサンプルグラニュール数(0があり得る)
	//! @return		空き容量が足りなければ偽、空き容量が足り、ポインタが返されれば真
	//! @note		p1 と p2 のように２つのポインタとそのサイズが返されるのは、
	//!				このバッファが実際はリングバッファで、リングバッファ内部のリニアなバッファ
	//!				の終端をまたぐ可能性があるため。またがない場合はp2はNULLになるが、またぐ
	//!				場合は p1 のあとに p2 に続けて書き込まなければならない。
	bool GetInputBuffer(size_t numsamplegranules,
		float * & p1, size_t & p1size,
		float * & p2, size_t & p2size);

	//! @brief		出力バッファの準備済みサンプルグラニュール数を得る
	//! @return		出力バッファの準備済みサンプルグラニュール数
	size_t GetOutputReadySize();

	//! @brief		出力バッファの読み込みポインタを得る
	//! @param		numsamplegranules 読み込みたいサンプルグラニュール数
	//! @param		p1		ブロック1の先頭へのポインタを格納するための変数
	//! @param		p1size	p1の表すブロックのサンプルグラニュール数
	//! @param		p2		ブロック2の先頭へのポインタを格納するための変数(NULLがあり得る)
	//! @param		p2size	p2の表すブロックのサンプルグラニュール数(0があり得る)
	//! @return		準備されたサンプルが足りなければ偽、サンプルが足り、ポインタが返されれば真
	//! @note		p1 と p2 のように２つのポインタとそのサイズが返されるのは、
	//!				このバッファが実際はリングバッファで、リングバッファ内部のリニアなバッファ
	//!				の終端をまたぐ可能性があるため。またがない場合はp2はNULLになるが、またぐ
	//!				場合は p1 のあとに p2 を続けて読み出さなければならない。
	bool GetOutputBuffer(size_t numsamplegranules,
		const float * & p1, size_t & p1size,
		const float * & p2, size_t & p2size);

	//! @brief		処理を1ステップ行う
	//! @return		処理結果を表すenum
	tStatus Process();

	//! @brief		演算の根幹部分を処理する
	//! @param		ch			処理を行うチャンネル
	//! @note		ここの部分は各CPUごとに最適化されるため、
	//!				実装は opt_default ディレクトリ下などに置かれる。
	//!				(PhaseVocoderDSP.cpp内にはこれの実装はない)
	void ProcessCore(int ch);
#if defined(_M_IX86)||defined(_M_X64)
	void ProcessCore_sse(int ch);
#endif
};
//---------------------------------------------------------------------------

#endif
