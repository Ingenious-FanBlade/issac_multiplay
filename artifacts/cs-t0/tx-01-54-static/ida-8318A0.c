_DWORD *__thiscall sub_8318A0(_DWORD *this)
{
  int v2; // ecx
  size_t Size_1; // edi
  int v4; // eax
  _DWORD *n60_5; // eax
  int v6; // ecx
  int n19; // ecx
  int v8; // edi
  unsigned int i; // esi
  int v10; // ecx
  int *v11; // esi
  int v12; // edi
  int v13; // eax
  int v14; // eax
  int n604; // eax
  int n19_1; // eax
  int n1000_1; // eax
  _DWORD *ArgList; // eax
  int v19; // ecx
  int v20; // xmm1_4
  float *v21; // esi
  int v22; // ecx
  float v23; // xmm2_4
  float v24; // xmm1_4
  int v25; // eax
  int v26; // edi
  int v27; // eax
  int n2; // esi
  int v29; // eax
  float *v30; // eax
  float v31; // xmm2_4
  float v32; // xmm1_4
  int v33; // xmm1_4
  _DWORD *n60_1; // edi
  int *v35; // eax
  int v36; // ecx
  bool v37; // zf
  _DWORD *n60; // edi
  int *v39; // eax
  size_t Size_2; // esi
  int Size_3; // ecx
  int n2_1; // edx
  int v43; // esi
  int n6; // eax
  int v45; // eax
  int v46; // eax
  int n15; // eax
  _DWORD *v48; // eax
  _DWORD *j_1; // esi
  int **v50; // eax
  int j; // eax
  int *k; // ecx
  _DWORD *n60_2; // edi
  int *m; // eax
  int v55; // esi
  int n10; // ecx
  int v57; // eax
  int *n; // esi
  int v59; // edi
  int n13; // eax
  int v61; // eax
  _DWORD *n298; // esi
  _BYTE **v63; // esi
  _BYTE *v64; // ecx
  _DWORD *v65; // esi
  _BYTE *v66; // edi
  _DWORD *n298_1; // esi
  _DWORD *n60_3; // eax
  int *v69; // esi
  size_t Size_4; // ecx
  int v71; // edi
  int v72; // eax
  unsigned int n5; // ecx
  int v75; // eax
  unsigned int n15_2; // edi
  int v77; // ecx
  int n1000_2; // edx
  _DWORD *n60_4; // edi
  int *ii; // eax
  int v81; // esi
  int v82; // edx
  _DWORD *v83; // eax
  int v84; // [esp+24h] [ebp-68h]
  void *Src[2]; // [esp+28h] [ebp-64h] BYREF
  int n1000; // [esp+30h] [ebp-5Ch]
  unsigned __int8 Size_5; // [esp+37h] [ebp-55h]
  char v88[4]; // [esp+38h] [ebp-54h] BYREF
  size_t Size; // [esp+3Ch] [ebp-50h] BYREF
  int v90; // [esp+40h] [ebp-4Ch]
  int v91; // [esp+44h] [ebp-48h]
  char v92; // [esp+4Bh] [ebp-41h]
  _DWORD v93[4]; // [esp+4Ch] [ebp-40h] BYREF
  unsigned int n5_2; // [esp+5Ch] [ebp-30h]
  unsigned int n0x10; // [esp+60h] [ebp-2Ch]
  int v96; // [esp+64h] [ebp-28h] BYREF
  char v97; // [esp+69h] [ebp-23h]
  int n5_1; // [esp+74h] [ebp-18h]
  int n15_1; // [esp+78h] [ebp-14h]
  int v100; // [esp+88h] [ebp-4h]
  int savedregs; // [esp+8Ch] [ebp+0h] BYREF

  v2 = 0;
  v90 = 0;
  v91 = 0;
  Size_1 = n60[28381];
  v4 = this[141];
  Size = Size_1;
  if ( v4 && *(_DWORD *)(v4 + 40) == 965 )
  {
    if ( *(_DWORD *)(v4 + 44) == 10 )
      v2 = v4;
    v91 = v2;
  }
  n60_5 = (_DWORD *)(*this - 1);
  switch ( *this )
  {
    case 1:
      v92 = 1;
      v84 = 0;
      v6 = n60[24768] + 4632;
      v90 = 0;
      n1000 = 1000;
      sub_4186C0(v6, (int)&savedregs, 1);
      sub_800500(n60[24768]);
      n19 = this[7];
      if ( n19 == 19 )
      {
        v8 = 0;
        for ( i = 1828; i < 0x744; i += 4 )
        {
          if ( (unsigned int)++v8 <= 8 )
          {
            v10 = *(_DWORD *)(i + n60[24768]);
            if ( v10 )
              (*(void (__thiscall **)(int))(*(_DWORD *)v10 + 8))(v10);
          }
        }
        n19 = this[7];
      }
      v11 = (int *)n60[28330];
      if ( v11 != (int *)n60[28331] )
      {
        do
        {
          v12 = *v11;
          if ( *v11 )
          {
            if ( *(_BYTE *)(v12 + 371) )
            {
              (*(void (__thiscall **)(int))(*(_DWORD *)v12 + 12))(*v11);
            }
            else if ( (*(_BYTE *)(v12 + 5016) || *(_BYTE *)(v12 + 5018))
                   && ((v13 = *(_DWORD *)(v12 + 124)) == 0 || !*(_BYTE *)(v13 + 52)) )
            {
              if ( !this[7] )
              {
                *(_BYTE *)(v12 + 5016) = 0;
                *(_BYTE *)(v12 + 5018) = 0;
              }
              (*(void (__thiscall **)(int))(*(_DWORD *)v12 + 12))(v12);
              n604 = *(_DWORD *)(v12 + 5632);
              if ( n604 && n604 != 604 )
                sub_7C36B0(v12);
              n19_1 = this[7];
              if ( !n19_1 )
              {
                *(_BYTE *)(v12 + 5016) = 0;
                *(_BYTE *)(v12 + 5018) = 0;
                n19_1 = this[7];
              }
              if ( n19_1 == 19 && v91 && *(_DWORD *)(v12 + 124) && (unsigned __int8)sub_40ADD0(v12 + 120, "Poof") )
              {
                sub_7D9180(v91);
                *(_BYTE *)(v12 + 369) = 0;
              }
              if ( (*(_BYTE *)(v12 + 5016) || *(_BYTE *)(v12 + 5018)) && *(_DWORD *)(v12 + 1020) != 4 )
              {
                if ( *(_DWORD *)(v12 + 124) )
                {
                  *(float *)&Src[1] = floor(*(float *)(v12 + 136));
                  n1000_1 = (int)*(float *)&Src[1];
                }
                else
                {
                  n1000_1 = -1;
                }
                v92 = 0;
                if ( n1000_1 >= n1000 )
                {
                  v12 = v90;
                  n1000_1 = n1000;
                }
                v90 = v12;
                n1000 = n1000_1;
              }
              else
              {
                *(_BYTE *)(v12 + 369) = 0;
              }
            }
            else
            {
              v14 = v84;
              if ( !v84 )
              {
                if ( *(_BYTE *)(v12 + 369) )
                  v14 = *v11;
                v84 = v14;
              }
              if ( this[7] == 19 )
                (*(void (__thiscall **)(int))(*(_DWORD *)v12 + 12))(v12);
            }
          }
          ++v11;
        }
        while ( v11 != (int *)n60[28331] );
        n19 = this[7];
      }
      if ( (n19 == 11 || n19 == 16 || n19 == 19) && v90 && v84 && n1000 >= 6 )
      {
        ArgList = (_DWORD *)sub_452BF0(v93);
        v100 = 0;
        if ( ArgList[5] >= 0x10u )
          ArgList = (_DWORD *)*ArgList;
        sub_7ABE20((char)ArgList);
        v100 = -1;
        if ( n0x10 >= 0x10 )
        {
          v19 = v93[0];
          if ( n0x10 + 1 >= 0x1000 )
          {
            v19 = *(_DWORD *)(v93[0] - 4);
            if ( (unsigned int)(v93[0] - v19 - 4) > 0x1F )
LABEL_63:
              invalid_parameter_noinfo_noreturn();
          }
          sub_AEF15C(v19);
        }
        v20 = *(_DWORD *)(v90 + 6944);
        *(_DWORD *)(v84 + 6940) = *(_DWORD *)(v90 + 6940);
        *(_DWORD *)(v84 + 6944) = v20;
        n19 = this[7];
        n5_2 = 0;
        n0x10 = 15;
        LOBYTE(v93[0]) = 0;
        if ( n19 == 19 && v91 )
        {
          v21 = (float *)sub_67F070(Src);
          v23 = (float)(unsigned int)sub_6EEF60(v22) * 2.3283064e-10;
          v24 = (float)((float)(v23 * *v21) * 8.0) + *(float *)(v91 + 820);
          *(float *)(v84 + 6944) = (float)((float)(v23 * v21[1]) * 8.0) + *(float *)(v91 + 824);
          *(float *)(v84 + 6940) = v24;
          n19 = this[7];
        }
      }
      else if ( v92 )
      {
        goto LABEL_70;
      }
      if ( n19 != 21 )
        goto LABEL_245;
LABEL_70:
      if ( n19 == 19 )
      {
        v25 = this[141];
        if ( v25 && *(_DWORD *)(v25 + 40) == 965 && *(_DWORD *)(v25 + 44) == 10 )
        {
          v26 = v91;
          if ( *(_DWORD *)(v91 + 2916) != 4 )
          {
            v27 = *(_DWORD *)(v91 + 360);
            n2 = 1;
            *(_DWORD *)(v91 + 364) |= 0x1000u;
            *(_DWORD *)(v26 + 360) = v27;
            v29 = sub_812C90(v26 + 820);
            switch ( sub_4CE0F0(v29) )
            {
              case 7:
                n2 = 2;
                break;
              case 8:
                n2 = 0;
                break;
              case 9:
                n2 = 3;
                break;
              default:
                break;
            }
            v30 = (float *)sub_67F1D0(v88, n2);
            v31 = *v30;
            v32 = v30[1] * 0.1;
            *(_DWORD *)(v26 + 2916) = 4;
            *(float *)(v26 + 868) = v32;
            *(float *)(v26 + 864) = v31 * 0.1;
            v33 = *(&a5 + 1);
            *(_DWORD *)(v26 + 820) = a5;
            *(_DWORD *)(v26 + 824) = v33;
          }
          goto LABEL_245;
        }
        goto LABEL_88;
      }
      if ( n19 != 11 && n19 != 16 || (n60_1 = n60, this[5] != n60[24769]) )
      {
LABEL_88:
        v37 = this[4] == -1;
        this[1] = 0;
        if ( !v37 || n19 == 3 || n19 == 12 || n19 == 13 || n19 == 11 || n19 == 16 )
        {
          n60 = n60;
          v39 = (int *)n60[28330];
          if ( v39 != (int *)n60[28331] )
          {
            Size_2 = Size;
            do
            {
              Size_3 = *v39;
              if ( *v39 )
              {
                if ( (n2_1 = this[7]) != 0 && n2_1 != 2 || this[4] == -1 || Size_3 == Size_2 )
                  *(_BYTE *)(Size_3 + 369) = 0;
              }
              ++v39;
            }
            while ( v39 != (int *)n60[28331] );
          }
        }
        sub_80EA80(n60[24768]);
        *(_QWORD *)Src = 0LL;
        sub_6856C0(Src);
        v43 = sub_684F30();
        if ( v43 != -4 )
          *(_BYTE *)(v43 + 4) = (dword_C798E4 & 0x20) != 0;
        *(_DWORD *)v43 = dword_C79948;
        sub_A18300(&dword_C798E0, this[10], 1);
        sub_A18230(&dword_C798E0);
        n6 = 1;
        LODWORD(xmmword_C79900) = 0;
        if ( (dword_C798E4 & 4) == 0 )
          n6 = 6;
        *((_QWORD *)&xmmword_C79900 + 1) = 0x100000007LL;
        DWORD1(xmmword_C79900) = n6;
        n7 = 7;
        sub_80F200(n60[24768]);
        sub_A19180(&dword_C798E0, 1);
        if ( !dword_C79790 )
          sub_A112C0(16, "PopRenderTarget: stack is empty!\n");
        v45 = sub_684F30();
        sub_A18300(&dword_C798E0, *(_DWORD *)v45, *(unsigned __int8 *)(v45 + 4));
        v37 = dword_C79790-- == 1;
        v46 = dword_C7978C;
        if ( v37 )
          v46 = 0;
        dword_C7978C = v46;
        n15 = this[7];
        if ( n15 == 1
          || n15 == 2
          || this[4] == -1
          || n15 == 11
          || n15 == 15
          || n15 == 16
          || n15 == 17
          || n15 == 20
          || n15 == 18
          || n15 == 19
          || n15 == 21 )
        {
          *this = 2;
          if ( n15 == 15 || n15 == 21 )
            sub_830950(this);
        }
        else
        {
          *this = 3;
          sub_830950(this);
        }
        goto LABEL_245;
      }
      v35 = (int *)n60[28330];
      if ( v35 != (int *)n60[28331] )
      {
        do
        {
          v36 = *v35;
          if ( *v35 )
          {
            *(_BYTE *)(v36 + 369) = 0;
            *(_BYTE *)(v36 + 1040) = 1;
          }
          ++v35;
        }
        while ( v35 != (int *)n60_1[28331] );
      }
      goto LABEL_87;
    case 2:
      if ( this[142] )
      {
        sub_409100(this + 14);
        v48 = (_DWORD *)this[137];
        j_1 = (_DWORD *)*v48;
        if ( (_DWORD *)*v48 != v48 )
        {
          do
          {
            if ( *((_BYTE *)j_1 + 285) )
            {
              sub_409030(j_1 + 17);
              sub_409030(j_1 + 25);
            }
            v50 = (int **)j_1[2];
            if ( *((_BYTE *)v50 + 13) )
            {
              for ( j = j_1[1]; !*(_BYTE *)(j + 13); j = *(_DWORD *)(j + 4) )
              {
                if ( j_1 != *(_DWORD **)(j + 8) )
                  break;
                j_1 = (_DWORD *)j;
              }
              j_1 = (_DWORD *)j;
            }
            else
            {
              j_1 = (_DWORD *)j_1[2];
              for ( k = *v50; !*((_BYTE *)k + 13); k = (int *)*k )
                j_1 = k;
            }
          }
          while ( j_1 != (_DWORD *)this[137] );
        }
        if ( !this[27] || !*((_BYTE *)this + 124) )
        {
          sub_7EB1B0(n60[24768]);
          n60_2 = n60;
          *((_BYTE *)n60 + 143056) = 1;
          this[1] = this[2];
          *this = 3;
          for ( m = (int *)n60_2[28330]; m != (int *)n60_2[28331]; ++m )
          {
            v55 = *m;
            if ( *m )
            {
              n10 = 10;
              if ( *(int *)(v55 + 1048) > 10 )
                n10 = *(_DWORD *)(v55 + 1048);
              *(_DWORD *)(v55 + 1048) = n10;
            }
          }
        }
      }
      else
      {
        v57 = this[1];
        if ( v57 < this[2] )
        {
LABEL_149:
          this[1] = v57 + 1;
        }
        else
        {
          this[1] = 0;
          *this = 3;
          sub_830950(this);
        }
      }
      goto LABEL_245;
    case 3:
      v57 = this[1];
      if ( v57 < this[2] )
        goto LABEL_149;
      sub_421260(n60 + 29883);
      this[1] = 0;
      if ( (unsigned __int8)sub_82EB90(this) )
      {
        for ( n = (int *)n60[28330]; n != (int *)n60[28331]; ++n )
        {
          v59 = *n;
          if ( *n )
          {
            if ( this[7] == 3 )
              sub_7ABE20((char)"TeleportDown");
            if ( (*(_DWORD *)(v59 + 364) & 0x2000000) == 0 )
              *(_BYTE *)(v59 + 369) = 1;
          }
        }
        n13 = this[7];
        if ( n13 == 12 )
        {
          if ( Size )
          {
            sub_7AB380(422, (char)"Pickup", (char)"PlayerPickupSparkle");
            *this = 4;
            goto LABEL_245;
          }
        }
        else if ( n13 == 13 && Size )
        {
          sub_7AB380(437, (char)"Pickup", (char)"PlayerPickupSparkle");
          *this = 4;
          goto LABEL_245;
        }
        goto LABEL_168;
      }
      v61 = this[7];
      switch ( v61 )
      {
        case 5:
          if ( Size_1 )
            sub_7AB380(161, (char)"Pickup", (char)"PlayerPickupSparkle");
          break;
        case 6:
          if ( Size_1 )
          {
            sub_7AB380(81, (char)"Pickup", (char)"PlayerPickupSparkle");
            *this = 4;
            goto LABEL_245;
          }
          break;
        case 7:
          n298 = n298;
          Size = 1;
          sub_956780(&Size);
          sub_92DC30(n298 + 43209, Size, 1.0, 2, 0, 1065353216, 0);
          if ( Size_1 )
          {
            sub_7AB380(11, (char)"Pickup", (char)"PlayerPickupSparkle");
            *this = 4;
            goto LABEL_245;
          }
          break;
        case 8:
          if ( Size_1 )
          {
            sub_7AB380(212, (char)"Pickup", (char)"PlayerPickupSparkle");
            *this = 4;
            goto LABEL_245;
          }
          break;
        case 9:
          if ( Size_1 )
          {
            sub_7AB380(311, (char)"Pickup", (char)"PlayerPickupSparkle");
            *this = 4;
            goto LABEL_245;
          }
          break;
        case 10:
          if ( Size_1 )
          {
            sub_7AB380(332, (char)"Pickup", (char)"PlayerPickupSparkle");
            *this = 4;
            goto LABEL_245;
          }
          break;
        case 14:
          if ( Size_1 )
          {
            sub_7AB550(23, (char)"Pickup", (char)"PlayerPickupSparkle");
            *this = 4;
            goto LABEL_245;
          }
          break;
        default:
          switch ( v61 )
          {
            case 1:
            case 2:
            case 18:
              goto LABEL_168;
            case 11:
            case 16:
            case 17:
            case 19:
              sub_4186C0(n60[24768] + 4632, (int)&savedregs, 1);
              v65 = (_DWORD *)n60[28330];
              if ( v65 != (_DWORD *)n60[28331] )
              {
                do
                {
                  v66 = (_BYTE *)*v65;
                  if ( *v65 )
                  {
                    (*(void (__thiscall **)(_DWORD))(*(_DWORD *)v66 + 12))(*v65);
                    v66[1040] = 1;
                  }
                  ++v65;
                }
                while ( v65 != (_DWORD *)n60[28331] );
                this[1] = 0;
                *this = 0;
                goto LABEL_245;
              }
              break;
            case 20:
              goto LABEL_168;
            default:
              sub_4186C0(n60[24768] + 4632, (int)&savedregs, 1);
              v63 = (_BYTE **)n60[28330];
              if ( v63 != (_BYTE **)n60[28331] )
              {
                do
                {
                  v64 = *v63;
                  if ( *v63 )
                  {
                    if ( (int)n60[39301] < 2 )
                      v64[1040] = 1;
                    (*(void (__thiscall **)(_BYTE *))(*(_DWORD *)v64 + 12))(v64);
                  }
                  ++v63;
                }
                while ( v63 != (_BYTE **)n60[28331] );
                this[1] = 0;
                *this = 0;
                goto LABEL_245;
              }
              break;
          }
LABEL_87:
          this[1] = 0;
          *this = 0;
          goto LABEL_245;
      }
LABEL_168:
      *this = 4;
      goto LABEL_245;
    case 4:
      if ( this[7] == 3 && this[1] == 8 )
      {
        n298_1 = n298;
        Size = 214;
        sub_956780(&Size);
        sub_92DC30(n298_1 + 43209, Size, 1.0, 2, 0, 1065353216, 0);
      }
      n60_3 = n60;
      ++this[1];
      LOBYTE(Size) = 1;
      sub_4186C0(n60_3[24768] + 4632, (int)&savedregs, 1);
      v69 = (int *)n60[28330];
      if ( v69 == (int *)n60[28331] )
        goto LABEL_240;
      LOBYTE(Size_4) = Size;
      break;
    default:
      return n60_5;
  }
  do
  {
    v71 = *v69;
    Size_5 = Size_4;
    if ( !v71 || !*(_BYTE *)(v71 + 5016) && !*(_BYTE *)(v71 + 5018) )
      goto LABEL_238;
    v72 = *(_DWORD *)(v71 + 124);
    if ( v72 ? *(_BYTE *)(v72 + 52) : 0 )
      goto LABEL_238;
    (*(void (__thiscall **)(int))(*(_DWORD *)v71 + 12))(v71);
    if ( *(_DWORD *)(v71 + 124) )
    {
      sub_40CF50(*(void **)(v71 + 124));
      n5 = n5_2;
    }
    else
    {
      n5 = 0;
      n0x10 = 15;
      n5_2 = 0;
      v93[0] = 0;
    }
    v100 = 1;
    v75 = v90 | 0x20;
    v37 = *(_BYTE *)(v71 + 5016) == 0;
    v90 |= 0x20u;
    n1000 = v93[0];
    if ( v37 && !*(_BYTE *)(v71 + 5018) )
    {
      v90 = v75;
LABEL_225:
      n15_2 = n15_1;
      v91 = v96;
LABEL_226:
      v92 = 0;
      goto LABEL_227;
    }
    if ( n5 < 5 )
      goto LABEL_225;
    v96 = 0;
    Src[1] = (void *)(n5 - 5);
    n15_1 = 15;
    v83 = v93;
    Size = 5;
    if ( n0x10 >= 0x10 )
      v83 = (_DWORD *)v93[0];
    Src[1] = (char *)Src[1] + (unsigned int)v83;
    n5_1 = 5;
    memmove(&v96, Src[1], 5u);
    v97 = 0;
    n15_2 = n15_1;
    v91 = v96;
    v90 |= 0x41u;
    if ( (unsigned __int8)sub_40CAE0("Death", 5) )
      goto LABEL_226;
    v92 = 1;
LABEL_227:
    if ( (v90 & 1) != 0 )
    {
      v90 &= ~1u;
      if ( n15_2 >= 0x10 )
      {
        v77 = v91;
        if ( n15_2 + 1 >= 0x1000 )
        {
          v77 = *(_DWORD *)(v91 - 4);
          if ( (unsigned int)(v91 - v77 - 4) > 0x1F )
            goto LABEL_63;
        }
        sub_AEF15C(v77);
      }
    }
    Size_4 = 0;
    v100 = -1;
    if ( !v92 )
      Size_4 = Size_5;
    Size = Size_4;
    if ( n0x10 >= 0x10 )
    {
      n1000_2 = n1000;
      if ( n0x10 + 1 >= 0x1000 )
      {
        n1000_2 = *(_DWORD *)(n1000 - 4);
        if ( (unsigned int)(n1000 - n1000_2 - 4) > 0x1F )
          goto LABEL_63;
      }
      sub_AEF15C(n1000_2);
      LOBYTE(Size_4) = Size;
    }
LABEL_238:
    ++v69;
  }
  while ( v69 != (int *)n60[28331] );
  if ( (_BYTE)Size_4 )
  {
LABEL_240:
    n60_4 = n60;
    *this = 0;
    this[1] = 0;
    for ( ii = (int *)n60_4[28330]; ii != (int *)n60_4[28331]; ++ii )
    {
      v81 = *ii;
      if ( *ii )
      {
        v82 = *(_DWORD *)(v81 + 364) & 0x2000000;
        *(_BYTE *)(v81 + 1040) = 1;
        if ( !v82 )
          *(_BYTE *)(v81 + 369) = 1;
      }
    }
  }
LABEL_245:
  n60_5 = (_DWORD *)*this;
  *((float *)this + 3) = (float)(int)this[1] / (float)(int)this[2];
  if ( n60_5 == (_DWORD *)2 || n60_5 == (_DWORD *)3 )
  {
    n60_5 = n60;
    n60[35765] = 2;
    n60_5[35767] = 2;
  }
  return n60_5;
}
