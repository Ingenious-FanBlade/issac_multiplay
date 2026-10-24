unsigned int __thiscall sub_9346B0(_DWORD *this)
{
  _DWORD *v1; // ebx
  unsigned int v2; // edi
  int v3; // esi
  int v4; // esi
  int v5; // eax
  int v6; // eax
  unsigned int v7; // esi
  void *v8; // ecx
  unsigned int v9; // esi
  int v10; // esi
  unsigned int result; // eax
  _DWORD *n2; // edx
  int v13; // eax
  int v14; // esi
  int *v15; // esi
  unsigned int v16; // edi
  int *v17; // ebx
  int v18; // ecx
  int v19; // eax
  int ArgList_5; // eax
  bool v21; // zf
  _DWORD *v22; // edi
  char *v23; // edx
  int v24; // esi
  char *v25; // eax
  int v26; // edi
  int v27; // eax
  _DWORD *v28; // ecx
  int v29; // edx
  int v30; // edi
  int v31; // edi
  char *v32; // eax
  int v33; // esi
  unsigned int v34; // edi
  int v35; // esi
  int ArgList_2; // ebx
  _DWORD *v37; // edi
  char *v38; // eax
  int v39; // esi
  int v40; // ebx
  char *v41; // eax
  int v42; // esi
  _DWORD *n2_1; // edx
  int *v44; // esi
  unsigned int v45; // edi
  int v46; // ecx
  int v47; // eax
  _DWORD *v48; // edi
  _DWORD *v49; // esi
  _DWORD *v50; // edx
  _DWORD *v51; // eax
  _DWORD *v52; // esi
  int v53; // edi
  char *v54; // eax
  int v55; // esi
  unsigned int v56; // esi
  int v57; // edi
  int ArgList_3; // esi
  unsigned int v59; // edi
  _DWORD *n2_2; // eax
  char *v61; // eax
  char ArgList; // [esp+0h] [ebp-108h]
  _BYTE v63[20]; // [esp+10h] [ebp-F8h] BYREF
  int n3; // [esp+24h] [ebp-E4h]
  char v65[12]; // [esp+38h] [ebp-D0h] BYREF
  char v66[12]; // [esp+44h] [ebp-C4h] BYREF
  int v67; // [esp+50h] [ebp-B8h]
  int v68; // [esp+54h] [ebp-B4h]
  int v69; // [esp+58h] [ebp-B0h]
  int v70; // [esp+5Ch] [ebp-ACh]
  int v71; // [esp+60h] [ebp-A8h]
  int v72; // [esp+64h] [ebp-A4h]
  _DWORD v73[2]; // [esp+8Ch] [ebp-7Ch] BYREF
  int v74; // [esp+94h] [ebp-74h] BYREF
  int v75; // [esp+98h] [ebp-70h] BYREF
  _DWORD *v76; // [esp+9Ch] [ebp-6Ch]
  _DWORD *v77; // [esp+A0h] [ebp-68h]
  int v78; // [esp+A4h] [ebp-64h] BYREF
  const char *__GAME___CONTROLLER_DISCONNECTED_HEADER; // [esp+A8h] [ebp-60h]
  int v80; // [esp+ACh] [ebp-5Ch]
  int v81; // [esp+B0h] [ebp-58h]
  __int128 v82; // [esp+B4h] [ebp-54h]
  int v83; // [esp+C4h] [ebp-44h]
  int v84; // [esp+C8h] [ebp-40h]
  int n1101004800; // [esp+CCh] [ebp-3Ch]
  int v86; // [esp+D0h] [ebp-38h]
  void *v87; // [esp+D4h] [ebp-34h]
  int *v88; // [esp+D8h] [ebp-30h]
  int *v89; // [esp+DCh] [ebp-2Ch]
  unsigned int v90; // [esp+E0h] [ebp-28h]
  unsigned int v91; // [esp+E4h] [ebp-24h]
  int ArgList_1; // [esp+E8h] [ebp-20h]
  int v93; // [esp+ECh] [ebp-1Ch] BYREF
  int ArgList_4; // [esp+F0h] [ebp-18h] BYREF
  void *Block; // [esp+F4h] [ebp-14h]
  _DWORD *v96; // [esp+F8h] [ebp-10h]
  int v97; // [esp+104h] [ebp-4h]

  v1 = this; /*0x9346db*/
  v96 = this; /*0x9346dd*/
  if ( this[76337] == -1 ) /*0x9346e7*/
  {
    v2 = 0; /*0x9346f9*/
    ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x9346fb*/
    v3 = ((_BYTE *)dword_BCE440 - (_BYTE *)dword_BCE43C) >> 2; /*0x934714*/
    ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934717*/
    if ( v3 ) /*0x93471c*/
    {
      while ( 1 ) /*0x93472e*/
      {
        ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x93472e*/
        if ( v2 >= ((_BYTE *)dword_BCE440 - (_BYTE *)dword_BCE43C) >> 2 ) /*0x93474d*/
          break; /*0x93474d*/
        v10 = *((_DWORD *)dword_BCE43C + v2); /*0x93482c*/
        ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x93482f*/
        if ( !v10 ) /*0x934834*/
          goto LABEL_5; /*0x934834*/
        v4 = *(_DWORD *)(v10 + 8); /*0x93483a*/
        if ( v4 ) /*0x93483f*/
          goto LABEL_6; /*0x93483f*/
LABEL_13:
        ++v2; /*0x9347a9*/
        ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x9347b6*/
        v7 = ((_BYTE *)dword_BCE440 - (_BYTE *)dword_BCE43C) >> 2; /*0x9347cf*/
        ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x9347d2*/
        if ( v2 >= v7 ) /*0x9347d7*/
          goto LABEL_14; /*0x9347d7*/
      }
      ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934753*/
LABEL_5:
      v4 = -1; /*0x934756*/
LABEL_6:
      v5 = sub_9EEC40(v4); /*0x934759*/
      if ( !v5 || !*(_DWORD *)(v5 + 1164) || *(_DWORD *)(v5 + 1176) == 3 ) /*0x934778*/
      {
        v6 = sub_9EEC40(v4); /*0x934780*/
        if ( v6 ) /*0x934787*/
          (*(void (__thiscall **)(int, int, int *))(*(_DWORD *)v6 + 128))(v6, 14, &v74); /*0x934793*/
        if ( v74 == -1 && v75 == -1 ) /*0x9347a3*/
        {
          v93 = -1082130432; /*0x93484d*/
          ArgList_4 = -1082130432; /*0x934869*/
          sub_998BA0((char *)dword_BE4A1C + 305748, 0, 0, &v93); /*0x934871*/
          v97 = 0; /*0x93487c*/
          n3 = 3; /*0x934889*/
          sub_999F60(v63); /*0x934898*/
          v1[76337] = sub_999E40(v63); /*0x9348be*/
          return sub_41BEF0(v63); /*0x9348da*/
        }
      }
      goto LABEL_13; /*0x9347a3*/
    }
  }
LABEL_14:
  v8 = 0; /*0x9347dd*/
  v9 = 0; /*0x9347df*/
  Block = 0; /*0x9347e1*/
  v87 = 0; /*0x9347e4*/
  v88 = 0; /*0x9347e7*/
  v89 = 0; /*0x9347ea*/
  v97 = 1; /*0x9347ed*/
  if ( dword_BE5998 )
  {
    if ( ArgList_0 != -1 ) /*0x934808*/
    {
      ArgList_4 = ArgList_0; /*0x93480e*/
      sub_428D10(0, (int)&ArgList_4); /*0x934819*/
      v8 = v87; /*0x93481e*/
      v9 = (unsigned int)v88; /*0x934821*/
      Block = v87; /*0x934824*/
    }
  }
  else
  {
    n2 = n2_0; /*0x9348db*/
    if ( n2_0 )
    {
      v13 = v1[76783]; /*0x9348e9*/
      v14 = v1[76782]; /*0x9348f5*/
      if ( v14 == v13 || (unsigned int)(v13 - v14) < 4 )
      {
        if ( *((_DWORD *)n2_0 + 38605)
          || (v15 = (int *)((char *)n2_0 + 113320),
              v16 = 0,
              !((*((_DWORD *)n2_0 + 28331) - *((_DWORD *)n2_0 + 28330)) >> 2)) )
        {
          v9 = 0; /*0x9349db*/
          v8 = 0; /*0x9349dd*/
        }
        else
        {
          v17 = 0; /*0x93494e*/
          do
          {
            v18 = v15[1]; /*0x934950*/
            if ( !((v18 - *v15) >> 2) )
            {
              sub_9E0DF0(3, "PlayerManager::GetPlayer() : There are no players! \n", ArgList);
              v18 = v15[1]; /*0x93496a*/
              n2 = n2_0; /*0x934970*/
            }
            v19 = *v15; /*0x934976*/
            if ( v16 < (v18 - *v15) >> 2 ) /*0x93497f*/
              v19 += 4 * v16; /*0x934981*/
            ArgList_5 = *(_DWORD *)(*(_DWORD *)v19 + 5644); /*0x934986*/
            ArgList_4 = ArgList_5; /*0x93498c*/
            if ( v17 == v89 ) /*0x934992*/
            {
              sub_428D10(v17, (int)&ArgList_4); /*0x9349a6*/
              v17 = v88; /*0x9349ab*/
              n2 = n2_0; /*0x9349ae*/
            }
            else
            {
              *v17++ = ArgList_5; /*0x934994*/
              v88 = v17; /*0x934999*/
            }
            v15 = n2 + 28330; /*0x9349ba*/
            ++v16; /*0x9349c2*/
          }
          while ( v16 < (n2[28331] - n2[28330]) >> 2 );
          v8 = v87; /*0x9349ca*/
          v90 = (unsigned int)v17; /*0x9349cd*/
          v1 = v96; /*0x9349d0*/
          v9 = v90; /*0x9349d3*/
          Block = v87; /*0x9349d6*/
        }
      }
      else
      {
        ArgList_4 = sub_8E7380(); /*0x934907*/
        sub_428D10(0, (int)&ArgList_4); /*0x934913*/
        v8 = v87; /*0x934918*/
        v9 = (unsigned int)v88; /*0x93491b*/
        Block = v87; /*0x93491e*/
      }
    }
  }
  v21 = v1[76338] == -1; /*0x9349df*/
  v22 = v1 + 76338; /*0x9349e6*/
  v76 = v1 + 76338; /*0x9349ec*/
  if ( !v21 && v9 - (unsigned int)v8 < 4 ) /*0x9349f8*/
  {
    ArgList_4 = -1; /*0x9349fd*/
    sub_999F00(v1 + 76338, &ArgList_4); /*0x934a0c*/
    *v22 = -1; /*0x934a11*/
    *((_BYTE *)v1 + 305356) = 0; /*0x934a17*/
  }
  v23 = (char *)Block; /*0x934a1e*/
  result = 0; /*0x934a21*/
  v91 = 0; /*0x934a25*/
  v90 = (int)(v9 - (_DWORD)Block) >> 2; /*0x934a2b*/
  if ( !v90 ) /*0x934a30*/
    goto LABEL_134; /*0x934a30*/
  do
  {
    v21 = *v22 == -1; /*0x934a48*/
    ArgList_1 = -1; /*0x934a4b*/
    if ( v21 )
    {
      v24 = *(_DWORD *)&v23[4 * result]; /*0x934a56*/
      ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x934a5e*/
      v25 = (char *)dword_BCE43C; /*0x934a61*/
      v26 = 0; /*0x934a66*/
      if ( dword_BCE43C != dword_BCE440 ) /*0x934a70*/
      {
        while ( *(_DWORD *)(*(_DWORD *)v25 + 8) != v24 ) /*0x934a77*/
        {
          v25 += 4; /*0x934a79*/
          if ( v25 == dword_BCE440 ) /*0x934a7e*/
            goto LABEL_48; /*0x934a7e*/
        }
        v26 = *(_DWORD *)v25; /*0x934a82*/
      }
LABEL_48:
      ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934a84*/
      if ( !v26 )
      {
        v73[0] = -1082130432; /*0x934a9c*/
        v73[1] = -1082130432; /*0x934ab6*/
        sub_998BA0((char *)dword_BE4A1C + 305748, 0, 0, v73); /*0x934abe*/
        LOBYTE(v97) = 2; /*0x934ac3*/
        v21 = *((_BYTE *)v1 + 305356) == 0; /*0x934aca*/
        v82 = xmmword_B41F00; /*0x934ade*/
        v78 = 0; /*0x934ae2*/
        v84 = dword_BEB900; /*0x934aed*/
        v80 = -1; /*0x934afa*/
        v81 = -1; /*0x934b01*/
        v83 = 0; /*0x934b08*/
        v86 = 1; /*0x934b0b*/
        n1101004800 = n1101004800; /*0x934b13*/
        if ( v21 ) /*0x934b18*/
        {
          n3 = 3; /*0x934b2f*/
          __GAME___CONTROLLER_DISCONNECTED_HEADER = "##GAME###CONTROLLER_DISCONNECTED_HEADER"; /*0x934b39*/
          sub_9992C0(&v78); /*0x934b40*/
          __GAME___CONTROLLER_DISCONNECTED_HEADER = "##GAME###CONTROLLER_DISCONNECTED_MESSAGE"; /*0x934b45*/
        }
        else
        {
          __GAME___CONTROLLER_DISCONNECTED_HEADER = "##GAME###RESELECT_CONTROLLER_HEADER"; /*0x934b1a*/
          sub_9992C0(&v78); /*0x934b21*/
          __GAME___CONTROLLER_DISCONNECTED_HEADER = "##GAME###RESELECT_CONTROLLER_MESSAGE"; /*0x934b26*/
        }
        v78 = 0; /*0x934b5d*/
        v80 = -1; /*0x934b64*/
        v81 = -1; /*0x934b6b*/
        v82 = xmmword_B41F00; /*0x934b72*/
        v83 = 0; /*0x934b76*/
        v84 = 0; /*0x934b7d*/
        n1101004800 = 1101004800; /*0x934b84*/
        v86 = 1; /*0x934b8b*/
        sub_9992C0(&v78); /*0x934b92*/
        v27 = sub_999E40(v63); /*0x934ba4*/
        v28 = dword_BE5998; /*0x934ba9*/
        v1[76338] = v27; /*0x934baf*/
        if ( v28 )
        {
          if ( v28[16] == 12 ) /*0x934bbd*/
          {
            *((_BYTE *)dword_BE4A1C + 305347) = 1; /*0x934bc8*/
            v29 = v28[1]; /*0x934bcf*/
            if ( (unsigned int)((v29 - *v28) >> 2) > 1 ) /*0x934bdc*/
            {
              v29 -= 4; /*0x934bde*/
              v28[1] = v29; /*0x934be1*/
            }
            sub_962E20(v28, *(_DWORD *)(v29 - 4), 0); /*0x934be9*/
          }
        }
        else if ( n2_0 )
        {
          if ( v1[76782] == v1[76783] )
          {
            sub_4131B0(n2_0, v91); /*0x934c63*/
            sub_79E1B0(-1, 1); /*0x934c6a*/
          }
          else
          {
            sub_9E0DF0(0, "[Frame: %d] Setting local device ID to %d \n", v1[76785]);
            v30 = sub_8E74C0(v1 + 76782); /*0x934c2a*/
            if ( v30 )
              *(_DWORD *)(v30 + 1040) = sub_9EEC40(-1); /*0x934c3c*/
            else
              sub_9E0DF0(
                2,
                "[Frame: %d] NetManager::SetLocalDeviceID() failed to find device for local user! \n",
                v1[76785]);
          }
        }
        LOBYTE(v97) = 1; /*0x934c6f*/
        v71 = 0; /*0x934c79*/
        v72 = 0; /*0x934c83*/
        v69 = 0; /*0x934c8d*/
        v70 = 0; /*0x934c97*/
        v67 = 0; /*0x934ca1*/
        v68 = 0; /*0x934cab*/
        sub_41C240(v66); /*0x934cb5*/
        sub_41C240(v65); /*0x934cc0*/
      }
      goto LABEL_133; /*0x934cc5*/
    }
    v31 = *(_DWORD *)&v23[4 * result]; /*0x934cca*/
    ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x934cd2*/
    v32 = (char *)dword_BCE43C; /*0x934cd5*/
    v33 = 0; /*0x934cda*/
    if ( dword_BCE43C != dword_BCE440 ) /*0x934ce4*/
    {
      while ( 1 ) /*0x934ce6*/
      {
        v33 = *(_DWORD *)v32; /*0x934ce6*/
        if ( *(_DWORD *)(*(_DWORD *)v32 + 8) == v31 ) /*0x934ceb*/
          break; /*0x934ceb*/
        v32 += 4; /*0x934ced*/
        if ( v32 == dword_BCE440 ) /*0x934cf2*/
        {
          ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934cfe*/
          goto LABEL_69; /*0x934d01*/
        }
      }
    }
    ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934d0d*/
    if ( v33 ) /*0x934d12*/
      goto LABEL_133; /*0x934d12*/
LABEL_69:
    v34 = 0; /*0x934d18*/
    ArgList_4 = 0; /*0x934d26*/
    ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x934d29*/
    v35 = ((_BYTE *)dword_BCE440 - (_BYTE *)dword_BCE43C) >> 2; /*0x934d42*/
    ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934d45*/
    if ( !v35 ) /*0x934d4a*/
      goto LABEL_133; /*0x934d4a*/
    do
    {
      ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x934d5c*/
      if ( v34 >= ((_BYTE *)dword_BCE440 - (_BYTE *)dword_BCE43C) >> 2 ) /*0x934d7b*/
      {
        ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934d7d*/
LABEL_72:
        ArgList_2 = -1; /*0x934d80*/
        goto LABEL_73; /*0x934d80*/
      }
      v40 = *((_DWORD *)dword_BCE43C + v34); /*0x934dcd*/
      ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934dd0*/
      if ( !v40 ) /*0x934dd5*/
        goto LABEL_72; /*0x934dd5*/
      ArgList_2 = *(_DWORD *)(v40 + 8); /*0x934dd7*/
LABEL_73:
      v37 = v96; /*0x934d83*/
      if ( !*((_BYTE *)v96 + 305356) )
      {
        ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x934d9b*/
        v38 = (char *)dword_BCE43C; /*0x934d9e*/
        v39 = 0; /*0x934da3*/
        if ( dword_BCE43C != dword_BCE440 ) /*0x934dad*/
        {
          while ( 1 ) /*0x934db0*/
          {
            v39 = *(_DWORD *)v38; /*0x934db0*/
            if ( *(_DWORD *)(*(_DWORD *)v38 + 8) == ArgList_2 ) /*0x934db5*/
              break; /*0x934db5*/
            v38 += 4; /*0x934db7*/
            if ( v38 == dword_BCE440 ) /*0x934dbc*/
            {
              ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934dc8*/
              goto LABEL_82; /*0x934dcb*/
            }
          }
        }
        ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934de6*/
        if ( v39 && (*(unsigned __int8 (__thiscall **)(int))(*(_DWORD *)v39 + 104))(v39) )
        {
LABEL_89:
          n2_1 = n2_0; /*0x934e6d*/
          if ( n2_0
            && (v44 = (int *)((char *)n2_0 + 113320),
                v45 = 0,
                (*((_DWORD *)n2_0 + 28331) - *((_DWORD *)n2_0 + 28330)) >> 2) )
          {
            while ( 1 )
            {
              v46 = v44[1]; /*0x934e90*/
              if ( !((v46 - *v44) >> 2) )
              {
                sub_9E0DF0(3, "PlayerManager::GetPlayer() : There are no players! \n", ArgList);
                v46 = v44[1]; /*0x934eaa*/
                n2_1 = n2_0; /*0x934eb0*/
              }
              v47 = *v44; /*0x934eb6*/
              if ( v45 < (v46 - *v44) >> 2 ) /*0x934ebf*/
                v47 += 4 * v45; /*0x934ec1*/
              if ( *(_DWORD *)(*(_DWORD *)v47 + 5644) == ArgList_2 ) /*0x934ecc*/
                break; /*0x934ecc*/
              v44 = n2_1 + 28330; /*0x934ed8*/
              if ( ++v45 >= (n2_1[28331] - n2_1[28330]) >> 2 ) /*0x934ee6*/
                goto LABEL_97; /*0x934ee6*/
            }
          }
          else
          {
LABEL_97:
            v48 = v96 + 78678; /*0x934ee8*/
            ArgList_1 = ArgList_2; /*0x934ef1*/
            v75 = -1; /*0x934ef8*/
            v77 = v96 + 78678; /*0x934eff*/
            (*(void (__thiscall **)(_DWORD *, int))(v96[78678] + 12))(v96 + 78678, -1); /*0x934f04*/
            v49 = 0; /*0x934f0a*/
            v50 = (_DWORD *)v96[78676]; /*0x934f0c*/
            v51 = (_DWORD *)*v50; /*0x934f12*/
            if ( (_DWORD *)*v50 != v50 ) /*0x934f16*/
            {
              while ( 1 ) /*0x934f20*/
              {
                v49 = (_DWORD *)v51[2]; /*0x934f20*/
                if ( *v49 == *v76 ) /*0x934f25*/
                  break; /*0x934f25*/
                v51 = (_DWORD *)*v51; /*0x934f27*/
                if ( v51 == v50 ) /*0x934f2b*/
                {
                  (*(void (__thiscall **)(_DWORD *))(*v77 + 16))(v77); /*0x934f32*/
                  goto LABEL_104; /*0x934f35*/
                }
              }
              v48 = v77; /*0x934f37*/
            }
            (*(void (__thiscall **)(_DWORD *))(*v48 + 16))(v48); /*0x934f3e*/
            if ( v49 ) /*0x934f43*/
              sub_997E00(&v75, 0); /*0x934f4d*/
LABEL_104:
            *v76 = -1; /*0x934f52*/
            v52 = v96 + 76782; /*0x934f5e*/
            *((_BYTE *)v96 + 305356) = 0; /*0x934f64*/
            if ( *v52 != v52[1] )
            {
              sub_9E0DF0(0, "[Frame: %d] Setting local device ID to %d \n", v52[3]);
              v53 = sub_8E74C0(v52); /*0x934f90*/
              if ( v53 )
              {
                ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x934fa3*/
                v54 = (char *)dword_BCE43C; /*0x934fa6*/
                v55 = 0; /*0x934fab*/
                if ( dword_BCE43C != dword_BCE440 ) /*0x934fb5*/
                {
                  while ( *(_DWORD *)(*(_DWORD *)v54 + 8) != ArgList_2 ) /*0x934fbc*/
                  {
                    v54 += 4; /*0x934fbe*/
                    if ( v54 == dword_BCE440 ) /*0x934fc3*/
                      goto LABEL_111; /*0x934fc3*/
                  }
                  v55 = *(_DWORD *)v54; /*0x934fc7*/
                }
LABEL_111:
                ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934fc9*/
                *(_DWORD *)(v53 + 1040) = v55; /*0x934fd6*/
              }
              else
              {
                sub_9E0DF0(
                  2,
                  "[Frame: %d] NetManager::SetLocalDeviceID() failed to find device for local user! \n",
                  v52[3]);
              }
            }
          }
          goto LABEL_113; /*0x934ecc*/
        }
LABEL_82:
        if ( !*((_BYTE *)v37 + 305356) ) /*0x934e01*/
          goto LABEL_113; /*0x934e01*/
      }
      ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x934e13*/
      v41 = (char *)dword_BCE43C; /*0x934e16*/
      v42 = 0; /*0x934e1b*/
      if ( dword_BCE43C != dword_BCE440 ) /*0x934e25*/
      {
        while ( 1 ) /*0x934e27*/
        {
          v42 = *(_DWORD *)v41; /*0x934e27*/
          if ( *(_DWORD *)(*(_DWORD *)v41 + 8) == ArgList_2 ) /*0x934e2c*/
            break; /*0x934e2c*/
          v41 += 4; /*0x934e2e*/
          if ( v41 == dword_BCE440 ) /*0x934e33*/
          {
            ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934e3f*/
            goto LABEL_113; /*0x934e42*/
          }
        }
      }
      ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x934e51*/
      if ( v42 && (*(unsigned __int8 (__thiscall **)(int))(*(_DWORD *)v42 + 108))(v42) ) /*0x934e63*/
        goto LABEL_89; /*0x934e67*/
LABEL_113:
      v34 = ++ArgList_4; /*0x934ffd*/
      ((void (__thiscall *)(void ***, int))off_BCE480[3])(&off_BCE480, -1); /*0x935003*/
      v56 = ((_BYTE *)dword_BCE440 - (_BYTE *)dword_BCE43C) >> 2; /*0x93501c*/
      ((void (__thiscall *)(void ***))off_BCE480[4])(&off_BCE480); /*0x93501f*/
    }
    while ( v34 < v56 );
    if ( ArgList_1 == -1 )
    {
      v1 = v96; /*0x935202*/
    }
    else
    {
      if ( dword_BE5998 ) /*0x93503d*/
      {
        ArgList_0 = ArgList_1; /*0x93503f*/
        goto LABEL_117; /*0x93503f*/
      }
      if ( n2_0 )
      {
        v1 = v96; /*0x935082*/
        if ( v96[76782] != v96[76783] )
        {
          sub_9E0DF0(0, "[Frame: %d] Setting local device ID to %d \n", v96[76785]);
          v57 = sub_8E74C0(v1 + 76782); /*0x9350b4*/
          if ( v57 )
          {
            ArgList_3 = ArgList_1; /*0x9350ba*/
            *(_DWORD *)(v57 + 1040) = sub_9EEC40(ArgList_1); /*0x9350c8*/
          }
          else
          {
            sub_9E0DF0(
              2,
              "[Frame: %d] NetManager::SetLocalDeviceID() failed to find device for local user! \n",
              v1[76785]);
            ArgList_3 = ArgList_1; /*0x9350e2*/
          }
          goto LABEL_129; /*0x9350ce*/
        }
        if ( !((*((_DWORD *)n2_0 + 28331) - *((_DWORD *)n2_0 + 28330)) >> 2) )
          sub_9E0DF0(3, "PlayerManager::GetPlayer() : There are no players! \n", ArgList);
        v59 = v91; /*0x93511c*/
        ArgList_3 = ArgList_1; /*0x93512b*/
        sub_79E1B0(ArgList_1, 1); /*0x935133*/
        if ( !v59 ) /*0x93513a*/
LABEL_129:
          ArgList_0 = ArgList_3; /*0x93513c*/
        n2_2 = n2_0; /*0x935142*/
        *((_DWORD *)n2_0 + 28333) = -1; /*0x935147*/
        n2_2[28338] = -1; /*0x935151*/
        n2_2[28335] = -1; /*0x93515b*/
        n2_2[28336] = 0; /*0x935165*/
        n2_2[28334] = 0; /*0x93516f*/
        n2_2[28340] = -1; /*0x935179*/
        n2_2[28341] = 0; /*0x935183*/
        n2_2[28339] = 0; /*0x93518d*/
        n2_2[28343] = -1; /*0x935197*/
        n2_2[28345] = -1; /*0x9351a1*/
        n2_2[28346] = 0; /*0x9351ab*/
        n2_2[28344] = 0; /*0x9351b5*/
        n2_2[28348] = -1; /*0x9351bf*/
        n2_2[28350] = -1; /*0x9351c9*/
        n2_2[28351] = 0; /*0x9351d3*/
        n2_2[28349] = 0; /*0x9351dd*/
      }
      else
      {
LABEL_117:
        v1 = v96; /*0x935045*/
      }
      if ( byte_BE65A5 ) /*0x93504f*/
        sub_A2A390(); /*0x9351ec*/
      else
        sub_A2F820(65537, 0, ArgList); /*0x93505c*/
      sub_A31960(); /*0x935064*/
      sub_9EDCC0(&off_BCE438); /*0x93506e*/
    }
LABEL_133:
    v22 = v1 + 76338; /*0x935205*/
    v23 = (char *)Block; /*0x93520e*/
    result = v91 + 1; /*0x935211*/
    v91 = result; /*0x935212*/
  }
  while ( result < v90 );
LABEL_134:
  if ( v23 ) /*0x935220*/
  {
    v61 = v23; /*0x935225*/
    if ( (((char *)v89 - v23) & 0xFFFFFFFC) >= 0x1000 ) /*0x935232*/
    {
      v23 = (char *)*((_DWORD *)v23 - 1); /*0x935234*/
      if ( (unsigned int)(v61 - v23 - 4) > 0x1F ) /*0x935242*/
        invalid_parameter_noinfo_noreturn(); /*0x935244*/
    }
    return sub_A8717B(v23); /*0x93524c*/
  }
  return result; /*0x9348c9*/
}
