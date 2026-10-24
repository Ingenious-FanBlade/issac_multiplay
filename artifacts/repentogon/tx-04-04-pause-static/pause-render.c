void __thiscall sub_98D4E0(char *this)
{
  unsigned int v2; // esi
  int v3; // edi
  int v4; // ecx
  _DWORD *n2; // eax
  int v6; // eax
  int v7; // edi
  int v8; // eax
  int v9; // edi
  int v10; // eax
  int v11; // edi
  int v12; // eax
  int v13; // edi
  int v14; // eax
  int v15; // edi
  int v16; // eax
  int v17; // edi
  int v18; // ecx
  int n372; // esi
  int n372_1; // eax
  int n388; // edi
  int n388_1; // eax
  int n372_2; // eax
  const char *v24; // eax
  int v25; // eax
  int v26; // eax
  int v27; // edx
  int n404; // eax
  int n400; // ecx
  const char *v30; // eax
  int v31; // eax
  int v32; // edi
  unsigned int v33; // esi
  int v34; // eax
  int v35; // esi
  int v36; // ecx
  int v37; // edi
  int v38; // edx
  int n24; // edi
  int v40; // esi
  int v41; // edi
  char n64; // si
  char n64_1; // cl
  int n585; // eax
  int v45; // esi
  int n368; // ecx
  int n400_1; // eax
  char ArgList; // [esp+0h] [ebp-20h]
  char v49; // [esp+13h] [ebp-Dh]
  char v50; // [esp+13h] [ebp-Dh]
  char v51; // [esp+13h] [ebp-Dh]
  char v52; // [esp+13h] [ebp-Dh]
  int n24_1; // [esp+14h] [ebp-Ch]
  int v54; // [esp+18h] [ebp-8h]
  _DWORD *v55; // [esp+1Ch] [ebp-4h]

  if ( !*(_DWORD *)this || *((_DWORD *)n2_0 + 28175) && *((_DWORD *)n2_0 + 28317) ) /*0x98d506*/
    return; /*0x98d50d*/
  if ( *((_DWORD *)dword_BE4A1C + 76782) == *((_DWORD *)dword_BE4A1C + 76783) ) /*0x98d525*/
  {
    if ( sub_9950B0(689, (char *)n2_0 + 113320, 1) ) /*0x98d535*/
    {
      v2 = 0; /*0x98d543*/
      v3 = *((_DWORD *)n2_0 + 24768); /*0x98d545*/
      if ( *(_DWORD *)(v3 + 4708) ) /*0x98d54b*/
      {
        do /*0x98d574*/
        {
          v4 = *(_DWORD *)(*(_DWORD *)(v3 + 4700) + 4 * v2); /*0x98d559*/
          if ( *(_DWORD *)(v4 + 40) == 5 && *(_DWORD *)(v4 + 44) == 100 ) /*0x98d566*/
            sub_6EA710(); /*0x98d568*/
          ++v2; /*0x98d56d*/
        }
        while ( v2 < *(_DWORD *)(v3 + 4708) ); /*0x98d574*/
      }
    }
  }
  sub_420A70(this + 2112); /*0x98d57c*/
  n2 = n2_0; /*0x98d581*/
  if ( *((_DWORD *)n2_0 + 35749) != 2 ) /*0x98d58d*/
  {
    *((_DWORD *)n2_0 + 35749) = 1; /*0x98d58f*/
    n2[35751] = 2; /*0x98d599*/
  }
  if ( (unsigned __int8)sub_405F50((char)"Idle", 1) ) /*0x98d5b0*/
  {
    if ( *((_DWORD *)this + 162) ) /*0x98d5b9*/
    {
      sub_4042A0(this + 644); /*0x98d5cc*/
      if ( *((_DWORD *)this + 162) ) /*0x98d5d1*/
        this[664] = 0; /*0x98d5db*/
    }
  }
  if ( *((_DWORD *)this + 162) ) /*0x98d5e2*/
  {
    if ( *(int *)(*((_DWORD *)this + 161) + 128) > 1 /*0x98d61d*/
      || (sub_9E0DF0(3, "LayerId out of bounds\n", ArgList), *(int *)(*((_DWORD *)this + 161) + 128) > 1) )
    {
      v6 = *((_DWORD *)this + 162); /*0x98d61f*/
      v7 = *(_DWORD *)(v6 + 60); /*0x98d625*/
      if ( !v6 || v7 < 0 || v7 >= *(_DWORD *)(v6 + 28) ) /*0x98d633*/
        sub_9E0DF0(3, "id out of bounds\n", ArgList); /*0x98d63c*/
      *(_DWORD *)(*((_DWORD *)this + 163) + 4 * v7) = 0; /*0x98d65a*/
      sub_404030(this + 644, v7); /*0x98d661*/
    }
  }
  if ( *((_DWORD *)this + 162) ) /*0x98d666*/
  {
    if ( *(int *)(*((_DWORD *)this + 161) + 128) > 2 /*0x98d6a1*/
      || (sub_9E0DF0(3, "LayerId out of bounds\n", ArgList), *(int *)(*((_DWORD *)this + 161) + 128) > 2) )
    {
      v8 = *((_DWORD *)this + 162); /*0x98d6a3*/
      v9 = *(_DWORD *)(v8 + 64); /*0x98d6a9*/
      if ( !v8 || v9 < 0 || v9 >= *(_DWORD *)(v8 + 28) ) /*0x98d6b7*/
        sub_9E0DF0(3, "id out of bounds\n", ArgList); /*0x98d6c0*/
      *(_DWORD *)(*((_DWORD *)this + 163) + 4 * v9) = 0; /*0x98d6de*/
      sub_404030(this + 644, v9); /*0x98d6e5*/
    }
  }
  if ( *((_DWORD *)this + 162) ) /*0x98d6ea*/
  {
    if ( *(int *)(*((_DWORD *)this + 161) + 128) > 3 /*0x98d725*/
      || (sub_9E0DF0(3, "LayerId out of bounds\n", ArgList), *(int *)(*((_DWORD *)this + 161) + 128) > 3) )
    {
      v10 = *((_DWORD *)this + 162); /*0x98d727*/
      v11 = *(_DWORD *)(v10 + 68); /*0x98d72d*/
      if ( !v10 || v11 < 0 || v11 >= *(_DWORD *)(v10 + 28) ) /*0x98d73b*/
        sub_9E0DF0(3, "id out of bounds\n", ArgList); /*0x98d744*/
      *(_DWORD *)(*((_DWORD *)this + 163) + 4 * v11) = 0; /*0x98d762*/
      sub_404030(this + 644, v11); /*0x98d769*/
    }
  }
  if ( *((_DWORD *)this + 162) ) /*0x98d76e*/
  {
    if ( *(int *)(*((_DWORD *)this + 161) + 128) > 4 /*0x98d7a9*/
      || (sub_9E0DF0(3, "LayerId out of bounds\n", ArgList), *(int *)(*((_DWORD *)this + 161) + 128) > 4) )
    {
      v12 = *((_DWORD *)this + 162); /*0x98d7ab*/
      v13 = *(_DWORD *)(v12 + 72); /*0x98d7b1*/
      if ( !v12 || v13 < 0 || v13 >= *(_DWORD *)(v12 + 28) ) /*0x98d7bf*/
        sub_9E0DF0(3, "id out of bounds\n", ArgList); /*0x98d7c8*/
      *(_DWORD *)(*((_DWORD *)this + 163) + 4 * v13) = 0; /*0x98d7e6*/
      sub_404030(this + 644, v13); /*0x98d7ed*/
    }
  }
  if ( *((_DWORD *)this + 162) ) /*0x98d7f2*/
  {
    if ( *(int *)(*((_DWORD *)this + 161) + 128) > 5 /*0x98d82d*/
      || (sub_9E0DF0(3, "LayerId out of bounds\n", ArgList), *(int *)(*((_DWORD *)this + 161) + 128) > 5) )
    {
      v14 = *((_DWORD *)this + 162); /*0x98d82f*/
      v15 = *(_DWORD *)(v14 + 76); /*0x98d835*/
      if ( !v14 || v15 < 0 || v15 >= *(_DWORD *)(v14 + 28) ) /*0x98d843*/
        sub_9E0DF0(3, "id out of bounds\n", ArgList); /*0x98d84c*/
      *(_DWORD *)(*((_DWORD *)this + 163) + 4 * v15) = 0; /*0x98d86a*/
      sub_404030(this + 644, v15); /*0x98d871*/
    }
  }
  if ( *((_DWORD *)this + 162) ) /*0x98d876*/
  {
    if ( *(int *)(*((_DWORD *)this + 161) + 128) > 6 /*0x98d8b1*/
      || (sub_9E0DF0(3, "LayerId out of bounds\n", ArgList), *(int *)(*((_DWORD *)this + 161) + 128) > 6) )
    {
      v16 = *((_DWORD *)this + 162); /*0x98d8b3*/
      v17 = *(_DWORD *)(v16 + 80); /*0x98d8b9*/
      if ( !v16 || v17 < 0 || v17 >= *(_DWORD *)(v16 + 28) ) /*0x98d8c7*/
        sub_9E0DF0(3, "id out of bounds\n", ArgList); /*0x98d8d0*/
      *(_DWORD *)(*((_DWORD *)this + 163) + 4 * v17) = 0; /*0x98d8ee*/
      sub_404030(this + 644, v17); /*0x98d8f5*/
    }
  }
  v18 = *((_DWORD *)this + 1); /*0x98d8fa*/
  n372 = 372; /*0x98d8fd*/
  n372_1 = 372; /*0x98d905*/
  if ( v18 != 1 ) /*0x98d90c*/
    n372_1 = 96; /*0x98d90c*/
  if ( *(_DWORD *)&this[n372_1] )
  {
    n388 = 388; /*0x98d919*/
    n388_1 = 388; /*0x98d929*/
    if ( v18 != 1 ) /*0x98d92b*/
      n388_1 = 112; /*0x98d92b*/
    if ( !this[n388_1] )
    {
      n372_2 = 372; /*0x98d93d*/
      if ( v18 != 1 ) /*0x98d93f*/
        n372_2 = 96; /*0x98d93f*/
      v24 = *(const char **)&this[n372_2]; /*0x98d942*/
      if ( *((_DWORD *)v24 + 5) >= 0x10u ) /*0x98d949*/
        v24 = *(const char **)v24; /*0x98d94b*/
      v25 = strcmp(v24, "Appear"); /*0x98d956*/
      if ( v25 )
        v25 = v25 < 0 ? -1 : 1;
      if ( !v25 ) /*0x98d979*/
      {
        sub_405D00("Idle", 0); /*0x98d998*/
        v26 = *((_DWORD *)this + 1); /*0x98d99d*/
        if ( v26 != 1 ) /*0x98d9a8*/
          n372 = 96; /*0x98d9a8*/
        if ( *(_DWORD *)&this[n372] ) /*0x98d9ab*/
        {
          if ( v26 != 1 ) /*0x98d9b4*/
            n388 = 112; /*0x98d9b4*/
          this[n388] = 0; /*0x98d9b9*/
        }
        sub_406430((char)"Cursor", 1); /*0x98d9d9*/
        v27 = *((_DWORD *)this + 1); /*0x98d9de*/
        n404 = 404; /*0x98d9e1*/
        if ( v27 != 1 ) /*0x98d9ee*/
          n404 = 128; /*0x98d9ee*/
        if ( *(_DWORD *)&this[n404] ) /*0x98d9f1*/
        {
          n400 = 400; /*0x98da0b*/
          if ( v27 != 1 ) /*0x98da10*/
            n400 = 124; /*0x98da10*/
          sub_4042A0(&this[n400]); /*0x98da15*/
        }
        goto LABEL_104; /*0x98da1a*/
      }
      v18 = *((_DWORD *)this + 1); /*0x98da1c*/
    }
    if ( v18 != 1 ) /*0x98da27*/
      n388 = 112; /*0x98da27*/
    if ( !this[n388] )
    {
      if ( v18 != 1 ) /*0x98da35*/
        n372 = 96; /*0x98da35*/
      v30 = *(const char **)&this[n372]; /*0x98da38*/
      if ( v30 ) /*0x98da3d*/
      {
        if ( *((_DWORD *)v30 + 5) >= 0x10u ) /*0x98da43*/
          v30 = *(const char **)v30; /*0x98da45*/
      }
      else
      {
        v30 = byte_AAC314; /*0x98da49*/
      }
      v31 = strcmp(v30, "Dissapear"); /*0x98da57*/
      if ( v31 )
        v31 = v31 < 0 ? -1 : 1;
      if ( !v31 ) /*0x98da7a*/
        *(_DWORD *)this = 0; /*0x98da7c*/
    }
  }
LABEL_104:
  if ( *((_DWORD *)dword_BE4A1C + 76782) == *((_DWORD *)dword_BE4A1C + 76783) ) /*0x98da8f*/
    v32 = *(_DWORD *)(sub_4131B0(n2_0, *((_DWORD *)this + 599)) + 5644); /*0x98daab*/
  else
    v32 = sub_8E7380(); /*0x98da96*/
  if ( *((_DWORD *)dword_BE4A1C + 76782) == *((_DWORD *)dword_BE4A1C + 76783) /*0x98daf6*/
    || *((_DWORD *)n2_0 + 38634)
    || (v33 = (dword_BE746C - dword_BE7468) >> 3,
        v49 = 1,
        v33 >= (*(int (__thiscall **)(int *))(dword_BE7458 + 8))(&dword_BE7458)) )
  {
    v49 = 0; /*0x98daf8*/
  }
  *(_BYTE *)(sub_406C50("Invite") + 116) = v49; /*0x98db20*/
  if ( *((_DWORD *)dword_BE4A1C + 76782) == *((_DWORD *)dword_BE4A1C + 76783) /*0x98db4d*/
    || (v50 = 1, (unsigned int)((dword_BE746C - dword_BE7468) >> 3) <= 1) )
  {
    v50 = 0; /*0x98db4f*/
  }
  *(_BYTE *)(sub_406C50("Kick") + 116) = v50; /*0x98db77*/
  if ( !*(_BYTE *)(sub_406C50("Invite") + 116) || (v51 = 1, v32) ) /*0x98dba1*/
    v51 = 0; /*0x98dba3*/
  *(_BYTE *)(sub_406C50("InviteKey") + 116) = v51; /*0x98dbd2*/
  if ( !*(_BYTE *)(sub_406C50("Kick") + 116) || (v52 = 1, v32) ) /*0x98dbf7*/
    v52 = 0; /*0x98dbf9*/
  *(_BYTE *)(sub_406C50("KickKey") + 116) = v52; /*0x98dc1c*/
  v34 = sub_4131B0(n2_0, *((_DWORD *)this + 599)); /*0x98dc2b*/
  v35 = *(_DWORD *)(v34 + 7644); /*0x98dc30*/
  v55 = (_DWORD *)(v34 + 7640); /*0x98dc3b*/
  v36 = *(_DWORD *)(v34 + 7640); /*0x98dc41*/
  v37 = (v35 - v36) / 28; /*0x98dc56*/
  v38 = 0; /*0x98dc58*/
  if ( v37 - 24 > 0 ) /*0x98dc5f*/
    v38 = v37 - 24; /*0x98dc5f*/
  v54 = v38; /*0x98dc62*/
  if ( v37 )
  {
    n24 = 0; /*0x98dc71*/
    n24_1 = 0; /*0x98dc73*/
    v40 = v35 - 28 * *((_DWORD *)this + 9); /*0x98dc83*/
    if ( v40 != v36 )
    {
      do
      {
        if ( n24 >= 24 ) /*0x98dc93*/
          break; /*0x98dc93*/
        v41 = *(_BYTE *)(v40 - 24) ? 0 : *(_DWORD *)(v40 - 20);
        if ( sub_72A400(v41) && (sub_4131B0(n2_0, *((_DWORD *)this + 599)), (unsigned __int8)sub_7699D0(v41, 0)) ) /*0x98dcce*/
        {
          n24 = ++n24_1; /*0x98dcdb*/
        }
        else
        {
          --v54; /*0x98dce2*/
          n24 = n24_1; /*0x98dce6*/
        }
        v40 -= 28; /*0x98dcee*/
      }
      while ( v40 != *v55 );
    }
  }
  n64 = 64; /*0x98dcf9*/
  n64_1 = 64; /*0x98dcfe*/
  if ( *((_DWORD *)this + 1) != 1 ) /*0x98dd05*/
    n64_1 = 44; /*0x98dd05*/
  *(_BYTE *)(sub_406B60((_BYTE)this + n64_1, 6) + 116) = *((_DWORD *)this + 9) > 0; /*0x98dd1a*/
  if ( *((_DWORD *)this + 1) != 1 ) /*0x98dd21*/
    n64 = 44; /*0x98dd21*/
  *(_BYTE *)(sub_406B60((_BYTE)this + n64, 7) + 116) = *((_DWORD *)this + 9) < v54; /*0x98dd36*/
  n585 = 585; /*0x98dd39*/
  v45 = *((_DWORD *)this + 1); /*0x98dd3e*/
  if ( v45 != 1 ) /*0x98dd49*/
    n585 = 309; /*0x98dd49*/
  if ( this[n585] ) /*0x98dd4c*/
  {
    n368 = 368; /*0x98dd5a*/
    if ( v45 != 1 ) /*0x98dd5f*/
      n368 = 92; /*0x98dd5f*/
    sub_404480(&this[n368]); /*0x98dd64*/
    n400_1 = 400; /*0x98dd71*/
    if ( v45 != 1 ) /*0x98dd76*/
      n400_1 = 124; /*0x98dd76*/
    sub_404480(&this[n400_1]); /*0x98dd7c*/
  }
}
