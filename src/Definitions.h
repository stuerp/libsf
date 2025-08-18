
/** $VER: Definitions.h (2025.03.23) - Articulation connection graph definitions **/

#pragma once

#pragma region DLS Level 1

/* Generic Sources */
#define CONN_SRC_NONE               0x0000
#define CONN_SRC_LFO                0x0001
#define CONN_SRC_KEYONVELOCITY      0x0002
#define CONN_SRC_KEYNUMBER          0x0003
#define CONN_SRC_EG1                0x0004
#define CONN_SRC_EG2                0x0005
#define CONN_SRC_PITCHWHEEL         0x0006

/* Midi Controllers 0-127 */
#define CONN_SRC_CC1                0x0081
#define CONN_SRC_CC7                0x0087
#define CONN_SRC_CC10               0x008a
#define CONN_SRC_CC11               0x008b

/* Registered Parameter Numbers */
#define CONN_SRC_RPN0               0x0100
#define CONN_SRC_RPN1               0x0101
#define CONN_SRC_RPN2               0x0102

/* Generic Destinations */
#define CONN_DST_NONE               0x0000
#define CONN_DST_ATTENUATION        0x0001
#define CONN_DST_RESERVED           0x0002
#define CONN_DST_PITCH              0x0003
#define CONN_DST_PAN                0x0004

/* LFO Destinations */
#define CONN_DST_LFO_FREQUENCY      0x0104
#define CONN_DST_LFO_STARTDELAY     0x0105

/* EG1 Destinations */
#define CONN_DST_EG1_ATTACKTIME     0x0206
#define CONN_DST_EG1_DECAYTIME      0x0207
#define CONN_DST_EG1_RESERVED       0x0208
#define CONN_DST_EG1_RELEASETIME    0x0209
#define CONN_DST_EG1_SUSTAINLEVEL   0x020a

/* EG2 Destinations */
#define CONN_DST_EG2_ATTACKTIME     0x030a
#define CONN_DST_EG2_DECAYTIME      0x030b
#define CONN_DST_EG2_RESERVED       0x030c
#define CONN_DST_EG2_RELEASETIME    0x030d
#define CONN_DST_EG2_SUSTAINLEVEL   0x030e

#define CONN_TRN_NONE               0x0000
#define CONN_TRN_CONCAVE            0x0001

#pragma endregion

#pragma region DLS Level 2.2

/** These are in addition to the definitions in the DLS1 header. */

/* Generic Sources */
#define CONN_SRC_POLYPRESSURE       0x0007 /* Polyphonic Pressure */
#define CONN_SRC_CHANNELPRESSURE    0x0008 /* Channel Pressure */
#define CONN_SRC_VIBRATO            0x0009 /* Vibrato LFO */
#define CONN_SRC_MONOPRESSURE       0x000A /* MIDI Mono pressure */

/* Midi Controllers */
#define CONN_SRC_CC91               0x00db /* Reverb Send */
#define CONN_SRC_CC93               0x00dd /* Chorus Send */

/* Generic Destinations */
#define CONN_DST_GAIN               0x0001 /* Same as CONN_DST_ ATTENUATION */
#define CONN_DST_KEYNUMBER          0x0005 /* Key Number Generator */

/* Audio Channel Output Destinations */
#define CONN_DST_LEFT               0x0010 /* Left Channel Send */
#define CONN_DST_RIGHT              0x0011 /* Right Channel Send */
#define CONN_DST_CENTER             0x0012 /* Center Channel Send */
#define CONN_DST_LEFTREAR           0x0013 /* Left Rear Channel Send */
#define CONN_DST_RIGHTREAR          0x0014 /* Right Rear Channel Send */
#define CONN_DST_LFE_CHANNEL        0x0015 /* LFE Channel Send */
#define CONN_DST_CHORUS             0x0080 /* Chorus Send */
#define CONN_DST_REVERB             0x0081 /* Reverb Send */

/* Vibrato LFO Destinations */
#define CONN_DST_VIB_FREQUENCY      0x0114 /* Vibrato Frequency */
#define CONN_DST_VIB_STARTDELAY     0x0115 /* Vibrato Start Delay */

