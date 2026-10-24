void __thiscall sub_9F7FF0(_BYTE *this)
{
  _DWORD *v1; // esi

  if ( this[4] && !this[5] ) /*0x9f8022*/
  {
    ((void (__thiscall *)(void ***, int))off_BCE5C4[3])(&off_BCE5C4, -1); /*0x9f8038*/
    ((void (__thiscall *)(void ***, int))off_BCE5B8[3])(&off_BCE5B8, -1); /*0x9f8047*/
    v1 = dword_BCE5AC; /*0x9f805f*/
    if ( dword_BCE5AC != dword_BCE5B0 ) /*0x9f806b*/
    {
      do /*0x9f80a6*/
      {
        if ( (*(unsigned __int8 (__thiscall **)(_DWORD))(*(_DWORD *)*v1 + 116))(*v1) ) /*0x9f8077*/
          sub_4CAE50(0, (int)v1); /*0x9f8092*/
        ++v1; /*0x9f809d*/
      }
      while ( v1 != dword_BCE5B0 ); /*0x9f80a6*/
    }
    ((void (__thiscall *)(void ***))off_BCE5B8[4])(&off_BCE5B8); /*0x9f80bc*/
    ((void (__thiscall *)(void ***))off_BCE5C4[4])(&off_BCE5C4); /*0x9f80db*/
  }
}
