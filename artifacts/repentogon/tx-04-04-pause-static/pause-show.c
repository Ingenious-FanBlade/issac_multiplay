int __thiscall sub_98ED80(_DWORD *this)
{
  _DWORD *v2; // ecx
  int v3; // eax
  int v4; // eax
  unsigned int v5; // edx
  _DWORD *v6; // esi
  unsigned int v7; // ecx
  bool v8; // zf
  char n64; // bl
  int n80; // esi
  _DWORD *v11; // esi
  char *v12; // esi
  int v13; // eax
  int v14; // eax
  float *v15; // esi
  int v16; // edx
  _DWORD *v17; // eax
  int v18; // ecx
  int v19; // eax
  bool v20; // al
  int result; // eax
  int n17; // [esp+20h] [ebp-4h] BYREF

  v2 = dword_BE4A1C; /*0x98ed8b*/
  this[1] = *((_DWORD *)dword_BE4A1C + 76782) != *((_DWORD *)dword_BE4A1C + 76783); /*0x98eda0*/
  this[599] = 0; /*0x98eda3*/
  if ( v2[76782] != v2[76783] ) /*0x98edb9*/
  {
    v3 = sub_8E74C0(v2 + 76782); /*0x98edc1*/
    v4 = sub_420D10(v3); /*0x98edc8*/
    v5 = 0; /*0x98edd3*/
    v6 = (_DWORD *)*((_DWORD *)n2_0 + 28330); /*0x98edd5*/
    v7 = (*((_DWORD *)n2_0 + 28331) - (int)v6) >> 2; /*0x98ede3*/
    if ( v7 ) /*0x98ede8*/
    {
      while ( *v6 != v4 ) /*0x98edf2*/
      {
        ++v5; /*0x98edf4*/
        ++v6; /*0x98edf5*/
        if ( v5 >= v7 ) /*0x98edfa*/
          goto LABEL_5; /*0x98edfa*/
      }
    }
    else
    {
LABEL_5:
      v5 = -1; /*0x98edfc*/
    }
    v2 = dword_BE4A1C; /*0x98edff*/
    this[599] = v5; /*0x98ee05*/
  }
  sub_908210(v2 + 43096); /*0x98ee11*/
  sub_6F0540(); /*0x98ee16*/
  v8 = this[1] == 1; /*0x98ee1b*/
  n64 = 64; /*0x98ee1f*/
  *this = 1; /*0x98ee29*/
  n80 = 80; /*0x98ee2f*/
  this[2] = 1; /*0x98ee31*/
  if ( !v8 ) /*0x98ee38*/
    n80 = 11; /*0x98ee38*/
  v11 = &this[n80]; /*0x98ee3d*/
  if ( (unsigned __int8)sub_405F50((char)"Appear", 1) ) /*0x98ee46*/
  {
    sub_405B30(v11 + 12); /*0x98ee52*/
    *((_BYTE *)v11 + 68) = 1; /*0x98ee57*/
  }
  v12 = (char *)dword_BE4A1C; /*0x98ee5b*/
  if ( *((_DWORD *)dword_BE4A1C + 76782) == *((_DWORD *)dword_BE4A1C + 76783) ) /*0x98ee6d*/
  {
    v14 = sub_4131B0(n2_0, this[599]); /*0x98eea2*/
LABEL_16:
    this[528] = *(_DWORD *)(v14 + 5052); /*0x98eea7*/
    goto LABEL_17; /*0x98eead*/
  }
  if ( sub_8E74C0((char *)dword_BE4A1C + 307128) ) /*0x98ee75*/
  {
    v13 = sub_8E74C0(v12 + 307128); /*0x98ee84*/
    v14 = sub_420D10(v13); /*0x98ee8b*/
    if ( v14 ) /*0x98ee92*/
      goto LABEL_16; /*0x98ee92*/
  }
LABEL_17:
  n17 = 17; /*0x98eeb3*/
  sub_92FAD0(&n17); /*0x98eec3*/
  sub_907C60(n17, 1060320051, 2, 0, 1065353216, 0); /*0x98eeef*/
  v15 = (float *)sub_4131B0(n2_0, this[599]); /*0x98ef05*/
  this[3] = (int)fminf(fmaxf((float)(v15[1367] * 4.5) - 2.0, 0.0), 7.0); /*0x98ef3a*/
  this[4] = (int)fminf(fmaxf((float)(sub_4D1370().m128_f32[0] * 2.1203909) - 2.0, 0.0), 7.0); /*0x98ef89*/
  this[5] = (int)fminf(fmaxf((float)(sub_4D1370().m128_f32[0] * 2.231179) - 2.0, 0.0), 7.0); /*0x98efce*/
  this[6] = (int)fminf(fmaxf((float)((float)(v15[1311] - 230.0) / 60.0) + 2.0, 0.0), 7.0); /*0x98effd*/
  this[7] = (int)fminf(fmaxf((float)(v15[1304] * 5.0) - 2.5, 0.0), 7.0); /*0x98f024*/
  this[8] = (int)fminf(fmaxf(v15[1368] + 2.5, 0.0), 7.0); /*0x98f04c*/
  n17 = sub_406C50("Invite"); /*0x98f054*/
  v16 = sub_406C50("Kick"); /*0x98f066*/
  v17 = dword_BE4A1C; /*0x98f068*/
  if ( n17 ) /*0x98f06f*/
    *(_BYTE *)(n17 + 116) = *((_DWORD *)dword_BE4A1C + 76782) != *((_DWORD *)dword_BE4A1C + 76783); /*0x98f080*/
  if ( v16 ) /*0x98f085*/
  {
    v18 = v17[76782]; /*0x98f087*/
    v19 = v17[76783]; /*0x98f08d*/
    v20 = v18 != v19 && (unsigned int)((v19 - v18) >> 2) > 1; /*0x98f0a5*/
    *(_BYTE *)(v16 + 116) = v20; /*0x98f0a7*/
  }
  if ( this[1] != 1 ) /*0x98f0b5*/
    n64 = 44; /*0x98f0b5*/
  result = sub_406B60((_BYTE)this + n64, 4); /*0x98f0bb*/
  *(_BYTE *)(result + 116) = *((_DWORD *)n2_0 + 38864) == 1; /*0x98f0d0*/
  this[9] = 0; /*0x98f0d3*/
  return result; /*0x98f0da*/
}
