/* Derived from PsyQ SYM records; secondary PSX evidence, not a PC layout proof. */
struct NStype { /* NAS, 308 bytes, incomplete */
    /* 0x01c */ unsigned char Palette;
    /* 0x01d */ unsigned char SysPal;
    /* 0x01e */ unsigned char PALIndex;
    /* 0x01f */ unsigned char pFlag;
    /* 0x020 */ unsigned char Container;
    /* 0x021 */ unsigned char StepRate2;
    /* 0x022 */ unsigned char Dbase;
    /* 0x023 */ unsigned char SpecialFX;
    /* 0x024 */ unsigned char Ambience;
    /* 0x025 */ unsigned char Failure;
    /* 0x026 */ short ClipOffset;
    /* 0x028 */ short ShadowYoffs;
    /* 0x02a */ unsigned short NAStatus;
    /* 0x02c */ unsigned short Light;
    /* 0x02e */ unsigned short Type;
    /* 0x030 */ short Aspeed;
    /* 0x032 */ short Aindex;
    /* 0x034 */ unsigned short Frame;
    /* 0x036 */ unsigned short SubFrame;
    /* 0x038 */ unsigned short CurMap;
    /* 0x03a */ unsigned short Scale;
    /* 0x03c */ unsigned short ScaleY;
    /* 0x03e */ unsigned short ScaleZ;
    /* 0x040 */ unsigned short LastFrame;
    /* 0x042 */ unsigned short LastSubFrame;
    /* 0x044 */ unsigned short LastDbase;
    /* 0x046 */ unsigned short VRAMindex;
    /* 0x048 */ unsigned short VRAMindexSub;
    /* 0x04a */ unsigned short FadeLevel;
    /* 0x04c */ long StepRate;
    /* 0x050 */ unsigned long Class;
    /* 0x054 */ unsigned long pCtrl;
    /* 0x058 */ void * Trigger;
    /* 0x05c */ void * Shadow;
    /* 0x060 */ struct TStype * Extra;
    /* 0x064 */ void * Atable;
    /* 0x068 */ void * Tag;
    /* 0x06c */ void * SFX;
    /* 0x070 */ struct .9fake Angle;
    /* 0x078 */ struct .8fake Lvtr;
    /* 0x088 */ union collision_type DefCol;
    /* 0x094 */ unsigned char Xdir;
    /* 0x095 */ unsigned char Ydir;
    /* 0x096 */ unsigned char Zdir;
    /* 0x097 */ unsigned char Boomshanka;
    /* 0x098 */ short Zmove;
    /* 0x09a */ short Bumblesprout;
    /* 0x09c */ long Old_Xpos;
    /* 0x0a0 */ long Old_Ypos;
    /* 0x0a4 */ long Old_Zpos;
    /* 0x0a8 */ long XvelActing;
    /* 0x0ac */ long XmaxActing;
    /* 0x0b0 */ long XaccActing;
    /* 0x0b4 */ long XdecActing;
    /* 0x0b8 */ long YvelActing;
    /* 0x0bc */ long YmaxActing;
    /* 0x0c0 */ long YaccActing;
    /* 0x0c4 */ long YdecActing;
    /* 0x0c8 */ long ZvelActing;
    /* 0x0cc */ long ZmaxActing;
    /* 0x0d0 */ long ZaccActing;
    /* 0x0d4 */ long ZdecActing;
    /* 0x0d8 */ long Xaim;
    /* 0x0dc */ long Yaim;
    /* 0x0e0 */ long Zaim;
    /* 0x0e4 */ struct NStype * PlParent;
    /* 0x0e8 */ struct NStype * PlChild;
    /* 0x0ec */ unsigned char Immunity;
    /* 0x0ed */ unsigned char WeaponLog;
    /* 0x0ee */ short Strength;
    /* 0x0f0 */ long Ptable;
    /* 0x0f4 */ long Pindex;
    /* 0x0f8 */ long Pspeed;
    /* 0x0fc */ long YvelReacting;
    /* 0x100 */ long YmaxReacting;
    /* 0x104 */ long YaccReacting;
    /* 0x108 */ long Special1;
    /* 0x10c */ long Special2;
    /* 0x110 */ void * Aux1Ptr;
    /* 0x114 */ short Aux1Index;
    /* 0x116 */ short Aux1Count;
    /* 0x118 */ void * Aux2Ptr;
    /* 0x11c */ short Aux2Index;
    /* 0x11e */ short Aux2Count;
    /* 0x120 */ void * Aux3Ptr;
    /* 0x124 */ short Aux3Index;
    /* 0x126 */ short Aux3Count;
    /* 0x128 */ void * Extension;
    /* 0x12c */ struct NStype * Parent;
    /* 0x130 */ struct NStype * Child;
};
struct SPtype { /* SPAS, 236 bytes */
    /* 0x000 */ long Xpos;
    /* 0x004 */ long Ypos;
    /* 0x008 */ long Zpos;
    /* 0x00c */ struct .9fake Vec;
    /* 0x014 */ struct .9fake XYZOffset;
    /* 0x01c */ unsigned char Palette;
    /* 0x01d */ unsigned char SysPal;
    /* 0x01e */ unsigned char PALIndex;
    /* 0x01f */ unsigned char pFlag;
    /* 0x020 */ unsigned char Container;
    /* 0x021 */ unsigned char StepRate2;
    /* 0x022 */ unsigned char Dbase;
    /* 0x023 */ unsigned char SpecialFX;
    /* 0x024 */ unsigned char Ambience;
    /* 0x025 */ unsigned char Failure;
    /* 0x026 */ short ClipOffset;
    /* 0x028 */ short ShadowYoffs;
    /* 0x02a */ unsigned short NAStatus;
    /* 0x02c */ unsigned short Light;
    /* 0x02e */ unsigned short Type;
    /* 0x030 */ short Aspeed;
    /* 0x032 */ short Aindex;
    /* 0x034 */ unsigned short Frame;
    /* 0x036 */ unsigned short SubFrame;
    /* 0x038 */ unsigned short CurMap;
    /* 0x03a */ unsigned short Scale;
    /* 0x03c */ unsigned short ScaleY;
    /* 0x03e */ unsigned short ScaleZ;
    /* 0x040 */ unsigned short LastFrame;
    /* 0x042 */ unsigned short LastSubFrame;
    /* 0x044 */ unsigned short LastDbase;
    /* 0x046 */ unsigned short VRAMindex;
    /* 0x048 */ unsigned short VRAMindexSub;
    /* 0x04a */ unsigned short FadeLevel;
    /* 0x04c */ long StepRate;
    /* 0x050 */ unsigned long Class;
    /* 0x054 */ unsigned long pCtrl;
    /* 0x058 */ void * Trigger;
    /* 0x05c */ void * Shadow;
    /* 0x060 */ struct TStype * Extra;
    /* 0x064 */ void * Atable;
    /* 0x068 */ void * Tag;
    /* 0x06c */ void * SFX;
    /* 0x070 */ struct .9fake Angle;
    /* 0x078 */ struct .8fake Lvtr;
    /* 0x088 */ union collision_type DefCol;
    /* 0x094 */ unsigned char Xdir;
    /* 0x095 */ unsigned char Ydir;
    /* 0x096 */ unsigned char Zdir;
    /* 0x097 */ unsigned char Boomshanka;
    /* 0x098 */ short Zmove;
    /* 0x09a */ short Bumblesprout;
    /* 0x09c */ long Old_Xpos;
    /* 0x0a0 */ long Old_Ypos;
    /* 0x0a4 */ long Old_Zpos;
    /* 0x0a8 */ long XvelActing;
    /* 0x0ac */ long XmaxActing;
    /* 0x0b0 */ long XaccActing;
    /* 0x0b4 */ long XdecActing;
    /* 0x0b8 */ long YvelActing;
    /* 0x0bc */ long YmaxActing;
    /* 0x0c0 */ long YaccActing;
    /* 0x0c4 */ long YdecActing;
    /* 0x0c8 */ long ZvelActing;
    /* 0x0cc */ long ZmaxActing;
    /* 0x0d0 */ long ZaccActing;
    /* 0x0d4 */ long ZdecActing;
    /* 0x0d8 */ long Xaim;
    /* 0x0dc */ long Yaim;
    /* 0x0e0 */ long Zaim;
    /* 0x0e4 */ struct NStype * PlParent;
    /* 0x0e8 */ struct NStype * PlChild;
};
struct TStype { /* TAS, 148 bytes */
    /* 0x000 */ long Xpos;
    /* 0x004 */ long Ypos;
    /* 0x008 */ long Zpos;
    /* 0x00c */ struct .9fake Vec;
    /* 0x014 */ struct .9fake XYZOffset;
    /* 0x01c */ unsigned char Palette;
    /* 0x01d */ unsigned char SysPal;
    /* 0x01e */ unsigned char PALIndex;
    /* 0x01f */ unsigned char pFlag;
    /* 0x020 */ unsigned char Container;
    /* 0x021 */ unsigned char StepRate2;
    /* 0x022 */ unsigned char Dbase;
    /* 0x023 */ unsigned char SpecialFX;
    /* 0x024 */ unsigned char Ambience;
    /* 0x025 */ unsigned char Failure;
    /* 0x026 */ short ClipOffset;
    /* 0x028 */ short ShadowYoffs;
    /* 0x02a */ unsigned short NAStatus;
    /* 0x02c */ unsigned short Light;
    /* 0x02e */ unsigned short Type;
    /* 0x030 */ short Aspeed;
    /* 0x032 */ short Aindex;
    /* 0x034 */ unsigned short Frame;
    /* 0x036 */ unsigned short SubFrame;
    /* 0x038 */ unsigned short CurMap;
    /* 0x03a */ unsigned short Scale;
    /* 0x03c */ unsigned short ScaleY;
    /* 0x03e */ unsigned short ScaleZ;
    /* 0x040 */ unsigned short LastFrame;
    /* 0x042 */ unsigned short LastSubFrame;
    /* 0x044 */ unsigned short LastDbase;
    /* 0x046 */ unsigned short VRAMindex;
    /* 0x048 */ unsigned short VRAMindexSub;
    /* 0x04a */ unsigned short FadeLevel;
    /* 0x04c */ long StepRate;
    /* 0x050 */ unsigned long Class;
    /* 0x054 */ unsigned long pCtrl;
    /* 0x058 */ void * Trigger;
    /* 0x05c */ void * Shadow;
    /* 0x060 */ struct TStype * Extra;
    /* 0x064 */ void * Atable;
    /* 0x068 */ void * Tag;
    /* 0x06c */ void * SFX;
    /* 0x070 */ struct .9fake Angle;
    /* 0x078 */ struct .8fake Lvtr;
    /* 0x088 */ union collision_type DefCol;
};
struct .123fake { /* AnimT1, 8 bytes */
    /* 0x000 */ short frame;
    /* 0x002 */ short x;
    /* 0x004 */ short y;
    /* 0x006 */ short delay;
};
struct .124fake { /* AnimT2, 4 bytes */
    /* 0x000 */ short frame;
    /* 0x002 */ short delay;
};
struct .125fake { /* Anim3D, 6 bytes */
    /* 0x000 */ short frame;
    /* 0x002 */ short mval;
    /* 0x004 */ short delay;
};
struct .126fake { /* PathT1, 4 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
};
struct .127fake { /* PathT2, 4 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
};
struct .128fake { /* Path3D1, 6 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
    /* 0x004 */ short z;
};
struct .129fake { /* Path3D2, 6 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
    /* 0x004 */ short z;
};
struct .130fake { /* Path3D3, 12 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
    /* 0x004 */ short z;
    /* 0x006 */ short ax;
    /* 0x008 */ short ay;
    /* 0x00a */ short az;
};
struct .131fake { /* PathX, 8 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
    /* 0x004 */ short z;
    /* 0x006 */ short delay;
};
struct .132fake { /* Offset3D3, 14 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
    /* 0x004 */ short z;
    /* 0x006 */ short ax;
    /* 0x008 */ short ay;
    /* 0x00a */ short az;
    /* 0x00c */ short delay;
};
struct .133fake { /* ExtIdent, 2 bytes */
    /* 0x000 */ unsigned short id;
};
struct .134fake { /* ExtAnim3D, 20 bytes */
    /* 0x000 */ unsigned short id;
    /* 0x002 */ short mst;
    /* 0x004 */ short mend;
    /* 0x006 */ short dm;
    /* 0x008 */ short Ax;
    /* 0x00a */ short dAx;
    /* 0x00c */ short Ay;
    /* 0x00e */ short dAy;
    /* 0x010 */ short Az;
    /* 0x012 */ short dAz;
};
struct NT_type { /* NAStrig, 20 bytes */
    /* 0x000 */ long Xpos;
    /* 0x004 */ long Ypos;
    /* 0x008 */ long Zpos;
    /* 0x00c */ unsigned short status;
    /* 0x00e */ unsigned short special;
    /* 0x010 */ unsigned short Type;
    /* 0x012 */ unsigned short fred;
};
struct .135fake { /* TrigList, 6 bytes */
    /* 0x000 */ unsigned short Xpos;
    /* 0x002 */ unsigned short Ypos;
    /* 0x004 */ unsigned short Type;
};
struct .136fake { /* TrigList3, 8 bytes */
    /* 0x000 */ unsigned short Xpos;
    /* 0x002 */ unsigned short Ypos;
    /* 0x004 */ unsigned short Zpos;
    /* 0x006 */ unsigned short Type;
};
struct .137fake { /* TriggerWorld, 8 bytes */
    /* 0x000 */ struct NT_type * triggers;
    /* 0x004 */ int trigsize;
};
struct .138fake { /* AMB, 6 bytes */
    /* 0x000 */ unsigned char Ar;
    /* 0x001 */ unsigned char Ab;
    /* 0x002 */ unsigned char Ag;
    /* 0x003 */ unsigned char Pr;
    /* 0x004 */ unsigned char Pb;
    /* 0x005 */ unsigned char Pg;
};
struct .139fake { /* REL, 12 bytes */
    /* 0x000 */ short Dx;
    /* 0x002 */ short Dy;
    /* 0x004 */ short Dz;
    /* 0x006 */ unsigned short hypo;
    /* 0x008 */ unsigned short hypo3;
    /* 0x00a */ unsigned short dir;
};
struct .140fake { /* SPListTab, 12 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
    /* 0x004 */ short z;
    /* 0x006 */ short type;
    /* 0x008 */ short frame;
    /* 0x00a */ short val;
};
struct .141fake { /* NASjump, 12 bytes */
    /* 0x000 */ void * () init;
    /* 0x004 */ void * () preanim;
    /* 0x008 */ void * () postanim;
};
struct .142fake { /* SPASjump, 8 bytes */
    /* 0x000 */ void * () init;
    /* 0x004 */ void * () handler;
};
struct .143fake { /* NASspec, 4 bytes */
    /* 0x000 */ long * () contour;
};
struct .144fake { /* SPECjump, 4 bytes */
    /* 0x000 */ void * () special;
};
struct .145fake { /* NASfink, 4 bytes */
    /* 0x000 */ void * () fink;
};
struct .146fake { /* LUPtrig, 10 bytes */
    /* 0x000 */ short Xpos;
    /* 0x002 */ short Ypos;
    /* 0x004 */ short Zpos;
    /* 0x006 */ unsigned short status;
    /* 0x008 */ unsigned short Type;
};
struct .147fake { /* TypeFill, 16 bytes */
    /* 0x000 */ void * Atable;
    /* 0x004 */ void * Aux1Ptr;
    /* 0x008 */ void * Aux2Ptr;
    /* 0x00c */ unsigned short Dbase;
};
struct .148fake { /* TypeSFill, 8 bytes */
    /* 0x000 */ void * Atable;
    /* 0x004 */ short Athing;
    /* 0x006 */ unsigned short Dbase;
};
struct .149fake { /* SOXjump, 8 bytes */
    /* 0x000 */ void * () init;
    /* 0x004 */ void * () handler;
};
struct .150fake { /* NasMetrics, 72 bytes */
    /* 0x000 */ unsigned short LEVEL;
    /* 0x002 */ unsigned short TNas;
    /* 0x004 */ unsigned short TSpas;
    /* 0x006 */ unsigned short TTas;
    /* 0x008 */ unsigned short FNas;
    /* 0x00a */ unsigned short FSpas;
    /* 0x00c */ unsigned short FTas;
    /* 0x00e */ unsigned short LNas;
    /* 0x010 */ unsigned short LSpas;
    /* 0x012 */ unsigned short LTas;
    /* 0x014 */ unsigned short SpLMax;
    /* 0x016 */ unsigned short SpLNum;
    /* 0x018 */ struct .147fake * NFill;
    /* 0x01c */ struct .141fake * NProc;
    /* 0x020 */ struct .148fake * SFill;
    /* 0x024 */ struct .142fake * SProc;
    /* 0x028 */ struct .148fake * TFill;
    /* 0x02c */ struct .142fake * TProc;
    /* 0x030 */ struct .143fake * NSpec;
    /* 0x034 */ struct .145fake * NFink;
    /* 0x038 */ struct .144fake * SPJump;
    /* 0x03c */ struct .149fake * SXJump;
    /* 0x040 */ struct .146fake * LUPTab;
    /* 0x044 */ unsigned short * SpecBits;
};
struct .151fake { /* CamPath, 14 bytes */
    /* 0x000 */ short x;
    /* 0x002 */ short y;
    /* 0x004 */ short z;
    /* 0x006 */ short Ax;
    /* 0x008 */ short Ay;
    /* 0x00a */ short zoom;
    /* 0x00c */ short speed;
};
struct .152fake { /* FXPCamPath, 16 bytes */
    /* 0x000 */ long x;
    /* 0x004 */ long y;
    /* 0x008 */ long z;
    /* 0x00c */ long speed;
};
struct .153fake { /* LDBASE, 24 bytes */
    /* 0x000 */ unsigned short Xpos;
    /* 0x002 */ unsigned short Ypos;
    /* 0x004 */ unsigned char layer;
    /* 0x005 */ unsigned char map;
    /* 0x006 */ unsigned char dir;
    /* 0x008 */ unsigned long SprDbase;
    /* 0x00c */ unsigned short MapX;
    /* 0x00e */ unsigned short MapY;
    /* 0x010 */ unsigned short CamXpos;
    /* 0x012 */ unsigned short CamYpos;
    /* 0x014 */ unsigned short CamZpos;
    /* 0x016 */ unsigned short CamType;
};
struct .154fake { /* LvlDBASE, 28 bytes */
    /* 0x000 */ unsigned short * Loc1;
    /* 0x004 */ unsigned short * Loc2;
    /* 0x008 */ unsigned long Len1;
    /* 0x00c */ unsigned long Len2;
    /* 0x010 */ unsigned long Starts;
    /* 0x014 */ unsigned long SprDbase;
    /* 0x018 */ unsigned long SFXaddr;
};
struct .155fake { /* TIMING, 4 bytes */
    /* 0x000 */ unsigned short StartTime;
    /* 0x002 */ unsigned short EndTime;
};
struct .156fake { /* WORLDCOINS, 8 bytes */
    /* 0x000 */ int SmallCoins;
    /* 0x004 */ int LargeCoins;
};
enum dbs { /* , 4 bytes */
    Lev3DobjDB = 0,
    HerculesDB = 1,
    Xtra_DB1 = 2,
    TempDB = 3,
    tormentDB = 4,
    cyclopsDB = 5,
    peopleDB = 6,
    cyendDB = 7,
    lettersDB = 8,
    soulsDB = 9,
    MaxSprDbase = 10,
};
struct .157fake { /* SDBASE, 20 bytes */
    /* 0x000 */ unsigned short * Loc;
    /* 0x004 */ unsigned short frames;
    /* 0x008 */ unsigned long length;
    /* 0x00c */ unsigned long FRAMEAddress;
    /* 0x010 */ unsigned short Status;
    /* 0x012 */ unsigned short Type;
};
struct .158fake { /* WsType, 36 bytes */
    /* 0x000 */ unsigned short MaxLines;
    /* 0x002 */ unsigned short CurLines;
    /* 0x004 */ unsigned char Velocity;
    /* 0x005 */ unsigned char LineLenght;
    /* 0x006 */ unsigned char NumDrops;
    /* 0x007 */ unsigned char FadeType;
    /* 0x008 */ short X;
    /* 0x00a */ short Y;
    /* 0x00c */ short Z;
    /* 0x00e */ unsigned char R;
    /* 0x00f */ unsigned char G;
    /* 0x010 */ unsigned char B;
    /* 0x014 */ void * CalcPosFunc;
    /* 0x018 */ void * XyzFunc;
    /* 0x01c */ unsigned long * GfxTab;
    /* 0x020 */ unsigned short * PalAddr;
};
struct .159fake { /* WsInfo, 8 bytes */
    /* 0x000 */ struct .9fake Vec1;
};
struct .160fake { /* BGPOS, 12 bytes */
    /* 0x000 */ long x;
    /* 0x004 */ long y;
    /* 0x008 */ short w;
    /* 0x00a */ short h;
};
struct .161fake { /* SprtPAL, 4 bytes */
    /* 0x000 */ unsigned short Count;
    /* 0x002 */ unsigned short HashCode;
};
struct .162fake { /* SprtALLOC, 12 bytes */
    /* 0x000 */ unsigned short code;
    /* 0x002 */ unsigned short Tpage;
    /* 0x004 */ unsigned short link1;
    /* 0x006 */ unsigned short link2;
    /* 0x008 */ unsigned short link3;
    /* 0x00a */ unsigned short link4;
};
struct .163fake { /* SprtALLOCused, 2 bytes */
    /* 0x000 */ unsigned short index;
};
struct .164fake { /* DebugNAS, 12 bytes */
    /* 0x000 */ long Colour;
    /* 0x004 */ long Xpos;
    /* 0x008 */ long Ypos;
};
struct .165fake { /* SprtFRAME, 4 bytes */
    /* 0x000 */ unsigned short Count;
    /* 0x002 */ unsigned short Index;
};
struct .166fake { /* SprtHEADER, 8 bytes */
    /* 0x000 */ unsigned long Version;
    /* 0x004 */ unsigned short Frames;
    /* 0x006 */ unsigned short Palettes;
};
struct .167fake { /* SprtDATUM, 2 bytes */
    /* 0x000 */ unsigned char x;
    /* 0x001 */ unsigned char y;
};
struct .168fake { /* SprtCOLBOX, 6 bytes */
    /* 0x000 */ unsigned char x1;
    /* 0x001 */ unsigned char y1;
    /* 0x002 */ unsigned char z1;
    /* 0x003 */ unsigned char x2;
    /* 0x004 */ unsigned char y2;
    /* 0x005 */ unsigned char z2;
};
struct .169fake { /* SPRTSZE, 2 bytes */
    /* 0x000 */ short MinX : 4;
    /* 0x004 */ short MaxX : 4;
    /* 0x008 */ short MinY : 4;
    /* 0x00c */ short MaxY : 4;
};
struct .170fake { /* PA_UV_DATA, 3 bytes */
    /* 0x000 */ unsigned char x;
    /* 0x001 */ unsigned char y;
    /* 0x002 */ unsigned char w;
};
struct .171fake { /* MAPIndexTAB, 16 bytes */
    /* 0x000 */ long vx;
    /* 0x004 */ long vz;
    /* 0x008 */ short angle;
    /* 0x00a */ short angleIndex;
    /* 0x00c */ short index;
    /* 0x00e */ unsigned char flag;
    /* 0x00f */ unsigned char start;
};
struct .172fake { /* MAPindex, 4 bytes */
    /* 0x000 */ unsigned short Special;
    /* 0x002 */ unsigned short Light;
};
struct .173fake { /* TTstruct, 1 bytes */
    /* 0x000 */ char info;
};
struct .174fake { /* MAPmaxS, 4 bytes */
    /* 0x000 */ unsigned char Xmin;
    /* 0x001 */ unsigned char Xmax;
    /* 0x002 */ unsigned char Ymin;
    /* 0x003 */ unsigned char Ymax;
};
struct .175fake { /* MAPList, 16 bytes */
    /* 0x000 */ unsigned char MapLink;
    /* 0x001 */ unsigned char XposLink;
    /* 0x002 */ unsigned char DistIn;
    /* 0x003 */ unsigned char DistOut;
    /* 0x004 */ unsigned short Angle;
    /* 0x006 */ unsigned short MapList;
    /* 0x008 */ unsigned short MapConnect;
    /* 0x00a */ unsigned short MapTab;
    /* 0x00c */ unsigned short MapTrigSize;
    /* 0x00e */ unsigned short MapTrigger;
};
struct .176fake { /* , 428 bytes, incomplete */
    /* 0x000 */ long X;
    /* 0x004 */ long Y;
    /* 0x008 */ long Z;
    /* 0x00c */ long BlockX;
    /* 0x010 */ long BlockY;
    /* 0x014 */ long BlockZ;
    /* 0x018 */ long JudderX;
    /* 0x01c */ long JudderY;
    /* 0x020 */ long JudderZ;
    /* 0x024 */ long Xold;
    /* 0x028 */ long Yold;
    /* 0x02c */ long Zold;
    /* 0x030 */ long AngleX;
    /* 0x034 */ long AngleY;
    /* 0x038 */ long AngleZ;
    /* 0x03c */ long ViewX;
    /* 0x040 */ long ViewY;
    /* 0x044 */ long ViewZ;
    /* 0x048 */ struct .10fake AmbienceRGB;
    /* 0x04c */ unsigned long BgAmbience;
    /* 0x050 */ unsigned long Ambience;
    /* 0x054 */ unsigned char AmbienceLevel;
    /* 0x055 */ char RampXSpeed;
    /* 0x056 */ char RampYSpeed;
    /* 0x057 */ char RampZSpeed;
};
