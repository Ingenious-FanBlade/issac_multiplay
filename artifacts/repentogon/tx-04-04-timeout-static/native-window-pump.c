__int16 sub_A2A390()
{
  int i; // esi
  void (__cdecl *v1)(int); // eax
  int (__cdecl *n0x8000_2)(int, int, _DWORD, _DWORD, int); // eax
  unsigned int v3; // esi
  int v4; // edi
  int v5; // ebx
  int AsyncKeyState; // esi
  SHORT v7; // ax
  int v8; // ecx
  unsigned int n0x8000; // esi
  unsigned int n0x8000_1; // edi
  int v12; // [esp+10h] [ebp-38h]
  int v13; // [esp+14h] [ebp-34h]
  struct tagMSG Msg; // [esp+18h] [ebp-30h] BYREF
  struct tagRECT Rect; // [esp+34h] [ebp-14h] BYREF

  while ( PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE) ) /*0xa2a3ba*/
  {
    if ( Msg.message == 18 ) /*0xa2a3cb*/
    {
      for ( i = ::i; i; i = *(_DWORD *)i ) /*0xa2a3d5*/
      {
        v1 = *(void (__cdecl **)(int))(i + 492); /*0xa2a3e0*/
        *(_BYTE *)(i + 8) = 1; /*0xa2a3e6*/
        if ( v1 ) /*0xa2a3ec*/
          v1(i); /*0xa2a3ef*/
      }
    }
    else
    {
      TranslateMessage(&Msg); /*0xa2a401*/
      DispatchMessageA(&Msg); /*0xa2a408*/
    }
  }
  LOWORD(n0x8000_2) = dword_BE666C; /*0xa2a421*/
  v13 = dword_BE666C; /*0xa2a426*/
  if ( dword_BE666C )
  {
    v3 = (unsigned int)GetAsyncKeyState(16) >> 31; /*0xa2a441*/
    v4 = v3 | 2; /*0xa2a448*/
    if ( GetAsyncKeyState(17) >= 0 ) /*0xa2a450*/
      v4 = v3; /*0xa2a450*/
    v5 = v4 | 4; /*0xa2a457*/
    if ( GetAsyncKeyState(18) >= 0 ) /*0xa2a45f*/
      v5 = v4; /*0xa2a45f*/
    AsyncKeyState = GetAsyncKeyState(92); /*0xa2a46c*/
    v7 = GetAsyncKeyState(91); /*0xa2a46f*/
    v8 = v5 | 8; /*0xa2a474*/
    if ( (AsyncKeyState | v7) >= 0 ) /*0xa2a47e*/
      v8 = v5; /*0xa2a47e*/
    v12 = v8; /*0xa2a481*/
    n0x8000 = (unsigned __int16)GetAsyncKeyState(160); /*0xa2a48c*/
    LOWORD(n0x8000_2) = GetAsyncKeyState(161); /*0xa2a48f*/
    n0x8000_1 = (unsigned __int16)n0x8000_2; /*0xa2a49f*/
    if ( n0x8000 < 0x8000 && *(_BYTE *)(v13 + 440) == 1 )
    {
      *(_BYTE *)(v13 + 440) = *(_BYTE *)(v13 + 68) != 0 ? 3 : 0;
      n0x8000_2 = *(int (__cdecl **)(int, int, _DWORD, _DWORD, int))(v13 + 528); /*0xa2a4bc*/
      if ( n0x8000_2 ) /*0xa2a4c4*/
        LOWORD(n0x8000_2) = n0x8000_2(v13, 340, 0, 0, v12); /*0xa2a4d1*/
    }
    if ( n0x8000_1 < 0x8000 && *(_BYTE *)(v13 + 444) == 1 )
    {
      *(_BYTE *)(v13 + 444) = *(_BYTE *)(v13 + 68) != 0 ? 3 : 0;
      n0x8000_2 = *(int (__cdecl **)(int, int, _DWORD, _DWORD, int))(v13 + 528); /*0xa2a4f6*/
      if ( n0x8000_2 ) /*0xa2a4fe*/
        LOWORD(n0x8000_2) = n0x8000_2(v13, 344, 0, 0, v12); /*0xa2a50b*/
    }
    if ( *(_DWORD *)(v13 + 88) == 212995 && !*(_BYTE *)(v13 + 548) ) /*0xa2a519*/
    {
      GetClientRect(*(HWND *)(v13 + 536), &Rect); /*0xa2a52d*/
      LOWORD(n0x8000_2) = sub_A2A580(v13); /*0xa2a559*/
      *(_BYTE *)(v13 + 548) = 1; /*0xa2a55e*/
    }
  }
  return (__int16)n0x8000_2; /*0xa2a565*/
}