/* EG1 Destinations */
#define CONN_DST_EG1_DELAYTIME      0x020B /* EG1 Delay Time */
#define CONN_DST_EG1_HOLDTIME       0x020C /* EG1 Hold Time */
#define CONN_DST_EG1_SHUTDOWNTIME   0x020D /* EG1 Shutdown Time */

/* EG2 Destinations */
#define CONN_DST_EG2_DELAYTIME      0x030F /* EG2 Delay Time */
#define CONN_DST_EG2_HOLDTIME       0x0310 /* EG2 Hold Time */

/* Filter Destinations */
#define CONN_DST_FILTER_CUTOFF      0x0500 /* Filter Cutoff Frequency */
#define CONN_DST_FILTER_Q           0x0501 /* Filter Resonance */

/* Transforms */
#define CONN_TRN_CONVEX             0x0002 /* Convex Transform */
#define CONN_TRN_SWITCH             0x0003 /* Switch Transform */

/* Conditional chunk operators */
#define DLS_CDL_AND                 0x0001 /* X = X & Y */
#define DLS_CDL_OR                  0x0002 /* X = X | Y */
#define DLS_CDL_XOR                 0x0003 /* X = X ^ Y */
#define DLS_CDL_ADD                 0x0004 /* X = X + Y */
#define DLS_CDL_SUBTRACT            0x0005 /* X = X - Y */
#define DLS_CDL_MULTIPLY            0x0006 /* X = X * Y */
#define DLS_CDL_DIVIDE              0x0007 /* X = X / Y */
#define DLS_CDL_LOGICAL_AND         0x0008 /* X = X && Y */
#define DLS_CDL_LOGICAL_OR          0x0009 /* X = X || Y */
#define DLS_CDL_LT                  0x000A /* X = (X < Y) */
#define DLS_CDL_LE                  0x000B /* X = (X <= Y) */
#define DLS_CDL_GT                  0x000C /* X = (X > Y) */
#define DLS_CDL_GE                  0x000D /* X = (X >= Y) */
#define DLS_CDL_EQ                  0x000E /* X = (X == Y) */
#define DLS_CDL_NOT                 0x000F /* X = !X */
#define DLS_CDL_CONST               0x0010 /* 32-bit constant */
#define DLS_CDL_QUERY               0x0011 /* 32-bit value returned from query */
#define DLS_CDL_QUERYSUPPORTED      0x0012 /* 32-bit value returned from query */

/* DLSID queries for <cdl-ck> */

#ifndef _DLSID
typedef struct _DLSID
{
    ULONG   ulData1;
    USHORT  usData2;
    USHORT  usData3;
    BYTE    abData4[8];
} DLSID;
#endif

/* DEFINE_DLSID DLSID queries for <cdl-ck> */

#ifndef DEFINE_DLSID
#define DEFINE_DLSID(defName, ul1, us2, us3, ab40, ab41, ab42, ab43, ab44, ab45, ab46, ab47) \
static const DLSID defName = { ul1, us2, us3, { ab40, ab41, ab42, ab43, ab44, ab45, ab46, ab47 } }
#endif

DEFINE_DLSID(DLSID_GMInHardware,        0x178f2f24, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_DLSID(DLSID_GSInHardware,        0x178f2f25, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_DLSID(DLSID_XGInHardware,        0x178f2f26, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_DLSID(DLSID_SupportsDLS1,        0x178f2f27, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_DLSID(DLSID_SupportsDLS2,        0xf14599e5, 0x4689, 0x11d2, 0xaf, 0xa6, 0x00, 0xaa, 0x00, 0x24, 0xd8, 0xb6);
DEFINE_DLSID(DLSID_SampleMemorySize,    0x178f2f28, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_DLSID(DLSID_ManufacturersID,     0xb03e1181, 0x8095, 0x11d2, 0xa1, 0xef, 0x00, 0x60, 0x08, 0x33, 0xdb, 0xd8);
DEFINE_DLSID(DLSID_ProductID,           0xb03e1182, 0x8095, 0x11d2, 0xa1, 0xef, 0x00, 0x60, 0x08, 0x33, 0xdb, 0xd8);
DEFINE_DLSID(DLSID_SamplePlaybackRate,  0x2a91f713, 0xa4bf, 0x11d2, 0xbb, 0xdf, 0x00, 0x60, 0x08, 0x33, 0xdb, 0xd8);

#pragma endregion
