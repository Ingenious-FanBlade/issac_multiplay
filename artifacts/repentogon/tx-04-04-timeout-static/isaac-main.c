int __cdecl main(int argc, const char **argv, const char **envp)
{
  const char *__localhost_match; // ecx
  int i; // esi
  const char *v5; // eax
  bool v6; // cf
  unsigned __int8 v7; // dl
  int v8; // eax
  int v9; // ecx
  int v10; // ecx
  int j; // esi
  int v12; // esi
  int v13; // edi
  char v14; // bl
  double v15; // st7
  int v16; // eax
  void *v17; // ecx
  double v18; // xmm1_8
  int dwMilliseconds_1; // ecx
  char ArgList_2; // [esp+0h] [ebp-56Ch]
  char ArgList_3; // [esp+0h] [ebp-56Ch]
  char X; // [esp+8h] [ebp-564h]
  char ArgList; // [esp+10h] [ebp-55Ch]
  int ArgList_1; // [esp+10h] [ebp-55Ch]
  char ArgList_4; // [esp+10h] [ebp-55Ch]
  double v27; // [esp+20h] [ebp-54Ch]
  double v28; // [esp+28h] [ebp-544h]
  double v29; // [esp+38h] [ebp-534h]
  DWORD dwMilliseconds; // [esp+40h] [ebp-52Ch]
  char Buffer[1024]; // [esp+50h] [ebp-51Ch] BYREF
  char Format[256]; // [esp+450h] [ebp-11Ch] BYREF
  CPPEH_RECORD ms_exc; // [esp+554h] [ebp-18h]

  ms_exc.registration.TryLevel = 0; /*0x90ac9d*/
  sub_981060(); /*0x90aca4*/
  for ( i = 0; i < argc; ++i )
  {
    __localhost_match = "--localhost_match"; /*0x90acb8*/
    v5 = argv[i]; /*0x90acbd*/
    while ( 1 ) /*0x90acc2*/
    {
      v6 = *v5 < (unsigned int)*__localhost_match; /*0x90acc2*/
      if ( *v5 != *__localhost_match ) /*0x90acc4*/
        break; /*0x90acc4*/
      if ( !*v5 ) /*0x90acc8*/
        goto LABEL_8; /*0x90acc8*/
      v7 = v5[1]; /*0x90acca*/
      v6 = v7 < (unsigned int)__localhost_match[1]; /*0x90accd*/
      if ( v7 != __localhost_match[1] ) /*0x90acd0*/
        break; /*0x90acd0*/
      v5 += 2; /*0x90acd2*/
      __localhost_match += 2; /*0x90acd5*/
      if ( !v7 ) /*0x90acda*/
      {
LABEL_8:
        v8 = 0; /*0x90acdc*/
        goto LABEL_10; /*0x90acde*/
      }
    }
    v8 = v6 ? -1 : 1;
LABEL_10:
    if ( !v8 ) /*0x90ace7*/
    {
      time64(0); /*0x90acea*/
      sub_418B70(v9, Buffer, 0x400u, "%s%s%llu.txt", (char)&PathName); /*0x90ad0d*/
      goto LABEL_14; /*0x90ad15*/
    }
  }
  sub_418B70((int)__localhost_match, Buffer, 0x400u, "%s%s", (char)&PathName); /*0x90ad35*/
LABEL_14:
  sub_A2B090((int)Buffer, 15, 0); /*0x90ad3d*/
  off_BCE8E4[0] = sub_90AC40; /*0x90ad52*/
  CoInitialize(0); /*0x90ad5e*/
  sub_9EBCB0(epoxy_handle_external_wglMakeCurrent, 0); /*0x90ad70*/
  sub_A2AB60(argc, argv); /*0x90ad79*/
  sub_9E0DF0(0, "Command Line: \n", ArgList);
  for ( j = 0; j < argc; ++j ) /*0x90ad8d*/
  {
    sub_9E0DF0(0, "\t", ArgList_1); /*0x90ada0*/
    sub_9E0DF0(0, (char *)argv[j], X); /*0x90adaa*/
    sub_9E0DF0(0, "\n", ArgList_2); /*0x90adb6*/
  }
  sub_418B70(v10, Format, 0x100u, "Game Version: %s \n", (char)"J273");
  sub_9E0DF0(0, Format, ArgList_3); /*0x90ade5*/
  if ( (unsigned __int8)sub_A2BFB0("GL_ARB_color_buffer_float") || sub_A2BEB0(0) >= 30 ) /*0x90ae07*/
    epoxy_glClampColorARB(); /*0x90ae10*/
  sub_981AE0(argv, ArgList_1); /*0x90ae19*/
  v29 = 0.0; /*0x90ae24*/
  v12 = 0; /*0x90ae2c*/
  v13 = 0; /*0x90ae34*/
  v14 = 0; /*0x90ae3c*/
  while ( 1 ) /*0x90aef9*/
  {
    do /*0x90aef9*/
    {
      v28 = *(double *)sub_9F9690().m128_u64; /*0x90ae49*/
      v27 = v28 - v29; /*0x90ae59*/
      v15 = floor(1.0 / (v28 - v29) + 0.5); /*0x90ae89*/
      v16 = sub_A881D0(v15); /*0x90ae91*/
      v17 = dword_BE4A1C; /*0x90ae96*/
      *((_DWORD *)dword_BE4A1C + 76611) = v16; /*0x90ae9c*/
      v29 = v28; /*0x90aeaa*/
      if ( 0.01666666666666667 / v27 <= 1.1 ) /*0x90aeca*/
      {
        v12 = 0; /*0x90aeea*/
      }
      else if ( (unsigned int)++v12 > 0x1E ) /*0x90aed6*/
      {
        v14 = 1; /*0x90aed8*/
        v13 = 0; /*0x90aee0*/
      }
    }
    while ( byte_BAC6B4 ); /*0x90aef9*/
    LOBYTE(v17) = 0; /*0x90aeff*/
    sub_A2AC40(v17); /*0x90af01*/
    sub_92E3E0(1); /*0x90af08*/
    sub_92EC40(); /*0x90af0d*/
    if ( n2_0 ) /*0x90af19*/
      sub_84C1B0(); /*0x90af1b*/
    if ( !byte_BE65A5 ) /*0x90af2c*/
    {
      sub_A2F820(65537, 0, ArgList_4); /*0x90af39*/
      goto LABEL_30; /*0x90af39*/
    }
    if ( *(_BYTE *)(*(_DWORD *)dword_BAE270 + 8) == 1 ) /*0x90afc8*/
      break; /*0x90afc8*/
LABEL_30:
    if ( v14 || !HIBYTE(word_BAE274) ) /*0x90af4b*/
    {
      v18 = (0.01666666666666667 - (*(double *)sub_9F9690().m128_u64 - v28)) * -1000.0; /*0x90af6a*/
      dwMilliseconds_1 = 0; /*0x90af88*/
      if ( ~(int)v18 >= 0 && (int)v18 != -1 ) /*0x90af8a*/
        dwMilliseconds_1 = ~(int)v18; /*0x90af8c*/
      dwMilliseconds = dwMilliseconds_1; /*0x90af8f*/
      if ( dwMilliseconds_1 ) /*0x90af97*/
      {
        v13 = 0; /*0x90af99*/
        if ( !(unsigned __int8)sub_937390((char *)dword_BE4A1C + 134216) ) /*0x90afad*/
          Sleep(dwMilliseconds); /*0x90afbc*/
      }
      else if ( (unsigned int)++v13 > 0x1E ) /*0x90afeb*/
      {
        v14 = 0; /*0x90afed*/
        v12 = 0; /*0x90aff5*/
      }
      while ( *(double *)sub_9F9690().m128_u64 - v28 < 0.01666666666666667 ) /*0x90b019*/
        ; /*0x90b000*/
    }
  }
  sub_982C90(); /*0x90afce*/
  sub_A2AD80(); /*0x90afd3*/
  CoUninitialize(); /*0x90afd8*/
  ms_exc.registration.TryLevel = -2; /*0x90b02c*/
  return 0; /*0x90b035*/
}
