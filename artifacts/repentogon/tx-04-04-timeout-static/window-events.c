int __fastcall sub_A2AC40(char a1)
{
  double v2; // xmm0_8
  int v3; // edi
  int v4; // eax
  float v5; // xmm1_4
  int ArgList_2; // ecx
  _DWORD *v7; // esi
  struct _RTL_CRITICAL_SECTION *lpCriticalSection; // [esp-4h] [ebp-2Ch]
  int ArgList_1; // [esp+0h] [ebp-28h]
  int v11; // [esp+4h] [ebp-24h]
  char ArgList[4]; // [esp+10h] [ebp-18h] BYREF
  char v13; // [esp+14h] [ebp-14h]
  float v14; // [esp+18h] [ebp-10h]
  int v15; // [esp+24h] [ebp-4h]

  if ( *(_DWORD *)SteamInternal_ContextInit(&off_BD0218) ) /*0xa2ac78*/
    SteamAPI_RunCallbacks(); /*0xa2ac7d*/
  sub_9F7FF0(); /*0xa2ac88*/
  if ( a1 ) /*0xa2ac8f*/
  {
    if ( byte_BE65A5 ) /*0xa2ac98*/
      sub_A2A390(); /*0xa2acab*/
    else
      sub_A2F820(65537, 0, ArgList_1); /*0xa2aca1*/
    sub_A31960(); /*0xa2acb0*/
    sub_9EDCC0(&off_BCE438); /*0xa2acba*/
  }
  v2 = *(double *)sub_9F9690().m128_u64; /*0xa2acbf*/
  v3 = dword_BE6B04; /*0xa2acc4*/
  v4 = 0; /*0xa2acca*/
  *(float *)&v2 = v2; /*0xa2accc*/
  *(_DWORD *)ArgList = 0; /*0xa2acd0*/
  v13 = 0; /*0xa2acd3*/
  v5 = *(float *)&v2 - *(float *)&dword_BCE914; /*0xa2acd9*/
  dword_BCE914 = LODWORD(v2); /*0xa2ace1*/
  ArgList_2 = *(_DWORD *)(dword_BE6B04 + 40); /*0xa2ace9*/
  v14 = v5; /*0xa2acec*/
  if ( ArgList_2 ) /*0xa2acf3*/
  {
    *(_DWORD *)ArgList = ArgList_2; /*0xa2acf5*/
    sub_A383A0(ArgList_2, ArgList_1, v11); /*0xa2acf8*/
    v4 = *(_DWORD *)ArgList; /*0xa2acfd*/
  }
  v15 = 0; /*0xa2ad05*/
  v7 = *(_DWORD **)(v3 + 16); /*0xa2ad0c*/
  if ( v7 != *(_DWORD **)(v3 + 20) ) /*0xa2ad12*/
  {
    do /*0xa2ad2d*/
    {
      sub_A1E990(*v7); /*0xa2ad16*/
      (*(void (__thiscall **)(_DWORD))(*(_DWORD *)*v7 + 24))(*v7); /*0xa2ad1f*/
      ++v7; /*0xa2ad27*/
    }
    while ( v7 != *(_DWORD **)(v3 + 20) ); /*0xa2ad2d*/
    v4 = *(_DWORD *)ArgList; /*0xa2ad2f*/
  }
  if ( v4 ) /*0xa2ad34*/
  {
    lpCriticalSection = *(struct _RTL_CRITICAL_SECTION **)v4; /*0xa2ad36*/
    *(_BYTE *)(v4 + 4) = 0; /*0xa2ad38*/
    LeaveCriticalSection(lpCriticalSection); /*0xa2ad3c*/
    *(_DWORD *)ArgList = 0; /*0xa2ad42*/
  }
  v15 = -1; /*0xa2ad4c*/
  sub_A38270((char)ArgList, ArgList_1, v11); /*0xa2ad53*/
  return sub_A23FD0(&off_BCE970); /*0xa2ad62*/
}
