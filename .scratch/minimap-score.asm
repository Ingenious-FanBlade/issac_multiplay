
F:/projects_isaac/Isaac-for-test/host/The Binding of Isaac Rebirth/Repentogon/isaac-ng.exe:	file format coff-i386

Disassembly of section .text:

00408030 <epoxy_handle_external_wglMakeCurrent>:
  966680: 53                           	push	ebx
  966681: 8b dc                        	mov	ebx, esp
  966683: 83 ec 08                     	sub	esp, 0x8
  966686: 83 e4 f8                     	and	esp, -0x8
  966689: 83 c4 04                     	add	esp, 0x4
  96668c: 55                           	push	ebp
  96668d: 8b 6b 04                     	mov	ebp, dword ptr [ebx + 0x4]
  966690: 89 6c 24 04                  	mov	dword ptr [esp + 0x4], ebp
  966694: 8b ec                        	mov	ebp, esp
  966696: 6a ff                        	push	-0x1
  966698: 68 a1 19 aa 00               	push	0xaa19a1
  96669d: 64 a1 00 00 00 00            	mov	eax, dword ptr fs:[0x0]
  9666a3: 50                           	push	eax
  9666a4: 53                           	push	ebx
  9666a5: 81 ec 60 01 00 00            	sub	esp, 0x160
  9666ab: a1 b4 83 b8 00               	mov	eax, dword ptr [0xb883b4]
  9666b0: 33 c5                        	xor	eax, ebp
  9666b2: 89 45 ec                     	mov	dword ptr [ebp - 0x14], eax
  9666b5: 56                           	push	esi
  9666b6: 57                           	push	edi
  9666b7: 50                           	push	eax
  9666b8: 8d 45 f4                     	lea	eax, [ebp - 0xc]
  9666bb: 64 a3 00 00 00 00            	mov	dword ptr fs:[0x0], eax
  9666c1: 89 8d 3c ff ff ff            	mov	dword ptr [ebp - 0xc4], ecx
  9666c7: 8b 3d f4 49 be 00            	mov	edi, dword ptr [0xbe49f4]
  9666cd: 8b 87 00 83 01 00            	mov	eax, dword ptr [edi + 0x18300]
  9666d3: 8b 40 04                     	mov	eax, dword ptr [eax + 0x4]
  9666d6: 8b 40 10                     	mov	eax, dword ptr [eax + 0x10]
  9666d9: 85 c0                        	test	eax, eax
  9666db: 74 0f                        	je	0x9666ec <epoxy_handle_external_wglMakeCurrent+0x55e6bc>
  9666dd: 83 78 08 10                  	cmp	dword ptr [eax + 0x8], 0x10
  9666e1: 75 09                        	jne	0x9666ec <epoxy_handle_external_wglMakeCurrent+0x55e6bc>
  9666e3: 83 38 23                     	cmp	dword ptr [eax], 0x23
  9666e6: 0f 84 87 13 00 00            	je	0x967a73 <epoxy_handle_external_wglMakeCurrent+0x55fa43>
  9666ec: a1 1c 4a be 00               	mov	eax, dword ptr [0xbe4a1c]
  9666f1: 83 b8 1c a6 04 00 00         	cmp	dword ptr [eax + 0x4a61c], 0x0
  9666f8: 8d 88 54 aa 04 00            	lea	ecx, [eax + 0x4aa54]
  9666fe: 89 8d 38 ff ff ff            	mov	dword ptr [ebp - 0xc8], ecx
  966704: 75 0c                        	jne	0x966712 <epoxy_handle_external_wglMakeCurrent+0x55e6e2>
  966706: 8d 88 08 aa 04 00            	lea	ecx, [eax + 0x4aa08]
  96670c: 89 8d 38 ff ff ff            	mov	dword ptr [ebp - 0xc8], ecx
  966712: 80 bf 90 2e 02 00 00         	cmp	byte ptr [edi + 0x22e90], 0x0
  966719: f3 0f 10 05 fc 02 b4 00      	movss	xmm0, dword ptr [0xb402fc]
  966721: f3 0f 5c 80 c4 a1 02 00      	subss	xmm0, dword ptr [eax + 0x2a1c4]
  966729: 0f 28 d0                     	movaps	xmm2, xmm0
  96672c: f3 0f 11 85 c8 fe ff ff      	movss	dword ptr [ebp - 0x138], xmm0
  966734: f3 0f 59 15 e8 06 b4 00      	mulss	xmm2, dword ptr [0xb406e8]
  96673c: 0f 28 c8                     	movaps	xmm1, xmm0
  96673f: f3 0f 59 0d f4 0b b4 00      	mulss	xmm1, dword ptr [0xb40bf4]
  966747: f3 0f 59 05 1c 0c b4 00      	mulss	xmm0, dword ptr [0xb40c1c]
  96674f: f3 0f 11 95 1c ff ff ff      	movss	dword ptr [ebp - 0xe4], xmm2
  966757: f3 0f 11 8d 44 ff ff ff      	movss	dword ptr [ebp - 0xbc], xmm1
  96675f: f3 0f 11 85 30 ff ff ff      	movss	dword ptr [ebp - 0xd0], xmm0
  966767: 0f 84 06 13 00 00            	je	0x967a73 <epoxy_handle_external_wglMakeCurrent+0x55fa43>
  96676d: 8b b7 d8 bb 01 00            	mov	esi, dword ptr [edi + 0x1bbd8]
  966773: 8d 85 28 ff ff ff            	lea	eax, [ebp - 0xd8]
  966779: 50                           	push	eax
  96677a: 8d 85 e0 fe ff ff            	lea	eax, [ebp - 0x120]
  966780: c7 85 28 ff ff ff 0a 00 00 00	mov	dword ptr [ebp - 0xd8], 0xa
  96678a: 8d 8f d8 bb 01 00            	lea	ecx, [edi + 0x1bbd8]
  966790: 50                           	push	eax
  966791: e8 1a d9 ab ff               	call	0x4240b0 <epoxy_handle_external_wglMakeCurrent+0x1c080>
  966796: 8b 85 e8 fe ff ff            	mov	eax, dword ptr [ebp - 0x118]
  96679c: 80 78 0d 00                  	cmp	byte ptr [eax + 0xd], 0x0
  9667a0: 75 0e                        	jne	0x9667b0 <epoxy_handle_external_wglMakeCurrent+0x55e780>
  9667a2: 83 78 10 0a                  	cmp	dword ptr [eax + 0x10], 0xa
  9667a6: 7f 08                        	jg	0x9667b0 <epoxy_handle_external_wglMakeCurrent+0x55e780>
  9667a8: 3b c6                        	cmp	eax, esi
  9667aa: 0f 85 c3 12 00 00            	jne	0x967a73 <epoxy_handle_external_wglMakeCurrent+0x55fa43>
  9667b0: f3 0f 10 87 84 5a 02 00      	movss	xmm0, dword ptr [edi + 0x25a84]
  9667b8: 8b b7 c8 5a 02 00            	mov	esi, dword ptr [edi + 0x25ac8]
  9667be: 0b 77 0c                     	or	esi, dword ptr [edi + 0xc]
  9667c1: f3 0f 11 85 48 ff ff ff      	movss	dword ptr [ebp - 0xb8], xmm0
  9667c9: f3 0f 10 87 88 5a 02 00      	movss	xmm0, dword ptr [edi + 0x25a88]
  9667d1: f3 0f 11 85 34 ff ff ff      	movss	dword ptr [ebp - 0xcc], xmm0
  9667d9: e8 e2 ed d8 ff               	call	0x6f55c0 <epoxy_handle_external_wglMakeCurrent+0x2ed590>
  9667de: 8b f8                        	mov	edi, eax
  9667e0: 0b fe                        	or	edi, esi
  9667e2: e8 69 ef d8 ff               	call	0x6f5750 <epoxy_handle_external_wglMakeCurrent+0x2ed720>
  9667e7: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  9667ed: f7 d0                        	not	eax
  9667ef: 23 c7                        	and	eax, edi
  9667f1: 81 c1 3c b8 01 00            	add	ecx, 0x1b83c
  9667f7: c1 e8 02                     	shr	eax, 0x2
  9667fa: 24 01                        	and	al, 0x1
  9667fc: 89 85 58 ff ff ff            	mov	dword ptr [ebp - 0xa8], eax
  966802: e8 49 0b ec ff               	call	0x827350 <epoxy_handle_external_wglMakeCurrent+0x41f320>
  966807: 8b bd 3c ff ff ff            	mov	edi, dword ptr [ebp - 0xc4]
  96680d: 0f 28 e0                     	movaps	xmm4, xmm0
  966810: f3 0f 10 35 ac 06 b4 00      	movss	xmm6, dword ptr [0xb406ac]
  966818: 0f 57 db                     	xorps	xmm3, xmm3
  96681b: f3 0f 11 85 10 ff ff ff      	movss	dword ptr [ebp - 0xf0], xmm0
  966823: f3 0f 11 b5 28 ff ff ff      	movss	dword ptr [ebp - 0xd8], xmm6
  96682b: f3 0f 59 a7 70 01 00 00      	mulss	xmm4, dword ptr [edi + 0x170]
  966833: 0f 2f e3                     	comiss	xmm4, xmm3
  966836: f3 0f 11 a5 60 ff ff ff      	movss	dword ptr [ebp - 0xa0], xmm4
  96683e: 0f 86 04 06 00 00            	jbe	0x966e48 <epoxy_handle_external_wglMakeCurrent+0x55ee18>
  966844: 0f 57 c0                     	xorps	xmm0, xmm0
  966847: c7 45 bc 00 00 80 3f         	mov	dword ptr [ebp - 0x44], 0x3f800000
  96684e: 0f 11 45 cc                  	movups	xmmword ptr [ebp - 0x34], xmm0
  966852: 8d 4f 18                     	lea	ecx, [edi + 0x18]
  966855: c7 45 c0 00 00 80 3f         	mov	dword ptr [ebp - 0x40], 0x3f800000
  96685c: f3 0f 10 05 2c 6f be 00      	movss	xmm0, dword ptr [0xbe6f2c]
  966864: f3 0f 5c 05 d4 07 b4 00      	subss	xmm0, dword ptr [0xb407d4]
  96686c: 6a 00                        	push	0x0
  96686e: 68 30 e3 b2 00               	push	0xb2e330
  966873: c7 45 c4 00 00 80 3f         	mov	dword ptr [ebp - 0x3c], 0x3f800000
  96687a: f3 0f 11 65 c8               	movss	dword ptr [ebp - 0x38], xmm4
  96687f: f3 0f 58 87 20 06 00 00      	addss	xmm0, dword ptr [edi + 0x620]
  966887: c7 45 dc 00 00 00 00         	mov	dword ptr [ebp - 0x24], 0x0
  96688e: c7 45 e0 00 00 00 00         	mov	dword ptr [ebp - 0x20], 0x0
  966895: c7 45 e4 00 00 00 00         	mov	dword ptr [ebp - 0x1c], 0x0
  96689c: c7 85 c4 fe ff ff 00 00 00 00	mov	dword ptr [ebp - 0x13c], 0x0
  9668a6: f3 0f 58 c2                  	addss	xmm0, xmm2
  9668aa: c7 85 b4 fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x14c], 0x3f800000
  9668b4: c7 85 b8 fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x148], 0x3f800000
  9668be: c7 85 bc fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x144], 0x3f800000
  9668c8: f3 0f 11 a5 c0 fe ff ff      	movss	dword ptr [ebp - 0x140], xmm4
  9668d0: f3 0f 11 85 14 ff ff ff      	movss	dword ptr [ebp - 0xec], xmm0
  9668d8: f3 0f 10 87 24 06 00 00      	movss	xmm0, dword ptr [edi + 0x624]
  9668e0: f3 0f 58 c6                  	addss	xmm0, xmm6
  9668e4: f3 0f 58 c1                  	addss	xmm0, xmm1
  9668e8: f3 0f 11 85 18 ff ff ff      	movss	dword ptr [ebp - 0xe8], xmm0
  9668f0: e8 6b f7 a9 ff               	call	0x406060 <.text+0x5060>
  9668f5: 8d 45 bc                     	lea	eax, [ebp - 0x44]
  9668f8: 50                           	push	eax
  9668f9: 8d 4f 18                     	lea	ecx, [edi + 0x18]
  9668fc: e8 af c0 a9 ff               	call	0x4029b0 <.text+0x19b0>
  966901: 68 00 b9 be 00               	push	0xbeb900
  966906: 68 00 b9 be 00               	push	0xbeb900
  96690b: 8d 85 14 ff ff ff            	lea	eax, [ebp - 0xec]
  966911: 50                           	push	eax
  966912: 8d 4f 18                     	lea	ecx, [edi + 0x18]
  966915: e8 c6 f0 a9 ff               	call	0x4059e0 <.text+0x49e0>
  96691a: 80 bd 58 ff ff ff 00         	cmp	byte ptr [ebp - 0xa8], 0x0
  966921: 0f 84 9e 00 00 00            	je	0x9669c5 <epoxy_handle_external_wglMakeCurrent+0x55e995>
  966927: 6a ff                        	push	-0x1
  966929: 8d 8d fc fe ff ff            	lea	ecx, [ebp - 0x104]
  96692f: e8 7c cc 07 00               	call	0x9e35b0 <epoxy_handle_external_wglMakeCurrent+0x5db580>
  966934: f3 0f 10 85 14 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xec]
  96693c: 8d b7 14 06 00 00            	lea	esi, [edi + 0x614]
  966942: f3 0f 58 05 38 06 b4 00      	addss	xmm0, dword ptr [0xb40638]
  96694a: f3 0f 10 8d 18 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xe8]
  966952: 8d 95 fc fe ff ff            	lea	edx, [ebp - 0x104]
  966958: f3 0f 58 0d 38 06 b4 00      	addss	xmm1, dword ptr [0xb40638]
  966960: f3 0f 10 95 48 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xb8]
  966968: 8b 0e                        	mov	ecx, dword ptr [esi]
  96696a: f3 0f 58 d0                  	addss	xmm2, xmm0
  96696e: f3 0f 10 85 34 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xcc]
  966976: 52                           	push	edx
  966977: f3 0f 58 c1                  	addss	xmm0, xmm1
  96697b: 8d 95 e4 fe ff ff            	lea	edx, [ebp - 0x11c]
  966981: 52                           	push	edx
  966982: f3 0f 11 95 e4 fe ff ff      	movss	dword ptr [ebp - 0x11c], xmm2
  96698a: f3 0f 11 85 e8 fe ff ff      	movss	dword ptr [ebp - 0x118], xmm0
  966992: 8b 01                        	mov	eax, dword ptr [ecx]
  966994: 8b 40 24                     	mov	eax, dword ptr [eax + 0x24]
  966997: ff d0                        	call	eax
  966999: 89 85 50 ff ff ff            	mov	dword ptr [ebp - 0xb0], eax
  96699f: 85 c0                        	test	eax, eax
  9669a1: 0f 84 30 04 00 00            	je	0x966dd7 <epoxy_handle_external_wglMakeCurrent+0x55eda7>
  9669a7: 83 ec 08                     	sub	esp, 0x8
  9669aa: 8b cc                        	mov	ecx, esp
  9669ac: 56                           	push	esi
  9669ad: e8 fe 18 aa ff               	call	0x4082b0 <epoxy_handle_external_wglMakeCurrent+0x280>
  9669b2: ff b5 50 ff ff ff            	push	dword ptr [ebp - 0xb0]
  9669b8: 8d 4d bc                     	lea	ecx, [ebp - 0x44]
  9669bb: e8 f0 7d d1 ff               	call	0x67e7b0 <epoxy_handle_external_wglMakeCurrent+0x276780>
  9669c0: e9 12 04 00 00               	jmp	0x966dd7 <epoxy_handle_external_wglMakeCurrent+0x55eda7>
  9669c5: 8d 4d bc                     	lea	ecx, [ebp - 0x44]
  9669c8: e8 93 bd a9 ff               	call	0x402760 <.text+0x1760>
  9669cd: 50                           	push	eax
  9669ce: 8d 4f 18                     	lea	ecx, [edi + 0x18]
  9669d1: e8 da bf a9 ff               	call	0x4029b0 <.text+0x19b0>
  9669d6: 8b 87 4c 01 00 00            	mov	eax, dword ptr [edi + 0x14c]
  9669dc: 83 ec 08                     	sub	esp, 0x8
  9669df: f3 0f 10 8f 48 01 00 00      	movss	xmm1, dword ptr [edi + 0x148]
  9669e7: 48                           	dec	eax
  9669e8: 0f 28 c1                     	movaps	xmm0, xmm1
  9669eb: f3 0f 58 87 3c 01 00 00      	addss	xmm0, dword ptr [edi + 0x13c]
  9669f3: 66 0f 6e d0                  	movd	xmm2, eax
  9669f7: 8b 87 50 01 00 00            	mov	eax, dword ptr [edi + 0x150]
  9669fd: 0f 5b d2                     	cvtdq2ps	xmm2, xmm2
  966a00: 48                           	dec	eax
  966a01: f3 0f 59 d0                  	mulss	xmm2, xmm0
  966a05: 0f 28 c1                     	movaps	xmm0, xmm1
  966a08: f3 0f 58 87 40 01 00 00      	addss	xmm0, dword ptr [edi + 0x140]
  966a10: f3 0f 58 d1                  	addss	xmm2, xmm1
  966a14: f3 0f 11 95 4c ff ff ff      	movss	dword ptr [ebp - 0xb4], xmm2
  966a1c: 66 0f 6e d0                  	movd	xmm2, eax
  966a20: 0f 5b d2                     	cvtdq2ps	xmm2, xmm2
  966a23: f3 0f 59 d0                  	mulss	xmm2, xmm0
  966a27: f3 0f 10 87 5c 01 00 00      	movss	xmm0, dword ptr [edi + 0x15c]
  966a2f: f3 0f 11 85 50 ff ff ff      	movss	dword ptr [ebp - 0xb0], xmm0
  966a37: d9 85 50 ff ff ff            	fld	dword ptr [ebp - 0xb0]
  966a3d: f3 0f 58 d1                  	addss	xmm2, xmm1
  966a41: dd 1c 24                     	fstp	qword ptr [esp]
  966a44: f3 0f 11 95 40 ff ff ff      	movss	dword ptr [ebp - 0xc0], xmm2
  966a4c: e8 71 18 12 00               	call	0xa882c2 <epoxy_has_wgl_extension+0x2bbf2>
  966a51: f3 0f 10 85 40 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xc0]
  966a59: f3 0f 59 05 78 01 b4 00      	mulss	xmm0, dword ptr [0xb40178]
  966a61: d9 9d 2c ff ff ff            	fstp	dword ptr [ebp - 0xd4]
  966a67: f3 0f 11 85 54 ff ff ff      	movss	dword ptr [ebp - 0xac], xmm0
  966a6f: d9 85 54 ff ff ff            	fld	dword ptr [ebp - 0xac]
  966a75: dd 1c 24                     	fstp	qword ptr [esp]
  966a78: e8 45 18 12 00               	call	0xa882c2 <epoxy_has_wgl_extension+0x2bbf2>
  966a7d: f3 0f 10 87 58 01 00 00      	movss	xmm0, dword ptr [edi + 0x158]
  966a85: d9 9d 5c ff ff ff            	fstp	dword ptr [ebp - 0xa4]
  966a8b: f3 0f 11 85 50 ff ff ff      	movss	dword ptr [ebp - 0xb0], xmm0
  966a93: d9 85 50 ff ff ff            	fld	dword ptr [ebp - 0xb0]
  966a99: dd 1c 24                     	fstp	qword ptr [esp]
  966a9c: e8 21 18 12 00               	call	0xa882c2 <epoxy_has_wgl_extension+0x2bbf2>
  966aa1: f3 0f 10 85 4c ff ff ff      	movss	xmm0, dword ptr [ebp - 0xb4]
  966aa9: f3 0f 59 05 78 01 b4 00      	mulss	xmm0, dword ptr [0xb40178]
  966ab1: d9 9d 40 ff ff ff            	fstp	dword ptr [ebp - 0xc0]
  966ab7: f3 0f 11 85 64 ff ff ff      	movss	dword ptr [ebp - 0x9c], xmm0
  966abf: d9 85 64 ff ff ff            	fld	dword ptr [ebp - 0x9c]
  966ac5: dd 1c 24                     	fstp	qword ptr [esp]
  966ac8: e8 f5 17 12 00               	call	0xa882c2 <epoxy_has_wgl_extension+0x2bbf2>
  966acd: f3 0f 10 85 40 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xc0]
  966ad5: d9 9d 50 ff ff ff            	fstp	dword ptr [ebp - 0xb0]
  966adb: f3 0f 5c 85 50 ff ff ff      	subss	xmm0, dword ptr [ebp - 0xb0]
  966ae3: d9 85 54 ff ff ff            	fld	dword ptr [ebp - 0xac]
  966ae9: dd 1c 24                     	fstp	qword ptr [esp]
  966aec: f3 0f 11 85 50 ff ff ff      	movss	dword ptr [ebp - 0xb0], xmm0
  966af4: f3 0f 11 85 e4 fe ff ff      	movss	dword ptr [ebp - 0x11c], xmm0
  966afc: f3 0f 10 85 2c ff ff ff      	movss	xmm0, dword ptr [ebp - 0xd4]
  966b04: f3 0f 5c 85 5c ff ff ff      	subss	xmm0, dword ptr [ebp - 0xa4]
  966b0c: f3 0f 11 85 4c ff ff ff      	movss	dword ptr [ebp - 0xb4], xmm0
  966b14: f3 0f 11 85 e8 fe ff ff      	movss	dword ptr [ebp - 0x118], xmm0
  966b1c: e8 9b 17 12 00               	call	0xa882bc <epoxy_has_wgl_extension+0x2bbec>
  966b21: d9 9d 54 ff ff ff            	fstp	dword ptr [ebp - 0xac]
  966b27: d9 85 64 ff ff ff            	fld	dword ptr [ebp - 0x9c]
  966b2d: dd 1c 24                     	fstp	qword ptr [esp]
  966b30: e8 87 17 12 00               	call	0xa882bc <epoxy_has_wgl_extension+0x2bbec>
  966b35: 8b 8f 2c 01 00 00            	mov	ecx, dword ptr [edi + 0x12c]
  966b3b: 83 c4 08                     	add	esp, 0x8
  966b3e: d9 9d 5c ff ff ff            	fstp	dword ptr [ebp - 0xa4]
  966b44: f3 0f 10 85 5c ff ff ff      	movss	xmm0, dword ptr [ebp - 0xa4]
  966b4c: f3 0f 58 85 40 ff ff ff      	addss	xmm0, dword ptr [ebp - 0xc0]
  966b54: 8b 01                        	mov	eax, dword ptr [ecx]
  966b56: 8b 40 2c                     	mov	eax, dword ptr [eax + 0x2c]
  966b59: f3 0f 11 85 20 ff ff ff      	movss	dword ptr [ebp - 0xe0], xmm0
  966b61: f3 0f 10 85 54 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xac]
  966b69: f3 0f 58 85 2c ff ff ff      	addss	xmm0, dword ptr [ebp - 0xd4]
  966b71: f3 0f 11 85 5c ff ff ff      	movss	dword ptr [ebp - 0xa4], xmm0
  966b79: f3 0f 11 85 24 ff ff ff      	movss	dword ptr [ebp - 0xdc], xmm0
  966b81: ff d0                        	call	eax
  966b83: 66 0f 6e c0                  	movd	xmm0, eax
  966b87: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  966b8b: 8b 8f 2c 01 00 00            	mov	ecx, dword ptr [edi + 0x12c]
  966b91: c1 e8 1f                     	shr	eax, 0x1f
  966b94: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  966b9d: 8b 01                        	mov	eax, dword ptr [ecx]
  966b9f: 8b 40 28                     	mov	eax, dword ptr [eax + 0x28]
  966ba2: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  966ba6: f3 0f 11 85 2c ff ff ff      	movss	dword ptr [ebp - 0xd4], xmm0
  966bae: ff d0                        	call	eax
  966bb0: f3 0f 10 8d 2c ff ff ff      	movss	xmm1, dword ptr [ebp - 0xd4]
  966bb8: 8d b5 24 ff ff ff            	lea	esi, [ebp - 0xdc]
  966bbe: 8d 95 20 ff ff ff            	lea	edx, [ebp - 0xe0]
  966bc4: c7 85 54 ff ff ff 00 00 00 00	mov	dword ptr [ebp - 0xac], 0x0
  966bce: 8d 8d e8 fe ff ff            	lea	ecx, [ebp - 0x118]
  966bd4: c7 85 64 ff ff ff 00 00 00 00	mov	dword ptr [ebp - 0x9c], 0x0
  966bde: 66 0f 6e c0                  	movd	xmm0, eax
  966be2: 8d bd 64 ff ff ff            	lea	edi, [ebp - 0x9c]
  966be8: c1 e8 1f                     	shr	eax, 0x1f
  966beb: 0f 2f 8d 5c ff ff ff         	comiss	xmm1, dword ptr [ebp - 0xa4]
  966bf2: 6a 00                        	push	0x0
  966bf4: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  966bf8: f3 0f 10 8d 4c ff ff ff      	movss	xmm1, dword ptr [ebp - 0xb4]
  966c00: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  966c09: 8d 85 2c ff ff ff            	lea	eax, [ebp - 0xd4]
  966c0f: 0f 46 f0                     	cmovbe	esi, eax
  966c12: 8d 85 40 ff ff ff            	lea	eax, [ebp - 0xc0]
  966c18: 56                           	push	esi
  966c19: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  966c1d: 0f 2f 85 20 ff ff ff         	comiss	xmm0, dword ptr [ebp - 0xe0]
  966c24: f3 0f 11 85 40 ff ff ff      	movss	dword ptr [ebp - 0xc0], xmm0
  966c2c: 0f 57 c0                     	xorps	xmm0, xmm0
  966c2f: 0f 46 d0                     	cmovbe	edx, eax
  966c32: 8d 85 54 ff ff ff            	lea	eax, [ebp - 0xac]
  966c38: 0f 2f c8                     	comiss	xmm1, xmm0
  966c3b: 52                           	push	edx
  966c3c: f3 0f 10 8d 50 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xb0]
  966c44: 0f 46 c8                     	cmovbe	ecx, eax
  966c47: 8d 85 e4 fe ff ff            	lea	eax, [ebp - 0x11c]
  966c4d: 0f 2f c8                     	comiss	xmm1, xmm0
  966c50: 51                           	push	ecx
  966c51: 8d 8d ec fe ff ff            	lea	ecx, [ebp - 0x114]
  966c57: 0f 46 c7                     	cmovbe	eax, edi
  966c5a: 50                           	push	eax
  966c5b: e8 40 a8 07 00               	call	0x9e14a0 <epoxy_handle_external_wglMakeCurrent+0x5d9470>
  966c60: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  966c66: 8b 89 00 83 01 00            	mov	ecx, dword ptr [ecx + 0x18300]
  966c6c: e8 9f 5e eb ff               	call	0x81cb10 <epoxy_handle_external_wglMakeCurrent+0x414ae0>
  966c71: 8b bd 3c ff ff ff            	mov	edi, dword ptr [ebp - 0xc4]
  966c77: 84 c0                        	test	al, al
  966c79: 0f 84 b5 00 00 00            	je	0x966d34 <epoxy_handle_external_wglMakeCurrent+0x55ed04>
  966c7f: f3 0f 10 85 04 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xfc]
  966c87: 8d 85 08 ff ff ff            	lea	eax, [ebp - 0xf8]
  966c8d: f3 0f 10 ad 20 ff ff ff      	movss	xmm5, dword ptr [ebp - 0xe0]
  966c95: 8d 4d c4                     	lea	ecx, [ebp - 0x3c]
  966c98: f3 0f 5c ad ec fe ff ff      	subss	xmm5, dword ptr [ebp - 0x114]
  966ca0: f3 0f 5c 85 ec fe ff ff      	subss	xmm0, dword ptr [ebp - 0x114]
  966ca8: f3 0f 10 a5 f0 fe ff ff      	movss	xmm4, dword ptr [ebp - 0x110]
  966cb0: f3 0f 5c a5 4c ff ff ff      	subss	xmm4, dword ptr [ebp - 0xb4]
  966cb8: 6a 00                        	push	0x0
  966cba: 50                           	push	eax
  966cbb: 8d 85 ec fe ff ff            	lea	eax, [ebp - 0x114]
  966cc1: 50                           	push	eax
  966cc2: 8d 85 f0 fe ff ff            	lea	eax, [ebp - 0x110]
  966cc8: 50                           	push	eax
  966cc9: 8d 85 04 ff ff ff            	lea	eax, [ebp - 0xfc]
  966ccf: 50                           	push	eax
  966cd0: f3 0f 5c e8                  	subss	xmm5, xmm0
  966cd4: e8 c7 a7 07 00               	call	0x9e14a0 <epoxy_handle_external_wglMakeCurrent+0x5d9470>
  966cd9: f3 0f 10 8f 44 01 00 00      	movss	xmm1, dword ptr [edi + 0x144]
  966ce1: f3 0f 10 95 14 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xec]
  966ce9: f3 0f 10 9d 18 ff ff ff      	movss	xmm3, dword ptr [ebp - 0xe8]
  966cf1: 0f 10 00                     	movups	xmm0, xmmword ptr [eax]
  966cf4: 0f 11 85 ec fe ff ff         	movups	xmmword ptr [ebp - 0x114], xmm0
  966cfb: 0f 10 40 10                  	movups	xmm0, xmmword ptr [eax + 0x10]
  966cff: 0f 11 85 fc fe ff ff         	movups	xmmword ptr [ebp - 0x104], xmm0
  966d06: 8b 40 20                     	mov	eax, dword ptr [eax + 0x20]
  966d09: 0f 28 c1                     	movaps	xmm0, xmm1
  966d0c: 89 85 0c ff ff ff            	mov	dword ptr [ebp - 0xf4], eax
  966d12: f3 0f 59 05 b8 e2 ba 00      	mulss	xmm0, dword ptr [0xbae2b8]
  966d1a: f3 0f 59 0d bc e2 ba 00      	mulss	xmm1, dword ptr [0xbae2bc]
  966d22: f3 0f 58 d0                  	addss	xmm2, xmm0
  966d26: f3 0f 58 d9                  	addss	xmm3, xmm1
  966d2a: f3 0f 58 d5                  	addss	xmm2, xmm5
  966d2e: f3 0f 58 dc                  	addss	xmm3, xmm4
  966d32: eb 5b                        	jmp	0x966d8f <epoxy_handle_external_wglMakeCurrent+0x55ed5f>
  966d34: f3 0f 10 8f 44 01 00 00      	movss	xmm1, dword ptr [edi + 0x144]
  966d3c: f3 0f 10 a5 ec fe ff ff      	movss	xmm4, dword ptr [ebp - 0x114]
  966d44: 0f 28 c1                     	movaps	xmm0, xmm1
  966d47: f3 0f 59 05 b8 e2 ba 00      	mulss	xmm0, dword ptr [0xbae2b8]
  966d4f: f3 0f 59 0d bc e2 ba 00      	mulss	xmm1, dword ptr [0xbae2bc]
  966d57: f3 0f 10 ad f0 fe ff ff      	movss	xmm5, dword ptr [ebp - 0x110]
  966d5f: f3 0f 10 95 14 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xec]
  966d67: f3 0f 10 9d 18 ff ff ff      	movss	xmm3, dword ptr [ebp - 0xe8]
  966d6f: f3 0f 58 d0                  	addss	xmm2, xmm0
  966d73: f3 0f 5c a5 50 ff ff ff      	subss	xmm4, dword ptr [ebp - 0xb0]
  966d7b: f3 0f 5c ad 4c ff ff ff      	subss	xmm5, dword ptr [ebp - 0xb4]
  966d83: f3 0f 58 d9                  	addss	xmm3, xmm1
  966d87: f3 0f 58 d4                  	addss	xmm2, xmm4
  966d8b: f3 0f 58 dd                  	addss	xmm3, xmm5
  966d8f: f3 0f 10 8d 48 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xb8]
  966d97: 8d 95 b4 fe ff ff            	lea	edx, [ebp - 0x14c]
  966d9d: f3 0f 10 85 34 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xcc]
  966da5: f3 0f 58 ca                  	addss	xmm1, xmm2
  966da9: 8b 8f 2c 01 00 00            	mov	ecx, dword ptr [edi + 0x12c]
  966daf: f3 0f 58 c3                  	addss	xmm0, xmm3
  966db3: 52                           	push	edx
  966db4: 8d 95 e4 fe ff ff            	lea	edx, [ebp - 0x11c]
  966dba: 52                           	push	edx
  966dbb: f3 0f 11 8d e4 fe ff ff      	movss	dword ptr [ebp - 0x11c], xmm1
  966dc3: 8d 95 ec fe ff ff            	lea	edx, [ebp - 0x114]
  966dc9: f3 0f 11 85 e8 fe ff ff      	movss	dword ptr [ebp - 0x118], xmm0
  966dd1: 8b 01                        	mov	eax, dword ptr [ecx]
  966dd3: 52                           	push	edx
  966dd4: ff 50 14                     	call	dword ptr [eax + 0x14]
  966dd7: f3 0f 10 8d 14 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xec]
  966ddf: 8d 85 e4 fe ff ff            	lea	eax, [ebp - 0x11c]
  966de5: f3 0f 10 85 18 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xe8]
  966ded: 8b cf                        	mov	ecx, edi
  966def: f3 0f 58 0d 6c 07 b4 00      	addss	xmm1, dword ptr [0xb4076c]
  966df7: f3 0f 58 05 50 07 b4 00      	addss	xmm0, dword ptr [0xb40750]
  966dff: f3 0f 10 95 60 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xa0]
  966e07: 50                           	push	eax
  966e08: f3 0f 5c 0d 38 06 b4 00      	subss	xmm1, dword ptr [0xb40638]
  966e10: f3 0f 58 05 38 06 b4 00      	addss	xmm0, dword ptr [0xb40638]
  966e18: f3 0f 11 8d e4 fe ff ff      	movss	dword ptr [ebp - 0x11c], xmm1
  966e20: f3 0f 11 85 e8 fe ff ff      	movss	dword ptr [ebp - 0x118], xmm0
  966e28: e8 d3 f4 ff ff               	call	0x966300 <epoxy_handle_external_wglMakeCurrent+0x55e2d0>
  966e2d: f3 0f 10 95 1c ff ff ff      	movss	xmm2, dword ptr [ebp - 0xe4]
  966e35: 0f 57 db                     	xorps	xmm3, xmm3
  966e38: f3 0f 10 8d 44 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xbc]
  966e40: f3 0f 10 85 10 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xf0]
  966e48: f3 0f 59 87 d0 02 00 00      	mulss	xmm0, dword ptr [edi + 0x2d0]
  966e50: 0f 2f c3                     	comiss	xmm0, xmm3
  966e53: f3 0f 11 85 54 ff ff ff      	movss	dword ptr [ebp - 0xac], xmm0
  966e5b: f3 0f 11 85 24 ff ff ff      	movss	dword ptr [ebp - 0xdc], xmm0
  966e63: 0f 86 1c 05 00 00            	jbe	0x967385 <epoxy_handle_external_wglMakeCurrent+0x55f355>
  966e69: 80 bd 58 ff ff ff 00         	cmp	byte ptr [ebp - 0xa8], 0x0
  966e70: f3 0f 58 cb                  	addss	xmm1, xmm3
  966e74: f3 0f 10 9f c4 02 00 00      	movss	xmm3, dword ptr [edi + 0x2c4]
  966e7c: f3 0f 58 15 2c 6f be 00      	addss	xmm2, dword ptr [0xbe6f2c]
  966e84: c7 85 0c ff ff ff 00 00 00 00	mov	dword ptr [ebp - 0xf4], 0x0
  966e8e: c7 85 fc fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x104], 0x3f800000
  966e98: f3 0f 11 8d 44 ff ff ff      	movss	dword ptr [ebp - 0xbc], xmm1
  966ea0: f3 0f 10 8f c8 02 00 00      	movss	xmm1, dword ptr [edi + 0x2c8]
  966ea8: f3 0f 11 8d 5c ff ff ff      	movss	dword ptr [ebp - 0xa4], xmm1
  966eb0: f3 0f 10 8f cc 02 00 00      	movss	xmm1, dword ptr [edi + 0x2cc]
  966eb8: f3 0f 5c cb                  	subss	xmm1, xmm3
  966ebc: c7 85 00 ff ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x100], 0x3f800000
  966ec6: c7 85 04 ff ff ff 00 00 80 3f	mov	dword ptr [ebp - 0xfc], 0x3f800000
  966ed0: f3 0f 11 85 08 ff ff ff      	movss	dword ptr [ebp - 0xf8], xmm0
  966ed8: f3 0f 11 95 1c ff ff ff      	movss	dword ptr [ebp - 0xe4], xmm2
  966ee0: f3 0f 11 9d 60 ff ff ff      	movss	dword ptr [ebp - 0xa0], xmm3
  966ee8: f3 0f 11 8d 50 ff ff ff      	movss	dword ptr [ebp - 0xb0], xmm1
  966ef0: 0f 85 43 04 00 00            	jne	0x967339 <epoxy_handle_external_wglMakeCurrent+0x55f309>
  966ef6: 8b b7 8c 02 00 00            	mov	esi, dword ptr [edi + 0x28c]
  966efc: 8b ce                        	mov	ecx, esi
  966efe: 8b 06                        	mov	eax, dword ptr [esi]
  966f00: 8b 40 28                     	mov	eax, dword ptr [eax + 0x28]
  966f03: ff d0                        	call	eax
  966f05: 8b ce                        	mov	ecx, esi
  966f07: 66 0f 6e c0                  	movd	xmm0, eax
  966f0b: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  966f0f: c1 e8 1f                     	shr	eax, 0x1f
  966f12: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  966f1b: 8b 06                        	mov	eax, dword ptr [esi]
  966f1d: 8b 40 28                     	mov	eax, dword ptr [eax + 0x28]
  966f20: 66 0f 5a d0                  	cvtpd2ps	xmm2, xmm0
  966f24: f3 0f 11 95 4c ff ff ff      	movss	dword ptr [ebp - 0xb4], xmm2
  966f2c: ff d0                        	call	eax
  966f2e: f3 0f 10 8d 4c ff ff ff      	movss	xmm1, dword ptr [ebp - 0xb4]
  966f36: 8b b7 8c 02 00 00            	mov	esi, dword ptr [edi + 0x28c]
  966f3c: 8b ce                        	mov	ecx, esi
  966f3e: 66 0f 6e c0                  	movd	xmm0, eax
  966f42: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  966f46: c1 e8 1f                     	shr	eax, 0x1f
  966f49: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  966f52: 8b 06                        	mov	eax, dword ptr [esi]
  966f54: 8b 40 2c                     	mov	eax, dword ptr [eax + 0x2c]
  966f57: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  966f5b: f3 0f 59 47 0c               	mulss	xmm0, dword ptr [edi + 0xc]
  966f60: f3 0f 5c c8                  	subss	xmm1, xmm0
  966f64: f3 0f 59 0d 78 01 b4 00      	mulss	xmm1, dword ptr [0xb40178]
  966f6c: f3 0f 11 8d 4c ff ff ff      	movss	dword ptr [ebp - 0xb4], xmm1
  966f74: ff d0                        	call	eax
  966f76: 8b ce                        	mov	ecx, esi
  966f78: 66 0f 6e c0                  	movd	xmm0, eax
  966f7c: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  966f80: c1 e8 1f                     	shr	eax, 0x1f
  966f83: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  966f8c: 8b 06                        	mov	eax, dword ptr [esi]
  966f8e: 8b 40 2c                     	mov	eax, dword ptr [eax + 0x2c]
  966f91: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  966f95: f3 0f 11 85 64 ff ff ff      	movss	dword ptr [ebp - 0x9c], xmm0
  966f9d: ff d0                        	call	eax
  966f9f: f3 0f 10 8d 64 ff ff ff      	movss	xmm1, dword ptr [ebp - 0x9c]
  966fa7: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  966fad: 66 0f 6e c0                  	movd	xmm0, eax
  966fb1: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  966fb5: c1 e8 1f                     	shr	eax, 0x1f
  966fb8: 8b 89 00 83 01 00            	mov	ecx, dword ptr [ecx + 0x18300]
  966fbe: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  966fc7: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  966fcb: f3 0f 59 47 0c               	mulss	xmm0, dword ptr [edi + 0xc]
  966fd0: f3 0f 5c c8                  	subss	xmm1, xmm0
  966fd4: f3 0f 10 85 44 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xbc]
  966fdc: f3 0f 5c 85 60 ff ff ff      	subss	xmm0, dword ptr [ebp - 0xa0]
  966fe4: f3 0f 59 0d 78 01 b4 00      	mulss	xmm1, dword ptr [0xb40178]
  966fec: f3 0f 11 85 58 ff ff ff      	movss	dword ptr [ebp - 0xa8], xmm0
  966ff4: f3 0f 11 8d 64 ff ff ff      	movss	dword ptr [ebp - 0x9c], xmm1
  966ffc: e8 0f 5b eb ff               	call	0x81cb10 <epoxy_handle_external_wglMakeCurrent+0x414ae0>
  967001: 8b 8f 8c 02 00 00            	mov	ecx, dword ptr [edi + 0x28c]
  967007: 84 c0                        	test	al, al
  967009: 8b 01                        	mov	eax, dword ptr [ecx]
  96700b: 0f 84 c8 01 00 00            	je	0x9671d9 <epoxy_handle_external_wglMakeCurrent+0x55f1a9>
  967011: 8b 40 28                     	mov	eax, dword ptr [eax + 0x28]
  967014: ff d0                        	call	eax
  967016: f3 0f 10 8d 1c ff ff ff      	movss	xmm1, dword ptr [ebp - 0xe4]
  96701e: 8d 4d c4                     	lea	ecx, [ebp - 0x3c]
  967021: f3 0f 10 95 58 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xa8]
  967029: f3 0f 58 15 98 06 b4 00      	addss	xmm2, dword ptr [0xb40698]
  967031: 66 0f 6e c0                  	movd	xmm0, eax
  967035: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  967039: c1 e8 1f                     	shr	eax, 0x1f
  96703c: 6a 01                        	push	0x1
  96703e: c7 85 60 ff ff ff 00 00 80 3f	mov	dword ptr [ebp - 0xa0], 0x3f800000
  967048: c7 85 5c ff ff ff 00 00 00 00	mov	dword ptr [ebp - 0xa4], 0x0
  967052: c7 85 3c ff ff ff 00 00 00 00	mov	dword ptr [ebp - 0xc4], 0x0
  96705c: f3 0f 58 57 14               	addss	xmm2, dword ptr [edi + 0x14]
  967061: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  96706a: 8d 85 60 ff ff ff            	lea	eax, [ebp - 0xa0]
  967070: c7 85 2c ff ff ff 00 00 80 3f	mov	dword ptr [ebp - 0xd4], 0x3f800000
  96707a: 50                           	push	eax
  96707b: 8d 85 5c ff ff ff            	lea	eax, [ebp - 0xa4]
  967081: f3 0f 58 97 24 06 00 00      	addss	xmm2, dword ptr [edi + 0x624]
  967089: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  96708d: 50                           	push	eax
  96708e: 8d 85 3c ff ff ff            	lea	eax, [ebp - 0xc4]
  967094: 50                           	push	eax
  967095: 8d 85 2c ff ff ff            	lea	eax, [ebp - 0xd4]
  96709b: 50                           	push	eax
  96709c: f3 0f 5c 87 c0 02 00 00      	subss	xmm0, dword ptr [edi + 0x2c0]
  9670a4: f3 0f 5c c8                  	subss	xmm1, xmm0
  9670a8: f3 0f 10 85 48 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xb8]
  9670b0: f3 0f 5c 0d e8 06 b4 00      	subss	xmm1, dword ptr [0xb406e8]
  9670b8: f3 0f 58 4f 10               	addss	xmm1, dword ptr [edi + 0x10]
  9670bd: f3 0f 58 8f 20 06 00 00      	addss	xmm1, dword ptr [edi + 0x620]
  9670c5: f3 0f 58 c1                  	addss	xmm0, xmm1
  9670c9: f3 0f 11 85 40 ff ff ff      	movss	dword ptr [ebp - 0xc0], xmm0
  9670d1: f3 0f 10 85 34 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xcc]
  9670d9: f3 0f 58 c2                  	addss	xmm0, xmm2
  9670dd: f3 0f 11 85 58 ff ff ff      	movss	dword ptr [ebp - 0xa8], xmm0
  9670e5: e8 b6 a3 07 00               	call	0x9e14a0 <epoxy_handle_external_wglMakeCurrent+0x5d9470>
  9670ea: 8b 8f 8c 02 00 00            	mov	ecx, dword ptr [edi + 0x28c]
  9670f0: 8b 01                        	mov	eax, dword ptr [ecx]
  9670f2: 8b 40 2c                     	mov	eax, dword ptr [eax + 0x2c]
  9670f5: ff d0                        	call	eax
  9670f7: 8b 8f 8c 02 00 00            	mov	ecx, dword ptr [edi + 0x28c]
  9670fd: 66 0f 6e c0                  	movd	xmm0, eax
  967101: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  967105: c1 e8 1f                     	shr	eax, 0x1f
  967108: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  967111: 8b 01                        	mov	eax, dword ptr [ecx]
  967113: 8b 40 28                     	mov	eax, dword ptr [eax + 0x28]
  967116: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  96711a: f3 0f 58 85 58 ff ff ff      	addss	xmm0, dword ptr [ebp - 0xa8]
  967122: f3 0f 5c 85 64 ff ff ff      	subss	xmm0, dword ptr [ebp - 0x9c]
  96712a: f3 0f 11 85 60 ff ff ff      	movss	dword ptr [ebp - 0xa0], xmm0
  967132: ff d0                        	call	eax
  967134: f3 0f 10 8d 40 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xc0]
  96713c: 8d 8d cc fe ff ff            	lea	ecx, [ebp - 0x134]
  967142: 66 0f 6e c0                  	movd	xmm0, eax
  967146: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  96714a: c1 e8 1f                     	shr	eax, 0x1f
  96714d: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  967156: 8d 85 60 ff ff ff            	lea	eax, [ebp - 0xa0]
  96715c: 50                           	push	eax
  96715d: 8d 85 5c ff ff ff            	lea	eax, [ebp - 0xa4]
  967163: 50                           	push	eax
  967164: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  967168: 8d 85 58 ff ff ff            	lea	eax, [ebp - 0xa8]
  96716e: 50                           	push	eax
  96716f: 8d 85 3c ff ff ff            	lea	eax, [ebp - 0xc4]
  967175: 50                           	push	eax
  967176: f3 0f 58 c1                  	addss	xmm0, xmm1
  96717a: f3 0f 58 8d 4c ff ff ff      	addss	xmm1, dword ptr [ebp - 0xb4]
  967182: f3 0f 5c 85 4c ff ff ff      	subss	xmm0, dword ptr [ebp - 0xb4]
  96718a: f3 0f 11 8d 3c ff ff ff      	movss	dword ptr [ebp - 0xc4], xmm1
  967192: f3 0f 11 85 5c ff ff ff      	movss	dword ptr [ebp - 0xa4], xmm0
  96719a: f3 0f 10 85 58 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xa8]
  9671a2: f3 0f 58 85 64 ff ff ff      	addss	xmm0, dword ptr [ebp - 0x9c]
  9671aa: f3 0f 11 85 58 ff ff ff      	movss	dword ptr [ebp - 0xa8], xmm0
  9671b2: e8 99 9e 07 00               	call	0x9e1050 <epoxy_handle_external_wglMakeCurrent+0x5d9020>
  9671b7: 8b 8f 8c 02 00 00            	mov	ecx, dword ptr [edi + 0x28c]
  9671bd: 8d 95 fc fe ff ff            	lea	edx, [ebp - 0x104]
  9671c3: 8b 01                        	mov	eax, dword ptr [ecx]
  9671c5: 52                           	push	edx
  9671c6: 8d 95 cc fe ff ff            	lea	edx, [ebp - 0x134]
  9671cc: 52                           	push	edx
  9671cd: 8d 55 c4                     	lea	edx, [ebp - 0x3c]
  9671d0: 52                           	push	edx
  9671d1: ff 50 0c                     	call	dword ptr [eax + 0xc]
  9671d4: e9 48 01 00 00               	jmp	0x967321 <epoxy_handle_external_wglMakeCurrent+0x55f2f1>
  9671d9: f3 0f 10 85 1c ff ff ff      	movss	xmm0, dword ptr [ebp - 0xe4]
  9671e1: f3 0f 5c 85 5c ff ff ff      	subss	xmm0, dword ptr [ebp - 0xa4]
  9671e9: f3 0f 10 95 58 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xa8]
  9671f1: f3 0f 58 15 98 06 b4 00      	addss	xmm2, dword ptr [0xb40698]
  9671f9: f3 0f 10 8d 48 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xb8]
  967201: 8b 40 2c                     	mov	eax, dword ptr [eax + 0x2c]
  967204: f3 0f 5c 05 e8 06 b4 00      	subss	xmm0, dword ptr [0xb406e8]
  96720c: f3 0f 58 57 14               	addss	xmm2, dword ptr [edi + 0x14]
  967211: f3 0f 58 47 10               	addss	xmm0, dword ptr [edi + 0x10]
  967216: f3 0f 58 97 24 06 00 00      	addss	xmm2, dword ptr [edi + 0x624]
  96721e: f3 0f 58 87 20 06 00 00      	addss	xmm0, dword ptr [edi + 0x620]
  967226: f3 0f 58 c8                  	addss	xmm1, xmm0
  96722a: f3 0f 10 85 34 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xcc]
  967232: f3 0f 58 c2                  	addss	xmm0, xmm2
  967236: f3 0f 11 8d 60 ff ff ff      	movss	dword ptr [ebp - 0xa0], xmm1
  96723e: f3 0f 11 85 58 ff ff ff      	movss	dword ptr [ebp - 0xa8], xmm0
  967246: ff d0                        	call	eax
  967248: 8b 8f 8c 02 00 00            	mov	ecx, dword ptr [edi + 0x28c]
  96724e: 66 0f 6e c0                  	movd	xmm0, eax
  967252: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  967256: c1 e8 1f                     	shr	eax, 0x1f
  967259: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  967262: 8b 01                        	mov	eax, dword ptr [ecx]
  967264: 8b 40 28                     	mov	eax, dword ptr [eax + 0x28]
  967267: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  96726b: f3 0f 58 85 58 ff ff ff      	addss	xmm0, dword ptr [ebp - 0xa8]
  967273: f3 0f 5c 85 64 ff ff ff      	subss	xmm0, dword ptr [ebp - 0x9c]
  96727b: f3 0f 11 85 5c ff ff ff      	movss	dword ptr [ebp - 0xa4], xmm0
  967283: ff d0                        	call	eax
  967285: f3 0f 10 8d 60 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xa0]
  96728d: 8d 8d cc fe ff ff            	lea	ecx, [ebp - 0x134]
  967293: 66 0f 6e c0                  	movd	xmm0, eax
  967297: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  96729b: c1 e8 1f                     	shr	eax, 0x1f
  96729e: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  9672a7: 8d 85 5c ff ff ff            	lea	eax, [ebp - 0xa4]
  9672ad: 50                           	push	eax
  9672ae: 8d 85 60 ff ff ff            	lea	eax, [ebp - 0xa0]
  9672b4: 50                           	push	eax
  9672b5: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  9672b9: 8d 85 58 ff ff ff            	lea	eax, [ebp - 0xa8]
  9672bf: 50                           	push	eax
  9672c0: 8d 85 3c ff ff ff            	lea	eax, [ebp - 0xc4]
  9672c6: 50                           	push	eax
  9672c7: f3 0f 58 c1                  	addss	xmm0, xmm1
  9672cb: f3 0f 58 8d 4c ff ff ff      	addss	xmm1, dword ptr [ebp - 0xb4]
  9672d3: f3 0f 5c 85 4c ff ff ff      	subss	xmm0, dword ptr [ebp - 0xb4]
  9672db: f3 0f 11 8d 3c ff ff ff      	movss	dword ptr [ebp - 0xc4], xmm1
  9672e3: f3 0f 11 85 60 ff ff ff      	movss	dword ptr [ebp - 0xa0], xmm0
  9672eb: f3 0f 10 85 58 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xa8]
  9672f3: f3 0f 58 85 64 ff ff ff      	addss	xmm0, dword ptr [ebp - 0x9c]
  9672fb: f3 0f 11 85 58 ff ff ff      	movss	dword ptr [ebp - 0xa8], xmm0
  967303: e8 48 9d 07 00               	call	0x9e1050 <epoxy_handle_external_wglMakeCurrent+0x5d9020>
  967308: 8b 8f 8c 02 00 00            	mov	ecx, dword ptr [edi + 0x28c]
  96730e: 8d 95 fc fe ff ff            	lea	edx, [ebp - 0x104]
  967314: 52                           	push	edx
  967315: 8d 95 cc fe ff ff            	lea	edx, [ebp - 0x134]
  96731b: 52                           	push	edx
  96731c: 8b 01                        	mov	eax, dword ptr [ecx]
  96731e: ff 50 1c                     	call	dword ptr [eax + 0x1c]
  967321: f3 0f 10 95 1c ff ff ff      	movss	xmm2, dword ptr [ebp - 0xe4]
  967329: f3 0f 10 8d 50 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xb0]
  967331: f3 0f 10 85 54 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xac]
  967339: f3 0f 58 0d ac 06 b4 00      	addss	xmm1, dword ptr [0xb406ac]
  967341: f3 0f 5c 15 18 07 b4 00      	subss	xmm2, dword ptr [0xb40718]
  967349: 8d 85 e4 fe ff ff            	lea	eax, [ebp - 0x11c]
  96734f: 8b cf                        	mov	ecx, edi
  967351: 50                           	push	eax
  967352: f3 0f 58 0d 38 06 b4 00      	addss	xmm1, dword ptr [0xb40638]
  96735a: f3 0f 11 95 e4 fe ff ff      	movss	dword ptr [ebp - 0x11c], xmm2
  967362: 0f 28 d0                     	movaps	xmm2, xmm0
  967365: f3 0f 58 8d 44 ff ff ff      	addss	xmm1, dword ptr [ebp - 0xbc]
  96736d: f3 0f 11 8d e8 fe ff ff      	movss	dword ptr [ebp - 0x118], xmm1
  967375: e8 86 ef ff ff               	call	0x966300 <epoxy_handle_external_wglMakeCurrent+0x55e2d0>
  96737a: f3 0f 10 85 54 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xac]
  967382: 0f 57 db                     	xorps	xmm3, xmm3
  967385: 8b 0d 1c 4a be 00            	mov	ecx, dword ptr [0xbe4a1c]
  96738b: 0f 28 c8                     	movaps	xmm1, xmm0
  96738e: 83 b9 c0 a1 02 00 01         	cmp	dword ptr [ecx + 0x2a1c0], 0x1
  967395: 75 33                        	jne	0x9673ca <epoxy_handle_external_wglMakeCurrent+0x55f39a>
  967397: f3 0f 10 0d 78 01 b4 00      	movss	xmm1, dword ptr [0xb40178]
  96739f: 8d 85 60 ff ff ff            	lea	eax, [ebp - 0xa0]
  9673a5: 0f 2f c8                     	comiss	xmm1, xmm0
  9673a8: 8d 95 24 ff ff ff            	lea	edx, [ebp - 0xdc]
  9673ae: c7 85 60 ff ff ff 00 00 00 3f	mov	dword ptr [ebp - 0xa0], 0x3f000000
  9673b8: 0f 46 c2                     	cmovbe	eax, edx
  9673bb: f3 0f 10 00                  	movss	xmm0, dword ptr [eax]
  9673bf: f3 0f 11 85 54 ff ff ff      	movss	dword ptr [ebp - 0xac], xmm0
  9673c7: 0f 28 c8                     	movaps	xmm1, xmm0
  9673ca: 0f 2f cb                     	comiss	xmm1, xmm3
  9673cd: 0f 86 a0 06 00 00            	jbe	0x967a73 <epoxy_handle_external_wglMakeCurrent+0x55fa43>
  9673d3: 68 1c e3 b2 00               	push	0xb2e31c
  9673d8: 8d 81 1c a6 04 00            	lea	eax, [ecx + 0x4a61c]
  9673de: 50                           	push	eax
  9673df: e8 ec a5 fc ff               	call	0x9319d0 <epoxy_handle_external_wglMakeCurrent+0x5299a0>
  9673e4: 8b 8d 38 ff ff ff            	mov	ecx, dword ptr [ebp - 0xc8]
  9673ea: 6a ff                        	push	-0x1
  9673ec: 50                           	push	eax
  9673ed: e8 4e 2a 08 00               	call	0x9e9e40 <epoxy_handle_external_wglMakeCurrent+0x5e1e10>
  9673f2: 8b 8d 38 ff ff ff            	mov	ecx, dword ptr [ebp - 0xc8]
  9673f8: 8b f0                        	mov	esi, eax
  9673fa: 6a ff                        	push	-0x1
  9673fc: 68 10 e3 b2 00               	push	0xb2e310
  967401: e8 3a 2a 08 00               	call	0x9e9e40 <epoxy_handle_external_wglMakeCurrent+0x5e1e10>
  967406: 68 1c e3 b2 00               	push	0xb2e31c
  96740b: 66 0f 6e c0                  	movd	xmm0, eax
  96740f: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  967413: c1 e8 1f                     	shr	eax, 0x1f
  967416: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  96741f: a1 1c 4a be 00               	mov	eax, dword ptr [0xbe4a1c]
  967424: 05 1c a6 04 00               	add	eax, 0x4a61c
  967429: 50                           	push	eax
  96742a: 66 0f 5a c8                  	cvtpd2ps	xmm1, xmm0
  96742e: 66 0f 6e c6                  	movd	xmm0, esi
  967432: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  967436: c1 ee 1f                     	shr	esi, 0x1f
  967439: f2 0f 58 04 f5 10 28 b4 00   	addsd	xmm0, qword ptr [8*esi + 0xb42810]
  967442: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  967446: f3 0f 58 c8                  	addss	xmm1, xmm0
  96744a: f3 0f 11 8d 5c ff ff ff      	movss	dword ptr [ebp - 0xa4], xmm1
  967452: f3 0f 11 8d 60 ff ff ff      	movss	dword ptr [ebp - 0xa0], xmm1
  96745a: e8 71 a5 fc ff               	call	0x9319d0 <epoxy_handle_external_wglMakeCurrent+0x5299a0>
  96745f: 8b 8d 38 ff ff ff            	mov	ecx, dword ptr [ebp - 0xc8]
  967465: 6a ff                        	push	-0x1
  967467: 50                           	push	eax
  967468: e8 d3 29 08 00               	call	0x9e9e40 <epoxy_handle_external_wglMakeCurrent+0x5e1e10>
  96746d: 8b 8d 38 ff ff ff            	mov	ecx, dword ptr [ebp - 0xc8]
  967473: 8b f0                        	mov	esi, eax
  967475: 6a ff                        	push	-0x1
  967477: 68 6c e3 b2 00               	push	0xb2e36c
  96747c: e8 bf 29 08 00               	call	0x9e9e40 <epoxy_handle_external_wglMakeCurrent+0x5e1e10>
  967481: 8d 8d 60 ff ff ff            	lea	ecx, [ebp - 0xa0]
  967487: 66 0f 6e c0                  	movd	xmm0, eax
  96748b: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  96748f: c1 e8 1f                     	shr	eax, 0x1f
  967492: f2 0f 58 04 c5 10 28 b4 00   	addsd	xmm0, qword ptr [8*eax + 0xb42810]
  96749b: 8d 85 24 ff ff ff            	lea	eax, [ebp - 0xdc]
  9674a1: 66 0f 5a c8                  	cvtpd2ps	xmm1, xmm0
  9674a5: 66 0f 6e c6                  	movd	xmm0, esi
  9674a9: f3 0f e6 c0                  	cvtdq2pd	xmm0, xmm0
  9674ad: c1 ee 1f                     	shr	esi, 0x1f
  9674b0: f2 0f 58 04 f5 10 28 b4 00   	addsd	xmm0, qword ptr [8*esi + 0xb42810]
  9674b9: 66 0f 5a c0                  	cvtpd2ps	xmm0, xmm0
  9674bd: f3 0f 58 c8                  	addss	xmm1, xmm0
  9674c1: f3 0f 10 05 2c 6f be 00      	movss	xmm0, dword ptr [0xbe6f2c]
  9674c9: 0f 2f 8d 5c ff ff ff         	comiss	xmm1, dword ptr [ebp - 0xa4]
  9674d0: f3 0f 11 8d 24 ff ff ff      	movss	dword ptr [ebp - 0xdc], xmm1
  9674d8: 0f 46 c1                     	cmovbe	eax, ecx
  9674db: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  9674e1: f3 0f 10 08                  	movss	xmm1, dword ptr [eax]
  9674e5: f3 0f 5c c1                  	subss	xmm0, xmm1
  9674e9: f3 0f 59 05 78 01 b4 00      	mulss	xmm0, dword ptr [0xb40178]
  9674f1: f3 0f 58 c8                  	addss	xmm1, xmm0
  9674f5: f3 0f 11 85 64 ff ff ff      	movss	dword ptr [ebp - 0x9c], xmm0
  9674fd: f3 0f 11 8d 44 ff ff ff      	movss	dword ptr [ebp - 0xbc], xmm1
  967505: e8 46 af dd ff               	call	0x742450 <epoxy_handle_external_wglMakeCurrent+0x33a420>
  96750a: a8 04                        	test	al, 0x4
  96750c: 0f 85 b0 01 00 00            	jne	0x9676c2 <epoxy_handle_external_wglMakeCurrent+0x55f692>
  967512: f3 0f 10 9f c8 02 00 00      	movss	xmm3, dword ptr [edi + 0x2c8]
  96751a: f3 0f 5c 9f c0 02 00 00      	subss	xmm3, dword ptr [edi + 0x2c0]
  967522: f3 0f 10 af 9c 02 00 00      	movss	xmm5, dword ptr [edi + 0x29c]
  96752a: f3 0f 10 0d 2c 6f be 00      	movss	xmm1, dword ptr [0xbe6f2c]
  967532: 0f 28 c5                     	movaps	xmm0, xmm5
  967535: f3 0f 10 95 44 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xbc]
  96753d: f3 0f 5c ca                  	subss	xmm1, xmm2
  967541: f3 0f 58 c3                  	addss	xmm0, xmm3
  967545: 0f 2f c1                     	comiss	xmm0, xmm1
  967548: 0f 86 74 01 00 00            	jbe	0x9676c2 <epoxy_handle_external_wglMakeCurrent+0x55f692>
  96754e: f3 0f 10 a5 10 ff ff ff      	movss	xmm4, dword ptr [ebp - 0xf0]
  967556: 0f 57 c0                     	xorps	xmm0, xmm0
  967559: f3 0f 59 a7 d0 02 00 00      	mulss	xmm4, dword ptr [edi + 0x2d0]
  967561: 0f 2f e0                     	comiss	xmm4, xmm0
  967564: 0f 86 58 01 00 00            	jbe	0x9676c2 <epoxy_handle_external_wglMakeCurrent+0x55f692>
  96756a: f3 0f 10 05 fc 02 b4 00      	movss	xmm0, dword ptr [0xb402fc]
  967572: f3 0f 5c cb                  	subss	xmm1, xmm3
  967576: f3 0f 5c 85 c8 fe ff ff      	subss	xmm0, dword ptr [ebp - 0x138]
  96757e: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  967584: 6a 00                        	push	0x0
  967586: f3 0f 5c cd                  	subss	xmm1, xmm5
  96758a: f3 0f 11 85 10 ff ff ff      	movss	dword ptr [ebp - 0xf0], xmm0
  967592: f3 0f 59 05 e8 06 b4 00      	mulss	xmm0, dword ptr [0xb406e8]
  96759a: f3 0f 5c c8                  	subss	xmm1, xmm0
  96759e: 0f 28 c1                     	movaps	xmm0, xmm1
  9675a1: f3 0f 58 d1                  	addss	xmm2, xmm1
  9675a5: f3 0f 58 85 64 ff ff ff      	addss	xmm0, dword ptr [ebp - 0x9c]
  9675ad: f3 0f 11 95 44 ff ff ff      	movss	dword ptr [ebp - 0xbc], xmm2
  9675b5: f3 0f 11 85 64 ff ff ff      	movss	dword ptr [ebp - 0x9c], xmm0
  9675bd: e8 ee bb aa ff               	call	0x4131b0 <epoxy_handle_external_wglMakeCurrent+0xb180>
  9675c2: 8b c8                        	mov	ecx, eax
  9675c4: e8 77 9d e5 ff               	call	0x7c1340 <epoxy_handle_external_wglMakeCurrent+0x3b9310>
  9675c9: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  9675cf: 8b f0                        	mov	esi, eax
  9675d1: 6a 00                        	push	0x0
  9675d3: e8 d8 bb aa ff               	call	0x4131b0 <epoxy_handle_external_wglMakeCurrent+0xb180>
  9675d8: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  9675de: 6a 00                        	push	0x0
  9675e0: c7 85 60 ff ff ff 00 00 90 42	mov	dword ptr [ebp - 0xa0], 0x42900000
  9675ea: 03 b0 4c 13 00 00            	add	esi, dword ptr [eax + 0x134c]
  9675f0: 66 0f 6e c6                  	movd	xmm0, esi
  9675f4: 0f 5b c0                     	cvtdq2ps	xmm0, xmm0
  9675f7: f3 0f 59 05 00 06 b4 00      	mulss	xmm0, dword ptr [0xb40600]
  9675ff: f3 0f 11 85 5c ff ff ff      	movss	dword ptr [ebp - 0xa4], xmm0
  967607: f3 0f 11 85 24 ff ff ff      	movss	dword ptr [ebp - 0xdc], xmm0
  96760f: e8 9c bb aa ff               	call	0x4131b0 <epoxy_handle_external_wglMakeCurrent+0xb180>
  967614: 8b c8                        	mov	ecx, eax
  967616: e8 85 f1 e4 ff               	call	0x7b67a0 <epoxy_handle_external_wglMakeCurrent+0x3ae770>
  96761b: 85 c0                        	test	eax, eax
  96761d: 7f 0b                        	jg	0x96762a <epoxy_handle_external_wglMakeCurrent+0x55f5fa>
  96761f: 0f 57 c0                     	xorps	xmm0, xmm0
  967622: f3 0f 11 85 28 ff ff ff      	movss	dword ptr [ebp - 0xd8], xmm0
  96762a: f3 0f 10 05 b8 07 b4 00      	movss	xmm0, dword ptr [0xb407b8]
  967632: 8d 85 24 ff ff ff            	lea	eax, [ebp - 0xdc]
  967638: 0f 2f 85 5c ff ff ff         	comiss	xmm0, dword ptr [ebp - 0xa4]
  96763f: 8d 8d 60 ff ff ff            	lea	ecx, [ebp - 0xa0]
  967645: f3 0f 10 8d c8 fe ff ff      	movss	xmm1, dword ptr [ebp - 0x138]
  96764d: f3 0f 59 0d c8 06 b4 00      	mulss	xmm1, dword ptr [0xb406c8]
  967655: 0f 46 c1                     	cmovbe	eax, ecx
  967658: f3 0f 10 95 64 ff ff ff      	movss	xmm2, dword ptr [ebp - 0x9c]
  967660: f3 0f 10 00                  	movss	xmm0, dword ptr [eax]
  967664: f3 0f 58 05 88 07 b4 00      	addss	xmm0, dword ptr [0xb40788]
  96766c: f3 0f 5c c1                  	subss	xmm0, xmm1
  967670: f3 0f 58 85 28 ff ff ff      	addss	xmm0, dword ptr [ebp - 0xd8]
  967678: 0f 2f c2                     	comiss	xmm0, xmm2
  96767b: 72 4d                        	jb	0x9676ca <epoxy_handle_external_wglMakeCurrent+0x55f69a>
  96767d: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  967683: e8 c8 ad dd ff               	call	0x742450 <epoxy_handle_external_wglMakeCurrent+0x33a420>
  967688: f3 0f 10 95 64 ff ff ff      	movss	xmm2, dword ptr [ebp - 0x9c]
  967690: a8 08                        	test	al, 0x8
  967692: 75 36                        	jne	0x9676ca <epoxy_handle_external_wglMakeCurrent+0x55f69a>
  967694: f3 0f 10 8d 10 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xf0]
  96769c: f3 0f 59 0d 84 06 b4 00      	mulss	xmm1, dword ptr [0xb40684]
  9676a4: f3 0f 10 05 e8 06 b4 00      	movss	xmm0, dword ptr [0xb406e8]
  9676ac: f3 0f 5c c1                  	subss	xmm0, xmm1
  9676b0: f3 0f 58 85 30 ff ff ff      	addss	xmm0, dword ptr [ebp - 0xd0]
  9676b8: f3 0f 11 85 30 ff ff ff      	movss	dword ptr [ebp - 0xd0], xmm0
  9676c0: eb 08                        	jmp	0x9676ca <epoxy_handle_external_wglMakeCurrent+0x55f69a>
  9676c2: f3 0f 10 95 64 ff ff ff      	movss	xmm2, dword ptr [ebp - 0x9c]
  9676ca: 8b 3d f4 49 be 00            	mov	edi, dword ptr [0xbe49f4]
  9676d0: f3 0f 10 8d 54 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xac]
  9676d8: c7 85 e8 fe ff ff 00 00 00 00	mov	dword ptr [ebp - 0x118], 0x0
  9676e2: c7 85 d8 fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x128], 0x3f800000
  9676ec: 83 bf fc 5a 02 00 16         	cmp	dword ptr [edi + 0x25afc], 0x16
  9676f3: c7 85 dc fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x124], 0x3f800000
  9676fd: c7 85 e0 fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x120], 0x3f800000
  967707: f3 0f 11 8d e4 fe ff ff      	movss	dword ptr [ebp - 0x11c], xmm1
  96770f: 75 52                        	jne	0x967763 <epoxy_handle_external_wglMakeCurrent+0x55f733>
  967711: b8 80 70 00 00               	mov	eax, 0x7080
  967716: c7 85 fc fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x104], 0x3f800000
  967720: 2b 87 74 5a 02 00            	sub	eax, dword ptr [edi + 0x25a74]
  967726: 33 f6                        	xor	esi, esi
  967728: c7 85 00 ff ff ff 00 00 00 3f	mov	dword ptr [ebp - 0x100], 0x3f000000
  967732: 85 c0                        	test	eax, eax
  967734: c7 85 04 ff ff ff 00 00 00 3f	mov	dword ptr [ebp - 0xfc], 0x3f000000
  96773e: f3 0f 11 8d 08 ff ff ff      	movss	dword ptr [ebp - 0xf8], xmm1
  967746: 0f 4f f0                     	cmovg	esi, eax
  967749: 0f 10 85 fc fe ff ff         	movups	xmm0, xmmword ptr [ebp - 0x104]
  967750: c7 85 e8 fe ff ff 00 00 00 00	mov	dword ptr [ebp - 0x118], 0x0
  96775a: 0f 11 85 d8 fe ff ff         	movups	xmmword ptr [ebp - 0x128], xmm0
  967761: eb 06                        	jmp	0x967769 <epoxy_handle_external_wglMakeCurrent+0x55f739>
  967763: 8b b7 74 5a 02 00            	mov	esi, dword ptr [edi + 0x25a74]
  967769: b8 39 37 58 9b               	mov	eax, 0x9b583739
  96776e: 8b ce                        	mov	ecx, esi
  967770: f7 ee                        	imul	esi
  967772: 03 d6                        	add	edx, esi
  967774: c1 fa 10                     	sar	edx, 0x10
  967777: 8b c2                        	mov	eax, edx
  967779: c1 e8 1f                     	shr	eax, 0x1f
  96777c: 03 c2                        	add	eax, edx
  96777e: 89 85 60 ff ff ff            	mov	dword ptr [ebp - 0xa0], eax
  967784: 69 c0 e0 a5 01 00            	imul	eax, eax, 0x1a5e0
  96778a: 2b c8                        	sub	ecx, eax
  96778c: b8 c5 b3 a2 91               	mov	eax, 0x91a2b3c5
  967791: f7 e9                        	imul	ecx
  967793: 03 d1                        	add	edx, ecx
  967795: c1 fa 0a                     	sar	edx, 0xa
  967798: 8b c2                        	mov	eax, edx
  96779a: c1 e8 1f                     	shr	eax, 0x1f
  96779d: 03 c2                        	add	eax, edx
  96779f: 89 85 24 ff ff ff            	mov	dword ptr [ebp - 0xdc], eax
  9677a5: 69 c0 f8 f8 ff ff            	imul	eax, eax, 0xfffff8f8
  9677ab: 03 c8                        	add	ecx, eax
  9677ad: b8 89 88 88 88               	mov	eax, 0x88888889
  9677b2: f7 e9                        	imul	ecx
  9677b4: 03 d1                        	add	edx, ecx
  9677b6: c1 fa 04                     	sar	edx, 0x4
  9677b9: 8b ca                        	mov	ecx, edx
  9677bb: c1 e9 1f                     	shr	ecx, 0x1f
  9677be: 03 ca                        	add	ecx, edx
  9677c0: 85 f6                        	test	esi, esi
  9677c2: 7f 32                        	jg	0x9677f6 <epoxy_handle_external_wglMakeCurrent+0x55f7c6>
  9677c4: 8b 87 70 5a 02 00            	mov	eax, dword ptr [edi + 0x25a70]
  9677ca: 25 07 00 00 80               	and	eax, 0x80000007
  9677cf: 79 05                        	jns	0x9677d6 <epoxy_handle_external_wglMakeCurrent+0x55f7a6>
  9677d1: 48                           	dec	eax
  9677d2: 83 c8 f8                     	or	eax, -0x8
  9677d5: 40                           	inc	eax
  9677d6: 83 f8 04                     	cmp	eax, 0x4
  9677d9: 7c 1b                        	jl	0x9677f6 <epoxy_handle_external_wglMakeCurrent+0x55f7c6>
  9677db: f3 0f 58 95 48 ff ff ff      	addss	xmm2, dword ptr [ebp - 0xb8]
  9677e3: 8b bd 38 ff ff ff            	mov	edi, dword ptr [ebp - 0xc8]
  9677e9: f3 0f 11 95 64 ff ff ff      	movss	dword ptr [ebp - 0x9c], xmm2
  9677f1: e9 1f 01 00 00               	jmp	0x967915 <epoxy_handle_external_wglMakeCurrent+0x55f8e5>
  9677f6: 51                           	push	ecx
  9677f7: ff b5 24 ff ff ff            	push	dword ptr [ebp - 0xdc]
  9677fd: 8d 45 a8                     	lea	eax, [ebp - 0x58]
  967800: ff b5 60 ff ff ff            	push	dword ptr [ebp - 0xa0]
  967806: 68 64 63 b2 00               	push	0xb26364
  96780b: 6a 40                        	push	0x40
  96780d: 50                           	push	eax
  96780e: e8 5d 13 ab ff               	call	0x418b70 <epoxy_handle_external_wglMakeCurrent+0x10b40>
  967813: 8b bd 38 ff ff ff            	mov	edi, dword ptr [ebp - 0xc8]
  967819: 8d 45 a8                     	lea	eax, [ebp - 0x58]
  96781c: 83 c4 18                     	add	esp, 0x18
  96781f: 8b cf                        	mov	ecx, edi
  967821: 6a ff                        	push	-0x1
  967823: 50                           	push	eax
  967824: e8 17 26 08 00               	call	0x9e9e40 <epoxy_handle_external_wglMakeCurrent+0x5e1e10>
  967829: 8d 8d f4 fe ff ff            	lea	ecx, [ebp - 0x10c]
  96782f: 8b f0                        	mov	esi, eax
  967831: e8 0a 13 08 00               	call	0x9e8b40 <epoxy_handle_external_wglMakeCurrent+0x5e0b10>
  967836: f3 0f 10 85 30 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xd0]
  96783e: c7 45 fc 00 00 00 00         	mov	dword ptr [ebp - 0x4], 0x0
  967845: f3 0f 58 05 0c 07 b4 00      	addss	xmm0, dword ptr [0xb4070c]
  96784d: 50                           	push	eax
  96784e: 8d 85 d8 fe ff ff            	lea	eax, [ebp - 0x128]
  967854: 50                           	push	eax
  967855: a1 1c 4a be 00               	mov	eax, dword ptr [0xbe4a1c]
  96785a: f3 0f 58 85 34 ff ff ff      	addss	xmm0, dword ptr [ebp - 0xcc]
  967862: 68 1c e3 b2 00               	push	0xb2e31c
  967867: 05 1c a6 04 00               	add	eax, 0x4a61c
  96786c: 50                           	push	eax
  96786d: f3 0f 11 85 28 ff ff ff      	movss	dword ptr [ebp - 0xd8], xmm0
  967875: f3 0f 10 85 48 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xb8]
  96787d: f3 0f 58 85 64 ff ff ff      	addss	xmm0, dword ptr [ebp - 0x9c]
  967885: f3 0f 11 85 64 ff ff ff      	movss	dword ptr [ebp - 0x9c], xmm0
  96788d: e8 3e a1 fc ff               	call	0x9319d0 <epoxy_handle_external_wglMakeCurrent+0x5299a0>
  967892: f3 0f 10 9d 28 ff ff ff      	movss	xmm3, dword ptr [ebp - 0xd8]
  96789a: 8b cf                        	mov	ecx, edi
  96789c: f3 0f 10 95 64 ff ff ff      	movss	xmm2, dword ptr [ebp - 0x9c]
  9678a4: 50                           	push	eax
  9678a5: e8 56 1c 08 00               	call	0x9e9500 <epoxy_handle_external_wglMakeCurrent+0x5e14d0>
  9678aa: 8d 8d f4 fe ff ff            	lea	ecx, [ebp - 0x10c]
  9678b0: e8 8b 12 08 00               	call	0x9e8b40 <epoxy_handle_external_wglMakeCurrent+0x5e0b10>
  9678b5: f3 0f 10 95 44 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xbc]
  9678bd: 8b cf                        	mov	ecx, edi
  9678bf: 66 0f 6e c6                  	movd	xmm0, esi
  9678c3: 0f 5b c0                     	cvtdq2ps	xmm0, xmm0
  9678c6: 50                           	push	eax
  9678c7: 8d 85 d8 fe ff ff            	lea	eax, [ebp - 0x128]
  9678cd: c7 45 fc 01 00 00 00         	mov	dword ptr [ebp - 0x4], 0x1
  9678d4: 50                           	push	eax
  9678d5: 8d 45 a8                     	lea	eax, [ebp - 0x58]
  9678d8: f3 0f 10 9d 28 ff ff ff      	movss	xmm3, dword ptr [ebp - 0xd8]
  9678e0: f3 0f 5c d0                  	subss	xmm2, xmm0
  9678e4: 50                           	push	eax
  9678e5: f3 0f 58 95 48 ff ff ff      	addss	xmm2, dword ptr [ebp - 0xb8]
  9678ed: e8 0e 1c 08 00               	call	0x9e9500 <epoxy_handle_external_wglMakeCurrent+0x5e14d0>
  9678f2: c7 45 fc ff ff ff ff         	mov	dword ptr [ebp - 0x4], 0xffffffff
  9678f9: f3 0f 10 8d 54 ff ff ff      	movss	xmm1, dword ptr [ebp - 0xac]
  967901: c7 85 08 ff ff ff 00 00 00 00	mov	dword ptr [ebp - 0xf8], 0x0
  96790b: c7 85 0c ff ff ff 00 00 00 00	mov	dword ptr [ebp - 0xf4], 0x0
  967915: a1 f4 49 be 00               	mov	eax, dword ptr [0xbe49f4]
  96791a: 8b 80 40 5f 02 00            	mov	eax, dword ptr [eax + 0x25f40]
  967920: 83 f8 02                     	cmp	eax, 0x2
  967923: 0f 84 4a 01 00 00            	je	0x967a73 <epoxy_handle_external_wglMakeCurrent+0x55fa43>
  967929: 83 f8 03                     	cmp	eax, 0x3
  96792c: 0f 84 41 01 00 00            	je	0x967a73 <epoxy_handle_external_wglMakeCurrent+0x55fa43>
  967932: 8d 8d ac fe ff ff            	lea	ecx, [ebp - 0x154]
  967938: c7 85 0c ff ff ff 00 00 00 00	mov	dword ptr [ebp - 0xf4], 0x0
  967942: c7 85 fc fe ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x104], 0x3f800000
  96794c: c7 85 00 ff ff ff 00 00 80 3f	mov	dword ptr [ebp - 0x100], 0x3f800000
  967956: c7 85 04 ff ff ff 00 00 80 3f	mov	dword ptr [ebp - 0xfc], 0x3f800000
  967960: f3 0f 11 8d 08 ff ff ff      	movss	dword ptr [ebp - 0xf8], xmm1
  967968: e8 d3 11 08 00               	call	0x9e8b40 <epoxy_handle_external_wglMakeCurrent+0x5e0b10>
  96796d: f3 0f 10 85 30 ff ff ff      	movss	xmm0, dword ptr [ebp - 0xd0]
  967975: c7 45 fc 02 00 00 00         	mov	dword ptr [ebp - 0x4], 0x2
  96797c: f3 0f 58 05 4c 07 b4 00      	addss	xmm0, dword ptr [0xb4074c]
  967984: 50                           	push	eax
  967985: 8d 85 fc fe ff ff            	lea	eax, [ebp - 0x104]
  96798b: 50                           	push	eax
  96798c: a1 1c 4a be 00               	mov	eax, dword ptr [0xbe4a1c]
  967991: f3 0f 58 85 34 ff ff ff      	addss	xmm0, dword ptr [ebp - 0xcc]
  967999: 68 54 e3 b2 00               	push	0xb2e354
  96799e: 05 1c a6 04 00               	add	eax, 0x4a61c
  9679a3: 50                           	push	eax
  9679a4: f3 0f 11 85 30 ff ff ff      	movss	dword ptr [ebp - 0xd0], xmm0
  9679ac: e8 1f a0 fc ff               	call	0x9319d0 <epoxy_handle_external_wglMakeCurrent+0x5299a0>
  9679b1: f3 0f 10 9d 30 ff ff ff      	movss	xmm3, dword ptr [ebp - 0xd0]
  9679b9: 8b cf                        	mov	ecx, edi
  9679bb: f3 0f 10 95 64 ff ff ff      	movss	xmm2, dword ptr [ebp - 0x9c]
  9679c3: 50                           	push	eax
  9679c4: e8 37 1b 08 00               	call	0x9e9500 <epoxy_handle_external_wglMakeCurrent+0x5e14d0>
  9679c9: c7 45 fc ff ff ff ff         	mov	dword ptr [ebp - 0x4], 0xffffffff
  9679d0: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  9679d6: c7 85 c0 fe ff ff 00 00 00 00	mov	dword ptr [ebp - 0x140], 0x0
  9679e0: c7 85 c4 fe ff ff 00 00 00 00	mov	dword ptr [ebp - 0x13c], 0x0
  9679ea: 8d 89 5c 5c 02 00            	lea	ecx, [ecx + 0x25c5c]
  9679f0: e8 9b 32 05 00               	call	0x9bac90 <epoxy_handle_external_wglMakeCurrent+0x5b2c60>
  9679f5: a1 f4 49 be 00               	mov	eax, dword ptr [0xbe49f4]
  9679fa: ff b0 90 5c 02 00            	push	dword ptr [eax + 0x25c90]
  967a00: 8d 85 68 ff ff ff            	lea	eax, [ebp - 0x98]
  967a06: 68 00 dd b0 00               	push	0xb0dd00
  967a0b: 6a 40                        	push	0x40
  967a0d: 50                           	push	eax
  967a0e: e8 5d 11 ab ff               	call	0x418b70 <epoxy_handle_external_wglMakeCurrent+0x10b40>
  967a13: 83 c4 10                     	add	esp, 0x10
  967a16: 8d 85 68 ff ff ff            	lea	eax, [ebp - 0x98]
  967a1c: 8b cf                        	mov	ecx, edi
  967a1e: 6a ff                        	push	-0x1
  967a20: 50                           	push	eax
  967a21: e8 1a 24 08 00               	call	0x9e9e40 <epoxy_handle_external_wglMakeCurrent+0x5e1e10>
  967a26: 8d 8d 90 fe ff ff            	lea	ecx, [ebp - 0x170]
  967a2c: 8b d0                        	mov	edx, eax
  967a2e: e8 0d 11 08 00               	call	0x9e8b40 <epoxy_handle_external_wglMakeCurrent+0x5e0b10>
  967a33: f3 0f 10 95 44 ff ff ff      	movss	xmm2, dword ptr [ebp - 0xbc]
  967a3b: 8b cf                        	mov	ecx, edi
  967a3d: f3 0f 10 9d 30 ff ff ff      	movss	xmm3, dword ptr [ebp - 0xd0]
  967a45: 66 0f 6e c2                  	movd	xmm0, edx
  967a49: 0f 5b c0                     	cvtdq2ps	xmm0, xmm0
  967a4c: 50                           	push	eax
  967a4d: 8d 85 fc fe ff ff            	lea	eax, [ebp - 0x104]
  967a53: c7 45 fc 03 00 00 00         	mov	dword ptr [ebp - 0x4], 0x3
  967a5a: 50                           	push	eax
  967a5b: 8d 85 68 ff ff ff            	lea	eax, [ebp - 0x98]
  967a61: 50                           	push	eax
  967a62: f3 0f 5c d0                  	subss	xmm2, xmm0
  967a66: f3 0f 58 95 48 ff ff ff      	addss	xmm2, dword ptr [ebp - 0xb8]
  967a6e: e8 8d 1a 08 00               	call	0x9e9500 <epoxy_handle_external_wglMakeCurrent+0x5e14d0>
  967a73: 8b 4d f4                     	mov	ecx, dword ptr [ebp - 0xc]
  967a76: 64 89 0d 00 00 00 00         	mov	dword ptr fs:[0x0], ecx
  967a7d: 59                           	pop	ecx
  967a7e: 5f                           	pop	edi
  967a7f: 5e                           	pop	esi
  967a80: 8b 4d ec                     	mov	ecx, dword ptr [ebp - 0x14]
  967a83: 33 cd                        	xor	ecx, ebp
  967a85: e8 31 f1 11 00               	call	0xa86bbb <epoxy_has_wgl_extension+0x2a4eb>
  967a8a: 8b e5                        	mov	esp, ebp
  967a8c: 5d                           	pop	ebp
  967a8d: 8b e3                        	mov	esp, ebx
  967a8f: 5b                           	pop	ebx
  967a90: c3                           	ret
  967a91: cc                           	int3
  967a92: cc                           	int3
  967a93: cc                           	int3
  967a94: cc                           	int3
  967a95: cc                           	int3
  967a96: cc                           	int3
  967a97: cc                           	int3
  967a98: cc                           	int3
  967a99: cc                           	int3
  967a9a: cc                           	int3
  967a9b: cc                           	int3
  967a9c: cc                           	int3
  967a9d: cc                           	int3
  967a9e: cc                           	int3
  967a9f: cc                           	int3
  967aa0: c7 01 00 00 00 00            	mov	dword ptr [ecx], 0x0
  967aa6: 8b c1                        	mov	eax, ecx
  967aa8: c7 41 10 00 00 00 00         	mov	dword ptr [ecx + 0x10], 0x0
  967aaf: c7 41 14 00 00 00 00         	mov	dword ptr [ecx + 0x14], 0x0
  967ab6: c7 41 18 00 00 80 3f         	mov	dword ptr [ecx + 0x18], 0x3f800000
  967abd: c7 41 1c 00 00 00 00         	mov	dword ptr [ecx + 0x1c], 0x0
  967ac4: c7 41 20 00 00 80 3f         	mov	dword ptr [ecx + 0x20], 0x3f800000
  967acb: c7 41 24 00 00 00 00         	mov	dword ptr [ecx + 0x24], 0x0
  967ad2: c6 41 30 00                  	mov	byte ptr [ecx + 0x30], 0x0
  967ad6: c7 41 2c 00 00 00 00         	mov	dword ptr [ecx + 0x2c], 0x0
  967add: c7 41 28 00 00 00 00         	mov	dword ptr [ecx + 0x28], 0x0
  967ae4: c7 41 04 00 00 00 00         	mov	dword ptr [ecx + 0x4], 0x0
  967aeb: c7 41 08 00 00 00 00         	mov	dword ptr [ecx + 0x8], 0x0
  967af2: c7 41 0c 00 00 00 00         	mov	dword ptr [ecx + 0xc], 0x0
  967af9: c3                           	ret
  967afa: cc                           	int3
  967afb: cc                           	int3
  967afc: cc                           	int3
  967afd: cc                           	int3
  967afe: cc                           	int3
  967aff: cc                           	int3
  967b00: c7 41 10 00 00 00 00         	mov	dword ptr [ecx + 0x10], 0x0
  967b07: 8b c1                        	mov	eax, ecx
  967b09: c7 01 00 00 80 3f            	mov	dword ptr [ecx], 0x3f800000
  967b0f: c7 41 04 00 00 80 3f         	mov	dword ptr [ecx + 0x4], 0x3f800000
  967b16: c7 41 08 00 00 80 3f         	mov	dword ptr [ecx + 0x8], 0x3f800000
  967b1d: c7 41 0c 00 00 00 00         	mov	dword ptr [ecx + 0xc], 0x0
  967b24: c7 41 14 00 00 00 00         	mov	dword ptr [ecx + 0x14], 0x0
  967b2b: c7 41 18 00 00 80 3f         	mov	dword ptr [ecx + 0x18], 0x3f800000
  967b32: c6 41 1c 00                  	mov	byte ptr [ecx + 0x1c], 0x0
  967b36: c7 41 30 00 00 00 00         	mov	dword ptr [ecx + 0x30], 0x0
  967b3d: c7 41 20 0a d7 23 3d         	mov	dword ptr [ecx + 0x20], 0x3d23d70a
  967b44: c7 41 24 0a d7 23 3d         	mov	dword ptr [ecx + 0x24], 0x3d23d70a
  967b4b: c7 41 28 0a d7 23 3d         	mov	dword ptr [ecx + 0x28], 0x3d23d70a
  967b52: c7 41 2c 9a 99 19 3f         	mov	dword ptr [ecx + 0x2c], 0x3f19999a
  967b59: c7 41 44 00 00 00 00         	mov	dword ptr [ecx + 0x44], 0x0
  967b60: c7 41 34 00 00 80 3f         	mov	dword ptr [ecx + 0x34], 0x3f800000
  967b67: c7 41 38 00 00 80 3f         	mov	dword ptr [ecx + 0x38], 0x3f800000
  967b6e: c7 41 3c 00 00 80 3f         	mov	dword ptr [ecx + 0x3c], 0x3f800000
  967b75: c7 41 40 00 00 80 3f         	mov	dword ptr [ecx + 0x40], 0x3f800000
  967b7c: c7 41 48 00 00 00 3f         	mov	dword ptr [ecx + 0x48], 0x3f000000
  967b83: c7 41 5c 00 00 00 00         	mov	dword ptr [ecx + 0x5c], 0x0
  967b8a: c7 41 4c 8f c2 75 3d         	mov	dword ptr [ecx + 0x4c], 0x3d75c28f
  967b91: c7 41 50 0a d7 a3 3d         	mov	dword ptr [ecx + 0x50], 0x3da3d70a
  967b98: c7 41 54 b8 1e 05 3e         	mov	dword ptr [ecx + 0x54], 0x3e051eb8
  967b9f: c7 41 58 00 00 80 3f         	mov	dword ptr [ecx + 0x58], 0x3f800000
  967ba6: c7 41 70 00 00 00 00         	mov	dword ptr [ecx + 0x70], 0x0
  967bad: c7 41 60 00 00 80 3f         	mov	dword ptr [ecx + 0x60], 0x3f800000
  967bb4: c7 41 64 00 00 80 3f         	mov	dword ptr [ecx + 0x64], 0x3f800000
  967bbb: c7 41 68 00 00 80 3f         	mov	dword ptr [ecx + 0x68], 0x3f800000
  967bc2: c7 41 6c 9a 99 19 3f         	mov	dword ptr [ecx + 0x6c], 0x3f19999a
  967bc9: c7 41 74 00 00 80 3f         	mov	dword ptr [ecx + 0x74], 0x3f800000
  967bd0: c7 41 78 00 00 80 3f         	mov	dword ptr [ecx + 0x78], 0x3f800000
  967bd7: c7 41 7c 00 00 80 3f         	mov	dword ptr [ecx + 0x7c], 0x3f800000
  967bde: c7 81 80 00 00 00 00 00 80 3f	mov	dword ptr [ecx + 0x80], 0x3f800000
  967be8: c7 81 84 00 00 00 00 00 00 00	mov	dword ptr [ecx + 0x84], 0x0
  967bf2: c7 81 88 00 00 00 00 00 00 00	mov	dword ptr [ecx + 0x88], 0x0
  967bfc: c7 81 8c 00 00 00 00 00 00 00	mov	dword ptr [ecx + 0x8c], 0x0
  967c06: c7 81 90 00 00 00 00 00 00 00	mov	dword ptr [ecx + 0x90], 0x0
  967c10: c7 81 94 00 00 00 00 00 00 00	mov	dword ptr [ecx + 0x94], 0x0
  967c1a: c7 81 98 00 00 00 00 00 00 00	mov	dword ptr [ecx + 0x98], 0x0
  967c24: c7 81 9c 00 00 00 00 00 00 00	mov	dword ptr [ecx + 0x9c], 0x0
  967c2e: c3                           	ret
  967c2f: cc                           	int3
  967c30: 55                           	push	ebp
  967c31: 8b ec                        	mov	ebp, esp
  967c33: 6a ff                        	push	-0x1
  967c35: 68 60 8b a8 00               	push	0xa88b60
  967c3a: 64 a1 00 00 00 00            	mov	eax, dword ptr fs:[0x0]
  967c40: 50                           	push	eax
  967c41: 56                           	push	esi
  967c42: a1 b4 83 b8 00               	mov	eax, dword ptr [0xb883b4]
  967c47: 33 c5                        	xor	eax, ebp
  967c49: 50                           	push	eax
  967c4a: 8d 45 f4                     	lea	eax, [ebp - 0xc]
  967c4d: 64 a3 00 00 00 00            	mov	dword ptr fs:[0x0], eax
  967c53: 8b f1                        	mov	esi, ecx
  967c55: 8d 4e 0c                     	lea	ecx, [esi + 0xc]
  967c58: e8 53 6e ab ff               	call	0x41eab0 <epoxy_handle_external_wglMakeCurrent+0x16a80>
  967c5d: 83 c6 04                     	add	esi, 0x4
  967c60: c7 45 fc 00 00 00 00         	mov	dword ptr [ebp - 0x4], 0x0
  967c67: 8b 4e 04                     	mov	ecx, dword ptr [esi + 0x4]
  967c6a: 85 c9                        	test	ecx, ecx
  967c6c: 74 1a                        	je	0x967c88 <epoxy_handle_external_wglMakeCurrent+0x55fc58>
  967c6e: 8b 01                        	mov	eax, dword ptr [ecx]
  967c70: 8b 40 0c                     	mov	eax, dword ptr [eax + 0xc]
  967c73: ff d0                        	call	eax
  967c75: 84 c0                        	test	al, al
  967c77: 74 0f                        	je	0x967c88 <epoxy_handle_external_wglMakeCurrent+0x55fc58>
  967c79: a1 b4 49 be 00               	mov	eax, dword ptr [0xbe49b4]
  967c7e: 85 c0                        	test	eax, eax
  967c80: 74 06                        	je	0x967c88 <epoxy_handle_external_wglMakeCurrent+0x55fc58>
  967c82: 56                           	push	esi
  967c83: ff d0                        	call	eax
  967c85: 83 c4 04                     	add	esp, 0x4
  967c88: 8b 4d f4                     	mov	ecx, dword ptr [ebp - 0xc]
  967c8b: 64 89 0d 00 00 00 00         	mov	dword ptr fs:[0x0], ecx
  967c92: 59                           	pop	ecx
  967c93: 5e                           	pop	esi
  967c94: 8b e5                        	mov	esp, ebp
  967c96: 5d                           	pop	ebp
  967c97: c3                           	ret
  967c98: cc                           	int3
  967c99: cc                           	int3
  967c9a: cc                           	int3
  967c9b: cc                           	int3
  967c9c: cc                           	int3
  967c9d: cc                           	int3
  967c9e: cc                           	int3
  967c9f: cc                           	int3
  967ca0: 55                           	push	ebp
  967ca1: 8b ec                        	mov	ebp, esp
  967ca3: 6a ff                        	push	-0x1
  967ca5: 68 f0 83 a8 00               	push	0xa883f0
  967caa: 64 a1 00 00 00 00            	mov	eax, dword ptr fs:[0x0]
  967cb0: 50                           	push	eax
  967cb1: 56                           	push	esi
  967cb2: 57                           	push	edi
  967cb3: a1 b4 83 b8 00               	mov	eax, dword ptr [0xb883b4]
  967cb8: 33 c5                        	xor	eax, ebp
  967cba: 50                           	push	eax
  967cbb: 8d 45 f4                     	lea	eax, [ebp - 0xc]
  967cbe: 64 a3 00 00 00 00            	mov	dword ptr fs:[0x0], eax
  967cc4: 8b f1                        	mov	esi, ecx
  967cc6: 68 90 37 40 00               	push	0x403790
  967ccb: 6a 02                        	push	0x2
  967ccd: 68 14 01 00 00               	push	0x114
  967cd2: 8d 86 c8 02 00 00            	lea	eax, [esi + 0x2c8]
  967cd8: 50                           	push	eax
  967cd9: e8 37 f1 11 00               	call	0xa86e15 <epoxy_has_wgl_extension+0x2a745>
  967cde: 8d 4e 44                     	lea	ecx, [esi + 0x44]
  967ce1: e8 ca 6d ab ff               	call	0x41eab0 <epoxy_handle_external_wglMakeCurrent+0x16a80>
  967ce6: 8d 7e 3c                     	lea	edi, [esi + 0x3c]
  967ce9: c7 45 fc 00 00 00 00         	mov	dword ptr [ebp - 0x4], 0x0
  967cf0: 8b 4f 04                     	mov	ecx, dword ptr [edi + 0x4]
  967cf3: 85 c9                        	test	ecx, ecx
  967cf5: 74 1a                        	je	0x967d11 <epoxy_handle_external_wglMakeCurrent+0x55fce1>
  967cf7: 8b 01                        	mov	eax, dword ptr [ecx]
  967cf9: 8b 40 0c                     	mov	eax, dword ptr [eax + 0xc]
  967cfc: ff d0                        	call	eax
  967cfe: 84 c0                        	test	al, al
  967d00: 74 0f                        	je	0x967d11 <epoxy_handle_external_wglMakeCurrent+0x55fce1>
  967d02: a1 b4 49 be 00               	mov	eax, dword ptr [0xbe49b4]
  967d07: 85 c0                        	test	eax, eax
  967d09: 74 06                        	je	0x967d11 <epoxy_handle_external_wglMakeCurrent+0x55fce1>
  967d0b: 57                           	push	edi
  967d0c: ff d0                        	call	eax
  967d0e: 83 c4 04                     	add	esp, 0x4
  967d11: 8d 4e 24                     	lea	ecx, [esi + 0x24]
  967d14: e8 97 78 00 00               	call	0x96f5b0 <epoxy_handle_external_wglMakeCurrent+0x567580>
  967d19: 8d 4e 18                     	lea	ecx, [esi + 0x18]
  967d1c: e8 0f 79 00 00               	call	0x96f630 <epoxy_handle_external_wglMakeCurrent+0x567600>
  967d21: 8b 4d f4                     	mov	ecx, dword ptr [ebp - 0xc]
  967d24: 64 89 0d 00 00 00 00         	mov	dword ptr fs:[0x0], ecx
  967d2b: 59                           	pop	ecx
  967d2c: 5f                           	pop	edi
  967d2d: 5e                           	pop	esi
  967d2e: 8b e5                        	mov	esp, ebp
  967d30: 5d                           	pop	ebp
  967d31: c3                           	ret
  967d32: cc                           	int3
  967d33: cc                           	int3
  967d34: cc                           	int3
  967d35: cc                           	int3
  967d36: cc                           	int3
  967d37: cc                           	int3
  967d38: cc                           	int3
  967d39: cc                           	int3
  967d3a: cc                           	int3
  967d3b: cc                           	int3
  967d3c: cc                           	int3
  967d3d: cc                           	int3
  967d3e: cc                           	int3
  967d3f: cc                           	int3
  967d40: c7 41 04 00 00 00 00         	mov	dword ptr [ecx + 0x4], 0x0
  967d47: 8b c1                        	mov	eax, ecx
  967d49: c7 41 08 00 00 00 00         	mov	dword ptr [ecx + 0x8], 0x0
  967d50: c7 41 0c 00 00 00 00         	mov	dword ptr [ecx + 0xc], 0x0
  967d57: c7 41 10 00 00 00 00         	mov	dword ptr [ecx + 0x10], 0x0
  967d5e: c7 41 14 00 00 00 00         	mov	dword ptr [ecx + 0x14], 0x0
  967d65: c7 41 18 00 00 00 00         	mov	dword ptr [ecx + 0x18], 0x0
  967d6c: c7 41 1c 00 00 00 00         	mov	dword ptr [ecx + 0x1c], 0x0
  967d73: c7 41 20 00 00 00 00         	mov	dword ptr [ecx + 0x20], 0x0
  967d7a: c7 41 24 00 00 00 00         	mov	dword ptr [ecx + 0x24], 0x0
  967d81: c7 41 28 00 00 00 00         	mov	dword ptr [ecx + 0x28], 0x0
  967d88: c7 41 30 00 00 00 00         	mov	dword ptr [ecx + 0x30], 0x0
  967d8f: c7 41 34 00 00 00 00         	mov	dword ptr [ecx + 0x34], 0x0
  967d96: c6 41 38 00                  	mov	byte ptr [ecx + 0x38], 0x0
  967d9a: c7 41 40 00 00 00 00         	mov	dword ptr [ecx + 0x40], 0x0
  967da1: c7 41 44 01 00 00 00         	mov	dword ptr [ecx + 0x44], 0x1
  967da8: c6 41 48 00                  	mov	byte ptr [ecx + 0x48], 0x0
  967dac: c3                           	ret
  967dad: cc                           	int3
  967dae: cc                           	int3
  967daf: cc                           	int3
  967db0: 55                           	push	ebp
  967db1: 8b ec                        	mov	ebp, esp
  967db3: 53                           	push	ebx
  967db4: 56                           	push	esi
  967db5: 8b f1                        	mov	esi, ecx
  967db7: 8b da                        	mov	ebx, edx
  967db9: 8b 0d f4 49 be 00            	mov	ecx, dword ptr [0xbe49f4]
  967dbf: 8b 01                        	mov	eax, dword ptr [ecx]
  967dc1: 83 f8 01                     	cmp	eax, 0x1
  967dc4: 7c 3a                        	jl	0x967e00 <epoxy_handle_external_wglMakeCurrent+0x55fdd0>
  967dc6: 83 f8 06                     	cmp	eax, 0x6
  967dc9: 7f 35                        	jg	0x967e00 <epoxy_handle_external_wglMakeCurrent+0x55fdd0>
  967dcb: 8b 89 c4 5a 02 00            	mov	ecx, dword ptr [ecx + 0x25ac4]
  967dd1: 33 c0                        	xor	eax, eax
  967dd3: 81 e1 00 00 01 00            	and	ecx, 0x10000
  967dd9: 0b c1                        	or	eax, ecx
  967ddb: 74 23                        	je	0x967e00 <epoxy_handle_external_wglMakeCurrent+0x55fdd0>
  967ddd: 83 fe 09                     	cmp	esi, 0x9
  967de0: 75 1e                        	jne	0x967e00 <epoxy_handle_external_wglMakeCurrent+0x55fdd0>
  967de2: f6 c3 10                     	test	bl, 0x10
  967de5: 75 1d                        	jne	0x967e04 <epoxy_handle_external_wglMakeCurrent+0x55fdd4>
  967de7: 39 75 08                     	cmp	dword ptr [ebp + 0x8], esi
  967dea: 75 0e                        	jne	0x967dfa <epoxy_handle_external_wglMakeCurrent+0x55fdca>
  967dec: 8b 4d 0c                     	mov	ecx, dword ptr [ebp + 0xc]
  967def: b8 01 00 00 00               	mov	eax, 0x1
  967df4: d3 e0                        	shl	eax, cl
  967df6: 85 c3                        	test	ebx, eax
  967df8: 75 0a                        	jne	0x967e04 <epoxy_handle_external_wglMakeCurrent+0x55fdd4>
  967dfa: 5e                           	pop	esi
  967dfb: 32 c0                        	xor	al, al
  967dfd: 5b                           	pop	ebx
  967dfe: 5d                           	pop	ebp
  967dff: c3                           	ret
  967e00: 85 f6                        	test	esi, esi
  967e02: 75 06                        	jne	0x967e0a <epoxy_handle_external_wglMakeCurrent+0x55fdda>
  967e04: 5e                           	pop	esi
  967e05: b0 01                        	mov	al, 0x1
  967e07: 5b                           	pop	ebx
  967e08: 5d                           	pop	ebp
  967e09: c3                           	ret
  967e0a: 83 fe 08                     	cmp	esi, 0x8
  967e0d: 7f d8                        	jg	0x967de7 <epoxy_handle_external_wglMakeCurrent+0x55fdb7>
  967e0f: 8d 46 ff                     	lea	eax, [esi - 0x1]
  967e12: 99                           	cdq
  967e13: 2b c2                        	sub	eax, edx
  967e15: 8b f0                        	mov	esi, eax
  967e17: 8b 45 08                     	mov	eax, dword ptr [ebp + 0x8]
  967e1a: d1 fe                        	sar	esi
  967e1c: 8d 40 ff                     	lea	eax, [eax - 0x1]
  967e1f: 99                           	cdq
  967e20: 2b c2                        	sub	eax, edx
  967e22: d1 f8                        	sar	eax
  967e24: 3b c6                        	cmp	eax, esi
  967e26: eb c2                        	jmp	0x967dea <epoxy_handle_external_wglMakeCurrent+0x55fdba>
  967e28: cc                           	int3
  967e29: cc                           	int3
  967e2a: cc                           	int3
  967e2b: cc                           	int3
  967e2c: cc                           	int3
  967e2d: cc                           	int3
  967e2e: cc                           	int3
  967e2f: cc                           	int3
  967e30: cc                           	int3
  967e31: cc                           	int3
  967e32: cc                           	int3
  967e33: cc                           	int3
  967e34: cc                           	int3
  967e35: cc                           	int3
  967e36: cc                           	int3
  967e37: cc                           	int3
  967e38: cc                           	int3
  967e39: cc                           	int3
  967e3a: cc                           	int3
  967e3b: cc                           	int3
  967e3c: cc                           	int3
  967e3d: cc                           	int3
  967e3e: cc                           	int3
  967e3f: cc                           	int3
  967e40: 55                           	push	ebp
  967e41: 8b ec                        	mov	ebp, esp
  967e43: 83 e4 f8                     	and	esp, -0x8
  967e46: 83 ec 3c                     	sub	esp, 0x3c
  967e49: a1 b4 83 b8 00               	mov	eax, dword ptr [0xb883b4]
  967e4e: 33 c4                        	xor	eax, esp
  967e50: 89 44 24 38                  	mov	dword ptr [esp + 0x38], eax
  967e54: 53                           	push	ebx
  967e55: 56                           	push	esi
  967e56: 57                           	push	edi
  967e57: 83 ec 08                     	sub	esp, 0x8
  967e5a: 89 4c 24 18                  	mov	dword ptr [esp + 0x18], ecx
  967e5e: 8b 4d 08                     	mov	ecx, dword ptr [ebp + 0x8]
  967e61: 68 24 e4 b2 00               	push	0xb2e424
  967e66: e8 55 77 aa ff               	call	0x40f5c0 <epoxy_handle_external_wglMakeCurrent+0x7590>
  967e6b: 8b d8                        	mov	ebx, eax
  967e6d: 89 44 24 1c                  	mov	dword ptr [esp + 0x1c], eax
  967e71: 85 db                        	test	ebx, ebx
  967e73: 0f 84 70 0f 00 00            	je	0x968de9 <epoxy_handle_external_wglMakeCurrent+0x560db9>
  967e79: 0f 1f 80 00 00 00 00         	nop	dword ptr [eax]
  967e80: c7 44 24 24 00 00 00 00      	mov	dword ptr [esp + 0x24], 0x0
  967e88: bf d4 68 b1 00               	mov	edi, 0xb168d4
  967e8d: c7 44 24 20 ff ff ff ff      	mov	dword ptr [esp + 0x20], 0xffffffff
  967e95: c7 44 24 28 00 00 00 00      	mov	dword ptr [esp + 0x28], 0x0
  967e9d: 0f 1f 00                     	nop	dword ptr [eax]
  967ea0: 47                           	inc	edi
  967ea1: 80 3f 00                     	cmp	byte ptr [edi], 0x0
  967ea4: 75 fa                        	jne	0x967ea0 <epoxy_handle_external_wglMakeCurrent+0x55fe70>
  967ea6: 8b 53 20                     	mov	edx, dword ptr [ebx + 0x20]
  967ea9: 81 ef d4 68 b1 00            	sub	edi, 0xb168d4
  967eaf: 8b da                        	mov	ebx, edx
  967eb1: 89 54 24 14                  	mov	dword ptr [esp + 0x14], edx
  967eb5: 89 54 24 18                  	mov	dword ptr [esp + 0x18], edx
  967eb9: 85 db                        	test	ebx, ebx
  967ebb: 74 48                        	je	0x967f05 <epoxy_handle_external_wglMakeCurrent+0x55fed5>
  967ebd: 0f 1f 00                     	nop	dword ptr [eax]
  967ec0: 8b 0b                        	mov	ecx, dword ptr [ebx]
  967ec2: 85 c9                        	test	ecx, ecx
  967ec4: 74 05                        	je	0x967ecb <epoxy_handle_external_wglMakeCurrent+0x55fe9b>
  967ec6: 8b 43 08                     	mov	eax, dword ptr [ebx + 0x8]
  967ec9: eb 07                        	jmp	0x967ed2 <epoxy_handle_external_wglMakeCurrent+0x55fea2>
  967ecb: 33 c0                        	xor	eax, eax
  967ecd: b9 b8 49 be 00               	mov	ecx, 0xbe49b8
  967ed2: 3b c7                        	cmp	eax, edi
  967ed4: 75 20                        	jne	0x967ef6 <epoxy_handle_external_wglMakeCurrent+0x55fec6>
  967ed6: 8d 34 08                     	lea	esi, [eax + ecx]
  967ed9: 3b ce                        	cmp	ecx, esi
  967edb: 73 13                        	jae	0x967ef0 <epoxy_handle_external_wglMakeCurrent+0x55fec0>
  967edd: ba d4 68 b1 00               	mov	edx, 0xb168d4
  967ee2: 2b d1                        	sub	edx, ecx
  967ee4: 8a 01                        	mov	al, byte ptr [ecx]
  967ee6: 3a 04 11                     	cmp	al, byte ptr [ecx + edx]
  967ee9: 75 0b                        	jne	0x967ef6 <epoxy_handle_external_wglMakeCurrent+0x55fec6>
  967eeb: 41                           	inc	ecx
  967eec: 3b ce                        	cmp	ecx, esi
  967eee: 72 f4                        	jb	0x967ee4 <epoxy_handle_external_wglMakeCurrent+0x55feb4>
  967ef0: 8b 54 24 14                  	mov	edx, dword ptr [esp + 0x14]
  967ef4: eb 17                        	jmp	0x967f0d <epoxy_handle_external_wglMakeCurrent+0x55fedd>
  967ef6: 8b 5b 18                     	mov	ebx, dword ptr [ebx + 0x18]
  967ef9: 89 5c 24 18                  	mov	dword ptr [esp + 0x18], ebx
  967efd: 85 db                        	test	ebx, ebx
  967eff: 75 bf                        	jne	0x967ec0 <epoxy_handle_external_wglMakeCurrent+0x55fe90>
  967f01: 8b 54 24 14                  	mov	edx, dword ptr [esp + 0x14]
  967f05: c7 44 24 18 00 00 00 00      	mov	dword ptr [esp + 0x18], 0x0
  967f0d: bf 18 e4 b2 00               	mov	edi, 0xb2e418
  967f12: 47                           	inc	edi
  967f13: 80 3f 00                     	cmp	byte ptr [edi], 0x0
  967f16: 75 fa                        	jne	0x967f12 <epoxy_handle_external_wglMakeCurrent+0x55fee2>
  967f18: 8b da                        	mov	ebx, edx
  967f1a: 81 ef 18 e4 b2 00            	sub	edi, 0xb2e418
  967f20: 85 db                        	test	ebx, ebx
  967f22: 74 41                        	je	0x967f65 <epoxy_handle_external_wglMakeCurrent+0x55ff35>
  967f24: 8b 0b                        	mov	ecx, dword ptr [ebx]
  967f26: 85 c9                        	test	ecx, ecx
  967f28: 74 05                        	je	0x967f2f <epoxy_handle_external_wglMakeCurrent+0x55feff>
  967f2a: 8b 43 08                     	mov	eax, dword ptr [ebx + 0x8]
  967f2d: eb 07                        	jmp	0x967f36 <epoxy_handle_external_wglMakeCurrent+0x55ff06>
  967f2f: 33 c0                        	xor	eax, eax
  967f31: b9 b8 49 be 00               	mov	ecx, 0xbe49b8
  967f36: 3b c7                        	cmp	eax, edi
  967f38: 75 20                        	jne	0x967f5a <epoxy_handle_external_wglMakeCurrent+0x55ff2a>
  967f3a: 8d 34 08                     	lea	esi, [eax + ecx]
  967f3d: 3b ce                        	cmp	ecx, esi
  967f3f: 73 13                        	jae	0x967f54 <epoxy_handle_external_wglMakeCurrent+0x55ff24>
  967f41: ba 18 e4 b2 00               	mov	edx, 0xb2e418
  967f46: 2b d1                        	sub	edx, ecx
  967f48: 8a 01                        	mov	al, byte ptr [ecx]
  967f4a: 3a 04 11                     	cmp	al, byte ptr [ecx + edx]
  967f4d: 75 0b                        	jne	0x967f5a <epoxy_handle_external_wglMakeCurrent+0x55ff2a>
  967f4f: 41                           	inc	ecx
  967f50: 3b ce                        	cmp	ecx, esi
  967f52: 72 f4                        	jb	0x967f48 <epoxy_handle_external_wglMakeCurrent+0x55ff18>
  967f54: 8b 54 24 14                  	mov	edx, dword ptr [esp + 0x14]
  967f58: eb 0d                        	jmp	0x967f67 <epoxy_handle_external_wglMakeCurrent+0x55ff37>
  967f5a: 8b 5b 18                     	mov	ebx, dword ptr [ebx + 0x18]
  967f5d: 85 db                        	test	ebx, ebx
  967f5f: 75 c3                        	jne	0x967f24 <epoxy_handle_external_wglMakeCurrent+0x55fef4>
  967f61: 8b 54 24 14                  	mov	edx, dword ptr [esp + 0x14]
  967f65: 33 db                        	xor	ebx, ebx
  967f67: bf e4 db b1 00               	mov	edi, 0xb1dbe4
  967f6c: 0f 1f 40 00                  	nop	dword ptr [eax]
  967f70: 47                           	inc	edi
  967f71: 80 3f 00                     	cmp	byte ptr [edi], 0x0
  967f74: 75 fa                        	jne	0x967f70 <epoxy_handle_external_wglMakeCurrent+0x55ff40>
  967f76: 81 ef e4 db b1 00            	sub	edi, 0xb1dbe4
  967f7c: 85 d2                        	test	edx, edx
  967f7e: 74 45                        	je	0x967fc5 <epoxy_handle_external_wglMakeCurrent+0x55ff95>
  967f80: 8b 0a                        	mov	ecx, dword ptr [edx]
  967f82: 85 c9                        	test	ecx, ecx
  967f84: 74 05                        	je	0x967f8b <epoxy_handle_external_wglMakeCurrent+0x55ff5b>
  967f86: 8b 42 08                     	mov	eax, dword ptr [edx + 0x8]
  967f89: eb 07                        	jmp	0x967f92 <epoxy_handle_external_wglMakeCurrent+0x55ff62>
  967f8b: 33 c0                        	xor	eax, eax
  967f8d: b9 b8 49 be 00               	mov	ecx, 0xbe49b8
  967f92: 3b c7                        	cmp	eax, edi
  967f94: 75 24                        	jne	0x967fba <epoxy_handle_external_wglMakeCurrent+0x55ff8a>
  967f96: 8d 34 08                     	lea	esi, [eax + ecx]
  967f99: 3b ce                        	cmp	ecx, esi
  967f9b: 73 13                        	jae	0x967fb0 <epoxy_handle_external_wglMakeCurrent+0x55ff80>
  967f9d: ba e4 db b1 00               	mov	edx, 0xb1dbe4
  967fa2: 2b d1                        	sub	edx, ecx
  967fa4: 8a 01                        	mov	al, byte ptr [ecx]
  967fa6: 3a 04 11                     	cmp	al, byte ptr [ecx + edx]
  967fa9: 75 0b                        	jne	0x967fb6 <epoxy_handle_external_wglMakeCurrent+0x55ff86>
  967fab: 41                           	inc	ecx
  967fac: 3b ce                        	cmp	ecx, esi
  967fae: 72 f4                        	jb	0x967fa4 <epoxy_handle_external_wglMakeCurrent+0x55ff74>
  967fb0: 8b 74 24 14                  	mov	esi, dword ptr [esp + 0x14]
  967fb4: eb 11                        	jmp	0x967fc7 <epoxy_handle_external_wglMakeCurrent+0x55ff97>
  967fb6: 8b 54 24 14                  	mov	edx, dword ptr [esp + 0x14]
  967fba: 8b 52 18                     	mov	edx, dword ptr [edx + 0x18]
  967fbd: 89 54 24 14                  	mov	dword ptr [esp + 0x14], edx
  967fc1: 85 d2                        	test	edx, edx
  967fc3: 75 bb                        	jne	0x967f80 <epoxy_handle_external_wglMakeCurrent+0x55ff50>
  967fc5: 33 f6                        	xor	esi, esi
  967fc7: 8b 44 24 18                  	mov	eax, dword ptr [esp + 0x18]
  967fcb: 85 c0                        	test	eax, eax
  967fcd: 74 32                        	je	0x968001 <epoxy_handle_external_wglMakeCurrent+0x55ffd1>
  967fcf: 8b 40 04                     	mov	eax, dword ptr [eax + 0x4]
  967fd2: b9 b8 49 be 00               	mov	ecx, 0xbe49b8
  967fd7: 85 c0                        	test	eax, eax
  967fd9: 0f 45 c8                     	cmovne	ecx, eax
  967fdc: 51                           	push	ecx
  967fdd: ff 15 58 a5 aa 00            	call	dword ptr [0xaaa558]
  967fe3: 83 c4 04                     	add	esp, 0x4
  967fe6: 85 c0                        	test	eax, eax
  967fe8: 75 04                        	jne	0x967fee <epoxy_handle_external_wglMakeCurrent+0x55ffbe>
  967fea: 33 ff                        	xor	edi, edi
  967fec: eb 17                        	jmp	0x968005 <epoxy_handle_external_wglMakeCurrent+0x55ffd5>
  967fee: 83 f8 04                     	cmp	eax, 0x4
  967ff1: 7f 09                        	jg	0x967ffc <epoxy_handle_external_wglMakeCurrent+0x55ffcc>
  967ff3: 8d 3c 45 ff ff ff ff         	lea	edi, [2*eax - 0x1]
  967ffa: eb 09                        	jmp	0x968005 <epoxy_handle_external_wglMakeCurrent+0x55ffd5>
  967ffc: 8d 78 04                     	lea	edi, [eax + 0x4]
  967fff: eb 04                        	jmp	0x968005 <epoxy_handle_external_wglMakeCurrent+0x55ffd5>
  968001: 8b 7c 24 24                  	mov	edi, dword ptr [esp + 0x24]
  968005: 85 db                        	test	ebx, ebx
  968007: 74 1f                        	je	0x968028 <epoxy_handle_external_wglMakeCurrent+0x55fff8>
  968009: 8b 43 04                     	mov	eax, dword ptr [ebx + 0x4]
  96800c: b9 b8 49 be 00               	mov	ecx, 0xbe49b8
  968011: 8b 1d 58 a5 aa 00            	mov	ebx, dword ptr [0xaaa558]
  968017: 85 c0                        	test	eax, eax
  968019: 0f 45 c8                     	cmovne	ecx, eax
  96801c: 51                           	push	ecx
  96801d: ff d3                        	call	ebx
  96801f: 83 c4 04                     	add	esp, 0x4
  968022: 89 44 24 20                  	mov	dword ptr [esp + 0x20], eax
  968026: eb 06                        	jmp	0x96802e <epoxy_handle_external_wglMakeCurrent+0x55fffe>
  968028: 8b 1d 58 a5 aa 00            	mov	ebx, dword ptr [0xaaa558]
  96802e: 85 f6                        	test	esi, esi
  968030: 74 17                        	je	0x968049 <epoxy_handle_external_wglMakeCurrent+0x560019>
  968032: 8b 4e 04                     	mov	ecx, dword ptr [esi + 0x4]
  968035: ba b8 49 be 00               	mov	edx, 0xbe49b8
  96803a: 85 c9                        	test	ecx, ecx
  96803c: 0f 45 d1                     	cmovne	edx, ecx
  96803f: 52                           	push	edx
  968040: ff d3                        	call	ebx
  968042: 83 c4 04                     	add	esp, 0x4
  968045: 8b d8                        	mov	ebx, eax
  968047: eb 04                        	jmp	0x96804d <epoxy_handle_external_wglMakeCurrent+0x56001d>
  968049: 8b 5c 24 28                  	mov	ebx, dword ptr [esp + 0x28]
  96804d: 8b 74 24 10                  	mov	esi, dword ptr [esp + 0x10]
  968051: 8b cf                        	mov	ecx, edi
  968053: 8b 54 24 20                  	mov	edx, dword ptr [esp + 0x20]
  968057: ff 76 10                     	push	dword ptr [esi + 0x10]
  96805a: ff 76 0c                     	push	dword ptr [esi + 0xc]
  96805d: e8 4e fd ff ff               	call	0x967db0 <epoxy_handle_external_wglMakeCurrent+0x55fd80>
  968062: 83 c4 08                     	add	esp, 0x8
  968065: 84 c0                        	test	al, al
  968067: 0f 84 69 0d 00 00            	je	0x968dd6 <epoxy_handle_external_wglMakeCurrent+0x560da6>
  96806d: 8b fe                        	mov	edi, esi
  96806f: 85 db                        	test	ebx, ebx
  968071: 74 09                        	je	0x96807c <epoxy_handle_external_wglMakeCurrent+0x56004c>
  968073: 39 5f 14                     	cmp	dword ptr [edi + 0x14], ebx
  968076: 0f 85 5a 0d 00 00            	jne	0x968dd6 <epoxy_handle_external_wglMakeCurrent+0x560da6>
  96807c: 8b 5c 24 1c                  	mov	ebx, dword ptr [esp + 0x1c]
  968080: 8b 5b 20                     	mov	ebx, dword ptr [ebx + 0x20]
  968083: 89 5c 24 18                  	mov	dword ptr [esp + 0x18], ebx
  968087: 85 db                        	test	ebx, ebx
  968089: 0f 84 9e 01 00 00            	je	0x96822d <epoxy_handle_external_wglMakeCurrent+0x5601fd>
  96808f: eb 04                        	jmp	0x968095 <epoxy_handle_external_wglMakeCurrent+0x560065>
  968091: 8b 7c 24 10                  	mov	edi, dword ptr [esp + 0x10]
  968095: 8b 33                        	mov	esi, dword ptr [ebx]
  968097: b8 b8 49 be 00               	mov	eax, 0xbe49b8
  96809c: 85 f6                        	test	esi, esi
  96809e: b9 10 e4 b2 00               	mov	ecx, 0xb2e410
  9680a3: 0f 45 c6                     	cmovne	eax, esi
  9680a6: 8a 10                        	mov	dl, byte ptr [eax]
  9680a8: 3a 11                        	cmp	dl, byte ptr [ecx]
  9680aa: 75 1a                        	jne	0x9680c6 <epoxy_handle_external_wglMakeCurrent+0x560096>
  9680ac: 84 d2                        	test	dl, dl
  9680ae: 74 12                        	je	0x9680c2 <epoxy_handle_external_wglMakeCurrent+0x560092>
  9680b0: 8a 50 01                     	mov	dl, byte ptr [eax + 0x1]
  9680b3: 3a 51 01                     	cmp	dl, byte ptr [ecx + 0x1]
  9680b6: 75 0e                        	jne	0x9680c6 <epoxy_handle_external_wglMakeCurrent+0x560096>
  9680b8: 83 c0 02                     	add	eax, 0x2
  9680bb: 83 c1 02                     	add	ecx, 0x2
  9680be: 84 d2                        	test	dl, dl
  9680c0: 75 e4                        	jne	0x9680a6 <epoxy_handle_external_wglMakeCurrent+0x560076>
  9680c2: 33 c0                        	xor	eax, eax
  9680c4: eb 05                        	jmp	0x9680cb <epoxy_handle_external_wglMakeCurrent+0x56009b>
  9680c6: 1b c0                        	sbb	eax, eax
  9680c8: 83 c8 01                     	or	eax, 0x1
  9680cb: 85 c0                        	test	eax, eax
  9680cd: 0f 85 9e 01 00 00            	jne	0x968271 <epoxy_handle_external_wglMakeCurrent+0x560241>
  9680d3: 8b 43 04                     	mov	eax, dword ptr [ebx + 0x4]
  9680d6: ba b8 49 be 00               	mov	edx, 0xbe49b8
  9680db: 85 c0                        	test	eax, eax
  9680dd: c7 44 24 2c 00 00 00 00      	mov	dword ptr [esp + 0x2c], 0x0
  9680e5: c7 44 24 3c 00 00 00 00      	mov	dword ptr [esp + 0x3c], 0x0
  9680ed: 0f 45 d0                     	cmovne	edx, eax
  9680f0: c7 44 24 40 0f 00 00 00      	mov	dword ptr [esp + 0x40], 0xf
  9680f8: 8b ca                        	mov	ecx, edx
  9680fa: 8d 71 01                     	lea	esi, [ecx + 0x1]
  9680fd: 0f 1f 00                     	nop	dword ptr [eax]
