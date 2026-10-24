char __thiscall sub_98CCC0(int *this)
{
  _DWORD *v1; // esi
  char *v3; // edi
  int v4; // esi
  char result; // al
  int v6; // edi
  int n4; // eax
  _DWORD *v8; // ecx
  unsigned int v9; // esi
  int n30; // eax
  int *n4_6; // eax
  int n4_1; // ecx
  int v14; // esi
  int v15; // edi
  int v16; // eax
  int v17; // esi
  int n30_1; // eax
  int v19; // eax
  int v20; // esi
  int v21; // ecx
  int v22; // edi
  int n4_4; // edx
  int n4_3; // eax
  int n24; // edi
  int v26; // esi
  int v27; // eax
  int v28; // esi
  int v29; // eax
  int v30; // eax
  int v31; // edx
  int n101; // eax
  int n100; // ecx
  int v34; // eax
  int v35; // eax
  int v36; // eax
  int v37; // eax
  char *n2; // eax
  _DWORD *v39; // edx
  _DWORD *v40; // ecx
  bool v41; // zf
  int v42; // [esp-10h] [ebp-60h]
  char *Buffer; // [esp-Ch] [ebp-5Ch]
  int v44; // [esp-8h] [ebp-58h]
  __int64 v45; // [esp+10h] [ebp-40h]
  int v46; // [esp+18h] [ebp-38h]
  __int128 v47; // [esp+24h] [ebp-2Ch]
  int v48; // [esp+38h] [ebp-18h] BYREF
  _DWORD *v49; // [esp+3Ch] [ebp-14h]
  int v50; // [esp+40h] [ebp-10h]
  int n4_5; // [esp+44h] [ebp-Ch]
  int n4_2; // [esp+48h] [ebp-8h]
  int v53; // [esp+4Ch] [ebp-4h]

  v1 = dword_BE4A1C; /*0x98ccc8*/
  v3 = (char *)dword_BE4A1C + 314712; /*0x98ccd7*/
  (*(void (__thiscall **)(char *, int))(*((_DWORD *)dword_BE4A1C + 78678) + 12))((char *)dword_BE4A1C + 314712, -1); /*0x98cce1*/
  v4 = v1[78677]; /*0x98cce8*/
  result = (*(int (__thiscall **)(char *))(*(_DWORD *)v3 + 16))(v3); /*0x98ccee*/
  if ( v4 ) /*0x98ccf3*/
    return result; /*0x98ccf3*/
  if ( *this == 2 ) /*0x98ccfe*/
  {
    sub_989970(this + 364); /*0x98cd06*/
  }
  else if ( *this == 5 ) /*0x98cd10*/
  {
    sub_8B6F80(this + 754); /*0x98cd18*/
  }
  if ( *((_DWORD *)dword_BE4A1C + 76782) == *((_DWORD *)dword_BE4A1C + 76783) ) /*0x98cd2e*/
    v6 = *(_DWORD *)(sub_4131B0(n2_0, this[599]) + 5644); /*0x98cd4d*/
  else
    v6 = sub_8E7380(); /*0x98cd35*/
  v53 = v6; /*0x98cd37*/
  n4 = *this; /*0x98cd56*/
  if ( *this )
  {
    if ( n4 != 1 ) /*0x98cdce*/
    {
      if ( n4 == 2 ) /*0x98d42e*/
      {
        if ( sub_9EE200(&off_BCE438, 15, v6, 0) ) /*0x98d43a*/
          *this = 1; /*0x98d443*/
      }
      else if ( n4 == 4 ) /*0x98d44e*/
      {
        sub_99AE70(this + 600); /*0x98d456*/
      }
      goto LABEL_111; /*0x98d449*/
    }
    if ( sub_9EE200(&off_BCE438, 12, v6, 0) || sub_9EE200(&off_BCE438, 15, v6, 0) ) /*0x98cdf5*/
    {
      sub_98EC00(this); /*0x98d413*/
      result = (char)dword_BE4A1C; /*0x98d418*/
      *((_BYTE *)dword_BE4A1C + 307114) = 1; /*0x98d41d*/
      return result; /*0x98d42a*/
    }
    v8 = dword_BE4A1C; /*0x98ce02*/
    if ( *((_DWORD *)dword_BE4A1C + 76782) != *((_DWORD *)dword_BE4A1C + 76783) ) /*0x98ce14*/
    {
      if ( sub_9EE200(&off_BCE438, 28, v6, 0) ) /*0x98ce20*/
      {
        if ( (unsigned int)((dword_BE746C - dword_BE7468) >> 3) > 1 ) /*0x98ce3a*/
        {
          this[754] = 0; /*0x98ce40*/
          *this = 5; /*0x98ce4a*/
        }
        goto LABEL_77; /*0x98ce50*/
      }
      v8 = dword_BE4A1C; /*0x98ce55*/
    }
    if ( v8[76782] == v8[76783]
      || *((_DWORD *)n2_0 + 38634)
      || !sub_9EE200(&off_BCE438, 24, v6, 0)
      || (v9 = (dword_BE746C - dword_BE7468) >> 3, v9 >= (*(int (__thiscall **)(int *))(dword_BE7458 + 8))(&dword_BE7458)) )
    {
      if ( sub_9EE200(&off_BCE438, 22, v6, 0) )
      {
        if ( --this[2] < 0 ) /*0x98ced3*/
          this[2] = 2; /*0x98cedd*/
      }
      else if ( sub_9EE200(&off_BCE438, 23, v6, 0) )
      {
        if ( ++this[2] > 2 ) /*0x98cf03*/
          this[2] = 0; /*0x98cf09*/
      }
      else if ( sub_9EE140(&off_BCE438, 20, v6, 0) )
      {
        n30 = this[10]; /*0x98cf2c*/
        if ( !n30 || n30 >= 30 && !(n30 % 10) ) /*0x98cf42*/
        {
          n4_6 = (int *)(sub_4131B0(n2_0, this[599]) + 7640); /*0x98cf5d*/
          n4_1 = 4; /*0x98cf62*/
          n4_5 = (int)n4_6; /*0x98cf67*/
          n4_2 = 4; /*0x98cf6a*/
          do /*0x98d003*/
          {
            v14 = this[9] - 1; /*0x98cf73*/
            this[9] = v14; /*0x98cf74*/
            if ( v14 > 0 ) /*0x98cf79*/
            {
              while ( 1 ) /*0x98cf80*/
              {
                v15 = *n4_6; /*0x98cf80*/
                v16 = (n4_6[1] - *n4_6) / 28 - v14; /*0x98cf9a*/
                v17 = 7 * v16; /*0x98cfa3*/
                if ( !*(_BYTE *)(v15 + 28 * v16 + 4) ) /*0x98cfa5*/
                {
                  if ( sub_72A400(*(_DWORD *)(v15 + 28 * v16 + 8)) ) /*0x98cfbc*/
                  {
                    sub_4131B0(n2_0, this[599]); /*0x98cfd1*/
                    if ( (unsigned __int8)sub_7699D0(*(_DWORD *)(v15 + 4 * v17 + 8), 1) ) /*0x98cfde*/
                      break; /*0x98cfde*/
                  }
                }
                v14 = this[9] - 1; /*0x98cfea*/
                this[9] = v14; /*0x98cfeb*/
                if ( v14 <= 0 ) /*0x98cff0*/
                  break; /*0x98cff0*/
                n4_6 = (int *)n4_5; /*0x98cff2*/
              }
              n4_6 = (int *)n4_5; /*0x98cff7*/
              n4_1 = n4_2; /*0x98cffa*/
            }
            n4_2 = --n4_1; /*0x98d000*/
          }
          while ( n4_1 ); /*0x98d003*/
          v6 = v53; /*0x98d00d*/
          if ( this[9] < 0 ) /*0x98d010*/
            this[9] = 0; /*0x98d016*/
        }
      }
      else if ( sub_9EE140(&off_BCE438, 21, v6, 0) )
      {
        n30_1 = this[10]; /*0x98d039*/
        if ( !n30_1 || n30_1 >= 30 && !(n30_1 % 10) )
        {
          v19 = sub_4131B0(n2_0, this[599]); /*0x98d065*/
          v20 = *(_DWORD *)(v19 + 7644); /*0x98d06a*/
          v49 = (_DWORD *)(v19 + 7640); /*0x98d075*/
          v21 = *(_DWORD *)(v19 + 7640); /*0x98d07a*/
          v22 = (v20 - v21) / 28; /*0x98d08f*/
          n4_4 = 0; /*0x98d091*/
          if ( v22 - 24 > 0 ) /*0x98d098*/
            n4_4 = v22 - 24; /*0x98d098*/
          n4_3 = 4; /*0x98d09b*/
          n4_2 = n4_4; /*0x98d0a0*/
          n4_5 = 4; /*0x98d0a3*/
          if ( v22 )
          {
            n24 = 0; /*0x98d0b1*/
            v26 = v20 - 28 * this[9]; /*0x98d0bf*/
            if ( v26 == v21 )
            {
              n4_3 = 4; /*0x98d131*/
            }
            else
            {
              do
              {
                if ( n24 >= 24 ) /*0x98d0c8*/
                  break; /*0x98d0c8*/
                v27 = *(_BYTE *)(v26 - 24) ? 0 : *(_DWORD *)(v26 - 20);
                v50 = v27; /*0x98d0de*/
                if ( sub_72A400(v27) && (sub_4131B0(n2_0, this[599]), (unsigned __int8)sub_7699D0(v50, 1)) ) /*0x98d108*/
                {
                  ++n24; /*0x98d111*/
                }
                else
                {
                  --n4_2; /*0x98d114*/
                  if ( n24 < n4_5 ) /*0x98d11c*/
                    ++n4_5; /*0x98d11f*/
                }
                v26 -= 28; /*0x98d125*/
              }
              while ( v26 != *v49 );
              n4_3 = n4_5; /*0x98d12c*/
            }
            n4_4 = n4_2; /*0x98d136*/
          }
          v28 = this[9]; /*0x98d139*/
          v6 = v53; /*0x98d13c*/
          this[9] = v28 + n4_3; /*0x98d145*/
          if ( v28 + n4_3 > n4_4 + 3 ) /*0x98d14a*/
          {
            v29 = 0; /*0x98d14c*/
            if ( v28 > 0 ) /*0x98d150*/
              v29 = v28; /*0x98d150*/
            this[9] = v29; /*0x98d153*/
          }
        }
      }
    }
    else
    {
      sub_99A4A0(); /*0x98ceb0*/
      *this = 4; /*0x98ceb5*/
    }
LABEL_77:
    if ( *((_DWORD *)dword_BE4A1C + 76782) != *((_DWORD *)dword_BE4A1C + 76783) ) /*0x98d168*/
    {
      v30 = sub_4131B0(n2_0, 0); /*0x98d172*/
      if ( (unsigned __int8)sub_7C5980(v30) ) /*0x98d179*/
      {
        if ( *((_DWORD *)dword_BE4A1C + 76606) && sub_9EE200(&off_BCE438, 29, v6, 0) ) /*0x98d19a*/
          *((_BYTE *)this + 3588) = *((_BYTE *)this + 3588) == 0; /*0x98d1ad*/
      }
    }
    v31 = this[1]; /*0x98d1b3*/
    n101 = 101; /*0x98d1b6*/
    if ( v31 != 1 ) /*0x98d1c8*/
      n101 = 32; /*0x98d1c8*/
    if ( this[n101] ) /*0x98d1cb*/
    {
      n100 = 100; /*0x98d1e1*/
      if ( v31 != 1 ) /*0x98d1e3*/
        n100 = 31; /*0x98d1e3*/
      sub_4042A0(&this[n100]); /*0x98d1e8*/
    }
    if ( sub_9EE200(&off_BCE438, 14, v6, 0) ) /*0x98d1f7*/
    {
      v34 = this[2]; /*0x98d204*/
      if ( v34 ) /*0x98d20a*/
      {
        v35 = v34 - 1; /*0x98d210*/
        if ( !v35 ) /*0x98d213*/
          return sub_98EC00(this); /*0x98d3e5*/
        if ( v35 == 1 ) /*0x98d21c*/
        {
          if ( *((_DWORD *)n2_0 + 38634) ) /*0x98d229*/
          {
            v44 = sub_9319D0((char *)dword_BE4A1C + 304668, "#QUIT_DAILY_RUN_4"); /*0x98d24f*/
            Buffer = (char *)sub_9319D0((char *)dword_BE4A1C + 304668, "#QUIT_DAILY_RUN_3"); /*0x98d265*/
            v42 = sub_9319D0((char *)dword_BE4A1C + 304668, "#QUIT_DAILY_RUN_2"); /*0x98d27f*/
            v36 = sub_9319D0((char *)dword_BE4A1C + 304668, "#QUIT_DAILY_RUN_1"); /*0x98d290*/
            sub_82CBB0(v36, v42, Buffer, v44, 0); /*0x98d29c*/
            sub_82CA70(0); /*0x98d2a9*/
            *this = 3; /*0x98d2ae*/
          }
          else
          {
            sub_403DE0(0); /*0x98d2c8*/
            if ( *(_BYTE *)(sub_4131B0(n2_0, 0) + 371) ) /*0x98d2da*/
            {
              v37 = sub_4131B0(n2_0, 0); /*0x98d2eb*/
              if ( sub_7B67A0(v37) < 1 ) /*0x98d2fa*/
                sub_9324A0(); /*0x98d2fc*/
            }
            sub_9E35B0(-16777216); /*0x98d309*/
            n2 = (char *)n2_0; /*0x98d30e*/
            v39 = dword_BE4A1C; /*0x98d318*/
            *(_QWORD *)&v47 = v45; /*0x98d31e*/
            DWORD2(v47) = v46; /*0x98d332*/
            HIDWORD(v47) = *((_DWORD *)n2_0 + 38602); /*0x98d33f*/
            *((_DWORD *)n2_0 + 38604) = 1034147594; /*0x98d348*/
            *((_DWORD *)n2 + 38605) = 2; /*0x98d352*/
            *(_OWORD *)(n2 + 154396) = v47; /*0x98d35c*/
            *((_DWORD *)n2 + 38603) = 0; /*0x98d363*/
            *((float *)n2 + 38602) = *((float *)n2 + 38602) + 0.079999998; /*0x98d37d*/
            if ( v39[76782] != v39[76783] && (*(int (__thiscall **)(int *))(dword_BE7458 + 40))(&dword_BE7458) == 3 ) /*0x98d3a7*/
            {
              v49 = (_DWORD *)dword_BE7464; /*0x98d3b8*/
              v48 = *(_DWORD *)dword_BE7460; /*0x98d3c1*/
              sub_8E8560(&v48); /*0x98d3ce*/
            }
          }
        }
      }
      else
      {
        v40 = this + 101; /*0x98d3f0*/
        if ( this[1] != 1 ) /*0x98d3f6*/
          v40 = this + 32; /*0x98d3f6*/
        if ( *v40 ) /*0x98d3f9*/
        {
          *this = 2; /*0x98d404*/
          sub_98C610(this + 364); /*0x98d40a*/
        }
      }
    }
    goto LABEL_111; /*0x98d2b4*/
  }
  if ( (v6 || !*((_DWORD *)dword_BE4A1C + 77948) && *((int *)dword_BE4A1C + 77972) <= 0) /*0x98cdb3*/
    && ((unsigned __int8)sub_92F6D0(12, v6, 0, 0)
     || !v6 && (unsigned __int8)sub_92F6D0(15, 0, 0, 0) && !*((_DWORD *)n2_0 + 37281)) )
  {
    sub_98ED80(this); /*0x98cdc1*/
  }
LABEL_111:
  sub_82C440(1); /*0x98d45b*/
  if ( (!this[300] || !*((_BYTE *)this + 1216)) && *this == 3 ) /*0x98d47d*/
  {
    v41 = this[362] == 1; /*0x98d47f*/
    *this = 1; /*0x98d486*/
    if ( v41 ) /*0x98d48c*/
      sub_6F5920(n2_0, 1); /*0x98d496*/
  }
  result = sub_9EE140(&off_BCE438, 21, -1, 0); /*0x98d4a6*/
  if ( result || (result = sub_9EE140(&off_BCE438, 20, -1, 0)) != 0 ) /*0x98d4c1*/
    ++this[10]; /*0x98d4d1*/
  else
    this[10] = 0; /*0x98d4c3*/
  return result; /*0x98d3df*/
}
