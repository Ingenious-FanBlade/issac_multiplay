char __thiscall sub_77B020(int ArgList)
{
  int ArgList_1; // edi
  float v2; // xmm2_4
  float v3; // xmm3_4
  int v4; // eax
  int v5; // eax
  char v6; // al
  int v7; // ecx
  char v8; // al
  float v9; // eax
  char v10; // cl
  char v11; // al
  float *v12; // eax
  unsigned __int64 v13; // rax
  int v14; // esi
  unsigned int v15; // kr08_4
  __int64 v16; // rax
  __int64 v17; // kr00_8
  float *v18; // esi
  float v19; // xmm0_4
  float v20; // xmm0_4
  float v21; // xmm2_4
  float v22; // xmm0_4
  float v23; // xmm0_4
  float v24; // xmm2_4
  float v25; // xmm0_4
  float v26; // xmm1_4
  float v27; // xmm3_4
  float v28; // xmm1_4
  float v29; // xmm1_4
  int v30; // ecx
  int n604; // eax
  float v32; // xmm0_4
  float v33; // xmm0_4
  float v34; // xmm1_4
  int v35; // ecx
  bool v36; // zf
  int n2_9; // edx
  int v38; // edx
  char v39; // al
  float v40; // esi
  int v41; // eax
  int v42; // ecx
  int v43; // esi
  _DWORD *v44; // ecx
  _DWORD **v45; // esi
  float v46; // xmm2_4
  float v47; // xmm3_4
  int v48; // eax
  float v49; // xmm0_4
  float v50; // eax
  int v51; // edx
  unsigned int n2_1; // esi
  float v53; // xmm0_4
  int v54; // eax
  float v55; // xmm2_4
  float v56; // xmm0_4
  int v57; // eax
  float v58; // xmm1_4
  float v59; // xmm0_4
  char n2_11; // al
  char v61; // al
  _DWORD *n60; // ecx
  char v63; // dl
  char v64; // al
  int n150; // eax
  int v66; // ecx
  int v67; // eax
  float v68; // xmm1_4
  float v69; // xmm0_4
  int v70; // edi
  int v71; // eax
  int v72; // esi
  __int128 v73; // xmm0
  _DWORD *n298; // esi
  int v75; // ecx
  int v76; // eax
  int v77; // edi
  void (__thiscall *v78)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD); // esi
  int v79; // ecx
  bool v80; // al
  int v81; // edx
  float v82; // esi
  int v83; // esi
  char v84; // al
  int v85; // eax
  int v86; // edx
  int ArgList_3; // esi
  float *v88; // edi
  _DWORD *n60_1; // eax
  int v90; // esi
  int v91; // ecx
  unsigned int v92; // eax
  int ArgList_4; // ecx
  int v94; // edx
  float v95; // xmm0_4
  float v96; // xmm2_4
  int v97; // edi
  int v98; // ecx
  unsigned int v99; // eax
  double v100; // xmm0_8
  unsigned int *v101; // eax
  unsigned int v102; // xmm0_4
  _DWORD *v103; // eax
  void (__thiscall *v104)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD); // esi
  int v105; // eax
  float v106; // xmm0_4
  _DWORD *v107; // eax
  int v108; // eax
  int v109; // eax
  int v110; // esi
  int v111; // eax
  int v112; // eax
  int v113; // eax
  char *HeadRight; // eax
  void *v115; // ecx
  Concurrency::details::_ContextCallback *v116; // eax
  int v117; // eax
  _DWORD *v118; // eax
  int v119; // eax
  int v120; // eax
  int *v121; // eax
  void *v122; // eax
  _DWORD *v123; // eax
  int v124; // esi
  int v125; // eax
  int n1161527296; // xmm0_4
  _DWORD *v127; // eax
  int v128; // esi
  int v129; // eax
  __int128 *v130; // eax
  int n1161527296_1; // xmm0_4
  int v132; // eax
  char v133; // al
  int v134; // ecx
  _DWORD *v135; // esi
  _DWORD *v136; // eax
  int n4; // ecx
  int v138; // ecx
  int v139; // edi
  void (__thiscall *v140)(_DWORD, _DWORD); // esi
  int v141; // eax
  float *v142; // eax
  int n4_1; // ecx
  float v144; // eax
  void (__thiscall *v145)(_DWORD, _DWORD); // esi
  int v146; // eax
  int n2_2; // eax
  int v148; // eax
  int v149; // eax
  char v150; // al
  int v151; // ecx
  int v152; // eax
  int v153; // eax
  int v154; // esi
  int v155; // eax
  int v156; // eax
  int n2_4; // esi
  int n1056964608; // xmm0_4
  int v159; // eax
  int v160; // eax
  unsigned int n2_5; // esi
  int v162; // eax
  int v163; // eax
  int n4_2; // esi
  int v165; // eax
  int v166; // eax
  int v167; // eax
  unsigned int n7_1; // esi
  int v169; // eax
  int v170; // eax
  unsigned int v171; // esi
  _DWORD *v172; // eax
  int v173; // eax
  int v174; // esi
  int v175; // eax
  int v176; // ecx
  int v177; // eax
  int n2_6; // edi
  int v179; // eax
  int v180; // eax
  int v181; // eax
  void (__thiscall *v182)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD); // esi
  int v183; // eax
  int v184; // eax
  void *v185; // eax
  char v186; // al
  int v187; // ecx
  int v188; // eax
  int v189; // eax
  int v190; // edi
  int v191; // eax
  int v192; // eax
  int v193; // eax
  int v194; // eax
  int n2_7; // esi
  unsigned int n7_2; // xmm0_4
  int v197; // eax
  int v198; // eax
  int v199; // eax
  unsigned int n2_8; // esi
  float v201; // xmm0_4
  int v202; // eax
  int v203; // eax
  int v204; // eax
  int v205; // ecx
  int v206; // eax
  int v207; // esi
  float v208; // xmm0_4
  int v209; // eax
  int n481; // eax
  int n13; // eax
  int v212; // eax
  int v213; // esi
  Concurrency::details::_ContextCallback *v214; // eax
  int v215; // esi
  float v216; // xmm0_4
  int v217; // esi
  char v218; // al
  int *v219; // ecx
  int n289; // ecx
  char v221; // al
  __int64 *v222; // esi
  int v223; // ecx
  int v224; // edi
  int v225; // eax
  int v226; // eax
  int v227; // eax
  int v228; // esi
  _DWORD *v229; // eax
  _DWORD *v230; // eax
  int v231; // eax
  int v232; // eax
  int v233; // eax
  double v234; // st7
  char v235; // al
  __int64 *v236; // esi
  int v237; // ecx
  int v238; // edi
  int v239; // eax
  int v240; // eax
  int v241; // eax
  int v242; // esi
  _DWORD *v243; // eax
  _DWORD *v244; // eax
  int v245; // eax
  int v246; // eax
  int v247; // eax
  double v248; // st7
  int v249; // edi
  int v250; // eax
  int v251; // eax
  int v252; // eax
  int v253; // esi
  int v254; // eax
  int v255; // eax
  _DWORD *v256; // eax
  float v257; // xmm0_4
  int v258; // eax
  int v259; // ecx
  int v260; // eax
  int v261; // eax
  int v262; // esi
  int v263; // eax
  int v264; // eax
  _DWORD *v265; // eax
  int v266; // edi
  void (__thiscall *v267)(_DWORD, _DWORD); // esi
  _DWORD *v268; // eax
  int v269; // eax
  int v270; // eax
  int v271; // ecx
  int v272; // eax
  int v273; // eax
  unsigned int n7_3; // eax
  int v275; // eax
  int v276; // ecx
  int v277; // esi
  int v278; // eax
  int v279; // eax
  int v280; // eax
  _DWORD *v281; // eax
  float v282; // xmm0_4
  int v283; // eax
  int v284; // eax
  int v285; // eax
  int v286; // esi
  int v287; // esi
  int v288; // ecx
  int v289; // esi
  int v290; // eax
  int v291; // esi
  _DWORD *v292; // eax
  int v293; // eax
  int v294; // eax
  int v295; // eax
  float v296; // xmm0_4
  int v297; // ecx
  int v298; // edi
  int v299; // eax
  int v300; // eax
  int v301; // eax
  int v302; // eax
  Concurrency::details::_ContextCallback *v303; // eax
  int v304; // eax
  int v305; // esi
  int v306; // eax
  int v307; // eax
  int v308; // eax
  int n45; // esi
  float v310; // xmm0_4
  int v311; // ecx
  int v312; // eax
  int v313; // esi
  _DWORD *v314; // eax
  float v315; // xmm0_4
  _DWORD *v316; // eax
  float v317; // xmm0_4
  int v318; // ecx
  int v319; // esi
  int v320; // eax
  int v321; // esi
  _DWORD *v322; // eax
  int v323; // eax
  int v324; // eax
  int v325; // eax
  float v326; // xmm0_4
  char v327; // al
  int v328; // eax
  int v329; // eax
  int v330; // ecx
  int v331; // eax
  int v332; // esi
  int v333; // eax
  int v334; // eax
  int v335; // ecx
  int v336; // eax
  int v337; // edi
  int v338; // esi
  _DWORD *v339; // eax
  int v340; // ecx
  int v341; // esi
  int v342; // eax
  int v343; // eax
  Concurrency::details::_ContextCallback *v344; // eax
  int v345; // esi
  float *v346; // eax
  int v347; // eax
  int v348; // esi
  float v349; // xmm0_4
  _BYTE *ArgList_6; // eax
  int v351; // esi
  int v352; // eax
  int v353; // eax
  int v354; // eax
  int v355; // eax
  int v356; // eax
  int v357; // eax
  void *v358; // edx
  int v359; // eax
  _BYTE *v360; // ecx
  int ArgList_5; // eax
  unsigned int n4_3; // esi
  float v363; // eax
  int i; // eax
  Concurrency::details::_ContextCallback *v365; // eax
  int v366; // eax
  int v367; // esi
  int v368; // ecx
  bool v369; // cc
  Concurrency::details::_ContextCallback *v370; // eax
  int v372; // [esp+5Ch] [ebp-338h]
  int v373; // [esp+5Ch] [ebp-338h]
  int v374; // [esp+5Ch] [ebp-338h]
  int v375; // [esp+5Ch] [ebp-338h]
  int v376; // [esp+64h] [ebp-330h]
  int v377; // [esp+64h] [ebp-330h]
  float v378; // [esp+6Ch] [ebp-328h]
  float v379; // [esp+6Ch] [ebp-328h]
  float v380; // [esp+6Ch] [ebp-328h]
  float v381; // [esp+6Ch] [ebp-328h]
  float v382; // [esp+6Ch] [ebp-328h]
  float v383; // [esp+6Ch] [ebp-328h]
  int v384; // [esp+6Ch] [ebp-328h]
  int v385; // [esp+6Ch] [ebp-328h]
  float v386; // [esp+6Ch] [ebp-328h]
  float v387; // [esp+6Ch] [ebp-328h]
  _BYTE v388[8]; // [esp+7Ch] [ebp-318h] BYREF
  _DWORD v389[2]; // [esp+84h] [ebp-310h] BYREF
  int v390; // [esp+8Ch] [ebp-308h]
  float v391; // [esp+90h] [ebp-304h] BYREF
  float v392; // [esp+94h] [ebp-300h]
  _BYTE v393[8]; // [esp+98h] [ebp-2FCh] BYREF
  float v394; // [esp+A0h] [ebp-2F4h]
  _BYTE v395[8]; // [esp+A4h] [ebp-2F0h] BYREF
  _BYTE v396[4]; // [esp+ACh] [ebp-2E8h] BYREF
  float v397; // [esp+B0h] [ebp-2E4h] BYREF
  __int64 v398; // [esp+B4h] [ebp-2E0h] BYREF
  int n2_3; // [esp+BCh] [ebp-2D8h]
  _BYTE v400[8]; // [esp+C0h] [ebp-2D4h] BYREF
  int v401; // [esp+C8h] [ebp-2CCh] BYREF
  float v402; // [esp+CCh] [ebp-2C8h] BYREF
  char v403; // [esp+D2h] [ebp-2C2h]
  bool v404; // [esp+D3h] [ebp-2C1h]
  _BYTE v405[4]; // [esp+D4h] [ebp-2C0h] BYREF
  float v406; // [esp+D8h] [ebp-2BCh] BYREF
  char v407[4]; // [esp+DCh] [ebp-2B8h] BYREF
  char ArgLista[4]; // [esp+E0h] [ebp-2B4h] BYREF
  unsigned int n2; // [esp+E4h] [ebp-2B0h] BYREF
  _BYTE v410[4]; // [esp+E8h] [ebp-2ACh] BYREF
  float v411; // [esp+ECh] [ebp-2A8h] BYREF
  float v412; // [esp+F0h] [ebp-2A4h] BYREF
  unsigned __int64 v413; // [esp+F4h] [ebp-2A0h] BYREF
  unsigned __int64 v414; // [esp+FCh] [ebp-298h] BYREF
  int ArgList_2; // [esp+104h] [ebp-290h]
  int v416; // [esp+108h] [ebp-28Ch] BYREF
  unsigned int n7; // [esp+10Ch] [ebp-288h] BYREF
  char v418; // [esp+111h] [ebp-283h]
  unsigned __int8 n2_12; // [esp+112h] [ebp-282h]
  char n2_10; // [esp+113h] [ebp-281h]
  float v421; // [esp+114h] [ebp-280h]
  bool v422; // [esp+11Bh] [ebp-279h]
  _BYTE v423[104]; // [esp+11Ch] [ebp-278h] BYREF
  _BYTE v424[104]; // [esp+184h] [ebp-210h] BYREF
  _BYTE v425[44]; // [esp+1ECh] [ebp-1A8h] BYREF
  _BYTE v426[56]; // [esp+218h] [ebp-17Ch] BYREF
  float v427; // [esp+260h] [ebp-134h]
  int v428; // [esp+268h] [ebp-12Ch]
  _BYTE v429[24]; // [esp+280h] [ebp-114h] BYREF
  _BYTE v430[24]; // [esp+298h] [ebp-FCh] BYREF
  _BYTE v431[24]; // [esp+2B0h] [ebp-E4h] BYREF
  _BYTE v432[24]; // [esp+2C8h] [ebp-CCh] BYREF
  _BYTE v433[8]; // [esp+2E0h] [ebp-B4h] BYREF
  _BYTE v434[8]; // [esp+2F0h] [ebp-A4h] BYREF
  _BYTE v435[24]; // [esp+2F8h] [ebp-9Ch] BYREF
  _BYTE v436[8]; // [esp+310h] [ebp-84h] BYREF
  int a2[4]; // [esp+318h] [ebp-7Ch] BYREF
  _BYTE v438[20]; // [esp+328h] [ebp-6Ch] BYREF
  _BYTE v439[24]; // [esp+33Ch] [ebp-58h] BYREF
  _BYTE v440[4]; // [esp+354h] [ebp-40h] BYREF
  int n206[4]; // [esp+35Ch] [ebp-38h] BYREF
  _BYTE Src[4]; // [esp+36Ch] [ebp-28h] BYREF
  _BYTE n96[20]; // [esp+370h] [ebp-24h] BYREF
  int n5; // [esp+390h] [ebp-4h]

  ArgList_1 = ArgList;
  ArgList_2 = ArgList;
  HIDWORD(v414) = 0;
  LODWORD(v2) = a5;
  v3 = *((float *)&a5 + 1);
  v390 = 0;
  v413 = *(_QWORD *)&a5;
  v422 = 0;
  v403 = 0;
  if ( !*(_BYTE *)(ArgList + 1040)
    || *(int *)(ArgList + 1044) > 0
    || (v4 = *(_DWORD *)(ArgList + 360) & 0x800, v418 = 1, v4) )
  {
    v418 = 0;
  }
  v5 = *(_DWORD *)(ArgList + 964);
  if ( v5 && *(_BYTE *)(v5 + 371) )
  {
    *(_DWORD *)(ArgList + 392) = 4;
    sub_6A9320(0);
    v3 = *((float *)&v413 + 1);
    LODWORD(v2) = v413;
  }
  if ( *(_DWORD *)(ArgList_1 + 5056) == 40 && !*(_DWORD *)(ArgList_1 + 5632) )
  {
    v418 = 0;
    goto LABEL_61;
  }
  if ( !v418 )
    goto LABEL_61;
  if ( (*(_BYTE *)(ArgList_1 + 5016) || *(_BYTE *)(ArgList_1 + 5018)) && !(unsigned __int8)sub_40A550("LiftItem")
    || !*(_DWORD *)(ArgList_1 + 392)
    && *(_DWORD *)(ArgList_1 + 7980) != 2
    && !*(_BYTE *)(ArgList_1 + 8361)
    && (*(_DWORD *)(ArgList_1 + 364) & 0x2000000) == 0
    && !sub_9305F0(ArgList_1 + 5384, 282)
    && !(unsigned __int8)sub_7DB700(ArgList_1) )
  {
LABEL_60:
    LODWORD(v2) = v413;
    v3 = *((float *)&v413 + 1);
    goto LABEL_61;
  }
  n2_12 = 0;
  n2_10 = 0;
  if ( (unsigned __int8)sub_456750(74) || (unsigned __int8)sub_456750(75) )
  {
    LOBYTE(v9) = sub_956110(0, *(_DWORD *)(ArgList_1 + 5656), ArgList_1, 0);
    BYTE1(v9) = v422;
    v10 = v422;
    v377 = *(_DWORD *)(ArgList_1 + 5656);
    if ( LOBYTE(v9) )
      v10 = 1;
    LOBYTE(v9) = v10;
    v421 = v9;
    v422 = v10;
    if ( sub_956110(1, v377, ArgList_1, 0) )
    {
      n2_10 = 1;
      LOBYTE(v421) = 1;
    }
    if ( sub_956110(2, *(_DWORD *)(ArgList_1 + 5656), ArgList_1, 0) )
    {
      n2_12 = 1;
      LOBYTE(v421) = 1;
    }
    v8 = sub_956110(3, *(_DWORD *)(ArgList_1 + 5656), ArgList_1, 0);
  }
  else
  {
    v6 = sub_956110(4, *(_DWORD *)(ArgList_1 + 5656), ArgList_1, 0);
    v7 = v422;
    v376 = *(_DWORD *)(ArgList_1 + 5656);
    if ( v6 )
      v7 = 1;
    v421 = *(float *)&v7;
    v422 = v7;
    if ( sub_956110(5, v376, ArgList_1, 0) )
    {
      n2_10 = 1;
      LOBYTE(v421) = 1;
    }
    if ( sub_956110(6, *(_DWORD *)(ArgList_1 + 5656), ArgList_1, 0) )
    {
      n2_12 = 1;
      LOBYTE(v421) = 1;
    }
    v8 = sub_956110(7, *(_DWORD *)(ArgList_1 + 5656), ArgList_1, 0);
  }
  if ( v8 && (LOBYTE(v421) = 1, n2_12) || v422 && n2_10 )
    v403 = 1;
  v11 = sub_7B9F60(ArgList_1, (float *)&v413, 0);
  if ( v11 || LOBYTE(v421) )
  {
    v422 = 1;
    if ( v11 )
    {
      v3 = *((float *)&v413 + 1);
      LODWORD(v2) = v413;
      goto LABEL_48;
    }
  }
  else
  {
    v422 = 0;
  }
  v12 = (float *)sub_779620((_DWORD *)ArgList_1, (int)&v414);
  v2 = *(float *)&v413 + *v12;
  *(float *)&v413 = v2;
  v3 = *((float *)&v413 + 1) + v12[1];
  *((float *)&v413 + 1) = v3;
