void sub_92EC40()
{
  _DWORD *v0; // edi
  int v1; // esi
  int v2; // eax
  bool v3; // zf
  char *v4; // esi
  _DWORD **v5; // eax
  _DWORD *v6; // esi
  int n2; // eax
  float v8; // xmm1_4
  char ArgList; // cl
  _DWORD *v10; // eax
  __m128 v11; // xmm3
  __m128 v12; // xmm2
  __m128 v13; // xmm1
  __m128 v14; // xmm0
  __m128 v15; // xmm2
  char ArgList_2; // cl
  __int128 v17; // xmm0
  int v18; // esi
  unsigned int v19; // eax
  float v20; // xmm1_4
  double v21; // xmm0_8
  float *v22; // eax
  float v23; // xmm2_4
  char ArgList_3; // cl
  int ArgList_1; // [esp+40h] [ebp-70h]
  int v26; // [esp+44h] [ebp-6Ch]
  char v27; // [esp+4Ah] [ebp-66h]
  char v28; // [esp+4Bh] [ebp-65h]
  float v29; // [esp+4Ch] [ebp-64h]
  float v30; // [esp+4Ch] [ebp-64h]
  float v31; // [esp+50h] [ebp-60h] BYREF
  float v32; // [esp+54h] [ebp-5Ch]
  __int128 v33; // [esp+58h] [ebp-58h] BYREF
  int v34; // [esp+68h] [ebp-48h]
  int v35; // [esp+6Ch] [ebp-44h] BYREF
  int v36; // [esp+70h] [ebp-40h]
  int v37; // [esp+74h] [ebp-3Ch]
  int v38; // [esp+78h] [ebp-38h]
  int v39; // [esp+7Ch] [ebp-34h]
  int v40; // [esp+80h] [ebp-30h]
  int v41; // [esp+84h] [ebp-2Ch]
  int v42; // [esp+88h] [ebp-28h]
  int v43; // [esp+8Ch] [ebp-24h] BYREF
  int v44; // [esp+90h] [ebp-20h]
  int n1065353216_1; // [esp+94h] [ebp-1Ch]
  int v46; // [esp+98h] [ebp-18h]
  int v47; // [esp+9Ch] [ebp-14h]
  int n1065353216_3; // [esp+A0h] [ebp-10h]
  int n1065353216; // [esp+A4h] [ebp-Ch]
  int n1065353216_2; // [esp+A8h] [ebp-8h]
  int v51; // [esp+ACh] [ebp-4h]

  v0 = dword_BE4A1C; /*0x92ec4b*/
  if ( (*((_BYTE *)dword_BE4A1C + 305336) & 1) != 0 || *((_BYTE *)dword_BE4A1C + 172539) )
  {
    if ( *((_BYTE *)dword_BE4A1C + 307113) ) /*0x92ec67*/
    {
      sub_408950(0); /*0x92ec77*/
      sub_408950(0); /*0x92ec83*/
      sub_408950(0); /*0x92ec8f*/
      sub_408950(0); /*0x92ec9b*/
      sub_92AD30(); /*0x92eca2*/
      if ( !(unsigned __int8)sub_92DFE0() ) /*0x92eca7*/
      {
        v0[43129] = dword_BAE278; /*0x92ecb5*/
        v0[43130] = dword_BAE27C; /*0x92ecc0*/
      }
      *((_BYTE *)v0 + 307113) = 0; /*0x92ecc6*/
    }
    epoxy_glBindFramebuffer(); /*0x92ecd4*/
    byte_BAE1B0 = 1; /*0x92ece1*/
    dword_BAE1AC = 0; /*0x92ece8*/
    sub_9E83C0(0); /*0x92ecf2*/
    epoxy_glCullFace(); /*0x92ecfc*/
    ((void (__thiscall *)(void ***))off_BAE190[31])(&off_BAE190); /*0x92ed0c*/
    sub_9E85B0(&off_BAE190); /*0x92ed14*/
    if ( v0[2] == 3 || sub_9E41E0(&off_BAE190) == *(_DWORD *)dword_BE729C ) /*0x92ed2f*/
    {
      v28 = 0; /*0x92ed4f*/
    }
    else
    {
      v28 = 1; /*0x92ed31*/
      sub_67EDD0(); /*0x92ed36*/
      sub_9E80F0(&off_BAE190, dword_BE73F4, 1); /*0x92ed48*/
    }
    if ( !*((_BYTE *)v0 + 172544) /*0x92ed94*/
      || !*((_BYTE *)v0 + 172445)
      || v0[33554] == 1
      && (v1 = v0[34070]) != 0
      && (!*(_BYTE *)(v1 + 116)
       || (v2 = sub_A37A00(*(_DWORD *)(v1 + 28))) != 0 && *(float *)(v2 + 4) < *(float *)(v1 + 88)) )
    {
      v27 = 0; /*0x92edb4*/
    }
    else
    {
      v27 = 1; /*0x92ed96*/
      sub_67EDD0(); /*0x92ed9b*/
      sub_9E80F0(&off_BAE190, dword_BE740C, 1); /*0x92edad*/
    }
    if ( *((_BYTE *)dword_BE4A1C + 172540) ) /*0x92edbe*/
    {
      sub_67EDD0(); /*0x92edc7*/
      sub_9E80F0(&off_BAE190, dword_BE7404, 1); /*0x92edd9*/
    }
    if ( *((_BYTE *)v0 + 304084) ) /*0x92edde*/
    {
      sub_8D16B0(); /*0x92edf5*/
    }
    else
    {
      switch ( v0[2] ) /*0x92ee08*/
      {
        case 1: /*0x92ee08*/
          sub_9623C0(); /*0x92ee1c*/
          break; /*0x92ee21*/
        case 2: /*0x92ee08*/
          sub_6F7A80(); /*0x92ee15*/
          break; /*0x92ee1a*/
        case 3: /*0x92ee08*/
          sub_937A10(v0 + 33554); /*0x92ee29*/
          break; /*0x92ee2e*/
        case 5: /*0x92ee08*/
          sub_8FAC20(v0 + 34086); /*0x92ee36*/
          break; /*0x92ee36*/
        default:
          break;
      }
      if ( v0[34474] && !*((_BYTE *)v0 + 171508) ) /*0x92ee50*/
      {
        v3 = *((_DWORD *)dword_BE4A1C + 43117) == 2; /*0x92ee7b*/
        v31 = *(float *)&dword_BE6F2C * 0.5; /*0x92ee82*/
        v32 = *(float *)&dword_BE7298 * 0.5; /*0x92ee88*/
        if ( v3 || *((_DWORD *)dword_BE4A1C + 76782) != *((_DWORD *)dword_BE4A1C + 76783) ) /*0x92ee9c*/
        {
          v4 = (char *)(v0 + 34475); /*0x92eece*/
          v31 = (float)(*(float *)&dword_BE6F2C * 0.5) * 0.25; /*0x92eee0*/
          v32 = (float)(*(float *)&dword_BE7298 * 0.5) * 1.75; /*0x92eee6*/
          *(_DWORD *)(sub_406B60((_BYTE)v0 - 84, 3) + 32) = 1; /*0x92eef5*/
          *(_DWORD *)(sub_406B60((_BYTE)v0 - 84, 3) + 36) = 1; /*0x92ef01*/
        }
        else
        {
          v4 = (char *)(v0 + 34475); /*0x92ee9e*/
          *(_DWORD *)(sub_406B60((_BYTE)v0 - 84, 3) + 32) = 0; /*0x92eeb1*/
          *(_DWORD *)(sub_406B60((_BYTE)v0 - 84, 3) + 36) = 0; /*0x92eebd*/
        }
        if ( v4[265] ) /*0x92ef08*/
        {
          sub_405990(&v31, &dword_BEB900, &dword_BEB900); /*0x92ef29*/
          sub_405990(&v31, &dword_BEB900, &dword_BEB900); /*0x92ef52*/
        }
      }
      sub_8E7E40(); /*0x92ef5d*/
      (*(void (__thiscall **)(_DWORD *, int))(v0[78678] + 12))(v0 + 78678, -1); /*0x92ef70*/
      v5 = (_DWORD **)v0[78676]; /*0x92ef73*/
      v6 = *v5; /*0x92ef79*/
      if ( *v5 != v5 ) /*0x92ef7d*/
      {
        do /*0x92ef90*/
        {
          sub_9978A0(ArgList_1, v26); /*0x92ef83*/
          v6 = (_DWORD *)*v6; /*0x92ef88*/
        }
        while ( v6 != (_DWORD *)v0[78676] ); /*0x92ef90*/
      }
      (*(void (__thiscall **)(_DWORD *))(v0[78678] + 16))(v0 + 78678); /*0x92ef9e*/
      sub_41F780(ArgList_1, v26); /*0x92efa7*/
      if ( *((_BYTE *)v0 + 172504) ) /*0x92efac*/
      {
        n2 = v0[2]; /*0x92efb9*/
        if ( (n2 == 2 || n2 == 1) && (int)v0[76777] > 0 ) /*0x92efcd*/
        {
          v29 = *(float *)&dword_B887B8 / (float)sub_9E4360(); /*0x92efe7*/
          v8 = (float)(*(float *)&dword_B887B4 / (float)sub_9E43C0()) * *((float *)v0 + 76776); /*0x92f028*/
          v31 = v29 * *((float *)v0 + 76775); /*0x92f030*/
          v32 = v8; /*0x92f036*/
          sub_4059E0(&v31, &dword_BEB900, &dword_BEB900); /*0x92f03c*/
        }
      }
    }
    if ( *((_BYTE *)dword_BE4A1C + 172540) ) /*0x92f046*/
    {
      sub_9E4170(&off_BAE190); /*0x92f058*/
      sub_67EEB0(ArgList); /*0x92f05d*/
      sub_9E8290(&off_BAE190); /*0x92f067*/
      v41 = dword_BE6F2C; /*0x92f07d*/
      v37 = dword_BE6F2C; /*0x92f083*/
      dword_BAE1A8 = (int)&off_BAB3C8; /*0x92f093*/
      v47 = 0; /*0x92f098*/
      v43 = 0; /*0x92f0a0*/
      n1065353216 = 1065353216; /*0x92f0a8*/
      n1065353216_1 = 1065353216; /*0x92f0b0*/
      v46 = 0; /*0x92f0b8*/
      v44 = 0; /*0x92f0c0*/
      n1065353216_2 = 1065353216; /*0x92f0c8*/
      n1065353216_3 = 1065353216; /*0x92f0d0*/
      v51 = 1; /*0x92f0d8*/
      v39 = 0; /*0x92f0e0*/
      v35 = 0; /*0x92f0e8*/
      v38 = 0; /*0x92f0f0*/
      v36 = 0; /*0x92f0f8*/
      v42 = dword_BE7298; /*0x92f100*/
      v40 = dword_BE7298; /*0x92f106*/
      sub_9E35B0(-1); /*0x92f10c*/
      v10 = (_DWORD *)(*(int (__thiscall **)(int, int *, int *, __int128 *))(*(_DWORD *)dword_BE7404 + 12))( /*0x92f12b*/
                        dword_BE7404,
                        &v43,
                        &v35,
                        &v33);
      v11 = (__m128)*((unsigned int *)dword_BE4A1C + 43113); /*0x92f13b*/
      v12 = _mm_shuffle_ps( /*0x92f143*/
              (__m128)*((unsigned int *)dword_BE4A1C + 43112),
              (__m128)*((unsigned int *)dword_BE4A1C + 43112),
              0);
      if ( v10 ) /*0x92f149*/
      {
        v13 = _mm_div_ps((__m128)xmmword_B424D0, v12); /*0x92f155*/
        v14 = _mm_shuffle_ps(v11, v11, 0); /*0x92f158*/
        v15 = _mm_unpackhi_ps(v13, v13); /*0x92f162*/
        v10[20] = v15.m128_i32[0]; /*0x92f165*/
        v10[5] = v11.m128_i32[0]; /*0x92f16d*/
        v10[12] = v11.m128_i32[0]; /*0x92f172*/
        v10[19] = v11.m128_i32[0]; /*0x92f177*/
        v10[26] = _mm_unpackhi_ps(v14, v14).m128_u32[0]; /*0x92f17c*/
        v10[6] = v13.m128_i32[0]; /*0x92f181*/
        v10[13] = v13.m128_i32[0]; /*0x92f186*/
        v10[27] = _mm_unpackhi_ps(v15, v15).m128_u32[0]; /*0x92f18b*/
      }
    }
    if ( v27 )
    {
      dword_BAE1A8 = 0; /*0x92f1a1*/
      sub_9EFA70(); /*0x92f1ab*/
      sub_9EFA10(); /*0x92f1b0*/
      dword_BA4714 = -1082130432; /*0x92f1bf*/
      dword_BAE1A8 = 0; /*0x92f1c9*/
      ((void (__thiscall *)(void ***))off_BAE190[26])(&off_BAE190); /*0x92f1d3*/
      if ( dword_BAE1AC ) /*0x92f1dd*/
      {
        epoxy_glClear(); /*0x92f20d*/
      }
      else if ( byte_BE65A5 ) /*0x92f1eb*/
      {
        SwapBuffers(*(HDC *)(*(_DWORD *)dword_BAE270 + 564)); /*0x92f204*/
      }
      else
      {
        sub_A2F820(65537, 0, ArgList_1); /*0x92f1f4*/
      }
      sub_67EEB0(ArgList_2); /*0x92f213*/
      sub_9E8290(&off_BAE190); /*0x92f21d*/
      v17 = byte_BAE1B3 ? xmmword_B40F60 : xmmword_B40F70;
      ((void (__thiscall *)(void ***, _DWORD, _DWORD, _DWORD, _DWORD, int))off_BAE190[14])( /*0x92f254*/
        &off_BAE190,
        v17,
        DWORD1(v17),
        DWORD2(v17),
        HIDWORD(v17),
        7);
      v41 = dword_BE6F2C; /*0x92f268*/
      v37 = dword_BE6F2C; /*0x92f26e*/
      dword_BAE1A8 = (int)&off_BAB39C; /*0x92f27e*/
      v47 = 0; /*0x92f283*/
      v43 = 0; /*0x92f28b*/
      n1065353216 = 1065353216; /*0x92f293*/
      n1065353216_1 = 1065353216; /*0x92f29b*/
      v46 = 0; /*0x92f2a3*/
      v44 = 0; /*0x92f2ab*/
      n1065353216_2 = 1065353216; /*0x92f2b3*/
      n1065353216_3 = 1065353216; /*0x92f2bb*/
      v51 = 1; /*0x92f2c3*/
      v39 = 0; /*0x92f2cb*/
      v35 = 0; /*0x92f2d3*/
      v38 = 0; /*0x92f2db*/
      v36 = 0; /*0x92f2e3*/
      v42 = dword_BE7298; /*0x92f2eb*/
      v40 = dword_BE7298; /*0x92f2f1*/
      sub_9E35B0(-1); /*0x92f2f7*/
      v31 = COERCE_FLOAT( /*0x92f318*/
              (*(int (__thiscall **)(int, int *, int *, __int128 *))(*(_DWORD *)dword_BE740C + 12))(
                dword_BE740C,
                &v43,
                &v35,
                &v33));
      if ( v31 != 0.0 ) /*0x92f31e*/
      {
        v18 = dword_BE740C; /*0x92f324*/
        v30 = (float)(unsigned int)(*(int (__thiscall **)(int))(*(_DWORD *)dword_BE740C + 48))(dword_BE740C); /*0x92f352*/
        v19 = (*(int (__thiscall **)(int))(*(_DWORD *)v18 + 52))(v18); /*0x92f358*/
        v20 = *(float *)&dword_B88730; /*0x92f35a*/
        v21 = (double)v19; /*0x92f37b*/
        v22 = (float *)LODWORD(v31); /*0x92f384*/
        v23 = v21; /*0x92f388*/
        LODWORD(v21) = dword_B88730; /*0x92f38c*/
        *(float *)(LODWORD(v31) + 20) = v30; /*0x92f38f*/
        v22[6] = v23; /*0x92f394*/
        *(float *)&v21 = *(float *)&v21 * *(float *)&dword_B883E8; /*0x92f399*/
        v22[13] = v30; /*0x92f3a1*/
        v22[14] = v23; /*0x92f3a6*/
        v22[7] = *(float *)&v21 * 0.5; /*0x92f3af*/
        *(float *)&v21 = v20 * *(float *)&dword_B883E8; /*0x92f3b7*/
        v22[21] = v30; /*0x92f3bf*/
        v22[22] = v23; /*0x92f3c4*/
        v22[15] = *(float *)&v21 * 0.5; /*0x92f3cd*/
        *(float *)&v21 = v20 * *(float *)&dword_B883E8; /*0x92f3d5*/
        v22[29] = v30; /*0x92f3dd*/
        v22[30] = v23; /*0x92f3e2*/
        v22[23] = *(float *)&v21 * 0.5; /*0x92f3eb*/
        v22[31] = (float)(v20 * *(float *)&dword_B883E8) * 0.5; /*0x92f3fc*/
      }
    }
    if ( v28 ) /*0x92f406*/
    {
      dword_BAE1A8 = 0; /*0x92f40c*/
      sub_9EFA70(); /*0x92f416*/
      sub_9EFA10(); /*0x92f41b*/
      dword_BA4714 = -1082130432; /*0x92f42a*/
      dword_BAE1A8 = 0; /*0x92f434*/
      ((void (__thiscall *)(void ***))off_BAE190[26])(&off_BAE190); /*0x92f43e*/
      if ( dword_BAE1AC ) /*0x92f448*/
      {
        epoxy_glClear(); /*0x92f478*/
      }
      else if ( byte_BE65A5 ) /*0x92f456*/
      {
        SwapBuffers(*(HDC *)(*(_DWORD *)dword_BAE270 + 564)); /*0x92f46f*/
      }
      else
      {
        sub_A2F820(65537, 0, ArgList_1); /*0x92f45f*/
      }
      sub_67EEB0(ArgList_3); /*0x92f47e*/
      sub_9E8290(&off_BAE190); /*0x92f488*/
      ((void (__thiscall *)(void ***, _DWORD, int, _DWORD, int, _DWORD))off_BAE190[14])(&off_BAE190, 0, 1, 0, 1, 0); /*0x92f4ad*/
      v41 = dword_BE6F2C; /*0x92f4c2*/
      v37 = dword_BE6F2C; /*0x92f4c8*/
      v42 = dword_BE7298; /*0x92f4d7*/
      v40 = dword_BE7298; /*0x92f4e1*/
      v47 = 0; /*0x92f4ee*/
      v43 = 0; /*0x92f4f6*/
      n1065353216 = 1065353216; /*0x92f4fe*/
      n1065353216_1 = 1065353216; /*0x92f506*/
      v46 = 0; /*0x92f50e*/
      v44 = 0; /*0x92f516*/
      n1065353216_2 = 1065353216; /*0x92f51e*/
      n1065353216_3 = 1065353216; /*0x92f526*/
      v51 = 1; /*0x92f52e*/
      v39 = 0; /*0x92f536*/
      v35 = 0; /*0x92f53e*/
      v38 = 0; /*0x92f546*/
      v36 = 0; /*0x92f54e*/
      v34 = 0; /*0x92f556*/
      v33 = xmmword_B424D0; /*0x92f55e*/
      (*(void (__thiscall **)(int, int *, int *, __int128 *))(*(_DWORD *)dword_BE73F4 + 12))( /*0x92f56b*/
        dword_BE73F4,
        &v43,
        &v35,
        &v33);
    }
    dword_BAE1A8 = 0; /*0x92f56e*/
    sub_9EFA70(); /*0x92f578*/
    sub_9EFA10(); /*0x92f57d*/
    dword_BA4714 = -1082130432; /*0x92f58c*/
    dword_BAE1A8 = 0; /*0x92f596*/
    ((void (__thiscall *)(void ***))off_BAE190[26])(&off_BAE190); /*0x92f5a0*/
    if ( dword_BAE1AC ) /*0x92f5aa*/
    {
      epoxy_glClear(); /*0x92f5e2*/
    }
    else if ( byte_BE65A5 ) /*0x92f5b8*/
    {
      SwapBuffers(*(HDC *)(*(_DWORD *)dword_BAE270 + 564)); /*0x92f5d5*/
    }
    else
    {
      sub_A2F820(65537, 0, ArgList_1); /*0x92f5c1*/
    }
  }
}
