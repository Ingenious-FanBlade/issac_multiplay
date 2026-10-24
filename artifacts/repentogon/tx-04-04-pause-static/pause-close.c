void __thiscall sub_98EC00(_DWORD *this)
{
  int v2; // ecx
  int n93; // edx
  int n93_1; // eax
  int n97; // eax
  const char *v6; // eax
  int v7; // eax
  const char *v8; // eax
  int v9; // eax
  int n18; // [esp+20h] [ebp-4h] BYREF

  if ( !*this ) /*0x98ec0c*/
    return; /*0x98ec0c*/
  v2 = this[1]; /*0x98ec12*/
  n93 = 93; /*0x98ec15*/
  n93_1 = 93; /*0x98ec1d*/
  if ( v2 != 1 ) /*0x98ec24*/
    n93_1 = 24; /*0x98ec24*/
  if ( !this[n93_1] ) /*0x98ec2b*/
    goto LABEL_23; /*0x98ec2b*/
  n97 = 97; /*0x98ec34*/
  if ( v2 != 1 ) /*0x98ec3e*/
    n97 = 28; /*0x98ec3e*/
  if ( LOBYTE(this[n97]) )
  {
    if ( v2 != 1 ) /*0x98ec4a*/
      n93 = 24; /*0x98ec4a*/
    v6 = (const char *)this[n93]; /*0x98ec4d*/
    if ( *((_DWORD *)v6 + 5) >= 0x10u ) /*0x98ec54*/
      v6 = *(const char **)v6; /*0x98ec56*/
    v7 = strcmp(v6, "Dissapear"); /*0x98ec64*/
    if ( v7 )
      v7 = v7 < 0 ? -1 : 1;
    if ( !v7 ) /*0x98ec87*/
      return; /*0x98ec87*/
LABEL_23:
    n18 = 18; /*0x98eccf*/
    sub_92FAD0(&n18); /*0x98ecdf*/
    sub_907C60(n18, 1060320051, 2, 0, 1065353216, 0); /*0x98ed0b*/
    sub_405D00("Dissapear", 0); /*0x98ed2a*/
    sub_403DE0(0); /*0x98ed44*/
    return; /*0x98ed44*/
  }
  if ( v2 != 1 ) /*0x98ec93*/
    n93 = 24; /*0x98ec93*/
  v8 = (const char *)this[n93]; /*0x98ec96*/
  if ( *((_DWORD *)v8 + 5) >= 0x10u ) /*0x98ec9d*/
    v8 = *(const char **)v8; /*0x98ec9f*/
  v9 = strcmp(v8, "Dissapear"); /*0x98ecaa*/
  if ( v9 )
    v9 = v9 < 0 ? -1 : 1;
  if ( v9 ) /*0x98eccd*/
    goto LABEL_23; /*0x98eccd*/
}