LABEL_48:
  if ( (int)n60[39301] >= 2 && (*(_DWORD *)(ArgList_1 + 360) & 0x200) != 0 )
  {
    v2 = v2 * -1.0;
    v3 = v3 * -1.0;
    v413 = __PAIR64__(LODWORD(v3), LODWORD(v2));
  }
  if ( (float)((float)(v3 * v3) + (float)(v2 * v2)) > 0.001 )
  {
    v13 = sub_A68490();
    *(_DWORD *)ArgLista = HIDWORD(v13);
    v3 = *((float *)&v413 + 1);
    LODWORD(v2) = v413;
    v15 = v13;
    v14 = (3619042523u * (unsigned __int64)(unsigned int)v13) >> 32;
    v16 = 1125899906LL * (unsigned int)v13;
    v412 = *((float *)&v16 + 1);
    v17 = 3619042523LL * *(unsigned int *)ArgLista;
    v394 = *((float *)&v17 + 1);
    LODWORD(v406) = (1125899906 * (unsigned __int64)*(unsigned int *)ArgLista) >> 32;
    n7 = 1125899906 * *(_DWORD *)ArgLista;
    LODWORD(v414) = ((3619042523u * __PAIR64__(*(unsigned int *)ArgLista, v15)) >> 32) + v16;
    ArgList_1 = ArgList_2;
    *(_DWORD *)(ArgList_2 + 8080) = (((v16 + ((3619042523u * __PAIR64__(*(unsigned int *)ArgLista, v15)) >> 32)) >> 32)
                                   + (unsigned int)__CFADD__(v14, (_DWORD)v17)
                                   + HIDWORD(v17)
                                   + 1125899906LL * *(unsigned int *)ArgLista) >> 18;
  }
  v18 = *(float **)(ArgList_1 + 5084);
  if ( v18 || (v18 = *(float **)(ArgList_1 + 5088)) != 0 )
  {
    v394 = ((double (__thiscall *)(float *))*(_DWORD *)(*(_DWORD *)v18 + 24))(v18);
    if ( v394 > 0.0 && v18[5] > 0.0 && !v403 )
    {
      v19 = (float)(*((float *)&v413 + 1) * *((float *)&v413 + 1)) + (float)(*(float *)&v413 * *(float *)&v413);
      sub_435A50();
      if ( v19 < 0.5 )
      {
        LODWORD(v2) = a5;
        v3 = *((float *)&a5 + 1);
        v413 = *(_QWORD *)&a5;
        v422 = 0;
        goto LABEL_61;
      }
    }
    goto LABEL_60;
  }
LABEL_61:
  if ( *(_DWORD *)(ArgList_1 + 44) == 1 && *(_DWORD *)(ArgList_1 + 6892) == 7 )
  {
    if ( v2 < 0.0 )
    {
      v23 = -v2;
      v21 = (float)-v2 * *(float *)(ArgList_1 + 6904);
      v22 = v23 * *(float *)(ArgList_1 + 6908);
    }
    else
    {
      v20 = v2;
      v21 = v2 * *(float *)(ArgList_1 + 6896);
      v22 = v20 * *(float *)(ArgList_1 + 6900);
    }
    v24 = v21 + 0.0;
    v25 = v22 + 0.0;
    if ( v3 < 0.0 )
    {
      v29 = -v3;
      v27 = (float)-v3 * *(float *)(ArgList_1 + 6920);
      v28 = v29 * *(float *)(ArgList_1 + 6924);
    }
    else
    {
      v26 = v3;
      v27 = v3 * *(float *)(ArgList_1 + 6912);
      v28 = v26 * *(float *)(ArgList_1 + 6916);
    }
    v2 = v24 + v27;
    v3 = v25 + v28;
    *(float *)&v413 = v2;
    *((float *)&v413 + 1) = v25 + v28;
  }
  v30 = *(_DWORD *)(ArgList_1 + 6392);
  v421 = v2;
  v397 = v3;
  n2_12 = 0;
  if ( v30 > 0 )
  {
    if ( v30 < *(_DWORD *)(ArgList_1 + 6396) - 5 )
    {
      v422 = 1;
      n2_12 = 1;
      if ( v2 == *(float *)&a5 && v3 == *((float *)&a5 + 1) )
      {
        v2 = *(float *)(ArgList_1 + 5692);
        v3 = *(float *)(ArgList_1 + 5696);
        v413 = __PAIR64__(LODWORD(v3), LODWORD(v2));
      }
      if ( v2 == *(float *)&a5 && v3 == *((float *)&a5 + 1) )
        v413 = *(_QWORD *)sub_67F1D0(&v414, *(_DWORD *)(ArgList_1 + 5672));
    }
    if ( (*(_DWORD *)(ArgList_1 + 360) & 0x4000) == 0 )
    {
      --*(_DWORD *)(ArgList_1 + 6392);
      *(_DWORD *)(ArgList_1 + 5492) |= 2u;
      sub_763570((_BYTE *)ArgList_1);
    }
    *(_DWORD *)(ArgList_1 + 4920) = 8;
  }
  if ( *(_BYTE *)(ArgList_1 + 5016) || *(_BYTE *)(ArgList_1 + 5018) )
  {
    if ( *(_DWORD *)(ArgList_1 + 5136) )
    {
      n604 = *(_DWORD *)(ArgList_1 + 5632);
      if ( n604 != 604 && n604 != 709 )
      {
        v32 = *(float *)(ArgList_1 + 868);
        *(float *)&v414 = *(float *)(ArgList_1 + 864) * 0.1;
        *((float *)&v414 + 1) = v32 * 0.1;
        sub_7D2470(&v414);
      }
    }
  }
  HIDWORD(v398) = ArgList_1 + 5384;
  if ( sub_9305F0(ArgList_1 + 5384, 93)
    || sub_9305F0(ArgList_1 + 5384, 77)
    || sub_9305F0(ArgList_1 + 5384, 298)
    || sub_9305F0(ArgList_1 + 5384, 299) && sub_930980(ArgList_1 + 5384, 299)[2]
    || *(int *)(ArgList_1 + 1048) > 0 )
  {
    v34 = *(float *)&a5;
    v33 = *((float *)&a5 + 1);
    v413 = *(_QWORD *)&a5;
    v422 = 0;
    n2_12 = 0;
  }
  else
  {
    v33 = *((float *)&v413 + 1);
    v34 = *(float *)&v413;
  }
  v35 = *(_DWORD *)(ArgList_1 + 5636);
  v36 = v35 == 0;
  if ( v35 <= 0 )
  {
LABEL_101:
    if ( v36 )
      goto LABEL_103;
    goto LABEL_102;
  }
  if ( (*(_DWORD *)(ArgList_1 + 360) & 0x4000) == 0 )
  {
    *(_DWORD *)(ArgList_1 + 5636) = --v35;
    if ( v35 <= 0 )
    {
      if ( (unsigned __int8)sub_730910(*(_DWORD *)(ArgList_1 + 5632)) )
      {
        sub_9304A0(*(_DWORD *)(ArgList_1 + 5632), 1);
        sub_7AD330(*(_DWORD *)(ArgList_1 + 5632), 0);
      }
      sub_7C36B0(ArgList_1);
      v35 = *(_DWORD *)(ArgList_1 + 5636);
      v36 = v35 == 0;
      v33 = *((float *)&v413 + 1);
      v34 = *(float *)&v413;
      goto LABEL_101;
    }
  }
LABEL_102:
  v34 = *(float *)&a5;
  v33 = *((float *)&a5 + 1);
  v413 = *(_QWORD *)&a5;
