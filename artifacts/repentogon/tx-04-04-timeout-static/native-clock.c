__int64 __usercall sub_9F9690@<xmm0>()
{
  __int64 result; // xmm0_8
  LARGE_INTEGER PerformanceCount_1; // rax
  LARGE_INTEGER PerformanceCount; // [esp+0h] [ebp-8h] BYREF

  if ( byte_BE65A5 ) /*0x9f969d*/
  {
    if ( byte_BE6690 ) /*0x9f96bc*/
    {
      QueryPerformanceCounter(&PerformanceCount); /*0x9f96c2*/
      PerformanceCount_1 = PerformanceCount; /*0x9f96c8*/
    }
    else
    {
      PerformanceCount_1.QuadPart = (unsigned int)dword_BE66B8(); /*0x9f96d0*/
    }
    *(double *)&result = (double)(unsigned __int64)(PerformanceCount_1.QuadPart - qword_BE66A0) /*0x9f96eb*/
                       * *(double *)&qword_BE6698;
  }
  else
  {
    sub_A2F820(65537, 0, PerformanceCount.QuadPart); /*0x9f96a6*/
    *(double *)&result = 0.0; /*0x9f96ae*/
  }
  return result; /*0x9f96b1*/
}
