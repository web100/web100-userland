/* Automaticly generated file: see autodefine */
DEF_GAUGE(MinRwinRcvd, unsigned int);
#define MinRwinRcvd web100_get_MinRwinRcvd(ns)
DEF_GAUGE(MaxRwinRcvd, unsigned int);
#define MaxRwinRcvd web100_get_MaxRwinRcvd(ns)
DEF_GAUGE(CurrentRwinRcvd, unsigned int);
#define CurrentRwinRcvd web100_get_CurrentRwinRcvd(ns)
DEF_COUNTER(SACKsRcvd, unsigned int);
#define SACKsRcvd web100_delta_SACKsRcvd(ns, os)
DEF_COUNTER(DSACKDups, unsigned int);
#define DSACKDups web100_delta_DSACKDups(ns, os)
DEF_COUNTER(DupAcksOut, unsigned int);
#define DupAcksOut web100_delta_DupAcksOut(ns, os)
DEF_GAUGE(MinRwinSent, unsigned int);
#define MinRwinSent web100_get_MinRwinSent(ns)
DEF_GAUGE(MaxRwinSent, unsigned int);
#define MaxRwinSent web100_get_MaxRwinSent(ns)
DEF_GAUGE(CurrentRwinSent, unsigned int);
#define CurrentRwinSent web100_get_CurrentRwinSent(ns)
DEF_COUNTER(syn_seq, unsigned int);
#define syn_seq web100_delta_syn_seq(ns, os)
DEF_COUNTER(rcv_nxt, unsigned int);
#define rcv_nxt web100_delta_rcv_nxt(ns, os)
DEF_COUNTER(DataBytesIn, unsigned long long);
#define DataBytesIn web100_delta_DataBytesIn(ns, os)
DEF_COUNTER(AckPktsIn, unsigned int);
#define AckPktsIn web100_delta_AckPktsIn(ns, os)
DEF_COUNTER(DataPktsIn, unsigned int);
#define DataPktsIn web100_delta_DataPktsIn(ns, os)
DEF_COUNTER(PktsIn, unsigned int);
#define PktsIn web100_delta_PktsIn(ns, os)
DEF_GAUGE(MinMSS, unsigned int);
#define MinMSS web100_get_MinMSS(ns)
DEF_GAUGE(MaxMSS, unsigned int);
#define MaxMSS web100_get_MaxMSS(ns)
DEF_GAUGE(CurrentMSS, unsigned int);
#define CurrentMSS web100_get_CurrentMSS(ns)
DEF_GAUGE(MinRTO, unsigned int);
#define MinRTO web100_get_MinRTO(ns)
DEF_GAUGE(MaxRTO, unsigned int);
#define MaxRTO web100_get_MaxRTO(ns)
DEF_GAUGE(CurrentRTO, unsigned int);
#define CurrentRTO web100_get_CurrentRTO(ns)
DEF_COUNTER(CountRTT, unsigned int);
#define CountRTT web100_delta_CountRTT(ns, os)
DEF_COUNTER(SumRTT, unsigned long long);
#define SumRTT web100_delta_SumRTT(ns, os)
DEF_GAUGE(MinRTT, unsigned int);
#define MinRTT web100_get_MinRTT(ns)
DEF_GAUGE(MaxRTT, unsigned int);
#define MaxRTT web100_get_MaxRTT(ns)
DEF_GAUGE(SmoothedRTT, unsigned int);
#define SmoothedRTT web100_get_SmoothedRTT(ns)
DEF_GAUGE(SampledRTT, unsigned int);
#define SampledRTT web100_get_SampledRTT(ns)
DEF_COUNTER(PostCongCountRTT, unsigned int);
#define PostCongCountRTT web100_delta_PostCongCountRTT(ns, os)
DEF_COUNTER(PostCongSumRTT, unsigned int);
#define PostCongSumRTT web100_delta_PostCongSumRTT(ns, os)
DEF_COUNTER(PreCongCountRTT, unsigned int);
#define PreCongCountRTT web100_delta_PreCongCountRTT(ns, os)
DEF_COUNTER(PreCongSumRTT, unsigned int);
#define PreCongSumRTT web100_delta_PreCongSumRTT(ns, os)
DEF_COUNTER(DupAcksIn, unsigned int);
#define DupAcksIn web100_delta_DupAcksIn(ns, os)
DEF_COUNTER(BytesRetrans, unsigned int);
#define BytesRetrans web100_delta_BytesRetrans(ns, os)
DEF_COUNTER(PktsRetrans, unsigned int);
#define PktsRetrans web100_delta_PktsRetrans(ns, os)
DEF_COUNTER(AbruptTimeouts, unsigned int);
#define AbruptTimeouts web100_delta_AbruptTimeouts(ns, os)
DEF_COUNTER(CurrTimeoutCount, unsigned int);
#define CurrTimeoutCount web100_delta_CurrTimeoutCount(ns, os)
DEF_COUNTER(SubsequentTimeouts, unsigned int);
#define SubsequentTimeouts web100_delta_SubsequentTimeouts(ns, os)
DEF_COUNTER(Timeouts, unsigned int);
#define Timeouts web100_delta_Timeouts(ns, os)
DEF_COUNTER(FastRetran, unsigned int);
#define FastRetran web100_delta_FastRetran(ns, os)
DEF_GAUGE(MinSsthresh, unsigned int);
#define MinSsthresh web100_get_MinSsthresh(ns)
DEF_GAUGE(MaxSsthresh, unsigned int);
#define MaxSsthresh web100_get_MaxSsthresh(ns)
DEF_GAUGE(CurrentSsthresh, unsigned int);
#define CurrentSsthresh web100_get_CurrentSsthresh(ns)
DEF_GAUGE(MaxCwnd, unsigned int);
#define MaxCwnd web100_get_MaxCwnd(ns)
DEF_GAUGE(CurrentCwnd, unsigned int);
#define CurrentCwnd web100_get_CurrentCwnd(ns)
DEF_COUNTER(_SumCwndAtRecv, unsigned int);
#define _SumCwndAtRecv web100_delta__SumCwndAtRecv(ns, os)
DEF_COUNTER(SumCwndAtCong, unsigned int);
#define SumCwndAtCong web100_delta_SumCwndAtCong(ns, os)
DEF_COUNTER(OtherReductions, unsigned int);
#define OtherReductions web100_delta_OtherReductions(ns, os)
DEF_COUNTER(_Recoveries, unsigned int);
#define _Recoveries web100_delta__Recoveries(ns, os)
DEF_COUNTER(CongestionOverCount, unsigned int);
#define CongestionOverCount web100_delta_CongestionOverCount(ns, os)
DEF_COUNTER(CongestionSignals, unsigned int);
#define CongestionSignals web100_delta_CongestionSignals(ns, os)
DEF_COUNTER(CongAvoid, unsigned int);
#define CongAvoid web100_delta_CongAvoid(ns, os)
DEF_COUNTER(SlowStart, unsigned int);
#define SlowStart web100_delta_SlowStart(ns, os)
DEF_COUNTER(SndLimTimeRwin, unsigned int);
#define SndLimTimeRwin web100_delta_SndLimTimeRwin(ns, os)
DEF_COUNTER(SndLimBytesRwin, unsigned long long);
#define SndLimBytesRwin web100_delta_SndLimBytesRwin(ns, os)
DEF_COUNTER(SndLimTransRwin, unsigned int);
#define SndLimTransRwin web100_delta_SndLimTransRwin(ns, os)
DEF_COUNTER(SndLimTimeCwnd, unsigned int);
#define SndLimTimeCwnd web100_delta_SndLimTimeCwnd(ns, os)
DEF_COUNTER(SndLimBytesCwnd, unsigned long long);
#define SndLimBytesCwnd web100_delta_SndLimBytesCwnd(ns, os)
DEF_COUNTER(SndLimTransCwnd, unsigned int);
#define SndLimTransCwnd web100_delta_SndLimTransCwnd(ns, os)
DEF_COUNTER(SndLimTimeSender, unsigned int);
#define SndLimTimeSender web100_delta_SndLimTimeSender(ns, os)
DEF_COUNTER(SndLimBytesSender, unsigned long long);
#define SndLimBytesSender web100_delta_SndLimBytesSender(ns, os)
DEF_COUNTER(SndLimTransSender, unsigned int);
#define SndLimTransSender web100_delta_SndLimTransSender(ns, os)
DEF_COUNTER(SequenceWraps, unsigned int);
#define SequenceWraps web100_delta_SequenceWraps(ns, os)
DEF_COUNTER(SendISS, unsigned int);
#define SendISS web100_delta_SendISS(ns, os)
DEF_COUNTER(snd_max, unsigned int);
#define snd_max web100_delta_snd_max(ns, os)
DEF_COUNTER(snd_nxt, unsigned int);
#define snd_nxt web100_delta_snd_nxt(ns, os)
DEF_COUNTER(snd_una, unsigned int);
#define snd_una web100_delta_snd_una(ns, os)
DEF_COUNTER(DataBytesOut, unsigned long long);
#define DataBytesOut web100_delta_DataBytesOut(ns, os)
DEF_COUNTER(AckPktsOut, unsigned int);
#define AckPktsOut web100_delta_AckPktsOut(ns, os)
DEF_COUNTER(DataPktsOut, unsigned int);
#define DataPktsOut web100_delta_DataPktsOut(ns, os)
DEF_COUNTER(PktsOut, unsigned int);
#define PktsOut web100_delta_PktsOut(ns, os)
DEF_GAUGE(ActiveOpen, int);
#define ActiveOpen web100_get_ActiveOpen(ns)
DEF_COUNTER(CurrTime, unsigned int);
#define CurrTime web100_delta_CurrTime(ns, os)
DEF_GAUGE(_SendWinScale, int);
#define _SendWinScale web100_get__SendWinScale(ns)
DEF_GAUGE(WinScaleSent, int);
#define WinScaleSent web100_get_WinScaleSent(ns)
DEF_GAUGE(_RecvWinScale, int);
#define _RecvWinScale web100_get__RecvWinScale(ns)
DEF_GAUGE(WinScaleRcvd, int);
#define WinScaleRcvd web100_get_WinScaleRcvd(ns)
DEF_GAUGE(NagleEnabled, int);
#define NagleEnabled web100_get_NagleEnabled(ns)
DEF_GAUGE(TimestampsEnabled, int);
#define TimestampsEnabled web100_get_TimestampsEnabled(ns)
DEF_GAUGE(SACKEnabled, int);
#define SACKEnabled web100_get_SACKEnabled(ns)
DEF_GAUGE(State, int);
#define State web100_get_State(ns)
DEF_GAUGE(RemotePort, unsigned short);
#define RemotePort web100_get_RemotePort(ns)
/* IP addess ignored: RemoteAddress */
DEF_GAUGE(LocalPort, unsigned short);
#define LocalPort web100_get_LocalPort(ns)
/* IP addess ignored: LocalAddress */