LABEL_103:
  n2_9 = 0;
  if ( !v35 )
    n2_9 = n2_12;
  n2_3 = n2_9;
  v38 = 0;
  if ( !v35 )
    v38 = v422;
  *(_DWORD *)ArgLista = v38;
  if ( v34 == 0.0 && v33 == 0.0 )
  {
    v404 = 0;
    if ( (_BYTE)v38 )
    {
      v34 = -*(float *)(ArgList_1 + 5692);
      v33 = -*(float *)(ArgList_1 + 5696);
      *(float *)&v413 = v34;
      *((float *)&v413 + 1) = v33;
    }
  }
  else
  {
    v404 = 1;
  }
  if ( v34 != 0.0 || (n2_12 = 0, v33 != 0.0) )
    n2_12 = 1;
  v39 = *(_BYTE *)(ArgList_1 + 8361);
  v391 = v34;
  v392 = v33;
  v422 = v39 || *(_BYTE *)(ArgList_1 + 6181);
  v40 = *(float *)(ArgList_1 + 7980);
  *(float *)&v41 = 0.0;
  LODWORD(v394) = ArgList_1 + 5088;
  v42 = *(_DWORD *)(ArgList_1 + 5088);
  *(float *)&n2 = v40;
  v402 = 0.0;
  if ( v42 )
  {
    *(float *)&v41 = COERCE_FLOAT((*(int (__thiscall **)(int))(*(_DWORD *)v42 + 48))(v42));
    LOBYTE(v38) = ArgLista[0];
    v402 = *(float *)&v41;
  }
  v36 = LODWORD(v40) == 2;
  v43 = *(_DWORD *)(ArgList_1 + 5132);
  if ( v36 )
  {
    LODWORD(v402) = 30;
LABEL_124:
    if ( v43 && sub_9561E0(n298, 11, *(_DWORD *)(ArgList_1 + 5656), ArgList_1, 0) && n2 != 2 )
    {
      (*(void (__thiscall **)(_DWORD))(**(_DWORD **)(ArgList_1 + 5132) + 40))(*(_DWORD *)(ArgList_1 + 5132));
      if ( *(_DWORD *)(ArgList_1 + 5132) )
      {
        LODWORD(v412) = ArgList_1 + 5132;
        sub_4CF210(&v412);
      }
      *(_DWORD *)(ArgList_1 + 5132) = 0;
    }
    if ( (*(_DWORD *)(ArgList_1 + 360) & 0x4000) != 0 )
      goto LABEL_145;
    v44 = *(_DWORD **)(ArgList_1 + 5132);
    v45 = (_DWORD **)(ArgList_1 + 5132);
    if ( v44 )
    {
      if ( v44[11] == LODWORD(v402) )
        goto LABEL_145;
      (*(void (__thiscall **)(_DWORD *))(*v44 + 40))(v44);
      v44 = *v45;
      if ( *v45 )
      {
        LODWORD(v412) = ArgList_1 + 5132;
        sub_4CF210(&v412);
      }
      *v45 = 0;
    }
    v46 = v421;
    if ( v421 == *(float *)&a5 )
    {
      v47 = v397;
      if ( v397 == *((float *)&a5 + 1) && n2 != 2 )
        goto LABEL_146;
    }
    v48 = sub_6EEF60(v44);
    v49 = (float)(v397 * 5.0) + *(float *)(ArgList_1 + 832);
    *(float *)&v414 = (float)(v421 * 5.0) + *(float *)(ArgList_1 + 828);
    *((float *)&v414 + 1) = v49;
    v50 = COERCE_FLOAT(sub_428B20(n60, 1000, SLODWORD(v402), (int)&v414, (int)&a5, ArgList_1, 0, v48));
    v402 = v50;
    if ( v50 == 0.0 || *(_DWORD *)(LODWORD(v50) + 40) != 1000 )
    {
      v50 = 0.0;
      v402 = 0.0;
    }
    *(_DWORD *)(LODWORD(v50) + 1116) = 0;
    *(_DWORD *)(LODWORD(v50) + 1120) = 0;
    if ( *v45 )
    {
      LODWORD(v412) = ArgList_1 + 5132;
      sub_4CF210(&v412);
      v50 = v402;
    }
    *(float *)v45 = v50;
    sub_4CDCF0(ArgList_1 + 12);
LABEL_145:
    v46 = v421;
    v47 = v397;
LABEL_146:
    v51 = *(_DWORD *)(ArgList_1 + 5132);
    LOBYTE(v421) = ArgLista[0];
    n2_10 = n2_3;
    if ( v51 )
    {
      n2_1 = n2;
      if ( !*(_DWORD *)(ArgList_1 + 5632) || (LOBYTE(v421) = ArgLista[0], n2_10 = n2_3, n2 == 2) )
      {
        if ( ArgLista[0] )
        {
          v414 = __PAIR64__(LODWORD(v47), LODWORD(v46));
          if ( (float)((float)(v47 * v47) + (float)(v46 * v46)) > 1.0 )
          {
            sub_A0FFC0(&v414);
            v51 = *(_DWORD *)(ArgList_1 + 5132);
            v47 = *((float *)&v414 + 1);
            LODWORD(v46) = v414;
          }
          v53 = *(float *)(v51 + 868) + (float)(v47 * 8.0);
          *(float *)(v51 + 864) = (float)(v46 * 8.0) + *(float *)(v51 + 864);
          *(float *)(v51 + 868) = v53;
        }
        if ( n2_1 == 2 )
        {
          v54 = *(_DWORD *)(ArgList_1 + 5132);
          v55 = *(float *)(ArgList_1 + 828) - *(float *)(v54 + 828);
          *(float *)&n7 = *(float *)(ArgList_1 + 832) - *(float *)(v54 + 832);
          v412 = v55;
          v56 = (float)(*(float *)&n7 * *(float *)&n7) + (float)(v55 * v55);
          sub_435A50();
          if ( v56 > 60.0 )
          {
            v57 = *(_DWORD *)(ArgList_1 + 5132);
            v58 = (float)((float)((float)(v56 - 60.0) * (float)(*(float *)&n7 / v56)) * 0.1) + *(float *)(v57 + 868);
            *(float *)(v57 + 864) = *(float *)(v57 + 864)
                                  + (float)((float)((float)(v412 / v56) * (float)(v56 - 60.0)) * 0.1);
            *(float *)(v57 + 868) = v58;
          }
        }
        if ( !(unsigned __int8)sub_7706E0(210, 0)
          || (v59 = *(float *)(ArgList_1 + 892), LOBYTE(v421) = ArgLista[0], n2_10 = n2_3, v59 != 1000.0) )
        {
          n2_11 = 1;
          LOBYTE(v421) = 1;
LABEL_165:
          n2_10 = n2_11;
          goto LABEL_166;
        }
      }
    }
    goto LABEL_166;
  }
  if ( *(float *)&v41 != 0.0 )
    goto LABEL_124;
  LOBYTE(v421) = v38;
  n2_10 = n2_3;
  if ( v43 )
  {
    if ( (*(_DWORD *)(ArgList_1 + 360) & 0x4000) == 0 )
    {
      (*(void (__thiscall **)(int))(*(_DWORD *)v43 + 40))(v43);
      if ( *(_DWORD *)(ArgList_1 + 5132) )
      {
        LODWORD(v412) = ArgList_1 + 5132;
        sub_4CF210(&v412);
      }
      LOBYTE(v421) = ArgLista[0];
      n2_11 = n2_3;
      *(_DWORD *)(ArgList_1 + 5132) = 0;
      goto LABEL_165;
    }
    n2_10 = n2_3;
    LOBYTE(v421) = v38;
  }
