void __stdcall sub_6F6270(int a1)
{
  _DWORD *n2; // ebx
  _DWORD *v2; // ecx
  int v3; // eax
  _BYTE *v4; // esi
  char *v5; // ecx
  _BYTE *v6; // edi
  int v7; // edi
  int v8; // esi
  _BYTE *v9; // esi
  _DWORD *n2_2; // eax
  char *v11; // eax
  char *v12; // edi
  int n2_3; // ebx
  unsigned int v14; // esi
  int v15; // edx
  bool v16; // zf
  char *n2_4; // ebx
  _DWORD *n37_3; // edi
  _DWORD *i_1; // esi
  _DWORD **v20; // ebx
  int n37_1; // eax
  int v22; // edi
  int n37_2; // esi
  int **i_3; // eax
  int i; // eax
  int *j; // ecx
  char *n2_1; // [esp+Ch] [ebp-8h]
  int n37; // [esp+10h] [ebp-4h] BYREF
  char *v29; // [esp+1Ch] [ebp+8h]
  _DWORD *i_2; // [esp+1Ch] [ebp+8h]

  n2 = n2_0; /*0x6f6277*/
  n2_1 = (char *)n2_0; /*0x6f627f*/
  if ( *((_BYTE *)n2_0 + 154370) ) /*0x6f6282*/
  {
    sub_84B840(a1); /*0x6f6292*/
    sub_908210((char *)dword_BE4A1C + 172384); /*0x6f62a3*/
    sub_908140(86); /*0x6f62b9*/
    sub_908140(87); /*0x6f62cf*/
    sub_908140(88); /*0x6f62e5*/
    sub_908140(89); /*0x6f62fb*/
    if ( n2[28685] == 2 ) /*0x6f630e*/
      sub_984110(n2 + 28685, 1); /*0x6f6317*/
    v2 = dword_BE4A1C; /*0x6f631c*/
    v3 = 97 * *((_DWORD *)dword_BE4A1C + 42878); /*0x6f632a*/
    *((_DWORD *)dword_BE4A1C + 43075) = 1065353216; /*0x6f6334*/
    v2[43076] = 1065353216; /*0x6f633e*/
    sub_7D5C40(&v2[v3 + 42879]); /*0x6f6350*/
    v4 = (char *)dword_BE4A1C + 20; /*0x6f635b*/
    if ( *((_BYTE *)dword_BE4A1C + 20) && *((_BYTE *)dword_BE4A1C + 4000) ) /*0x6f6363*/
    {
      v5 = (char *)dword_BE4A1C + 20; /*0x6f636c*/
      *v4 = 0; /*0x6f636e*/
      sub_9032A0(v5); /*0x6f6371*/
      v6 = dword_BE4A1C; /*0x6f6376*/
      if ( *(_DWORD *)SteamInternal_ContextInit(&off_BD00B4) && v6[172502] ) /*0x6f638f*/
        sub_902F00(v4); /*0x6f639a*/
      else
        sub_903460(v4); /*0x6f63a3*/
    }
    if ( !*((_BYTE *)n2 + 154371) ) /*0x6f63a8*/
    {
      if ( (_BYTE)a1 ) /*0x6f63b5*/
        sub_932160(); /*0x6f63b7*/
      else
        sub_9324A0(); /*0x6f63be*/
    }
    n2[37281] = 0; /*0x6f63cd*/
    n2[37805] = 0; /*0x6f63d7*/
    sub_8B80A0(0, 0); /*0x6f63e1*/
    n2[37808] = 0; /*0x6f63e6*/
    n2[37793] = 0; /*0x6f63f6*/
    n2[30002] = 0; /*0x6f6400*/
    sub_403530(); /*0x6f640a*/
    n2[30079] = 0; /*0x6f640f*/
    v7 = n2[38158]; /*0x6f6419*/
    v8 = n2[38157]; /*0x6f641f*/
    if ( v8 != v7 ) /*0x6f6427*/
    {
      do /*0x6f643d*/
      {
        sub_4085F0(v8 + 8); /*0x6f6433*/
        v8 += 56; /*0x6f6438*/
      }
      while ( v8 != v7 ); /*0x6f643d*/
      v8 = n2[38157]; /*0x6f643f*/
    }
    n2[38158] = v8; /*0x6f6445*/
    *((_BYTE *)n2 + 151244) = 0; /*0x6f644d*/
    sub_6F0770(n2); /*0x6f6454*/
    v9 = dword_BE4A1C; /*0x6f6459*/
    sub_9E0DF0(0, "Setting PersistentGameData ReadOnly to %s\n", (char)"False"); /*0x6f646b*/
    n2_2 = n2_0; /*0x6f6470*/
    v9[21] = 0; /*0x6f647b*/
    n2_2[28380] = 0; /*0x6f6482*/
    n2[28318] = 0; /*0x6f648c*/
    n2[28319] = 0; /*0x6f6496*/
    *((_BYTE *)n2 + 113284) = 0; /*0x6f64a0*/
    n2[28322] = 0; /*0x6f64a7*/
    n2[28324] = dword_BEB900; /*0x6f64b9*/
    n2[28325] = n1101004800; /*0x6f64c9*/
    n2[28320] = 50; /*0x6f64d4*/
    n2[28176] = 0; /*0x6f64de*/
    n2[28178] = 0; /*0x6f64e8*/
    n2[28175] = 0; /*0x6f64f2*/
    n2[28179] = 0; /*0x6f64fc*/
    n2[28182] = 0; /*0x6f6506*/
    n2[28183] = 0; /*0x6f6510*/
    n2[28184] = 0; /*0x6f651a*/
    n2_2[28381] = 0; /*0x6f6524*/
    *((_QWORD *)n2 + 19288) = 0LL; /*0x6f652e*/
    sub_403530(); /*0x6f6536*/
    sub_403530(); /*0x6f6541*/
    n2[28685] = 0; /*0x6f654c*/
    n2[28686] = 0; /*0x6f6556*/
    sub_41CAE0(n2 + 29861); /*0x6f6560*/
    v11 = (char *)(n2 + 25188); /*0x6f6565*/
    n37 = 37; /*0x6f656b*/
    v29 = (char *)(n2 + 25188); /*0x6f6572*/
    do /*0x6f65ba*/
    {
      v12 = v11; /*0x6f6575*/
      n2_3 = 2; /*0x6f6577*/
      do /*0x6f65a9*/
      {
        if ( v12[12] ) /*0x6f6580*/
        {
          v14 = 0; /*0x6f6586*/
          if ( *(_DWORD *)v12 ) /*0x6f6588*/
          {
            v15 = 0; /*0x6f658c*/
            do /*0x6f65a1*/
            {
              v15 += 92; /*0x6f6593*/
              ++v14; /*0x6f6596*/
              *(_DWORD *)(v15 + *((_DWORD *)v12 - 1) - 40) = *(_DWORD *)(v15 + *((_DWORD *)v12 - 1) - 44); /*0x6f659b*/
            }
            while ( v14 < *(_DWORD *)v12 ); /*0x6f65a1*/
          }
        }
        v12 += 44; /*0x6f65a3*/
        --n2_3; /*0x6f65a6*/
      }
      while ( n2_3 ); /*0x6f65a9*/
      v11 = v29 + 196; /*0x6f65ae*/
      v16 = n37-- == 1; /*0x6f65b3*/
      v29 += 196; /*0x6f65b7*/
    }
    while ( !v16 ); /*0x6f65ba*/
    n2_4 = n2_1; /*0x6f65bc*/
    sub_682030(n2_1 + 425940); /*0x6f65c5*/
    sub_732D20(); /*0x6f65cc*/
    sub_98F7C0(n2_1 + 113320); /*0x6f65d7*/
    sub_97AB10(n2_1 + 121260); /*0x6f65e2*/
    sub_979D80(n2_1 + 121260); /*0x6f65ed*/
    sub_40A140(); /*0x6f65f2*/
    n37_3 = (_DWORD *)*((_DWORD *)n2_1 + 27086); /*0x6f65f7*/
    i_1 = (_DWORD *)*n37_3; /*0x6f6603*/
    i_2 = (_DWORD *)*n37_3; /*0x6f6605*/
    if ( (_DWORD *)*n37_3 != n37_3 ) /*0x6f660a*/
    {
      v20 = (_DWORD **)(n2_1 + 108344); /*0x6f6610*/
      do /*0x6f66aa*/
      {
        sub_698BC0(&n37, i_1 + 4); /*0x6f661c*/
        n37_1 = n37; /*0x6f6621*/
        if ( (_DWORD *)n37 != n37_3 && *(_BYTE *)(n37 + 40) ) /*0x6f6628*/
        {
          v22 = *(_DWORD *)(n37 + 68); /*0x6f662e*/
          if ( v22 != *(_DWORD *)(n37 + 72) ) /*0x6f6634*/
          {
            n37_2 = n37; /*0x6f6636*/
            do /*0x6f6650*/
            {
              sub_408950(0); /*0x6f6645*/
              v22 += 32; /*0x6f664a*/
            }
            while ( v22 != *(_DWORD *)(n37_2 + 72) ); /*0x6f6650*/
            i_1 = i_2; /*0x6f6652*/
            n37_1 = n37; /*0x6f6655*/
          }
          *(_BYTE *)(n37_1 + 40) = 0; /*0x6f6658*/
        }
        sub_40A140(); /*0x6f665c*/
        i_3 = (int **)i_1[2]; /*0x6f6661*/
        if ( *((_BYTE *)i_3 + 13) ) /*0x6f6664*/
        {
          for ( i = i_1[1]; !*(_BYTE *)(i + 13); i = *(_DWORD *)(i + 4) ) /*0x6f666d*/
          {
            if ( i_1 != *(_DWORD **)(i + 8) ) /*0x6f6676*/
              break; /*0x6f6676*/
            i_1 = (_DWORD *)i; /*0x6f6678*/
          }
          i_1 = (_DWORD *)i; /*0x6f6683*/
          i_2 = (_DWORD *)i; /*0x6f6685*/
        }
        else
        {
          i_1 = (_DWORD *)i_1[2]; /*0x6f668a*/
          i_2 = i_3; /*0x6f668c*/
          for ( j = *i_3; !*((_BYTE *)j + 13); j = (int *)*j ) /*0x6f6691*/
          {
            i_1 = j; /*0x6f6699*/
            i_2 = j; /*0x6f669b*/
          }
        }
        n37_3 = *v20; /*0x6f66a6*/
      }
      while ( i_1 != *v20 ); /*0x6f66aa*/
      n2_4 = n2_1; /*0x6f66b0*/
    }
    sub_98CC60(n2_4 + 144956); /*0x6f66b9*/
    *((_DWORD *)n2_4 + 38606) = 0; /*0x6f66be*/
    *((_WORD *)n2_4 + 77185) = 0; /*0x6f66c8*/
  }
}
