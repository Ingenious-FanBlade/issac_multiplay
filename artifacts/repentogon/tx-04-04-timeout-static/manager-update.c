int __stdcall sub_92E3E0(char a1)
{
  char *v1; // edi
  int v2; // ecx
  int n2_3; // eax
  int n3; // eax
  int v5; // eax
  __int128 v6; // xmm0
  int v7; // ecx
  bool v8; // zf
  int v9; // eax
  char *n2; // eax
  unsigned int n2_1; // edi
  char *v12; // esi
  float v13; // xmm1_4
  float v14; // xmm0_4
  float v15; // xmm0_4
  char *v16; // edi
  int v17; // eax
  int v18; // ecx
  _DWORD *v19; // esi
  char *n2_2; // esi
  int v21; // eax
  int *n2_4; // eax
  unsigned int v23; // esi
  int v24; // ecx
  int v25; // edx
  _DWORD *n2_5; // esi
  int v27; // esi
  _DWORD *n2_6; // esi
  int n4; // edx
  float *v30; // ecx
  float v31; // xmm5_4
  float v32; // xmm4_4
  float i_1; // xmm1_4
  float v34; // xmm2_4
  float i; // xmm0_4
  float v36; // xmm0_4
  bool v37; // cl
  float v38; // xmm1_4
  float v39; // xmm3_4
  float v40; // xmm2_4
  int v41; // eax
  const char *Clicked; // edx
  char v43; // fl
  double v44; // xmm0_8
  double v45; // xmm1_8
  float v46; // xmm1_4
  float v47; // xmm0_4
  bool v48; // cf
  char v49; // sf
  char v50; // of
  char v51; // pf
  int ArgList; // [esp+4h] [ebp-470h]
  int v53; // [esp+4h] [ebp-470h]
  int v54; // [esp+4h] [ebp-470h]
  int ArgList_1; // [esp+4h] [ebp-470h]
  char *v56; // [esp+Ch] [ebp-468h]
  char *v57; // [esp+10h] [ebp-464h]
  int v58; // [esp+10h] [ebp-464h]
  __int128 v59; // [esp+14h] [ebp-460h]
  __int64 v60; // [esp+40h] [ebp-434h]
  int v61; // [esp+48h] [ebp-42Ch]
  char v62[4]; // [esp+54h] [ebp-420h] BYREF
  _QWORD v63[2]; // [esp+5Ch] [ebp-418h]
  CHAR PathName[1028]; // [esp+6Ch] [ebp-408h] BYREF

  v1 = (char *)dword_BE4A1C; /*0x92e3fc*/
  v57 = (char *)dword_BE4A1C; /*0x92e402*/
  v2 = *((_DWORD *)dword_BE4A1C + 76334); /*0x92e406*/
  n2_3 = v2 % 2; /*0x92e419*/
  if ( v2 % 2 == 1 && !*((_BYTE *)dword_BE4A1C + 172539) ) /*0x92e41f*/
  {
    if ( !*((_BYTE *)dword_BE4A1C + 305344) || *((_BYTE *)dword_BE4A1C + 305345) ) /*0x92e431*/
    {
      n2_3 = v2 + 1; /*0x92e43e*/
      *((_DWORD *)dword_BE4A1C + 76334) = v2 + 1; /*0x92e441*/
    }
    return n2_3; /*0x92e45a*/
  }
  if ( *(_BYTE *)dword_BE4A1C ) /*0x92e45d*/
  {
    *((_DWORD *)dword_BE4A1C + 2) = 4; /*0x92e462*/
    *v1 = 0; /*0x92e469*/
  }
  sub_932A20(); /*0x92e46e*/
  sub_933060(v1); /*0x92e475*/
  if ( v1[136328] ) /*0x92e47a*/
  {
    if ( v1[136336] ) /*0x92e483*/
    {
      v1[306788] = 0; /*0x92e48e*/
      sub_932970(v1); /*0x92e495*/
    }
    sub_937BB0(v1 + 134216, *((_DWORD *)v1 + 34083)); /*0x92e4a6*/
    *((_DWORD *)v1 + 3) = *((_DWORD *)v1 + 2); /*0x92e4ae*/
    n3 = 3; /*0x92e4b1*/
    *((_DWORD *)v1 + 2) = 3; /*0x92e4b6*/
    v1[136328] = 0; /*0x92e4bd*/
  }
  else
  {
    n3 = *((_DWORD *)v1 + 2); /*0x92e4c6*/
  }
  if ( v1[306820] ) /*0x92e4c9*/
  {
    sub_8FBE00((unsigned __int8)v1[306821]); /*0x92e4e0*/
    *((_DWORD *)v1 + 3) = *((_DWORD *)v1 + 2); /*0x92e4e8*/
    n3 = 5; /*0x92e4eb*/
    *((_DWORD *)v1 + 2) = 5; /*0x92e4f0*/
    v1[306820] = 0; /*0x92e4f7*/
  }
  else
  {
    if ( n3 == 3 ) /*0x92e503*/
    {
      if ( *((_DWORD *)v1 + 33554) ) /*0x92e505*/
        goto LABEL_28; /*0x92e50c*/
      goto LABEL_20; /*0x92e50c*/
    }
    if ( n3 != 5 ) /*0x92e516*/
      goto LABEL_28; /*0x92e516*/
  }
  if ( *((_DWORD *)v1 + 34086) ) /*0x92e51c*/
    goto LABEL_28; /*0x92e523*/
LABEL_20:
  v1[171508] = 0; /*0x92e529*/
  if ( n3 == 3 && v1[136336] ) /*0x92e535*/
  {
    *((_DWORD *)v1 + 2) = *((_DWORD *)v1 + 3); /*0x92e541*/
    v5 = 76 * *((_DWORD *)v1 + 34068); /*0x92e544*/
    v6 = *(_OWORD *)&v1[v5 + 134264]; /*0x92e54b*/
    v7 = *(_DWORD *)&v1[v5 + 134280]; /*0x92e553*/
    *((_DWORD *)v1 + 76699) = *((_DWORD *)v1 + 34085); /*0x92e560*/
    *((_OWORD *)v1 + 19175) = v6; /*0x92e566*/
    *((_DWORD *)v1 + 76704) = v7; /*0x92e56d*/
    v1[306792] = 1; /*0x92e575*/
    sub_933060(v1); /*0x92e57c*/
  }
  else
  {
    v8 = v1[307208] == 0; /*0x92e586*/
    v9 = *((_DWORD *)v1 + 3); /*0x92e58d*/
    *((_DWORD *)v1 + 2) = v9; /*0x92e590*/
    if ( !v8 ) /*0x92e593*/
    {
      sub_92E260(); /*0x92e599*/
      sub_9E35B0(-16777216); /*0x92e5a7*/
      n2 = (char *)n2_0; /*0x92e5ac*/
      *(_QWORD *)&v59 = v60; /*0x92e5b7*/
      DWORD2(v59) = v61; /*0x92e5cf*/
      HIDWORD(v59) = *((_DWORD *)n2_0 + 38602); /*0x92e5dd*/
      *((_DWORD *)n2_0 + 38604) = 1065353216; /*0x92e5e8*/
      *((_DWORD *)n2 + 38605) = 8; /*0x92e5f2*/
      *(_OWORD *)(n2 + 154396) = v59; /*0x92e5fc*/
      *((_DWORD *)n2 + 38603) = 0; /*0x92e603*/
      *((float *)n2 + 38602) = *((float *)n2 + 38602) + 1.0; /*0x92e61d*/
      v9 = *((_DWORD *)v1 + 2); /*0x92e625*/
    }
    if ( v9 == 1 ) /*0x92e62b*/
    {
      sub_9E0DF0(0, "Cutscene End\n", ArgList); /*0x92e633*/
      sub_7D8480((char *)dword_BE4A1C + 171512, 63, 1034147594); /*0x92e650*/
    }
  }
  v1[307208] = 0; /*0x92e655*/
LABEL_28:
  if ( a1 ) /*0x92e660*/
  {
    if ( byte_BE65A5 ) /*0x92e669*/
      sub_A2A390(); /*0x92e67c*/
    else
      sub_A2F820(65537, 0, ArgList); /*0x92e672*/
    sub_A31960(); /*0x92e681*/
    sub_9EDCC0(&off_BCE438); /*0x92e68b*/
  }
  sub_9346B0(ArgList); /*0x92e692*/
  sub_999990(v53); /*0x92e69d*/
  v56 = v1 + 304716; /*0x92e6a8*/
  sub_41FC20(); /*0x92e6ac*/
  n2_1 = 0; /*0x92e6b5*/
  v12 = v56 + 276; /*0x92e6bf*/
  do /*0x92e715*/
  {
    v13 = *(float *)v12; /*0x92e6d0*/
    v14 = *((float *)v12 + 1); /*0x92e6d4*/
    if ( *(float *)v12 <= v14 ) /*0x92e6dc*/
      v15 = fminf(v14, v13 + 6.0); /*0x92e6ec*/
    else
      v15 = fmaxf(v14, v13 - 6.0); /*0x92e6e2*/
    v8 = *((_DWORD *)v12 + 4) == 31; /*0x92e6f0*/
    *(float *)v12 = v15; /*0x92e6f4*/
    if ( !v8 ) /*0x92e6f8*/
      sub_41FB30(v62, n2_1); /*0x92e702*/
    ++n2_1; /*0x92e70b*/
    v12 += 304; /*0x92e70c*/
  }
  while ( n2_1 < 2 ); /*0x92e715*/
  v16 = v57; /*0x92e717*/
  v57[305345] = 0; /*0x92e71b*/
  if ( !*((_DWORD *)v57 + 77948) && *((int *)v57 + 77972) <= 0 ) /*0x92e732*/
  {
    sub_92E030(v57); /*0x92e736*/
    v17 = sub_9EEC40(0); /*0x92e742*/
    if ( v17 ) /*0x92e74b*/
    {
      if ( (*(unsigned __int8 (__thiscall **)(int, int))(*(_DWORD *)v17 + 24))(v17, 298) ) /*0x92e757*/
      {
        sub_41B800(v18, PathName, "%sscreenshots", (char)&::PathName); /*0x92e76c*/
        CreateDirectoryA(PathName, 0); /*0x92e77b*/
        sub_9BBDA0(v54); /*0x92e786*/
      }
    }
  }
  v19 = v57 + 307128; /*0x92e78b*/
  n2_3 = sub_8E75A0(v54); /*0x92e793*/
  if ( !(_BYTE)n2_3 ) /*0x92e79a*/
  {
    n2_3 = *((_DWORD *)v57 + 2); /*0x92e79c*/
    if ( n2_3 == 2 || n2_3 == 5 ) /*0x92e7a7*/
    {
      if ( n2_0 ) /*0x92e7b1*/
      {
        sub_686850((int)n2_0 + 425940); /*0x92e7b9*/
        n2_2 = (char *)n2_0; /*0x92e7be*/
        sub_687020((int *)n2_0 + 106485); /*0x92e7ca*/
        n2_3 = sub_6F58E0(n2_2); /*0x92e7d1*/
        if ( (_BYTE)n2_3 ) /*0x92e7d8*/
        {
          sub_98CCC0(n2_2 + 144956); /*0x92e7e4*/
          return sub_98D4E0((char *)n2_0 + 144956); /*0x92e7f5*/
        }
        return n2_3; /*0x92e80d*/
      }
      v19 = v57 + 307128; /*0x92e810*/
    }
  }
  if ( v57[305344] && !v57[305345] ) /*0x92e826*/
    return n2_3; /*0x92e826*/
  v21 = *((_DWORD *)v57 + 34474); /*0x92e82c*/
  if ( !v21 ) /*0x92e83a*/
    goto LABEL_61; /*0x92e83a*/
  if ( !v57[171508] ) /*0x92e83c*/
  {
    sub_90B780(v57 + 137868); /*0x92e845*/
    v21 = *((_DWORD *)v57 + 34474); /*0x92e84a*/
  }
  if ( !v21 || v57[171508] || *((_DWORD *)v57 + 43117) != 1 || *v19 != v19[1] ) /*0x92e86b*/
  {
LABEL_61:
    switch ( *((_DWORD *)v57 + 2) ) /*0x92e87e*/
    {
      case 1: /*0x92e87e*/
        v37 = (unsigned __int8)sub_999E10(v57 + 314704) == 0; /*0x92ea98*/
        if ( (*((_DWORD *)dword_BE5998 + 16) == 17 || v37) && !(unsigned __int8)sub_929E50(v19) ) /*0x92eaa7*/
          sub_961CB0(); /*0x92eab0*/
        break; /*0x92eab5*/
      case 2: /*0x92e87e*/
        if ( !(unsigned __int8)sub_999E10(v57 + 314704) && !(unsigned __int8)sub_929E50(v19) ) /*0x92e896*/
          sub_6F66E0(); /*0x92e89f*/
        if ( v57[306448] ) /*0x92e8a4*/
          break; /*0x92e8ab*/
        if ( (v57[305336] & 1) != 0 ) /*0x92e8b8*/
        {
          if ( v57[172539] ) /*0x92e918*/
            sub_6F9340(); /*0x92e921*/
        }
        else
        {
          n2_4 = (int *)n2_0; /*0x92e8ba*/
          v23 = 0; /*0x92e8bf*/
          v24 = *((_DWORD *)n2_0 + 24768); /*0x92e8c1*/
          if ( *(_DWORD *)(v24 + 4708) ) /*0x92e8c7*/
          {
            do /*0x92e908*/
            {
              v25 = *(_DWORD *)(*(_DWORD *)(v24 + 4700) + 4 * v23); /*0x92e8d6*/
              if ( *(_BYTE *)(v25 + 373) ) /*0x92e8d9*/
              {
                *(_DWORD *)(v25 + 828) = *(_DWORD *)(v25 + 836); /*0x92e8e8*/
                *(_DWORD *)(v25 + 832) = *(_DWORD *)(v25 + 840); /*0x92e8f4*/
                *(_BYTE *)(v25 + 373) = 0; /*0x92e8fa*/
              }
              ++v23; /*0x92e901*/
            }
            while ( v23 < *(_DWORD *)(v24 + 4708) ); /*0x92e908*/
            n2_4 = (int *)n2_0; /*0x92e90a*/
          }
          sub_6F6D30(n2_4); /*0x92e911*/
        }
        n2_5 = n2_0; /*0x92e92c*/
        LOBYTE(v56) = v57[305336] & 1; /*0x92e934*/
        v58 = *(_DWORD *)(*((_DWORD *)n2_0 + 24768) + 4600); /*0x92e946*/
        if ( (unsigned __int8)sub_6F92A0(n2_0) && (!n2_5[28175] || n2_5[28182] != 19) ) /*0x92e963*/
          goto LABEL_83; /*0x92e963*/
        if ( *((_DWORD *)dword_BE4A1C + 43119) == 2 ) /*0x92e973*/
        {
          v27 = v58; /*0x92e97a*/
          sub_91E5E0(v58); /*0x92e980*/
          if ( !(_BYTE)v56 ) /*0x92e98a*/
            goto LABEL_83; /*0x92e98a*/
        }
        else
        {
          v27 = v58; /*0x92e992*/
          sub_91D840(v56); /*0x92e998*/
        }
        *(_BYTE *)(v27 + 140) = 0; /*0x92e99d*/
LABEL_83:
        n2_6 = n2_0; /*0x92e9a4*/
        n4 = 4; /*0x92e9aa*/
        v30 = (float *)((char *)n2_0 + 113332); /*0x92e9c7*/
        do /*0x92ea7e*/
        {
          if ( *(int *)v30 >= 0 ) /*0x92e9d3*/
          {
            v31 = v30[3]; /*0x92e9ea*/
            v32 = (float)*((int *)v30 + 2); /*0x92e9f2*/
            i_1 = v32 - v31; /*0x92ea0c*/
            v34 = (double)((n2_6[28359] - n2_6[28358]) >> 2) /*0x92ea10*/
                + qword_B42810[(unsigned int)(n2_6[28359] - n2_6[28358]) >> 31];
            if ( (float)(v32 - v31) >= (float)(v34 * 0.5) ) /*0x92ea1e*/
            {
              do /*0x92ea27*/
                i_1 = i_1 - v34; /*0x92ea20*/
              while ( i_1 >= (float)(v34 * 0.5) ); /*0x92ea27*/
            }
            for ( i = (float)-v34 * 0.5; i > i_1; i_1 = i_1 + v34 ) /*0x92ea3a*/
              ; /*0x92ea40*/
            if ( fabs(i_1) >= 0.25 ) /*0x92ea56*/
            {
              if ( i_1 <= 0.0 ) /*0x92ea65*/
                v36 = -0.25; /*0x92ea6c*/
              else
                v36 = 0.25; /*0x92ea67*/
              v30[3] = v36 + v31; /*0x92ea73*/
            }
            else
            {
              v30[3] = v32; /*0x92ea58*/
            }
          }
          v30 += 5; /*0x92ea78*/
          --n4; /*0x92ea7b*/
        }
        while ( n4 ); /*0x92ea7e*/
        break; /*0x92ea7e*/
      case 3: /*0x92e87e*/
        sub_9373D0(ArgList_1); /*0x92eabd*/
        break; /*0x92eac2*/
      case 5: /*0x92e87e*/
        sub_8FA880(v57 + 136344); /*0x92eaca*/
        break; /*0x92eaca*/
      default:
        break;
    }
  }
  if ( n2_0 ) /*0x92ead7*/
  {
    if ( !(unsigned __int8)sub_6F92A0(n2_0) ) /*0x92eadb*/
    {
      v38 = *((float *)v16 + 43100); /*0x92eae4*/
      v39 = 1.0; /*0x92eaf2*/
      if ( v38 != 1.0 ) /*0x92eb01*/
      {
        v40 = *((float *)v16 + 43101); /*0x92eb03*/
        if ( v40 != 0.0 ) /*0x92eb12*/
        {
          if ( v40 < fabs(v38 - 1.0) ) /*0x92eb25*/
          {
            if ( v38 >= 1.0 ) /*0x92eb2d*/
              v39 = v38 - v40; /*0x92eb35*/
            else
              v39 = v38 + v40; /*0x92eb2f*/
          }
          *((float *)v16 + 43100) = v39; /*0x92eb39*/
          sub_908340(); /*0x92eb3e*/
        }
      }
    }
  }
  n2_3 = sub_7D7DA0(v16 + 171512); /*0x92eb49*/
  if ( v16[172504] ) /*0x92eb4e*/
  {
    v41 = sub_9F26E0(dword_BAE270[0]); /*0x92eb63*/
    Clicked = "Clicked"; /*0x92eb6f*/
    if ( !v41 ) /*0x92eb74*/
      Clicked = "Idle"; /*0x92eb74*/
    sub_405D00(Clicked, 0); /*0x92eb80*/
    n2_3 = *(_DWORD *)dword_BAE270; /*0x92eb8c*/
    if ( byte_BE65A5 ) /*0x92eb91*/
    {
      v45 = *(double *)(*(_DWORD *)dword_BAE270 + 72); /*0x92ebb0*/
      v44 = *(double *)(*(_DWORD *)dword_BAE270 + 80); /*0x92ebb5*/
    }
    else
    {
      n2_3 = sub_A2F820(65537, 0, ArgList_1); /*0x92eb9a*/
      v44 = *(double *)v63; /*0x92eb9f*/
      v45 = *(double *)&v63[1]; /*0x92eba8*/
    }
    v46 = v45; /*0x92ebba*/
    v47 = v44; /*0x92ebbe*/
    v48 = v46 < *((float *)v16 + 76775); /*0x92ebc2*/
    v51 = 0; /*0x92ebc2*/
    v8 = v46 == *((float *)v16 + 76775); /*0x92ebc2*/
    v49 = 0; /*0x92ebc2*/
    v50 = 0; /*0x92ebc2*/
    BYTE1(n2_3) = v43; /*0x92ebc9*/
    if ( v46 == *((float *)v16 + 76775) /*0x92ebda*/
      && (v48 = v47 < *((float *)v16 + 76776),
          v51 = 0,
          v8 = v47 == *((float *)v16 + 76776),
          v49 = 0,
          v50 = 0,
          BYTE1(n2_3) = v43,
          v47 == *((float *)v16 + 76776)) )
    {
      n2_3 = *((_DWORD *)v16 + 76777); /*0x92ebdc*/
      if ( n2_3 > 0 ) /*0x92ebe4*/
        *((_DWORD *)v16 + 76777) = --n2_3; /*0x92ebe7*/
    }
    else
    {
      *((_DWORD *)v16 + 76777) = 300; /*0x92ebef*/
    }
    *((float *)v16 + 76775) = v46; /*0x92ebf9*/
    *((float *)v16 + 76776) = v47; /*0x92ec01*/
  }
  ++*((_DWORD *)v16 + 76334); /*0x92ec09*/
  return n2_3; /*0x92e447*/
}
