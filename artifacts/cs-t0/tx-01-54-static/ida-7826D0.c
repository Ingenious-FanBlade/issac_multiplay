char __thiscall sub_7826D0(_DWORD *ArgList, unsigned __int64 *a2, int a3)
{
  unsigned int v4; // eax
  int v5; // ecx
  int *v6; // ebx
  int v7; // eax
  int v8; // edi
  float v9; // xmm1_4
  int v10; // edx
  _DWORD *n60; // edi
  unsigned int v12; // eax
  unsigned int v13; // et2
  int v14; // ecx
  int (__stdcall *v15)(unsigned __int64 *, int, int); // edi
  int v16; // eax
  _DWORD *v17; // edi
  int n4; // ebx
  int v19; // eax
  int (__stdcall *v21)(unsigned __int64 *, int, int); // [esp+Ch] [ebp-4h] BYREF

  if ( (int)n60[39301] < 2
    || (LOBYTE(v4) = (*(int (__thiscall **)(_DWORD, _DWORD *))(*(_DWORD *)n60[39304] + 72))(n60[39304], ArgList),
        (_BYTE)v4) )
  {
    if ( (unsigned __int8)sub_7706E0(446, 0) )
    {
      if ( (_BYTE)a3 )
      {
        v6 = ArgList + 1294;
        if ( !ArgList[1294] )
        {
          v7 = sub_6EEF60(v5);
          v8 = sub_6FE410(n60, (_DWORD *)0x3E8, 106, ArgList + 207, &a5, ArgList, 0, v7, 0);
          v9 = *(float *)&a20 * 0.80000001;
          *(float *)(v8 + 228) = *(float *)&a19 * 0.80000001;
          *(float *)(v8 + 232) = v9;
          if ( *v6 )
          {
            v21 = (int (__stdcall *)(unsigned __int64 *, int, int))(ArgList + 1294);
            sub_4CF210(&v21);
          }
          *v6 = v8;
          if ( v8 )
            sub_4CDCF0((_BYTE)ArgList + 56);
          if ( *(_DWORD *)(v8 + 956) )
          {
            v21 = (int (__stdcall *)(unsigned __int64 *, int, int))(v8 + 956);
            sub_4CF210(&v21);
          }
          *(_DWORD *)(v8 + 956) = ArgList;
          sub_4CDCF0(v8 - 68);
          (*(void (__thiscall **)(int))(*(_DWORD *)v8 + 12))(v8);
        }
      }
    }
    LOBYTE(v4) = sub_7706E0(502, 0);
    v10 = a3;
    if ( (_BYTE)v4 )
    {
      if ( (_BYTE)a3 )
      {
        v4 = ArgList[90] & 0x4000;
        if ( !v4 )
        {
          n60 = n60;
          LOBYTE(v4) = sub_6FD350(n60);
          if ( !(_BYTE)v4 )
          {
            v4 = n60[39230] / 30;
            if ( !(n60[39230] % 30) )
            {
              v12 = sub_6EEF60(30);
              v13 = v12 % 3;
              v4 = v12 / 3;
              if ( !v13 )
                LOBYTE(v4) = sub_7C9590(ArgList, a2);
            }
          }
          v10 = a3;
        }
      }
    }
    v14 = ArgList[1271];
    if ( v14 )
    {
      v15 = *(int (__stdcall **)(unsigned __int64 *, int, int))(*(_DWORD *)v14 + 12);
      v16 = ArgList[90] & 0x4000;
      if ( v16 )
      {
        LOBYTE(v16) = 1;
        LOBYTE(v4) = v15(a2, v10, v16);
      }
      else
      {
        LOBYTE(v4) = v15(a2, v10, 0);
      }
    }
    else
    {
      v17 = ArgList + 1272;
      n4 = 4;
      do
      {
        if ( *v17 )
        {
          v21 = *(int (__stdcall **)(unsigned __int64 *, int, int))(*(_DWORD *)*v17 + 12);
          v19 = ArgList[90] & 0x4000;
          LOBYTE(v19) = v19 != 0;
          LOBYTE(v4) = v21(a2, v10, v19);
        }
        v10 = a3;
        ++v17;
        --n4;
      }
      while ( n4 );
    }
  }
  return v4;
}
