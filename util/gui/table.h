/* category:  CONNECTION ID */
/* IP addess ignored: LocalAddress */
DEF_GAUGE(LocalPort, unsigned short);
/* IP addess ignored: RemoteAddress */
DEF_GAUGE(RemotePort, unsigned short);
/* category:  STATE */
DEF_GAUGE(State, int);
DEF_GAUGE(SACKEnabled, int);
DEF_GAUGE(TimestampsEnabled, int);
DEF_GAUGE(NagleEnabled, int);
DEF_GAUGE(WinScaleRcvd, int);
DEF_GAUGE(WinScaleSent, int);
DEF_COUNTER(CurrTime, unsigned int);
/* category:  SYN OPTIONS */
DEF_GAUGE(SACKSent, int);
DEF_GAUGE(SACKRcvd, int);
DEF_GAUGE(TimestampSent, int);
DEF_GAUGE(TimestampRcvd, int);
DEF_GAUGE(ActiveOpen, int);
/* category:  TRAFFIC OUT */
DEF_COUNTER(PktsOut, unsigned int);
DEF_COUNTER(DataPktsOut, unsigned int);
DEF_COUNTER(AckPktsOut, unsigned int);
DEF_COUNTER(DataBytesOut, unsigned long long);
/* category:  LOCAL SENDER SEQUENCE */
DEF_COUNTER(snd_una, unsigned int);
DEF_COUNTER(snd_nxt, unsigned int);
DEF_COUNTER(snd_max, unsigned int);
DEF_COUNTER(SendISS, unsigned int);
DEF_COUNTER(SequenceWraps, unsigned int);
/* category:  LOCAL SENDER TRIAGE */
DEF_COUNTER(SndLimTransSender, unsigned int);
DEF_COUNTER(SndLimTimeSender, unsigned int);
DEF_COUNTER(SndLimBytesSender, unsigned long long);
DEF_COUNTER(SndLimTransCwnd, unsigned int);
DEF_COUNTER(SndLimTimeCwnd, unsigned int);
DEF_COUNTER(SndLimBytesCwnd, unsigned long long);
DEF_COUNTER(SndLimTransRwin, unsigned int);
DEF_COUNTER(SndLimTimeRwin, unsigned int);
DEF_COUNTER(SndLimBytesRwin, unsigned long long);
/* category:  LOCAL SENDER CONGESTION MODEL */
DEF_COUNTER(SlowStart, unsigned int);
DEF_COUNTER(CongAvoid, unsigned int);
DEF_COUNTER(CongestionSignals, unsigned int);
DEF_COUNTER(OtherReductions, unsigned int);
DEF_COUNTER(CongestionOverCount, unsigned int);
DEF_GAUGE(CurrentCwnd, unsigned int);
DEF_COUNTER(SumCwndAtCong, unsigned int);
DEF_GAUGE(MaxCwnd, unsigned int);
DEF_GAUGE(CurrentSsthresh, unsigned int);
DEF_GAUGE(MaxSsthresh, unsigned int);
DEF_GAUGE(MinSsthresh, unsigned int);
/* category:  LOCAL SENDER LOSS MODEL */
DEF_COUNTER(FastRetran, unsigned int);
DEF_COUNTER(Timeouts, unsigned int);
DEF_COUNTER(SubsequentTimeouts, unsigned int);
DEF_GAUGE(CurrTimeoutCount, unsigned int);
DEF_COUNTER(AbruptTimeouts, unsigned int);
DEF_COUNTER(PktsRetrans, unsigned int);
DEF_COUNTER(BytesRetrans, unsigned int);
DEF_COUNTER(DupAcksIn, unsigned int);
DEF_COUNTER(PreCongSumRTT, unsigned int);
DEF_COUNTER(PreCongCountRTT, unsigned int);
DEF_COUNTER(PostCongSumRTT, unsigned int);
DEF_COUNTER(PostCongCountRTT, unsigned int);
/* category:  LOCAL SENDER REORDER MODEL */
/* category:  LOCAL SENDER RTT */
DEF_GAUGE(SampledRTT, unsigned int);
DEF_GAUGE(SmoothedRTT, unsigned int);
DEF_GAUGE(MaxRTT, unsigned int);
DEF_GAUGE(MinRTT, unsigned int);
DEF_COUNTER(SumRTT, unsigned long long);
DEF_COUNTER(CountRTT, unsigned int);
DEF_GAUGE(CurrentRTO, unsigned int);
DEF_GAUGE(MaxRTO, unsigned int);
DEF_GAUGE(MinRTO, unsigned int);
/* category:  LOCAL SENDER SEGMENT SIZE */
DEF_GAUGE(CurrentMSS, unsigned int);
DEF_GAUGE(MaxMSS, unsigned int);
DEF_GAUGE(MinMSS, unsigned int);
/* category:  LOCAL SENDER API */
DEF_GAUGE(SCurrentSBCC, unsigned int);
DEF_GAUGE(SMaxSBCC, unsigned int);
DEF_COUNTER(SndBufLimTrans, unsigned int);
DEF_COUNTER(SndBufLimmsec, unsigned int);
DEF_COUNTER(SndAppLimTrans, unsigned int);
DEF_COUNTER(SndAppLimmsec, unsigned int);
/* category:  LOCAL SENDER TUNING */
DEF_GAUGE(SAutoMode, int);
DEF_GAUGE(Sndbuf, unsigned int);
DEF_GAUGE(SndbufSet, unsigned int);
DEF_GAUGE(SndbufMax, unsigned int);
DEF_GAUGE(SndbufMin, unsigned int);
DEF_GAUGE(SAutoErr, int);
DEF_GAUGE(RetransMem, unsigned int);
DEF_GAUGE(RetransMemMax, unsigned int);
/* category:  TRAFFIC IN */
DEF_COUNTER(PktsIn, unsigned int);
DEF_COUNTER(DataPktsIn, unsigned int);
DEF_COUNTER(AckPktsIn, unsigned int);
DEF_COUNTER(DataBytesIn, unsigned long long);
/* category:  LOCAL RECEIVER */
DEF_COUNTER(rcv_nxt, unsigned int);
DEF_COUNTER(syn_seq, unsigned int);
DEF_GAUGE(CurrentRwinSent, unsigned int);
DEF_GAUGE(MaxRwinSent, unsigned int);
DEF_GAUGE(MinRwinSent, unsigned int);
DEF_COUNTER(DupAcksOut, unsigned int);
/* category:  LOCAL RECEIVER TUNING */
DEF_GAUGE(RAutoMode, int);
DEF_GAUGE(Rcvbuf, unsigned int);
DEF_GAUGE(RcvbufSet, unsigned int);
DEF_GAUGE(RAutoErr, int);
/* category:  OBSERVED RECEIVER */
DEF_COUNTER(DSACKDups, unsigned int);
DEF_COUNTER(SACKsRcvd, unsigned int);
DEF_COUNTER(SACKBlocksRcvd, unsigned int);
DEF_GAUGE(CurrentRwinRcvd, unsigned int);
DEF_GAUGE(MaxRwinRcvd, unsigned int);
DEF_GAUGE(MinRwinRcvd, unsigned int);
/* category:  OBSERVED RECEIVER WINDOW */
DEF_COUNTER(RcvStopTrans, unsigned int);
DEF_COUNTER(RcvStopmsec, unsigned int);
DEF_COUNTER(RcvAppLimTrans, unsigned int);
DEF_COUNTER(RcvAppLimmsec, unsigned int);
DEF_COUNTER(RcvBufLimTrans, unsigned int);
DEF_COUNTER(RcvBufLimmsec, unsigned int);
/* XXX */ int web100_get_FlightSize(web100_snapshot *a1, void *buf)
{  web100_get_snd_max(a1,buf) - web100_get_snd_una(a1,buf); }
struct {
  char *tname;
  int type;
  int (*getfn)(web100_snapshot *, void *);
  int (*deltafn)(web100_snapshot *, web100_snapshot *, void *);
} autotable[] = {
{ " CONNECTION ID", -1, /*  CONNECTION ID */ 0, /*  CONNECTION ID */ 0 },
/* IP addess ignored: " LocalAddress " */
{ "LocalPort", 8, &web100_get_LocalPort, /* LocalPort */ 0 },
/* IP addess ignored: " RemoteAddress " */
{ "RemotePort", 8, &web100_get_RemotePort, /* RemotePort */ 0 },
{ " STATE", -1, /*  STATE */ 0, /*  STATE */ 0 },
{ "State", 0, &web100_get_State, /* State */ 0 },
{ "SACKEnabled", 0, &web100_get_SACKEnabled, /* SACKEnabled */ 0 },
{ "TimestampsEnabled", 0, &web100_get_TimestampsEnabled, /* TimestampsEnabled */ 0 },
{ "NagleEnabled", 0, &web100_get_NagleEnabled, /* NagleEnabled */ 0 },
{ "WinScaleRcvd", 0, &web100_get_WinScaleRcvd, /* WinScaleRcvd */ 0 },
{ "WinScaleSent", 0, &web100_get_WinScaleSent, /* WinScaleSent */ 0 },
{ "CurrTime", 3, &web100_get_CurrTime, &web100_delta_CurrTime },
{ " SYN OPTIONS", -1, /*  SYN OPTIONS */ 0, /*  SYN OPTIONS */ 0 },
{ "SACKSent", 0, &web100_get_SACKSent, /* SACKSent */ 0 },
{ "SACKRcvd", 0, &web100_get_SACKRcvd, /* SACKRcvd */ 0 },
{ "TimestampSent", 0, &web100_get_TimestampSent, /* TimestampSent */ 0 },
{ "TimestampRcvd", 0, &web100_get_TimestampRcvd, /* TimestampRcvd */ 0 },
{ "ActiveOpen", 0, &web100_get_ActiveOpen, /* ActiveOpen */ 0 },
{ " TRAFFIC OUT", -1, /*  TRAFFIC OUT */ 0, /*  TRAFFIC OUT */ 0 },
{ "PktsOut", 3, &web100_get_PktsOut, &web100_delta_PktsOut },
{ "DataPktsOut", 3, &web100_get_DataPktsOut, &web100_delta_DataPktsOut },
{ "AckPktsOut", 3, &web100_get_AckPktsOut, &web100_delta_AckPktsOut },
{ "DataBytesOut", 7, &web100_get_DataBytesOut, &web100_delta_DataBytesOut },
{ " LOCAL SENDER SEQUENCE", -1, /*  LOCAL SENDER SEQUENCE */ 0, /*  LOCAL SENDER SEQUENCE */ 0 },
{ "snd_una", 3, &web100_get_snd_una, &web100_delta_snd_una },
{ "snd_nxt", 3, &web100_get_snd_nxt, &web100_delta_snd_nxt },
{ "snd_max", 3, &web100_get_snd_max, &web100_delta_snd_max },
{ "SendISS", 3, &web100_get_SendISS, &web100_delta_SendISS },
{ "SequenceWraps", 3, &web100_get_SequenceWraps, &web100_delta_SequenceWraps },
{ " LOCAL SENDER TRIAGE", -1, /*  LOCAL SENDER TRIAGE */ 0, /*  LOCAL SENDER TRIAGE */ 0 },
{ "SndLimTransSender", 3, &web100_get_SndLimTransSender, &web100_delta_SndLimTransSender },
{ "SndLimTimeSender", 3, &web100_get_SndLimTimeSender, &web100_delta_SndLimTimeSender },
{ "SndLimBytesSender", 7, &web100_get_SndLimBytesSender, &web100_delta_SndLimBytesSender },
{ "SndLimTransCwnd", 3, &web100_get_SndLimTransCwnd, &web100_delta_SndLimTransCwnd },
{ "SndLimTimeCwnd", 3, &web100_get_SndLimTimeCwnd, &web100_delta_SndLimTimeCwnd },
{ "SndLimBytesCwnd", 7, &web100_get_SndLimBytesCwnd, &web100_delta_SndLimBytesCwnd },
{ "SndLimTransRwin", 3, &web100_get_SndLimTransRwin, &web100_delta_SndLimTransRwin },
{ "SndLimTimeRwin", 3, &web100_get_SndLimTimeRwin, &web100_delta_SndLimTimeRwin },
{ "SndLimBytesRwin", 7, &web100_get_SndLimBytesRwin, &web100_delta_SndLimBytesRwin },
{ " LOCAL SENDER CONGESTION MODEL", -1, /*  LOCAL SENDER CONGESTION MODEL */ 0, /*  LOCAL SENDER CONGESTION MODEL */ 0 },
{ "SlowStart", 3, &web100_get_SlowStart, &web100_delta_SlowStart },
{ "CongAvoid", 3, &web100_get_CongAvoid, &web100_delta_CongAvoid },
{ "CongestionSignals", 3, &web100_get_CongestionSignals, &web100_delta_CongestionSignals },
{ "OtherReductions", 3, &web100_get_OtherReductions, &web100_delta_OtherReductions },
{ "CongestionOverCount", 3, &web100_get_CongestionOverCount, &web100_delta_CongestionOverCount },
{ "CurrentCwnd", 4, &web100_get_CurrentCwnd, /* CurrentCwnd */ 0 },
{ "SumCwndAtCong", 3, &web100_get_SumCwndAtCong, &web100_delta_SumCwndAtCong },
{ "MaxCwnd", 4, &web100_get_MaxCwnd, /* MaxCwnd */ 0 },
{ "CurrentSsthresh", 4, &web100_get_CurrentSsthresh, /* CurrentSsthresh */ 0 },
{ "MaxSsthresh", 4, &web100_get_MaxSsthresh, /* MaxSsthresh */ 0 },
{ "MinSsthresh", 4, &web100_get_MinSsthresh, /* MinSsthresh */ 0 },
{ " LOCAL SENDER LOSS MODEL", -1, /*  LOCAL SENDER LOSS MODEL */ 0, /*  LOCAL SENDER LOSS MODEL */ 0 },
{ "FastRetran", 3, &web100_get_FastRetran, &web100_delta_FastRetran },
{ "Timeouts", 3, &web100_get_Timeouts, &web100_delta_Timeouts },
{ "SubsequentTimeouts", 3, &web100_get_SubsequentTimeouts, &web100_delta_SubsequentTimeouts },
{ "CurrTimeoutCount", 4, &web100_get_CurrTimeoutCount, /* CurrTimeoutCount */ 0 },
{ "AbruptTimeouts", 3, &web100_get_AbruptTimeouts, &web100_delta_AbruptTimeouts },
{ "PktsRetrans", 3, &web100_get_PktsRetrans, &web100_delta_PktsRetrans },
{ "BytesRetrans", 3, &web100_get_BytesRetrans, &web100_delta_BytesRetrans },
{ "DupAcksIn", 3, &web100_get_DupAcksIn, &web100_delta_DupAcksIn },
{ "PreCongSumRTT", 3, &web100_get_PreCongSumRTT, &web100_delta_PreCongSumRTT },
{ "PreCongCountRTT", 3, &web100_get_PreCongCountRTT, &web100_delta_PreCongCountRTT },
{ "PostCongSumRTT", 3, &web100_get_PostCongSumRTT, &web100_delta_PostCongSumRTT },
{ "PostCongCountRTT", 3, &web100_get_PostCongCountRTT, &web100_delta_PostCongCountRTT },
{ " LOCAL SENDER REORDER MODEL", -1, /*  LOCAL SENDER REORDER MODEL */ 0, /*  LOCAL SENDER REORDER MODEL */ 0 },
{ " LOCAL SENDER RTT", -1, /*  LOCAL SENDER RTT */ 0, /*  LOCAL SENDER RTT */ 0 },
{ "SampledRTT", 4, &web100_get_SampledRTT, /* SampledRTT */ 0 },
{ "SmoothedRTT", 4, &web100_get_SmoothedRTT, /* SmoothedRTT */ 0 },
{ "MaxRTT", 4, &web100_get_MaxRTT, /* MaxRTT */ 0 },
{ "MinRTT", 4, &web100_get_MinRTT, /* MinRTT */ 0 },
{ "SumRTT", 7, &web100_get_SumRTT, &web100_delta_SumRTT },
{ "CountRTT", 3, &web100_get_CountRTT, &web100_delta_CountRTT },
{ "CurrentRTO", 4, &web100_get_CurrentRTO, /* CurrentRTO */ 0 },
{ "MaxRTO", 4, &web100_get_MaxRTO, /* MaxRTO */ 0 },
{ "MinRTO", 4, &web100_get_MinRTO, /* MinRTO */ 0 },
{ " LOCAL SENDER SEGMENT SIZE", -1, /*  LOCAL SENDER SEGMENT SIZE */ 0, /*  LOCAL SENDER SEGMENT SIZE */ 0 },
{ "CurrentMSS", 4, &web100_get_CurrentMSS, /* CurrentMSS */ 0 },
{ "MaxMSS", 4, &web100_get_MaxMSS, /* MaxMSS */ 0 },
{ "MinMSS", 4, &web100_get_MinMSS, /* MinMSS */ 0 },
{ " LOCAL SENDER API", -1, /*  LOCAL SENDER API */ 0, /*  LOCAL SENDER API */ 0 },
{ "SCurrentSBCC", 4, &web100_get_SCurrentSBCC, /* SCurrentSBCC */ 0 },
{ "SMaxSBCC", 4, &web100_get_SMaxSBCC, /* SMaxSBCC */ 0 },
{ "SndBufLimTrans", 3, &web100_get_SndBufLimTrans, &web100_delta_SndBufLimTrans },
{ "SndBufLimmsec", 3, &web100_get_SndBufLimmsec, &web100_delta_SndBufLimmsec },
{ "SndAppLimTrans", 3, &web100_get_SndAppLimTrans, &web100_delta_SndAppLimTrans },
{ "SndAppLimmsec", 3, &web100_get_SndAppLimmsec, &web100_delta_SndAppLimmsec },
{ " LOCAL SENDER TUNING", -1, /*  LOCAL SENDER TUNING */ 0, /*  LOCAL SENDER TUNING */ 0 },
{ "SAutoMode", 0, &web100_get_SAutoMode, /* SAutoMode */ 0 },
{ "Sndbuf", 4, &web100_get_Sndbuf, /* Sndbuf */ 0 },
{ "SndbufSet", 4, &web100_get_SndbufSet, /* SndbufSet */ 0 },
{ "SndbufMax", 4, &web100_get_SndbufMax, /* SndbufMax */ 0 },
{ "SndbufMin", 4, &web100_get_SndbufMin, /* SndbufMin */ 0 },
{ "SAutoErr", 0, &web100_get_SAutoErr, /* SAutoErr */ 0 },
{ "RetransMem", 4, &web100_get_RetransMem, /* RetransMem */ 0 },
{ "RetransMemMax", 4, &web100_get_RetransMemMax, /* RetransMemMax */ 0 },
{ " TRAFFIC IN", -1, /*  TRAFFIC IN */ 0, /*  TRAFFIC IN */ 0 },
{ "PktsIn", 3, &web100_get_PktsIn, &web100_delta_PktsIn },
{ "DataPktsIn", 3, &web100_get_DataPktsIn, &web100_delta_DataPktsIn },
{ "AckPktsIn", 3, &web100_get_AckPktsIn, &web100_delta_AckPktsIn },
{ "DataBytesIn", 7, &web100_get_DataBytesIn, &web100_delta_DataBytesIn },
{ " LOCAL RECEIVER", -1, /*  LOCAL RECEIVER */ 0, /*  LOCAL RECEIVER */ 0 },
{ "rcv_nxt", 3, &web100_get_rcv_nxt, &web100_delta_rcv_nxt },
{ "syn_seq", 3, &web100_get_syn_seq, &web100_delta_syn_seq },
{ "CurrentRwinSent", 4, &web100_get_CurrentRwinSent, /* CurrentRwinSent */ 0 },
{ "MaxRwinSent", 4, &web100_get_MaxRwinSent, /* MaxRwinSent */ 0 },
{ "MinRwinSent", 4, &web100_get_MinRwinSent, /* MinRwinSent */ 0 },
{ "DupAcksOut", 3, &web100_get_DupAcksOut, &web100_delta_DupAcksOut },
{ " LOCAL RECEIVER TUNING", -1, /*  LOCAL RECEIVER TUNING */ 0, /*  LOCAL RECEIVER TUNING */ 0 },
{ "RAutoMode", 0, &web100_get_RAutoMode, /* RAutoMode */ 0 },
{ "Rcvbuf", 4, &web100_get_Rcvbuf, /* Rcvbuf */ 0 },
{ "RcvbufSet", 4, &web100_get_RcvbufSet, /* RcvbufSet */ 0 },
{ "RAutoErr", 0, &web100_get_RAutoErr, /* RAutoErr */ 0 },
{ " OBSERVED RECEIVER", -1, /*  OBSERVED RECEIVER */ 0, /*  OBSERVED RECEIVER */ 0 },
{ "DSACKDups", 3, &web100_get_DSACKDups, &web100_delta_DSACKDups },
{ "SACKsRcvd", 3, &web100_get_SACKsRcvd, &web100_delta_SACKsRcvd },
{ "SACKBlocksRcvd", 3, &web100_get_SACKBlocksRcvd, &web100_delta_SACKBlocksRcvd },
{ "CurrentRwinRcvd", 4, &web100_get_CurrentRwinRcvd, /* CurrentRwinRcvd */ 0 },
{ "MaxRwinRcvd", 4, &web100_get_MaxRwinRcvd, /* MaxRwinRcvd */ 0 },
{ "MinRwinRcvd", 4, &web100_get_MinRwinRcvd, /* MinRwinRcvd */ 0 },
{ " OBSERVED RECEIVER WINDOW", -1, /*  OBSERVED RECEIVER WINDOW */ 0, /*  OBSERVED RECEIVER WINDOW */ 0 },
{ "RcvStopTrans", 3, &web100_get_RcvStopTrans, &web100_delta_RcvStopTrans },
{ "RcvStopmsec", 3, &web100_get_RcvStopmsec, &web100_delta_RcvStopmsec },
{ "RcvAppLimTrans", 3, &web100_get_RcvAppLimTrans, &web100_delta_RcvAppLimTrans },
{ "RcvAppLimmsec", 3, &web100_get_RcvAppLimmsec, &web100_delta_RcvAppLimmsec },
{ "RcvBufLimTrans", 3, &web100_get_RcvBufLimTrans, &web100_delta_RcvBufLimTrans },
{ "RcvBufLimmsec", 3, &web100_get_RcvBufLimmsec, &web100_delta_RcvBufLimmsec },
{ " APPLICATION INTERFACE", -1, /*  APPLICATION INTERFACE */ 0, /*  APPLICATION INTERFACE */ 0 },
{ "FlightSize", 4, &web100_get_FlightSize, /* FlightSize */ 0 },
0};