LABEL_166:
  v61 = sub_7706E0(378, 0);
  n60 = n60;
  v36 = v61 == 0;
  v63 = v418;
  v64 = LOBYTE(v421);
  if ( !v36 && !n60[28318] && v418 && *(int *)(ArgList_1 + 1048) <= 0 )
  {
    if ( LOBYTE(v421) )
    {
      n150 = *(_DWORD *)(ArgList_1 + 6328) + 1;
      *(_DWORD *)(ArgList_1 + 6328) = n150;
      if ( n150 == 150 )
      {
        sub_780560(n206, 0);
        v67 = sub_6EEF60(v66);
        v68 = *(float *)(ArgList_1 + 864);
        v69 = *(float *)(ArgList_1 + 868);
        v70 = ArgList_1 + 828;
        *(float *)&v414 = v68 * 0.1;
        *((float *)&v414 + 1) = v69 * 0.1;
        v71 = sub_428B20(n60, 4, 9, v70, (int)&v414, ArgList_2, 0, v67);
        v72 = _RTDynamicCast(
                v71,
                0,
                &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
                &IsaacRepentancePlus::Entity_Bomb `RTTI Type Descriptor',
                0);
        *(_DWORD *)ArgLista = 37;
        v73 = *(_OWORD *)sub_435C20(&dword_C34560);
        *(_BYTE *)(v72 + 1123) = 1;
        *(_OWORD *)(v72 + 1080) = v73;
        n298 = n298;
        sub_956780(ArgLista);
        sub_92DC30(n298 + 43209, *(int *)ArgLista, 1.0, 2, 0, 1065353216, 0);
        v76 = sub_6EEF60(v75);
        v77 = sub_428B20(n60, 1000, 34, v70, (int)&a5, 0, 0, v76);
        v78 = *(void (__thiscall **)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD))(*(_DWORD *)v77 + 60);
        sub_682ED0(v438);
        sub_682F20(1072064102, 1060320051, 1069547520, 1065353216);
        v78(v77, v438, -1, 255, 0, 1);
        (*(void (__thiscall **)(int))(*(_DWORD *)v77 + 12))(v77);
        ArgList_1 = ArgList_2;
        sub_7A6300(&a5);
        n60 = n60;
        v63 = v418;
      }
      v64 = LOBYTE(v421);
    }
    else
    {
      *(_DWORD *)(ArgList_1 + 6328) = 0;
    }
  }
  if ( n60[39311] == 41 )
  {
    if ( n60[28318] )
      goto LABEL_189;
    if ( v63 && v64 )
    {
      if ( sub_6AC710((_DWORD *)ArgList_1, 30, 0) && (sub_6EEF60(v79) & 3) == 0 )
        sub_704360(ArgList_1 + 828, 1123024896, ArgList_1, 1, 0);
      n60 = n60;
    }
  }
  if ( !n60[28318] && v418 && *(int *)(ArgList_1 + 1048) <= 0 )
  {
    if ( LOBYTE(v421) )
      ++*(_DWORD *)(ArgList_1 + 6384);
    else
      *(_DWORD *)(ArgList_1 + 6384) = 0;
  }
LABEL_189:
  v80 = *(int *)(ArgList_1 + 6388) <= 0 && !*(_DWORD *)(ArgList_1 + 7936) && v422;
  v81 = *(_DWORD *)(ArgList_1 + 1048);
  v82 = 0.0;
  n2 = 15;
  if ( v81 <= 0 )
    LODWORD(v82) = v80;
  v36 = *(_BYTE *)(ArgList_1 + 6408) == 0;
  v412 = v82;
  v83 = v81;
  v422 = LOBYTE(v412);
  if ( !v36 )
  {
    v422 = LOBYTE(v412);
    if ( v81 <= 0 )
    {
      v422 = LOBYTE(v412);
      v84 = v418;
      if ( !v418 )
        goto LABEL_212;
      if ( !LOBYTE(v421) || n2_10 && *(_DWORD *)(ArgList_1 + 6400) >= *(_DWORD *)(ArgList_1 + 6404) )
      {
        v86 = n60[24768];
        if ( *(_BYTE *)v86 && n60[39230] - *(_DWORD *)(v86 + 4592) > 5 )
        {
          if ( *(_DWORD *)(ArgList_1 + 6400) >= *(_DWORD *)(ArgList_1 + 6404) )
          {
            ArgList_3 = ArgList_2;
            v88 = (float *)(ArgList_1 + 5692);
            sub_7DAC80(v88);
            n60_1 = n60;
            *(_BYTE *)(ArgList_3 + 6408) = 0;
            *(_DWORD *)(ArgList_3 + 4920) = 10;
            v90 = n60_1[39230];
            v92 = sub_6EEF60(v91);
            ArgList_4 = ArgList_2;
            v94 = v92 % n2;
            *(_DWORD *)(ArgList_2 + 6396) = 180;
            *(_DWORD *)(ArgList_4 + 6392) = 180;
            *(_DWORD *)(ArgList_4 + 6412) = v90 + 30 * (v94 + 15);
            v95 = *(float *)(ArgList_4 + 5220);
            v96 = (float)(*v88 * 15.0) * v95;
            *((float *)&v414 + 1) = (float)(v88[1] * 15.0) * v95;
            *(float *)&v414 = v96;
            v97 = sub_790AF0((_DWORD *)ArgList_4, ArgList_4 + 828, LODWORD(v96), HIDWORD(v414), 69, 0, 1065353216);
            sub_67A400(22);
            (*(void (__thiscall **)(int, _DWORD))(*(_DWORD *)v97 + 64))(v97, *(float *)(v97 + 904) * 10.0);
            v412 = *(float *)(v97 + 1056);
            v99 = sub_6EEF60(v98);
            *(float *)&n7 = 0.0099999998;
            v100 = (double)v99;
            v101 = (unsigned int *)&v412;
            v412 = (float)((float)((float)((float)v100 * 2.3283064e-10) * 0.30000001) + 1.2) * v412;
            if ( v412 <= 0.0099999998 )
              v101 = &n7;
            v102 = *v101;
            *(_DWORD *)(v97 + 1060) = *v101;
            *(_DWORD *)(v97 + 1056) = v102;
            sub_677780(v97);
            *(_OWORD *)n206 = *(_OWORD *)(v97 + 1064);
            sub_435C20(&xmmword_C34B50);
            v103 = (_DWORD *)sub_435C20(&xmmword_C34BB0);
            sub_676150(*v103, v103[1], v103[2], v103[3]);
            sub_444C90(0);
            sub_444C70(-1069547520);
            sub_675FC0(-1054867456);
            v104 = *(void (__thiscall **)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD))(*(_DWORD *)v97 + 60);
            v105 = sub_407170(v438);
            v104(v97, v105, -1, 255, 0, 1);
            (*(void (__thiscall **)(int))(*(_DWORD *)v97 + 12))(v97);
            ArgList_1 = ArgList_2;
          }
          *(_DWORD *)(ArgList_1 + 6400) = 0;
        }
      }
      else
      {
        v85 = *(_DWORD *)(ArgList_1 + 6400);
        if ( v85 < *(_DWORD *)(ArgList_1 + 6404) )
          *(_DWORD *)(ArgList_1 + 6400) = v85 + 1;
      }
      v83 = *(_DWORD *)(ArgList_1 + 1048);
      v422 = 0;
    }
  }
  v84 = v418;
LABEL_212:
  if ( *(float *)(ArgList_1 + 8352) < 1.0 )
  {
    *(_BYTE *)(ArgList_1 + 8360) = 1;
    goto LABEL_221;
  }
  if ( v83 > 0 || !v84 || *(_DWORD *)(ArgList_1 + 5632) )
    goto LABEL_222;
  if ( !LOBYTE(v421) )
  {
    v108 = sub_5CBD00(725);
    v109 = sub_7E9020(v108, 11);
    sub_7D7290((_DWORD *)ArgList_1, v109 + 1);
    *(_DWORD *)(ArgList_1 + 8352) = -1082130432;
LABEL_221:
    *(_DWORD *)(ArgList_1 + 8356) = 0;
    goto LABEL_222;
  }
  v106 = *(float *)(ArgList_1 + 8356) + 0.033333335;
  *(float *)&n7 = 1.0;
  v412 = v106;
  v107 = (_DWORD *)sub_40C780(&n7, &v412);
  v36 = *(_BYTE *)(ArgList_1 + 8360) == 0;
  *(_DWORD *)(ArgList_1 + 8356) = *v107;
  if ( !v36 )
  {
    *(_BYTE *)(ArgList_1 + 8360) = 0;
    sub_9568E0(1065353216, 96, 2, 0, 1065353216);
  }
LABEL_222:
  if ( *(int *)(ArgList_1 + 4924) < 0 )
  {
    v110 = sub_779320(ArgList_1);
    *(_DWORD *)ArgLista = v110;
    if ( v110 )
    {
      sub_4172B0(&v414);
      if ( (unsigned __int8)sub_417430(0x4000, 0) )
      {
        sub_417290(v410);
        v111 = sub_4172B0(&v416);
        sub_A10420(v405, v111);
        v412 = 0.5;
        v112 = sub_A104E0(&v401, &v412);
        unknown_libname_8(v112);
        v110 = *(_DWORD *)ArgLista;
      }
      v113 = sub_A10480(v410, ArgList_1 + 828);
      unknown_libname_8(v113);
      if ( v110 == *(_DWORD *)(ArgList_1 + 5132) && !*(_DWORD *)(ArgList_1 + 5632) )
      {
        unknown_libname_8(&v391);
        v404 = sub_A10630((float *)&v413, (float *)&a5);
      }
    }
    if ( v391 != 0.0 || v392 != 0.0 )
    {
      v412 = sub_41D510().m128_f32[0];
      if ( v412 <= sub_41D510().m128_f32[0] )
      {
        v115 = (void *)(ArgList_1 + 4896);
        if ( v392 >= 0.0 )
        {
          sub_414550(v115, "HeadDown");
          *(_DWORD *)(ArgList_1 + 5672) = 3;
        }
        else
        {
          sub_414550(v115, "HeadUp");
          *(_DWORD *)(ArgList_1 + 5672) = 1;
        }
      }
      else
      {
        if ( v391 >= 0.0 )
        {
          *(_DWORD *)ArgLista = 2;
          HeadRight = "HeadRight";
        }
        else
        {
          *(_DWORD *)ArgLista = 0;
          HeadRight = "HeadLeft";
        }
        sub_414550((void *)(ArgList_1 + 4896), HeadRight);
        *(_DWORD *)(ArgList_1 + 5672) = *(_DWORD *)ArgLista;
      }
    }
  }
  if ( *(int *)(ArgList_1 + 1048) <= 0 )
  {
    *(_DWORD *)(ArgList_1 + 5676) = -1;
    v412 = sub_41D510().m128_f32[0];
    if ( (float)(v412 + sub_41D510().m128_f32[0]) > 0.0099999998 )
      *(_DWORD *)(ArgList_1 + 5676) = sub_67F260(&v413);
  }
  if ( (unsigned __int8)sub_7AB110(ArgList_1) )
  {
    v116 = (Concurrency::details::_ContextCallback *)sub_428B50(n60);
    if ( !Concurrency::details::_ContextCallback::_HasCapturedContext(v116) )
    {
      if ( v418 )
      {
        if ( (unsigned __int8)sub_7706E0(399, 0) )
        {
          if ( !LOBYTE(v421) || n2_10 && *(int *)(ArgList_1 + 6332) > 150 )
          {
            v118 = (_DWORD *)sub_407480(n60);
            if ( sub_813CE0(v118) > 5 )
            {
              if ( *(int *)(ArgList_1 + 6332) > 150 )
              {
                v119 = sub_7C8DC0((_DWORD *)ArgList_1, 60);
                (*(void (__thiscall **)(int))(*(_DWORD *)v119 + 12))(v119);
                if ( !n2_10 )
                  *(_DWORD *)(ArgList_1 + 1048) = *(_DWORD *)sub_41CB80(ArgList_1 + 1048, &unk_B1F488);
                sub_42CA00(n298);
                v120 = sub_72FDA0(19);
                sub_75D850(v120);
                sub_40C340(v433, "HeadDownShoot");
                n5 = 0;
                sub_40C340(v436, "HeadRightShoot");
                LOBYTE(n5) = 1;
                sub_40C340(v440, "HeadUpShoot");
                LOBYTE(n5) = 2;
                sub_40C340(Src, "HeadLeftShoot");
                LOBYTE(n5) = 3;
                v121 = (int *)sub_67F170(&v414, ArgList_1 + 5692);
                v122 = (void *)sub_7AD7E0(v435, *v121, v121[1], Src, v440, v436, v433);
                LOBYTE(n5) = 4;
                sub_759F50(399, 0, v122, -1, 0);
                sub_40BF20(v435);
                sub_40BF20(Src);
                sub_40BF20(v440);
                sub_40BF20(v436);
                n5 = -1;
                sub_40BF20(v433);
                *(_DWORD *)(ArgList_1 + 5676) = -1;
              }
              *(_DWORD *)(ArgList_1 + 6332) = 0;
            }
          }
          else if ( ++*(_DWORD *)(ArgList_1 + 6332) == 150 )
          {
            sub_42CA00(n298);
            v117 = sub_72FDA0(19);
            sub_75D1D0(v117, 0);
          }
        }
        if ( (unsigned __int8)sub_7706E0(643, 0) )
        {
          if ( !LOBYTE(v421) || n2_10 && *(int *)(ArgList_1 + 7900) > 150 )
          {
            v123 = (_DWORD *)sub_407480(n60);
            if ( sub_813CE0(v123) > 5 )
            {
              if ( *(int *)(ArgList_1 + 7900) > 150 )
              {
                v414 = *(_QWORD *)sub_753940(ArgList_1);
                v124 = sub_7AA850(v410, 4, &v414);
                v412 = sub_A10390(&v414);
                v125 = sub_96F310(5, (_DWORD *)(ArgList_1 + 828), LODWORD(v412), 30, v124, (_DWORD *)ArgList_1, 0);
                (*(void (__thiscall **)(int, _DWORD))(*(_DWORD *)v125 + 64))(v125, *(_DWORD *)(ArgList_1 + 5500));
                sub_456830(*(_OWORD *)(ArgList_1 + 5256));
                if ( v392 <= -0.5 )
                  n1161527296 = -1054867456;
                else
                  n1161527296 = 1161527296;
                sub_435CF0(n1161527296);
                if ( !n2_10 )
                  *(_DWORD *)(ArgList_1 + 1048) = *(_DWORD *)sub_41CB80(ArgList_1 + 1048, &unk_B1F488);
                *(_DWORD *)(ArgList_1 + 5676) = -1;
              }
              *(_DWORD *)(ArgList_1 + 7900) = 0;
            }
          }
          else
          {
            ++*(_DWORD *)(ArgList_1 + 7900);
          }
        }
        if ( (unsigned __int8)sub_7706E0(680, 0) )
        {
          if ( !LOBYTE(v421) || n2_10 && *(int *)(ArgList_1 + 7904) > 150 )
          {
            v127 = (_DWORD *)sub_407480(n60);
            if ( sub_813CE0(v127) > 5 )
            {
              if ( *(int *)(ArgList_1 + 7904) > 150 )
              {
                sub_A10350(v410, *(float *)(ArgList_1 + 6884) + 180.0);
                sub_7AA850(&v414, -1, v410);
                *((float *)&v414 + 1) = *((float *)&v414 + 1) - (float)(*(float *)(ArgList_1 + 5372) * 4.0);
                v128 = sub_96F310(
                         12,
                         (_DWORD *)(ArgList_1 + 828),
                         *(float *)(ArgList_1 + 6884) + 180.0,
                         30,
                         &v414,
                         (_DWORD *)ArgList_1,
                         0);
                (*(void (__thiscall **)(int, _DWORD))(*(_DWORD *)v128 + 64))(v128, *(_DWORD *)(ArgList_1 + 5500));
                v129 = sub_592200(&n96[4]);
                v130 = (__int128 *)sub_417170(v129);
                sub_456830(*v130);
                sub_435F30(1123024896);
                sub_6B5B50(ArgList_1 + 7908, v128);
                if ( v411 <= -0.5 )
                  n1161527296_1 = -1054867456;
                else
                  n1161527296_1 = 1161527296;
                sub_435CF0(n1161527296_1);
                if ( !n2_10 )
                  *(_DWORD *)(ArgList_1 + 1048) = *(_DWORD *)sub_41CB80(ArgList_1 + 1048, &unk_B1F488);
                *(_DWORD *)(ArgList_1 + 5676) = -1;
              }
              *(_DWORD *)(ArgList_1 + 7904) = 0;
            }
          }
          else
          {
            ++*(_DWORD *)(ArgList_1 + 7904);
          }
        }
        if ( *(_DWORD *)(ArgList_1 + 5056) == 26 && !(unsigned __int8)sub_417430(0x4000, 0) )
        {
          if ( LOBYTE(v421) && (unsigned __int8)sub_7DAC30(ArgList_1) )
          {
            if ( (int)++*(_DWORD *)(ArgList_1 + 8060) >= 60 )
            {
              *(_DWORD *)(ArgList_1 + 8060) = 0;
              sub_7DA810((_DWORD *)ArgList_1, (_DWORD *)(ArgList_1 + 828), 0);
            }
          }
          else
          {
            *(_DWORD *)(ArgList_1 + 8060) = 0;
          }
        }
      }
    }
  }
  *(float *)&v132 = COERCE_FLOAT(sub_779A20(ArgList_1));
  v36 = *(_DWORD *)(ArgList_1 + 5632) == 47;
  v402 = *(float *)&v132;
  if ( !v36 )
  {
    if ( sub_9305F0(ArgList_1 + 5384, 147) )
    {
      if ( !*(_DWORD *)(ArgList_1 + 5084) )
      {
LABEL_303:
        *(_DWORD *)(ArgList_1 + 5084) = sub_9F9020(11, ArgList_1);
        v422 = 1;
        goto LABEL_321;
      }
      if ( sub_417280(*(_DWORD *)(ArgList_1 + 5084)) != 11 )
      {
        if ( *(_DWORD *)(ArgList_1 + 5084) )
          sub_9F9050(ArgList_1 + 5084);
        goto LABEL_303;
      }
LABEL_310:
      v422 = 1;
      goto LABEL_321;
    }
    v133 = sub_9305F0(ArgList_1 + 5384, 640);
    v134 = *(_DWORD *)(ArgList_1 + 5084);
    if ( v133 )
    {
      if ( v134 )
      {
        if ( sub_417280(v134) == 12 )
          goto LABEL_310;
        if ( *(_DWORD *)(ArgList_1 + 5084) )
          sub_9F9050(ArgList_1 + 5084);
      }
      *(_DWORD *)(ArgList_1 + 5084) = sub_9F9020(12, ArgList_1);
      goto LABEL_310;
    }
    if ( *(_DWORD *)(ArgList_1 + 5056) != 32 )
    {
      if ( v134 )
        sub_9F9050(ArgList_1 + 5084);
      goto LABEL_321;
    }
    if ( v134 )
    {
      if ( sub_417280(v134) == 15 )
      {
LABEL_318:
        v422 = 1;
        goto LABEL_321;
      }
      v135 = (_DWORD *)(ArgList_1 + 5084);
      if ( *(_DWORD *)(ArgList_1 + 5084) )
        sub_9F9050(ArgList_1 + 5084);
    }
    else
    {
      v135 = (_DWORD *)(ArgList_1 + 5084);
    }
    *v135 = sub_9F9020(n2, ArgList_1);
    goto LABEL_318;
  }
  if ( !*(_DWORD *)(ArgList_1 + 5084) )
  {
LABEL_297:
    *(_DWORD *)(ArgList_1 + 5084) = sub_9F9020(6, ArgList_1);
    goto LABEL_321;
  }
  if ( sub_417280(*(_DWORD *)(ArgList_1 + 5084)) != 6 )
  {
    if ( *(_DWORD *)(ArgList_1 + 5084) )
      sub_9F9050(ArgList_1 + 5084);
    goto LABEL_297;
  }
LABEL_321:
  v136 = (_DWORD *)(ArgList_1 + 5088);
  n4 = 4;
  *(_DWORD *)ArgLista = ArgList_1 + 5088;
  n2 = 4;
  do
  {
    if ( *v136 )
    {
      sub_4316C0(*v136);
      sub_4316B0(LODWORD(v402));
      v136 = *(_DWORD **)ArgLista;
      n4 = n2;
    }
    ++v136;
    --n4;
    *(_DWORD *)ArgLista = v136;
    n2 = n4;
  }
  while ( *(float *)&n4 != 0.0 );
  v138 = *(_DWORD *)(ArgList_1 + 5084);
  if ( v138 )
  {
    sub_4316C0(v138);
    sub_4316B0(LODWORD(v402) | 0x80000000);
    v139 = *(_DWORD *)(ArgList_1 + 5084);
    v140 = *(void (__thiscall **)(_DWORD, _DWORD))(*(_DWORD *)v139 + 8);
    v141 = sub_417430(0x4000, 0);
    v140(v139, v141);
    ArgList_1 = ArgList_2;
  }
  else
  {
    v142 = (float *)(ArgList_1 + 5088);
    n4_1 = 4;
    *(_DWORD *)ArgLista = ArgList_1 + 5088;
    n2 = 4;
    do
    {
      v144 = *v142;
      v412 = v144;
      if ( v144 != 0.0 )
      {
        v145 = *(void (__thiscall **)(_DWORD, _DWORD))(*(_DWORD *)LODWORD(v144) + 8);
        v146 = sub_417430(0x4000, 0);
        v145(LODWORD(v412), v146);
        n4_1 = n2;
      }
      v142 = (float *)(*(_DWORD *)ArgLista + 4);
      --n4_1;
      *(_DWORD *)ArgLista += 4;
      n2 = n4_1;
    }
    while ( *(float *)&n4_1 != 0.0 );
  }
  if ( *(_DWORD *)(ArgList_1 + 5632) == 49 && n2_12 )
    *(_DWORD *)(ArgList_1 + 4920) = 10;
  v369 = *(_DWORD *)(ArgList_1 + 4920) <= 8;
  v412 = *(float *)(ArgList_1 + 4920);
  n2_2 = 2;
  if ( v369 )
    n2_2 = 0;
  n2_3 = n2_2;
  if ( !(unsigned __int8)sub_417430(0x4000, 0) )
  {
    v148 = *(_DWORD *)(ArgList_1 + 4920);
    if ( v148 > -1 )
      *(_DWORD *)(ArgList_1 + 4920) = v148 - 1;
    v149 = *(_DWORD *)(ArgList_1 + 4924);
    if ( v149 > -1 )
      *(_DWORD *)(ArgList_1 + 4924) = v149 - 1;
  }
  *(float *)&n7 = sub_A0FED0(&v413);
  if ( *(float *)&n7 > 1.0 )
    sub_A0FFC0(&v413);
  sub_67F170(v389, &v413);
  if ( (unsigned __int8)sub_7706E0(544, 0) || *(_DWORD *)(ArgList_1 + 5056) == 13 && (unsigned __int8)sub_7706E0(619, 0) )
  {
    n7 = 7;
    *(_DWORD *)(ArgList_1 + 4920) = *(_DWORD *)sub_41CB80(ArgList_1 + 4920, &n7);
  }
  v150 = sub_7706E0(243, 0);
  v151 = *(_DWORD *)(ArgList_1 + 5160);
  n7 = ArgList_1 + 5160;
  if ( v150 )
  {
    if ( v151 )
    {
      *(float *)&n2_4 = COERCE_FLOAT(
                          _RTDynamicCast(
                            v151,
                            0,
                            &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
                            &IsaacRepentancePlus::Entity_Effect `RTTI Type Descriptor',
                            0));
      n2 = n2_4;
      if ( LOBYTE(v421) )
      {
        v414 = v413;
        if ( *((float *)&v413 + 1) == 0.0 && *(float *)&v413 == 0.0 )
        {
          switch ( *(_DWORD *)(ArgList_1 + 5672) )
          {
            case 0:
              LODWORD(v414) = -1082130432;
              break;
            case 1:
              HIDWORD(v414) = -1082130432;
              break;
            case 2:
              LODWORD(v414) = 1065353216;
              break;
            case 3:
              HIDWORD(v414) = 1065353216;
              break;
            default:
              break;
          }
        }
        *(float *)&n7 = sub_A10390(&v414) - 90.0;
        n7 = sub_682740().m128_u32[0];
        sub_435EB0(n7);
        *(float *)(ArgList_1 + 5124) = *(float *)&n7;
        n1056964608 = 1056964608;
      }
      else
      {
        n1056964608 = 1061997773;
      }
      *(_DWORD *)ArgLista = n1056964608;
      *(float *)&n7 = sub_5784C0(n2_4);
      v406 = sub_417330(n2_4);
      n7 = sub_6828B0().m128_u32[0];
      sub_43EEA0(n7);
      sub_A10350(&v414, *(float *)&n7 + 90.0);
      *(float *)&n7 = 30.0;
      v159 = sub_A104E0(v410, &n7);
      v160 = sub_A10420(v405, v159);
      n2_5 = n2;
      sub_4288F0(v160);
      (*(void (__thiscall **)(unsigned int, int, int, int, _DWORD, int))(*(_DWORD *)n2_5 + 60))(
        n2_5,
        ArgList_1 + 5272,
        -1,
        255,
        0,
        1);
      n7 = 7;
      *(_DWORD *)(ArgList_1 + 4920) = *(_DWORD *)sub_41CB80(ArgList_1 + 4920, &n7);
    }
    else
    {
      v152 = sub_6EEF60(0);
      v153 = sub_428B20(n60, 1000, 47, ArgList_1 + 828, (int)&a5, 0, 0, v152);
      v154 = _RTDynamicCast(
               v153,
               0,
               &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
               &IsaacRepentancePlus::Entity_Effect `RTTI Type Descriptor',
               0);
      sub_6B5B50(n7, v154);
      sub_6A92A0(ArgList_2);
      sub_435EB0(*(_DWORD *)(ArgList_2 + 5124));
      sub_43EEA0(*(_DWORD *)(ArgList_2 + 5124));
      sub_A10350(&v414, *(float *)(ArgList_2 + 5124) + 90.0);
      *(float *)&n7 = 30.0;
      v155 = sub_A104E0(v410, &n7);
      v156 = sub_A10420(v405, v155);
      sub_4288F0(v156);
      ArgList_1 = ArgList_2;
      (*(void (__thiscall **)(int, int, int, int, _DWORD, int))(*(_DWORD *)v154 + 60))(
        v154,
        ArgList_2 + 5272,
        -1,
        255,
        0,
        1);
      (*(void (__thiscall **)(int))(*(_DWORD *)v154 + 12))(v154);
    }
    if ( (unsigned __int8)sub_423960(n298) )
    {
      *(_DWORD *)ArgLista = 1065353216;
      if ( !(unsigned __int8)sub_7CF8C0(ArgList_1) )
      {
        v162 = sub_417900(n298);
        *(_DWORD *)ArgLista = sub_925DF0(v162).m128_u32[0];
      }
      v163 = sub_505B60(*(_DWORD *)(ArgList_1 + 5160));
      sub_4071C0(v163);
      sub_407280(*(_DWORD *)ArgLista);
      (*(void (__thiscall **)(_DWORD, _BYTE *, int, int, _DWORD, int))(**(_DWORD **)(ArgList_1 + 5160) + 60))(
        *(_DWORD *)(ArgList_1 + 5160),
        v438,
        -1,
        255,
        0,
        1);
    }
  }
  else if ( v151 )
  {
    (*(void (__thiscall **)(int))(*(_DWORD *)v151 + 40))(v151);
    sub_6B5B50(ArgList_1 + 5160, 0);
  }
  if ( ((unsigned __int8)sub_7706E0(405, 0)
     || (unsigned __int8)sub_7706E0(568, 0)
     || (unsigned __int8)sub_7706E0(726, 0)
     || (unsigned __int8)sub_7706E0(681, 0))
    && *(_BYTE *)(ArgList_1 + 1040) )
  {
    n4_2 = 0;
    *(_OWORD *)&n96[4] = xmmword_BAB370;
    *(_DWORD *)ArgLista = 0;
    do
    {
      if ( sub_9561E0(n298, *(_DWORD *)&n96[4 * n4_2 + 4], *(_DWORD *)(ArgList_1 + 5656), ArgList_1, 0) )
      {
        if ( *(_DWORD *)(ArgList_1 + 8084) == n4_2 && *(int *)(ArgList_1 + 8088) <= 10 )
        {
          sub_67F1D0(&v414, n4_2);
          v165 = sub_407480(n60);
          if ( (unsigned __int8)sub_827B70(v165) )
            *(float *)&v414 = *(float *)&v414 * -1.0;
          n7 = 232;
          LODWORD(v406) = 681;
          v166 = sub_5CBFA0(&n7);
          unknown_libname_8(v166);
          n7 = 93;
          LODWORD(v406) = 405;
          v167 = sub_5CBFA0(&n7);
          unknown_libname_8(v167);
          *(float *)&n7_1 = 0.0;
          *(float *)&n2 = 0.0;
          *(float *)&n7 = 0.0;
          do
          {
            if ( (unsigned __int8)sub_7706E0(n206[2 * n7_1], 0) )
            {
              v169 = sub_407480(n60);
              v170 = sub_428A60(v169);
              sub_41AF60(v170, (int)a2, 3, n206[2 * n7_1 + 1], -1, 0, 0);
              n5 = 5;
              v171 = 0;
              v406 = 0.0;
              if ( sub_4176F0(a2) )
              {
                LODWORD(v402) = 12 * n2;
                do
                {
                  v172 = (_DWORD *)sub_417620(a2, v171);
                  v173 = sub_435DE0(*v172);
                  v174 = v173;
                  if ( v173 )
                  {
                    if ( sub_4361F0(v173) == ArgList_1 && sub_753D30(v174) >= 0 )
                    {
                      v175 = sub_753D30(v174);
                      if ( v175 <= SLODWORD(v402) )
                      {
                        sub_753D40(LODWORD(v402));
                        sub_435D60(v414, HIDWORD(v414));
                        ++n2;
                        LODWORD(v402) += 12;
                      }
                    }
                  }
                  v171 = ++LODWORD(v406);
                }
                while ( v171 < sub_4176F0(a2) );
              }
              n5 = -1;
              sub_4175B0(a2);
              n7_1 = n7;
            }
            n7 = ++n7_1;
          }
          while ( n7_1 < 2 );
          if ( (unsigned __int8)sub_7706E0(568, 0) && *(int *)(ArgList_1 + 8092) <= 0 )
          {
            *(_DWORD *)(ArgList_1 + 8092) = 90;
            sub_6B6470(56, 2, 0, 1065353216);
            v176 = *(_DWORD *)(ArgList_1 + 8096);
            n7 = ArgList_1 + 8096;
            if ( v176 )
            {
              (*(void (__thiscall **)(int))(*(_DWORD *)v176 + 40))(v176);
              sub_6B5B50(ArgList_1 + 8096, 0);
            }
            v177 = sub_6EEF60(v176);
            n2_6 = sub_428B20(n60, 1000, 188, ArgList_1 + 828, (int)&a5, 0, 0, v177);
            sub_6B5B50(n7, n2_6);
            sub_6A92A0(ArgList_2);
            v378 = sub_A10390(&v414);
            sub_435EB0(LODWORD(v378));
            v379 = sub_5784C0(n2_6);
            sub_A10350(v410, LODWORD(v379));
            *(float *)&n7 = 10.0;
            v179 = sub_A104E0(v393, &n7);
            v180 = sub_A10480(v395, v179);
            sub_4288F0(v180);
            *(float *)&n7 = 10.0;
            v181 = sub_A104E0(v400, &n7);
            sub_428910(v181);
            sub_435E90(30);
            (*(void (__thiscall **)(int))(*(_DWORD *)n2_6 + 12))(n2_6);
            ArgList_1 = ArgList_2;
            v182 = *(void (__thiscall **)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD))(*(_DWORD *)ArgList_2 + 60);
            v183 = sub_435B50(1065353216, 0, 0, 0, 0, 1067030938, 1067030938, 1065353216);
            v182(ArgList_1, v183, 8, 2, 1, 1);
          }
          if ( (unsigned __int8)sub_7706E0(726, 0) && !sub_9305F0(ArgList_1 + 5384, 726) )
          {
            v397 = COERCE_FLOAT(sub_9F9020(2, ArgList_1));
            sub_4316C0(LODWORD(v397));
            v184 = sub_779A20(ArgList_1);
            sub_4316B0(v184 | 0x200);
            sub_A02890(&v414, 0);
            sub_9F9050(&v397);
            sub_9302E0(726, 1, 1);
            sub_930390(117, 1, 1);
            sub_40C340(v435, "HeadDownShoot");
            n5 = 6;
            sub_40C340(v432, "HeadRightShoot");
            LOBYTE(n5) = 7;
            sub_40C340(v431, "HeadUpShoot");
            LOBYTE(n5) = 8;
            sub_40C340(v430, "HeadLeftShoot");
            LOBYTE(n5) = 9;
            v185 = (void *)sub_7AD7E0(v439, v414, SHIDWORD(v414), v430, v431, v432, v435);
            LOBYTE(n5) = 10;
            sub_75A3E0(117, 0, v185, -1, 0);
            sub_40BF20(v439);
            sub_40BF20(v430);
            sub_40BF20(v431);
            sub_40BF20(v432);
            n5 = -1;
            sub_40BF20(v435);
          }
          n4_2 = *(_DWORD *)ArgLista;
        }
        *(_DWORD *)(ArgList_1 + 8084) = n4_2;
        *(_DWORD *)(ArgList_1 + 8088) = 0;
      }
      *(_DWORD *)ArgLista = ++n4_2;
    }
    while ( n4_2 < 4 );
  }
  ++*(_DWORD *)(ArgList_1 + 8088);
  v186 = sub_7706E0(400, 0);
  v187 = *(_DWORD *)(ArgList_1 + 5164);
  n7 = ArgList_1 + 5164;
  if ( v186 )
  {
    if ( v187 )
    {
      *(float *)&n2_7 = COERCE_FLOAT(
                          _RTDynamicCast(
                            v187,
                            0,
                            &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
                            &IsaacRepentancePlus::Entity_Effect `RTTI Type Descriptor',
                            0));
      n2 = n2_7;
      if ( LOBYTE(v421) )
      {
        v414 = v413;
        if ( *((float *)&v413 + 1) == 0.0 && *(float *)&v413 == 0.0 )
        {
          switch ( *(_DWORD *)(ArgList_1 + 5672) )
          {
            case 0:
              LODWORD(v414) = -1082130432;
              break;
            case 1:
              HIDWORD(v414) = -1082130432;
              break;
            case 2:
              LODWORD(v414) = 1065353216;
              break;
            case 3:
              HIDWORD(v414) = 1065353216;
              break;
            default:
              break;
          }
        }
        *(float *)&n7 = sub_A10390(&v414) - 90.0;
        n7 = sub_682740().m128_u32[0];
        sub_435EB0(n7);
        *(float *)(ArgList_1 + 5128) = *(float *)&n7;
        *(float *)&n7_2 = 0.5;
      }
      else
      {
        *(float *)&n7_2 = 0.80000001;
      }
      n7 = n7_2;
      v406 = sub_5784C0(n2_7);
      *(float *)ArgLista = sub_417330(n2_7);
      n7 = sub_6828B0().m128_u32[0];
      sub_43EEA0(n7);
      sub_A10350(v410, *(float *)&n7 + 90.0);
      *(float *)&n7 = 45.0;
      v197 = sub_A104E0(v400, &n7);
      sub_A10420(v395, v197);
      sub_A102E0(v393, 1119092736);
      v406 = 10.0;
      v198 = sub_A104E0(v388, &v406);
      v199 = sub_A10420(v434, v198);
      n2_8 = n2;
      sub_4288F0(v199);
      (*(void (__thiscall **)(unsigned int, int, int, int, _DWORD, int))(*(_DWORD *)n2_8 + 60))(
        n2_8,
        ArgList_1 + 5272,
        -1,
        255,
        0,
        1);
      n7 = 7;
      *(_DWORD *)(ArgList_1 + 4920) = *(_DWORD *)sub_41CB80(ArgList_1 + 4920, &n7);
    }
    else
    {
      v188 = sub_6EEF60(0);
      v189 = sub_428B20(n60, 1000, 83, ArgList_1 + 828, (int)&a5, ArgList_1, 0, v188);
      v190 = _RTDynamicCast(
               v189,
               0,
               &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
               &IsaacRepentancePlus::Entity_Effect `RTTI Type Descriptor',
               0);
      sub_6B5B50(n7, v190);
      sub_6A92A0(ArgList_2);
      sub_435EB0(*(_DWORD *)(ArgList_2 + 5128));
      sub_43EEA0(*(_DWORD *)(ArgList_2 + 5128));
      sub_A10350(&v414, *(float *)(ArgList_2 + 5128) + 90.0);
      *(float *)&n7 = 45.0;
      v191 = sub_A104E0(v400, &n7);
      sub_A10420(v395, v191);
      sub_A102E0(v393, 1119092736);
      v406 = 10.0;
      v192 = sub_A104E0(v388, &v406);
      v193 = sub_A10420(v434, v192);
      sub_4288F0(v193);
      v194 = sub_A0FE90(v400);
      sub_4172F0(v194);
      (*(void (__thiscall **)(int, int, int, int, _DWORD, int))(*(_DWORD *)v190 + 60))(
        v190,
        ArgList_2 + 5272,
        -1,
        255,
        0,
        1);
      (*(void (__thiscall **)(int))(*(_DWORD *)v190 + 12))(v190);
      ArgList_1 = ArgList_2;
    }
  }
  else if ( v187 )
  {
    (*(void (__thiscall **)(int))(*(_DWORD *)v187 + 40))(v187);
    sub_6B5B50(ArgList_1 + 5164, 0);
  }
  if ( !(unsigned __int8)sub_417430(0x4000, 0)
    && sub_6AC710((_DWORD *)ArgList_1, 2, 0)
    && (sub_9305F0(ArgList_1 + 5384, 325)
     || (unsigned __int8)sub_417430(0, 4) && *(_DWORD *)(ArgList_1 + 652) == ArgList_1)
    && !(unsigned __int8)sub_7CB580(ArgList_1)
    && !sub_67F010(2) )
  {
    sub_A0FE80(&v414);
    v406 = *(float *)(ArgList_1 + 5500) * 0.75;
    v402 = (float)((float)(sub_431260().m128_f32[0] * 0.15000001) + (float)((float)(v406 * 0.039999999) + 0.60000002))
         * 0.89999998;
    v201 = (float)(unsigned int)sub_67F010(2);
    *(float *)&n7 = (float)(v201 * 0.40000001) + 0.89999998;
    *(float *)&n7 = (float)(sub_67EFE0().m128_f32[0] * 0.050000001) + *(float *)&n7;
    sub_A10030(v410);
    if ( n2_12 )
    {
      *(_DWORD *)ArgLista = 1090519040;
      v202 = sub_A104E0(v400, ArgLista);
      sub_A10450(v202);
    }
    sub_67F070(v400);
    *(_DWORD *)ArgLista = sub_67EFE0().m128_u32[0];
    sub_A104E0(v395, ArgLista);
    *(float *)&n2 = 2.0;
    v203 = sub_A104E0(v393, &n2);
    sub_A10450(v203);
    v204 = sub_7AAEA0(v388, v410, 1);
    sub_A10450(v204);
    v206 = sub_6EEF60(v205);
    v207 = sub_428B20(n60, 2, 1, ArgList_1 + 828, (int)&v414, ArgList_1, 0, v206);
    sub_676120(*(float *)&n7 * v402);
    sub_675FC0(-1052770304);
    v380 = sub_435E50(v207) + 0.5;
    sub_444C90(LODWORD(v380));
    *(float *)ArgLista = sub_456440(v207);
    v208 = sub_67EFE0().m128_f32[0];
    sub_444C70((float)(*(float *)ArgLista - (float)(v208 * 10.0)) - 4.0);
    (*(void (__thiscall **)(int, _DWORD))(*(_DWORD *)v207 + 64))(v207, *(float *)&n7 * v406);
    (*(void (__thiscall **)(int))(*(_DWORD *)v207 + 12))(v207);
  }
  HIDWORD(v414) = *(_DWORD *)(ArgList_1 + 5484);
  n2 = HIDWORD(v414);
  if ( (unsigned __int8)sub_771550(139, 0) )
  {
    v209 = sub_7CB6E0(139);
    *(float *)&n2 = (float)((float)((float)v209 + (float)v209) + 2.0) + *(float *)&n2;
    *(float *)(ArgList_1 + 5484) = *(float *)&n2;
  }
  n481 = *(_DWORD *)(ArgList_1 + 5632);
  if ( n481 > 481 )
  {
    switch ( n481 )
    {
      case 512:
        if ( (unsigned __int8)sub_7AB110(ArgList_1) )
        {
          sub_7ABE90((char)"LiftItem");
          sub_40A760(ArgList_1 + 1052);
        }
        v218 = LOBYTE(v421);
        if ( !LOBYTE(v421) )
          goto LABEL_588;
        if ( (unsigned __int8)sub_417430(0x4000, 0) )
          goto LABEL_587;
        v298 = sub_6EEF60(v297);
        v411 = 34.0;
        sub_A104E0(v400, &v411);
        *(float *)&n7 = 0.5;
        sub_A104E0(v395, &n7);
        v406 = 2.0;
        v299 = sub_A104E0(v393, &v406);
        v300 = sub_A10420(v388, v299);
        v375 = v298;
        ArgList_1 = ArgList_2;
        v301 = sub_428B20(n60, 1000, 107, ArgList_2 + 828, v300, ArgList_2, 0, v375);
        v302 = _RTDynamicCast(
                 v301,
                 0,
                 &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
                 &IsaacRepentancePlus::Entity_Effect `RTTI Type Descriptor',
                 0);
        (*(void (__thiscall **)(int))(*(_DWORD *)v302 + 12))(v302);
        sub_9568E0(1065353216, 44, 2, 0, 1065353216);
        goto LABEL_484;
      case 604:
        v303 = (Concurrency::details::_ContextCallback *)sub_420AE0(n60);
        if ( !Concurrency::details::_ContextCallback::_HasCapturedContext(v303)
          || (v304 = sub_420AE0(n60), !(unsigned __int8)sub_82EB90(v304)) )
        {
          if ( !*(_BYTE *)(ArgList_1 + 5018) && !*(_BYTE *)(ArgList_1 + 5016) && *(_DWORD *)(ArgList_1 + 5136) )
          {
            sub_7ABE90((char)"LiftItem");
            sub_40A760(ArgList_1 + 1052);
            sub_407F10((char *)(ArgList_1 + 4088));
          }
        }
        if ( (unsigned __int8)sub_417430(0x4000, 0) )
          goto LABEL_587;
        if ( *(_DWORD *)(ArgList_1 + 5056) == 40
          && *(_DWORD *)(ArgList_1 + 5136)
          && (v305 = *(_DWORD *)(ArgList_1 + 5136), v305 == sub_417820(ArgList_1))
          && sub_42D040(v305)
          && *(int *)(v305 + 6392) <= 0
          && ((v306 = sub_42D040(v305), (*(unsigned __int8 (__thiscall **)(int))(*(_DWORD *)v306 + 64))(v306))
           || *(_BYTE *)(v305 + 6408))
          && *(_BYTE *)(ArgList_1 + 6181) )
        {
          v218 = LOBYTE(v421);
        }
        else
        {
          v218 = LOBYTE(v421);
          if ( LOBYTE(v421) )
          {
            if ( *(_DWORD *)(ArgList_1 + 5136) )
            {
              v411 = 10.0;
              sub_A104E0(&v416, &v411);
              v307 = sub_7AAEA0(v400, &v413, 0);
              sub_A10450(v307);
              v411 = 1.2;
              sub_A10540(&v411);
              sub_7D2470(&v416);
              sub_407F10((char *)(ArgList_1 + 4088));
              *(_DWORD *)(ArgList_1 + 4920) = 1;
              *(_DWORD *)(ArgList_1 + 5636) = 5;
              sub_7ABE90((char)"HideItem");
              if ( *(_DWORD *)(ArgList_1 + 5504) == 604 )
                sub_7AC7E0(ArgList_1, 0, 0);
              goto LABEL_587;
            }
LABEL_545:
            if ( *(_DWORD *)(ArgList_1 + 5636) )
              goto LABEL_588;
            sub_407F10((char *)(ArgList_1 + 4088));
            *(_DWORD *)(ArgList_1 + 5636) = 5;
            goto LABEL_494;
          }
        }
        if ( *(_DWORD *)(ArgList_1 + 5136) )
          goto LABEL_588;
        goto LABEL_545;
      case 623:
      case 638:
        if ( (unsigned __int8)sub_7AB110(ArgList_1) )
        {
          sub_7ABE90((char)"LiftItem");
          sub_40A760(ArgList_1 + 1052);
        }
        if ( (unsigned __int8)sub_417430(0x4000, 0) )
          goto LABEL_587;
        if ( *(_DWORD *)(ArgList_1 + 5632) == 623 && *(int *)(ArgList_1 + 4956) <= 0 && !*(_DWORD *)(ArgList_1 + 5636) )
        {
          *(_DWORD *)(ArgList_1 + 5636) = 5;
          goto LABEL_494;
        }
        v218 = LOBYTE(v421);
        if ( !LOBYTE(v421) )
          goto LABEL_588;
        v411 = 10.0;
        sub_A104E0(v407, &v411);
        v308 = sub_7AAEA0(v400, &v413, 0);
        sub_A10450(v308);
        v36 = *(_DWORD *)(ArgList_1 + 5632) == 623;
        n45 = 45;
        n7 = 45;
        if ( v36 )
        {
          v310 = *(float *)(ArgList_1 + 5220) * 1.5;
          n45 = 44;
          n7 = 44;
          v411 = v310;
          sub_A10540(&v411);
        }
        sub_7BF6C0(v426, 1, 1065353216, 0, ArgList_1);
        v312 = sub_6EEF60(v311);
        v313 = sub_428B20(n60, 2, n45, ArgList_1 + 828, (int)v407, ArgList_1, 1, v312);
        sub_675FC0(v428);
        if ( n7 == 45 )
        {
          (*(void (__thiscall **)(int, int))(*(_DWORD *)v313 + 64))(v313, 1097859072);
        }
        else
        {
          v314 = sub_417850();
          if ( (unsigned __int8)sub_429550(v314) )
            v315 = (float)(v427 * 4.0) + 5.0;
          else
            v315 = (float)(v427 * 5.0) + 30.0;
          (*(void (__stdcall **)(float))(*(_DWORD *)v313 + 64))(COERCE_FLOAT(LODWORD(v315)));
          v316 = (_DWORD *)sub_435C20(&xmmword_C34B50);
          sub_676150(*v316, v316[1], v316[2], v316[3]);
        }
        sub_6A92A0(ArgList_1);
        (*(void (__thiscall **)(int))(*(_DWORD *)v313 + 12))(v313);
        *(_DWORD *)(ArgList_1 + 5636) = 5;
        sub_7AC7E0(ArgList_1, 0, 0);
        *(_DWORD *)(ArgList_1 + 4920) = 1;
        sub_7ABE90((char)"HideItem");
        v317 = sub_67EFE0().m128_f32[0];
        sub_6B6470(252, 2, 0, (float)(v317 * 0.2) + 0.89999998);
        if ( *(_DWORD *)(ArgList_1 + 5632) == 623 )
          sub_7595B0(-1);
        goto LABEL_587;
      case 709:
        goto LABEL_587;
      case 710:
        if ( (unsigned __int8)sub_7AB110(ArgList_1) )
        {
          sub_7ABE90((char)"LiftItem");
          sub_40A760(ArgList_1 + 1052);
        }
        v36 = (unsigned __int8)sub_417430(0x4000, 0) == 0;
        v218 = LOBYTE(v421);
        if ( !v36 || !LOBYTE(v421) )
          goto LABEL_588;
        sub_5CD690(v423);
        v319 = sub_6EEF60(v318);
        v320 = sub_417290(v400);
        v321 = sub_428B20(n60, 8, 4, v320, (int)&a5, ArgList_1, 0, v319);
        sub_6A92A0(ArgList_1);
        v322 = (_DWORD *)sub_A0FE90(v395);
        sub_435D60(*v322, v322[1]);
        v387 = sub_A10390(&v413);
        sub_5B15D0(LODWORD(v387));
        v323 = sub_A0FE90(v393);
        sub_4172F0(v323);
        v324 = sub_417320(ArgList_1);
        sub_42D9B0(v324);
        __std_stacktrace_to_string(v321);
        sub_665BC0(1);
        sub_753770(0);
        (*(void (__thiscall **)(int, int))(*(_DWORD *)v321 + 64))(v321, 1065353216);
        (*(void (__thiscall **)(int))(*(_DWORD *)v321 + 12))(v321);
        *(_DWORD *)(ArgList_1 + 5636) = 5;
        v325 = sub_7911C0(710);
        if ( v325 >= 0 )
          sub_7AC7E0(ArgList_1, v325, 0);
        *(_DWORD *)(ArgList_1 + 4920) = 1;
        sub_7ABE90((char)"HideItem");
        v326 = sub_67EFE0().m128_f32[0];
        sub_6B6470(2, 2, 0, (float)(v326 * 0.2) + 0.89999998);
        goto LABEL_587;
      case 728:
        if ( (unsigned __int8)sub_7AB110(ArgList_1) )
        {
          sub_7ABE90((char)"LiftItem");
          sub_40A760(ArgList_1 + 1052);
        }
        v36 = (unsigned __int8)sub_417430(0x4000, 0) == 0;
        v218 = LOBYTE(v421);
        if ( !v36 || !LOBYTE(v421) )
          goto LABEL_588;
        v327 = sub_7706E0(356, 0);
        v401 = 0;
        v402 = 0.0;
        *(_DWORD *)ArgLista = (v327 != 0) + 1;
        if ( v327 )
        {
          v401 = -1054867456;
          v402 = 10.0;
        }
        *(float *)&n7 = 0.0;
        do
        {
          v411 = 10.0;
          sub_A104E0(&v398, &v411);
          v328 = sub_7AAEA0(v400, &v413, 0);
          v406 = 0.40000001;
          v329 = sub_4171D0(v328);
          sub_A10450(v329);
          sub_A0FFC0(&v398);
          sub_A10270(&v398);
          v331 = sub_6EEF60(v330);
          v332 = sub_428B20(n60, 3, 240, ArgList_1 + 828, (int)&a5, ArgList_1, 0, v331);
          sub_417400(4, 0);
          sub_435D60(v398, HIDWORD(v398));
          sub_5B18F0(1);
          (*(void (__thiscall **)(int))(*(_DWORD *)v332 + 12))(v332);
          ++n7;
        }
        while ( n7 < *(_DWORD *)ArgLista );
        sub_6B6470(238, 2, 0, 1065353216);
        goto LABEL_493;
      case 729:
        if ( (unsigned __int8)sub_7AB110(ArgList_1) )
        {
          sub_7ABE90((char)"LiftItem");
          sub_40A760(ArgList_1 + 1052);
        }
        v36 = (unsigned __int8)sub_417430(0x4000, 0) == 0;
        v218 = LOBYTE(v421);
        if ( !v36 || !LOBYTE(v421) )
          goto LABEL_588;
        v411 = 10.0;
        sub_A104E0(&v401, &v411);
        v333 = sub_7AAEA0(v400, &v413, 0);
        v411 = 0.40000001;
        v334 = sub_4171D0(v333);
        sub_A10450(v334);
        v336 = sub_6EEF60(v335);
        v337 = ArgList_1 + 828;
        v338 = sub_428B20(n60, 3, 243, v337, (int)&a5, ArgList_2, 0, v336);
        sub_417400(4, 0);
        sub_4173E0(0, 0x4000000);
        v339 = (_DWORD *)sub_A0FE90(v393);
        sub_435D60(*v339, v339[1]);
        sub_428910(&v401);
        sub_5B18F0(1);
        (*(void (__thiscall **)(int))(*(_DWORD *)v338 + 12))(v338);
        v341 = sub_6EEF60(v340);
        v411 = 0.34999999;
        v342 = sub_A104E0(v400, &v411);
        v343 = sub_428B20(n60, 1000, 2, v337, v342, 0, 2, v341);
        (*(void (__thiscall **)(int))(*(_DWORD *)v343 + 12))(v343);
        sub_9302E0(729, 1, 2);
        ArgList_1 = ArgList_2;
        sub_6B6470(23, 2, 0, 1065353216);
        goto LABEL_493;
      default:
LABEL_586:
        sub_A112C0(1, "No Such State %d\n", *(_DWORD *)(ArgList_1 + 5632));
        goto LABEL_587;
    }
  }
  if ( n481 == 481 )
  {
    if ( (unsigned __int8)sub_7AB110(ArgList_1) )
    {
      sub_7ABE90((char)"LiftItem");
      sub_40A760(ArgList_1 + 1052);
    }
    v36 = (unsigned __int8)sub_417430(0x4000, 0) == 0;
    v218 = LOBYTE(v421);
    if ( !v36 || !LOBYTE(v421) )
      goto LABEL_588;
    sub_5CD690(v424);
    v289 = sub_6EEF60(v288);
    v290 = sub_417290(v400);
    v291 = sub_428B20(n60, 8, 8, v290, (int)&a5, ArgList_1, 0, v289);
    sub_6A92A0(ArgList_1);
    v292 = (_DWORD *)sub_A0FE90(v395);
    sub_435D60(*v292, v292[1]);
    v386 = sub_A10390(&v413);
    sub_5B15D0(LODWORD(v386));
    v293 = sub_A0FE90(v393);
    sub_4172F0(v293);
    v294 = sub_417320(ArgList_1);
    sub_42D9B0(v294);
    __std_stacktrace_to_string(v291);
    sub_665BC0(1);
    sub_753770(0);
    (*(void (__thiscall **)(int, int))(*(_DWORD *)v291 + 64))(v291, 1121583104);
    (*(void (__thiscall **)(int))(*(_DWORD *)v291 + 12))(v291);
    *(_DWORD *)(ArgList_1 + 5636) = 5;
    v295 = sub_7911C0(481);
    if ( v295 >= 0 )
      sub_7AC7E0(ArgList_1, v295, 0);
    *(_DWORD *)(ArgList_1 + 4920) = 1;
    sub_7ABE90((char)"HideItem");
    v296 = sub_67EFE0().m128_f32[0];
    sub_6B6470(252, 2, 0, (float)(v296 * 0.2) + 0.89999998);
    goto LABEL_587;
  }
  if ( n481 > 164 )
  {
    switch ( n481 )
    {
      case 289:
        goto LABEL_463;
      case 338:
        if ( n2_12 && !(unsigned __int8)sub_417430(0x4000, 0) )
        {
          *(_DWORD *)(ArgList_1 + 5636) = 15;
          v411 = 15.0;
          sub_A104E0(v400, &v411);
          *(float *)&n7 = 0.5;
          sub_A104E0(v395, &n7);
          v406 = 2.0;
          v258 = sub_A104E0(v393, &v406);
          sub_A10420(v396, v258);
          v260 = sub_6EEF60(v259);
          v261 = sub_428B20(n60, 1000, 60, ArgList_1 + 828, (int)v396, ArgList_1, 0, v260);
          v262 = _RTDynamicCast(
                   v261,
                   0,
                   &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
                   &IsaacRepentancePlus::Entity_Effect `RTTI Type Descriptor',
                   0);
          sub_435E90((int)(float)((float)-*(float *)(ArgList_1 + 5236) * 0.75));
          (*(void (__thiscall **)(int, _DWORD))(*(_DWORD *)v262 + 64))(
            v262,
            *(float *)(ArgList_1 + 5232) + *(float *)(ArgList_1 + 5232));
          sub_6A92A0(ArgList_1);
          (*(void (__thiscall **)(int))(*(_DWORD *)v262 + 12))(v262);
          sub_9568E0(1065353216, 68, 2, 0, 1065353216);
          *(_DWORD *)(ArgList_1 + 4920) = 15;
          sub_7AC7E0(ArgList_1, 0, 0);
          sub_407F10((char *)(ArgList_1 + 4088));
          sub_42CA00(n298);
          v263 = sub_72FDA0(30);
          sub_75D850(v263);
        }
        goto LABEL_587;
      case 352:
        if ( (unsigned __int8)sub_7AB110(ArgList_1) )
        {
          sub_7ABE90((char)"LiftItem");
          sub_40A760(ArgList_1 + 1052);
        }
        v218 = LOBYTE(v421);
        if ( !LOBYTE(v421) )
          goto LABEL_588;
        if ( (unsigned __int8)sub_417430(0x4000, 0) )
          goto LABEL_587;
        v411 = 10.0;
        sub_A104E0(&v398, &v411);
        v264 = sub_7AAEA0(v400, &v413, 0);
        sub_A10450(v264);
        v265 = (_DWORD *)sub_A104E0(v395, ArgList_1 + 5220);
        v266 = sub_790AF0((_DWORD *)ArgList_1, ArgList_1 + 828, *v265, v265[1], 1, 0, 1065353216);
        v383 = sub_557CD0(v266) * 5.0;
        sub_676120(LODWORD(v383));
        v267 = *(void (__thiscall **)(_DWORD, _DWORD))(*(_DWORD *)v266 + 64);
        v411 = sub_5B1570(v266) * 10.0 + 10.0;
        v267(v266, LODWORD(v411));
        sub_4177C0(&n96[4]);
        sub_435C20(&xmmword_C34B50);
        v268 = (_DWORD *)sub_435C20(&xmmword_C34BB0);
        sub_676150(*v268, v268[1], v268[2], v268[3]);
        ArgList_1 = ArgList_2;
        sub_6B6470(226, 2, 0, 1065353216);
        sub_A10660(v393);
        v411 = 0.5;
        v269 = sub_A104E0(v400, &v411);
        sub_4314B0(v269, 0);
        break;
      case 382:
        if ( (unsigned __int8)sub_7AB110(ArgList_1) )
        {
          sub_7ABE90((char)"LiftItem");
          sub_40A760(ArgList_1 + 1052);
          sub_9568E0(1065353216, 254, 2, 0, 1065353216);
        }
        v218 = LOBYTE(v421);
        if ( !LOBYTE(v421) )
          goto LABEL_588;
        if ( (unsigned __int8)sub_417430(0x4000, 0) )
          goto LABEL_587;
        v411 = 15.0;
        sub_A104E0(v405, &v411);
        v270 = sub_7AAEA0(v400, &v413, 0);
        sub_A10450(v270);
        if ( sub_421770(n60) == 33 )
        {
          v272 = sub_5CBD00(382);
          if ( sub_7E9020(v272, 2) )
          {
            v275 = sub_417910(n298);
            *(float *)&n7_3 = COERCE_FLOAT(sub_5CD4E0(v275));
          }
          else
          {
            v273 = sub_417910(n298);
            *(float *)&n7_3 = COERCE_FLOAT(sub_6EF3F0(v273));
          }
          n7 = n7_3;
          if ( sub_60C6E0(n7_3) )
          {
            v277 = sub_5CBD00(382);
            v278 = sub_60C6E0(n7);
            v279 = sub_7E9020(v277, v278);
            v280 = sub_60C6C0(v279);
            *(_OWORD *)n96 = *(_OWORD *)v280;
            *(_DWORD *)&n96[16] = *(_DWORD *)(v280 + 16);
            if ( (unsigned __int8)sub_6C07E0(n96, &n96[4]) )
            {
              sub_417910(n298);
              v281 = (_DWORD *)sub_694FB0(*(_DWORD *)n96, *(_DWORD *)&n96[4], 0);
              if ( v281 )
                *(_OWORD *)n96 = *(_OWORD *)sub_5CD4B0(*v281, v281[1], v281[2], v281[20], 0);
            }
            v282 = *(float *)&n96[12];
            v283 = *(_DWORD *)n96;
            *(_DWORD *)(ArgList_1 + 6436) = *(_DWORD *)&n96[12];
            *(_DWORD *)(ArgList_1 + 6416) = v283;
            *(_DWORD *)(ArgList_1 + 6420) = *(_DWORD *)&n96[4];
            v284 = *(_DWORD *)&n96[8];
            *(float *)(ArgList_1 + 6432) = v282 * 0.5;
            *(_DWORD *)(ArgList_1 + 6424) = v284;
          }
          v384 = sub_6EEF60(v276);
          v285 = sub_428B20(n60, 1000, 81, ArgList_1 + 828, (int)v405, ArgList_1, 1, v384);
        }
        else
        {
          v286 = *(_DWORD *)(ArgList_1 + 6416);
          v385 = sub_6EEF60(v271);
          v285 = sub_428B20(n60, 1000, 81, ArgList_1 + 828, (int)v405, ArgList_1, v286 != 0, v385);
        }
        v287 = v285;
        sub_6A92A0(ArgList_1);
        (*(void (__thiscall **)(int))(*(_DWORD *)v287 + 12))(v287);
        sub_9568E0(1065353216, 0, 2, 0, 1065353216);
        break;
      default:
        goto LABEL_586;
    }
LABEL_493:
    *(_DWORD *)(ArgList_1 + 5636) = 5;
    sub_7AC7E0(ArgList_1, 0, 0);
LABEL_494:
    *(_DWORD *)(ArgList_1 + 4920) = 1;
    sub_7ABE90((char)"HideItem");
    goto LABEL_587;
  }
  if ( n481 == 164 )
  {
LABEL_463:
    if ( (unsigned __int8)sub_7AB110(ArgList_1) )
    {
      sub_7ABE90((char)"LiftItem");
      sub_40A760(ArgList_1 + 1052);
    }
    v218 = LOBYTE(v421);
    if ( !LOBYTE(v421) )
      goto LABEL_588;
    if ( !(unsigned __int8)sub_417430(0x4000, 0) )
    {
      n289 = *(_DWORD *)(ArgList_1 + 5632);
      *(_DWORD *)ArgLista = ArgList_1 + 5728;
      if ( n289 == 164 )
      {
        v221 = sub_7706E0(356, 0);
        v398 = 0x40E0000041600000LL;
        v222 = &v398;
        *(float *)&n7 = COERCE_FLOAT(&v398);
        v223 = (v221 != 0) + 1;
        v406 = *(float *)&v223;
        do
        {
          v224 = sub_6EEF60(v223);
          sub_A104E0(v400, v222);
          v402 = 0.5;
          sub_A104E0(v395, &v402);
          v397 = 2.0;
          v225 = sub_A104E0(v393, &v397);
          v226 = sub_A10420(v388, v225);
          v372 = v224;
          ArgList_1 = ArgList_2;
          v227 = sub_428B20(n60, 1000, 10, ArgList_2 + 828, v226, ArgList_2, 0, v372);
          v228 = _RTDynamicCast(
                   v227,
                   0,
                   &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
                   &IsaacRepentancePlus::Entity_Effect `RTTI Type Descriptor',
                   0);
          sub_435E90(60);
          (*(void (__thiscall **)(int, int))(*(_DWORD *)v228 + 64))(v228, 1102577664);
          v229 = sub_417850();
          if ( (unsigned __int8)sub_429550(v229) )
          {
            v230 = sub_417850();
            v231 = sub_4176F0(v230);
            (*(void (__thiscall **)(int, int))(*(_DWORD *)v231 + 68))(v231, v228);
          }
          if ( !(unsigned __int8)sub_7CF8C0(ArgList_1) )
          {
            v232 = sub_505B60(v228);
            sub_4071C0(v232);
            v233 = sub_417900(n298);
            LODWORD(v411) = sub_925DF0(v233).m128_u32[0];
            v234 = sub_407270(v438);
            v381 = v234 * v411;
            sub_407280(LODWORD(v381));
            (*(void (__thiscall **)(int, _BYTE *, int, int, _DWORD, int))(*(_DWORD *)v228 + 60))(
              v228,
              v438,
              -1,
              255,
              0,
              1);
          }
          (*(void (__thiscall **)(int))(*(_DWORD *)v228 + 12))(v228);
          v222 = (__int64 *)(n7 + 4);
          v36 = LODWORD(v406)-- == 1;
          n7 += 4;
        }
        while ( !v36 );
      }
      else
      {
        if ( n289 != 289 )
        {
          v249 = sub_6EEF60(n289);
          v411 = 14.0;
          sub_A104E0(v400, &v411);
          *(float *)&n7 = 0.5;
          sub_A104E0(v395, &n7);
          v406 = 2.0;
          v250 = sub_A104E0(v393, &v406);
          v251 = sub_A10420(v388, v250);
          v374 = v249;
          ArgList_1 = ArgList_2;
          v252 = sub_428B20(n60, 2, 4, ArgList_2 + 828, v251, ArgList_2, 0, v374);
          v253 = _RTDynamicCast(
                   v252,
                   0,
                   &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
                   &IsaacRepentancePlus::Entity_Tear `RTTI Type Descriptor',
                   0);
          v254 = sub_67F010(2);
          sub_7BF6C0(v426, 5, 1065353216, 2 * v254 - 1, 0);
          v255 = sub_780560(&n96[4], 0);
          sub_435C20(v255);
          sub_435C20(&dword_C34BC0);
          v256 = (_DWORD *)sub_435C20(&xmmword_C34BD0);
          sub_676150(*v256, v256[1], v256[2], v256[3]);
          (*(void (__thiscall **)(int, _DWORD))(*(_DWORD *)v253 + 64))(v253, *(float *)(ArgList_1 + 5232) + 185.0);
          v257 = sub_67EFE0().m128_f32[0];
          sub_444C70((float)(v257 * 0.2) + 1.2);
          sub_675FC0(-1039925248);
          sub_6A92A0(ArgList_1);
          (*(void (__thiscall **)(int))(*(_DWORD *)v253 + 12))(v253);
          sub_6B6470(252, 2, 0, 1065353216);
          goto LABEL_484;
        }
        v235 = sub_7706E0(356, 0);
        v398 = 0x40E0000041600000LL;
        v236 = &v398;
        *(float *)&n7 = COERCE_FLOAT(&v398);
        v237 = (v235 != 0) + 1;
        v406 = *(float *)&v237;
        do
        {
          v238 = sub_6EEF60(v237);
          sub_A104E0(v400, v236);
          v411 = 0.5;
          sub_A104E0(v395, &v411);
          v402 = 2.0;
          v239 = sub_A104E0(v393, &v402);
          v240 = sub_A10420(v388, v239);
          v373 = v238;
          ArgList_1 = ArgList_2;
          v241 = sub_428B20(n60, 1000, 52, ArgList_2 + 828, v240, ArgList_2, 0, v373);
          v242 = _RTDynamicCast(
                   v241,
                   0,
                   &IsaacRepentancePlus::Entity `RTTI Type Descriptor',
                   &IsaacRepentancePlus::Entity_Effect `RTTI Type Descriptor',
                   0);
          (*(void (__thiscall **)(int, int))(*(_DWORD *)v242 + 64))(v242, 1102577664);
          v243 = sub_417850();
          if ( (unsigned __int8)sub_429550(v243) )
          {
            v244 = sub_417850();
            v245 = sub_4176F0(v244);
            (*(void (__thiscall **)(int, int))(*(_DWORD *)v245 + 68))(v245, v242);
          }
          if ( !(unsigned __int8)sub_7CF8C0(ArgList_1) )
          {
            v246 = sub_505B60(v242);
            sub_4071C0(v246);
            v247 = sub_417900(n298);
            LODWORD(v397) = sub_925DF0(v247).m128_u32[0];
            v248 = sub_407270(v438);
            v382 = v248 * v397;
            sub_407280(LODWORD(v382));
            (*(void (__thiscall **)(int, _BYTE *, int, int, _DWORD, int))(*(_DWORD *)v242 + 60))(
              v242,
              v438,
              -1,
              255,
              0,
              1);
          }
          (*(void (__thiscall **)(int))(*(_DWORD *)v242 + 12))(v242);
          v236 = (__int64 *)(n7 + 4);
          v36 = LODWORD(v406)-- == 1;
          n7 += 4;
        }
        while ( !v36 );
      }
      sub_6B6470(215, 2, 0, 1065353216);
LABEL_484:
      sub_7ABE90((char)"HideItem");
      *(_DWORD *)(ArgList_1 + 5636) = 5;
      sub_7AC7E0(ArgList_1, 0, 0);
      v218 = LOBYTE(v421);
      v219 = (int *)(ArgList_1 + 4920);
      *(_DWORD *)(ArgList_1 + 4920) = 1;
      goto LABEL_589;
    }
LABEL_587:
    v218 = LOBYTE(v421);
LABEL_588:
    v219 = (int *)(ArgList_1 + 4920);
    goto LABEL_589;
  }
  switch ( n481 )
  {
    case 0:
      sub_67F170(v410, &v391);
      if ( (unsigned __int8)sub_7706E0(465, 0) || *(_DWORD *)(ArgList_1 + 5132) )
        unknown_libname_8(&v391);
      if ( v422 || (n13 = *(_DWORD *)(ArgList_1 + 5056), n13 == 13) || n13 == 32 )
      {
        if ( (unsigned __int8)sub_7706E0(244, 0) )
        {
          v212 = *(_DWORD *)(ArgList_1 + 5224);
          if ( v212 <= 0 )
          {
            v36 = n2_12 == 0;
            *(_DWORD *)(ArgList_1 + 5224) = 15;
            if ( (!v36 || *(_DWORD *)(ArgList_1 + 5132)) && sub_67F010(3) )
            {
              v213 = sub_7AAB70(ArgList_1 + 828, 2, v410, 0, 1, 0, 1065353216);
              sub_97FE50(v213);
              (*(void (__thiscall **)(int))(*(_DWORD *)v213 + 12))(v213);
              *(_DWORD *)(ArgList_1 + 4920) = 5;
            }
          }
          else
          {
            *(_DWORD *)(ArgList_1 + 5224) = v212 - 1;
          }
        }
        if ( v422 )
        {
          if ( (unsigned __int8)sub_7AB110(ArgList_1) )
          {
            v214 = (Concurrency::details::_ContextCallback *)sub_428B50(n60);
            if ( !Concurrency::details::_ContextCallback::_HasCapturedContext(v214) )
            {
              if ( v418 )
                sub_7826D0((_DWORD *)ArgList_1, &v413, LODWORD(v421));
            }
          }
        }
      }
      goto LABEL_587;
    case 42:
      goto LABEL_463;
    case 47:
      if ( (unsigned __int8)sub_7AB110(ArgList_1) )
      {
        sub_7ABE90((char)"LiftItem");
        sub_40A760(ArgList_1 + 1052);
      }
      sub_7826D0((_DWORD *)ArgList_1, &v413, 1);
      if ( !*(_DWORD *)(ArgList_1 + 5632) )
        sub_7ABE90((char)"HideItem");
      goto LABEL_587;
    case 49:
      if ( !n2_12 || (unsigned __int8)sub_417430(0x4000, 0) )
        goto LABEL_587;
      *(_DWORD *)(ArgList_1 + 5636) = 30;
      sub_6B6470(5, 2, 0, 1065353216);
      v215 = sub_7AA850(v400, 3, v389);
      v216 = sub_456230().m128_f32[0];
      v217 = sub_96F310(3, (_DWORD *)(ArgList_1 + 828), v216 * 57.295776, 15, v215, (_DWORD *)ArgList_1, 0);
      (*(void (__thiscall **)(int, _DWORD))(*(_DWORD *)v217 + 64))(
        v217,
        *(float *)(ArgList_1 + 5232) + *(float *)(ArgList_1 + 5232));
      if ( *(float *)&v389[1] <= -0.5 )
        sub_435CF0(-1035468800);
      else
        sub_435CF0(1161527296);
      (*(void (__thiscall **)(int))(*(_DWORD *)v217 + 12))(v217);
      sub_7AC7E0(ArgList_1, 0, 0);
      v218 = LOBYTE(v421);
      v219 = (int *)(ArgList_1 + 4920);
      *(_DWORD *)(ArgList_1 + 4920) = 30;
      break;
    default:
      goto LABEL_586;
  }
LABEL_589:
  if ( *(float *)(ArgList_1 + 5484) == *(float *)&n2 )
    *(_DWORD *)(ArgList_1 + 5484) = HIDWORD(v414);
  if ( !v218 && *v219 < 0 && *(int *)(ArgList_1 + 4924) < 0 )
  {
    v344 = (Concurrency::details::_ContextCallback *)sub_420AE0(n60);
    if ( !Concurrency::details::_ContextCallback::_HasCapturedContext(v344) )
    {
      v345 = *(_DWORD *)(ArgList_1 + 5668);
      v346 = (float *)sub_779900(v400);
      if ( sub_A10630(v346, (float *)&a5) )
      {
        v347 = sub_779900(v400);
        v345 = sub_67F260(v347);
      }
      switch ( v345 )
      {
        case 0:
          sub_414550((void *)(ArgList_1 + 4896), "HeadLeft");
          *(_DWORD *)(ArgList_1 + 5672) = 0;
          break;
        case 1:
          sub_414550((void *)(ArgList_1 + 4896), "HeadUp");
          *(_DWORD *)(ArgList_1 + 5672) = 1;
          break;
        case 2:
          sub_414550((void *)(ArgList_1 + 4896), "HeadRight");
          *(_DWORD *)(ArgList_1 + 5672) = 2;
          break;
        default:
          sub_414550((void *)(ArgList_1 + 4896), "HeadDown");
          *(_DWORD *)(ArgList_1 + 5672) = 3;
          break;
      }
    }
  }
  if ( !(unsigned __int8)sub_7CB580(ArgList_1) )
  {
    if ( *(_DWORD *)(ArgList_1 + 44) || *(_BYTE *)(ArgList_1 + 8361) )
    {
      n4_3 = 0;
      LODWORD(v363) = ArgList_1 + 5088;
      do
      {
        if ( *(_DWORD *)LODWORD(v363) )
        {
          if ( sub_4E46B0().m128_f32[0] > 0.0 )
            goto LABEL_639;
          v363 = v394;
        }
        ++n4_3;
        LODWORD(v363) += 4;
        v394 = v363;
      }
      while ( n4_3 < 4 );
      sub_40A690(ArgList_1 + 72, ArgList_1 + 4896, n2_3);
    }
    else
    {
      if ( (unsigned __int8)sub_7706E0(683, 0) && !sub_40AC50(ArgList_1 + 72) && SLODWORD(v412) > 8 )
      {
        HIDWORD(v414) = *(_DWORD *)(ArgList_1 + 848);
        v348 = sub_407170(v425);
        v349 = sub_67EFE0().m128_f32[0];
        sub_703330(n60, (_DWORD *)(ArgList_1 + 828), 35, 1, (float)(v349 * 3.0) + 2.0, v348, HIDWORD(v414), 1);
      }
      sub_40ABB0((_DWORD *)(ArgList_1 + 72), ArgList_1 + 4896, n2_3);
      ArgList_6 = (_BYTE *)(ArgList_1 + 4664);
      n7 = 15;
      ArgList_2 = ArgList_1 + 4664;
      do
      {
        if ( *((int *)ArgList_6 - 3) >= 0 && !*ArgList_6 )
        {
          v351 = sub_5CBD10(*((_DWORD *)ArgList_6 - 3));
          if ( !*(_BYTE *)(v351 + 284) )
          {
            v352 = sub_40BCB0(ArgList_1 + 4896);
            if ( v352 )
            {
              if ( *(_BYTE *)(v352 + 52) )
              {
                sub_40A360(ArgList_1 + 4896, 0);
                v353 = sub_40B200(*(_DWORD *)(ArgList_2 - 8));
                *(float *)(v353 + 68) = (float)n2_3 * *(float *)(v353 + 56);
              }
              else
              {
                if ( SLODWORD(v412) > 8
                  || (v354 = sub_421620(v439, (void *)(ArgList_1 + 4896), "_Idle"),
                      n5 = 11,
                      v390 |= 1u,
                      HIDWORD(v414) = v390,
                      v355 = sub_40BCB0(v354),
                      n2_12 = 1,
                      !v355) )
                {
                  n2_12 = 0;
                }
                n5 = -1;
                if ( (v390 & 1) != 0 )
                {
                  v390 &= ~1u;
                  sub_40BF20(v439);
                }
                if ( n2_12 )
                {
                  v356 = sub_421620(v439, (void *)(ArgList_1 + 4896), "_Idle");
                  n5 = 12;
                  sub_40A360(v356, 0);
                  n5 = -1;
                  sub_40BF20(v439);
                }
                else
                {
                  sub_40A690(v351, ArgList_1 + 4896, n2_3);
                }
              }
              if ( *(_BYTE *)(v351 + 286) )
              {
                v357 = sub_421620(v439, (void *)(ArgList_1 + 4896), "_Overlay");
                n5 = 13;
                HIDWORD(v414) = sub_40BCB0(v357);
                n5 = -1;
                sub_40BF20(v439);
                if ( HIDWORD(v414) )
                {
                  v358 = (void *)(ArgList_1 + 4896);
                  if ( *(_BYTE *)(HIDWORD(v414) + 52) )
                  {
                    v359 = sub_421620(v435, v358, "_Overlay");
                    n5 = 14;
                    sub_40A920(v359, 0);
                    v360 = v435;
                  }
                  else
                  {
                    ArgList_5 = sub_421620(v429, v358, "_Overlay");
                    n5 = 15;
                    sub_40ABB0((_DWORD *)v351, ArgList_5, n2_3);
                    v360 = v429;
                  }
                  n5 = -1;
                  sub_40BF20(v360);
                }
              }
            }
          }
          ArgList_6 = (_BYTE *)ArgList_2;
        }
        ArgList_6 += 16;
        v36 = n7-- == 1;
        ArgList_2 = (int)ArgList_6;
      }
      while ( !v36 );
    }
  }
LABEL_639:
  LOBYTE(i) = sub_7AB110(ArgList_1);
  if ( (_BYTE)i )
  {
    v365 = (Concurrency::details::_ContextCallback *)sub_428B50(n60);
    LOBYTE(i) = Concurrency::details::_ContextCallback::_HasCapturedContext(v365);
    if ( !(_BYTE)i )
    {
      if ( v418 )
      {
        LOBYTE(i) = sub_7706E0(368, 0);
        if ( (_BYTE)i )
        {
          v366 = *(_DWORD *)(ArgList_1 + 6312);
          if ( v366 > 0 )
            *(_DWORD *)(ArgList_1 + 6312) = v366 - 1;
          if ( (unsigned __int8)sub_5B1780(10) )
          {
            if ( !LOBYTE(v421)
              || *(_DWORD *)(ArgList_1 + 6316) == -1
              || sub_67F260(&v413) == *(_DWORD *)(ArgList_1 + 6316) )
            {
              if ( sub_505CA0(ArgList_1) > 0 )
                *(_DWORD *)(ArgList_1 + 6312) = 10;
              if ( LOBYTE(v421) )
                *(_DWORD *)(ArgList_1 + 6316) = sub_67F260(&v413);
            }
            else
            {
              *(_DWORD *)(ArgList_1 + 6312) = -1;
              *(_DWORD *)(ArgList_1 + 6316) = sub_67F260(&v413);
            }
          }
          else
          {
            *(_DWORD *)(ArgList_1 + 6312) = 0;
          }
          v367 = sub_67F260(ArgList_1 + 5680);
          LOBYTE(i) = sub_67F260(&v413) != v367 && sub_A10630((float *)(ArgList_1 + 5680), (float *)&a5);
          v368 = *(_DWORD *)(ArgList_1 + 6312);
          if ( v368 >= 0 && (v368 || LOBYTE(v421) && !(_BYTE)i) )
          {
            LOBYTE(i) = sub_417430(0x4000, 0);
            if ( !(_BYTE)i )
            {
              i = *(_DWORD *)(ArgList_1 + 6308);
              if ( i < 180 )
              {
                *(_DWORD *)(ArgList_1 + 5492) |= 2u;
                *(_DWORD *)(ArgList_1 + 6308) = i + 1;
                LOBYTE(i) = sub_763570((_BYTE *)ArgList_1);
              }
            }
          }
          else
          {
            v369 = *(_DWORD *)(ArgList_1 + 1048) <= 0;
            if ( *(_DWORD *)(ArgList_1 + 1048) )
              goto LABEL_673;
            *(_DWORD *)(ArgList_1 + 5492) |= 2u;
            *(_DWORD *)(ArgList_1 + 6308) = 0;
            *(_DWORD *)(ArgList_1 + 6316) = -1;
            sub_763570((_BYTE *)ArgList_1);
            LOBYTE(i) = sub_417430(0x4000, 0);
            if ( (_BYTE)i )
            {
              if ( v422 )
              {
                LOBYTE(i) = sub_7AB110(ArgList_1);
                if ( (_BYTE)i )
                {
                  v370 = (Concurrency::details::_ContextCallback *)sub_428B50(n60);
                  LOBYTE(i) = Concurrency::details::_ContextCallback::_HasCapturedContext(v370);
                  if ( !(_BYTE)i )
                  {
                    sub_417400(0x4000, 0);
                    sub_7826D0((_DWORD *)ArgList_1, &v413, LODWORD(v421));
                    LOBYTE(i) = sub_4173E0(0x4000, 0);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  v369 = *(_DWORD *)(ArgList_1 + 1048) <= 0;
LABEL_673:
  if ( v369 )
  {
    unknown_libname_8(&v413);
    v36 = !v404;
    *(_BYTE *)(ArgList_1 + 5688) = v403;
    if ( v36 )
    {
      LOBYTE(i) = sub_498FC0(ArgList_1 + 5736);
    }
    else
    {
      unknown_libname_8(&v413);
      sub_7DC7B0(&v413);
      for ( i = sub_40C2E0(ArgList_1 + 5736); (unsigned int)i > 3; i = sub_40C2E0(ArgList_1 + 5736) )
        sub_499560(ArgList_1 + 5736);
    }
  }
  return i;
}
