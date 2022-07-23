#define NATIVIZED_CODE_AVAILABLE

// First occurrence: testA, line 89
static void exec_c04eb2790ca3681c(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(8), (uint16)context.getParameter<int64>());
}

// First occurrence: testA, line 90
static void exec_3799c9db3d218f0d(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint16>(0, (uint16)(uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const uint16 var0 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4));
	const int64 var1 = 0;
	context.writeValueStack<int16>(1, (int16)((int16)(var0) < (int16)(var1)));
	context.moveValueStack(2);
}

// First occurrence: testA, line 90
static void exec_515e4e046d501985(const RuntimeOpcodeContext context)
{
	const uint16 var0 = ((uint16)(context.readValueStack<uint16>(-2)) + (uint16)(context.readValueStack<uint16>(-1)));
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(), (uint16)var0);
	context.moveValueStack(-2);
}

// First occurrence: testA, line 91
static void exec_662dea665c3ed4db(const RuntimeOpcodeContext context)
{
	const uint16 var0 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>());
	const int64 var1 = 1;
	context.writeValueStack<uint16>(0, (uint16)((uint16)(var0) == (uint16)(var1)));
	context.moveValueStack(1);
}

// First occurrence: testB, line 96
static void exec_c08206c070abfdf4(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(8), (uint16)context.getParameter<int64>());
}

// First occurrence: testB, line 98
static void exec_8d8b5b4db9f26100(const RuntimeOpcodeContext context)
{
	const uint16 var0 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>());
	const uint16 var1 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4));
	const uint16 var2 = ((uint16)(var0) + (uint16)(var1));
	context.writeValueStack<uint16>(0, (uint16)((uint16)(var2) == (uint16)(context.getParameter<int64>(8))));
	context.moveValueStack(1);
}

// First occurrence: testC, line 103
static void exec_8f994c348b7b9912(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint32>(context.getParameter<uint32>(8), (uint32)context.getParameter<int64>());
}

// First occurrence: testC, line 104
static void exec_6445f332528563e5(const RuntimeOpcodeContext context)
{
	const uint32 var1 = (uint32)context.readLocalVariable<uint32>(context.getParameter<uint32>(8));
	const uint32 var3 = ((uint32)(var1) + (uint32)(context.getParameter<int64>(12)));
	const uint32 var4 = ((uint32)(context.getParameter<int64>()) - (uint32)(var3));
	const uint32 var6 = OpcodeExecUtils::safeDivide<uint32>((uint32)var4, (uint32)context.getParameter<int64>(20));
	const int16 var7 = (uint16)var6;
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(28), (uint16)var7);
}

// First occurrence: testC, line 105
static void exec_204f608a09580e3c(const RuntimeOpcodeContext context)
{
	const uint16 var0 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>());
	context.writeValueStack<uint16>(0, (uint16)((uint16)(var0) == (uint16)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: testD, line 112
static void exec_5ace2431dbe31ab3(const RuntimeOpcodeContext context)
{
	const uint16 var0 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>());
	const int16 var1 = (uint16)var0;
	const uint32 var3 = ((uint32)(var1) << (uint32)((context.getParameter<int64>(4)) & 0x1f));
	const uint16 var4 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(12));
	const int16 var5 = (uint16)var4;
	const int32 var6 = OpcodeExecUtils::safeDivide<int32>((int32)var3, (int32)var5);
	context.writeLocalVariable<uint32>(context.getParameter<uint32>(16), (uint32)var6);
}

// First occurrence: testD, line 113
static void exec_8857377091aaaab0(const RuntimeOpcodeContext context)
{
	const uint32 var0 = (uint32)context.readLocalVariable<uint32>(context.getParameter<uint32>());
	context.writeValueStack<uint32>(0, (uint32)((uint32)(var0) == (uint32)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: testE, line 118
static void exec_69fe009be9d9b35b(const RuntimeOpcodeContext context)
{
	OpcodeExecUtils::writeMemory<uint8>(*context.mControlFlow, context.getParameter<int64>(8), (uint8)context.getParameter<int64>());
}

// First occurrence: testE, line 119
static void exec_d3192f07ea740c8d(const RuntimeOpcodeContext context)
{
	const uint32 var0 = context.getParameter<int64>();
	const uint8 var1 = OpcodeExecUtils::readMemory<uint8>(*context.mControlFlow, var0);
	const uint8 var3 = ((uint8)(var1) & (uint8)(context.getParameter<int64>(8)));
	OpcodeExecUtils::writeMemory<uint8>(*context.mControlFlow, var0, (uint8)var3);
	context.writeLocalVariable<uint8>(context.getParameter<uint32>(16), (uint8)var3);
}

// First occurrence: testE, line 120
static void exec_55fd5b11bcf78dc0(const RuntimeOpcodeContext context)
{
	const uint8 var0 = (uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>());
	context.writeValueStack<uint8>(0, (uint8)((uint8)(var0) == (uint8)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: testF, line 125
static void exec_e152669fa11bfc82(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint8>(context.getParameter<uint32>(8), (uint8)context.getParameter<int64>());
}

// First occurrence: testF, line 127
static void exec_227c65efaaf8ecfe(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint16>(0, (uint16)(uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const uint8 var0 = (uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>(4));
	context.writeValueStack<uint8>(1, (uint8)(uint8)-(signed)var0);
	context.writeValueStack<uint8>(2, (uint8)(uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>(8)));
	context.moveValueStack(3);
}

// First occurrence: testF, line 127
static void exec_835f847c4e4dc37a(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(), (uint16)context.readValueStack<uint16>(-1));
	context.moveValueStack(-1);
}


void createNativizedCodeLookup(Nativizer::LookupDictionary& dict)
{
	const char emptyEntries0[] =
	{
		"\x7e\x9b\xa5\x5d\x5f\x7a\x60\xee\xbb\x34\x71\xd5\xde\x5b\x44\x14\x35\xc4\x1a\x1f\x7c\x22\x02\xd4\x70\xd7\x54\x9b\x7e\x95\x4b\x04\x2a\x37\x92\x0d\xb3\x74\x79\xde\x7d\x92\xd8\x90\x29\x06\xdb\xa9\x0f\x38\x8e\xfb\x6c\x38\xc8\x7c\x92\x51\x6e\xd5\xde\xf5\x40\x14\x69\x87\x8f\xb4\x94\xd3\xfc\x99\xf6\xfc\x5d\x71\x74\xac\x0f\x82\xc7\xce\xbd\xd2\xd3\xee\x02\xe7\x4f\x2d\xa3\x7b\x59\x7a\x60\xa3\x0c\x13\xe3\x09\x40\x15\xef\x1a\x61\x41\x5d\x44\x4a\x92\x20\xcb\x47\x16\xe0\x8a\x89\x8c\x93\xa5\x65\x53\x1f\x0e\x54\xb7\x7f\xb6"
		"\x2b\x92\x50\xf9\x79\x41\xc7\xa2\xf7\xa3\xe9\xeb\x06\xd6\x1a\xbb\xcc\xf4\x31\xa5\x8a\xed\xbd\xf1\xad\x30\x2b\xb4\x1b\x2a\x13\x17\x74\xb1\xbb\x30\xee\x54\x6b\xa0\x54\xa3\xfd\x48\x34\xf6\x66\x2c\x73\x89\x3b\x64\x0d\x83\xfb\xd0\xa2\xd9\x09\x80\x37\x46\x08\xaa\x16\x67\xa7\x52\xdc\xb2\x72\x5a\x66\xfd\x9c\x99\xdd\x7b\x20\x49\x6f\x32\x35\xb0\xe9\x82\xed\x0a\xe6\xe4\x5b\x9e\x1c\xc4\xca\x84\xa1\x85\x78\xb3\xfe\x5a\x7c\xc7\xa8\x28\x0f\x04\xce\x87\x3d\xaf\x7f\x29\x1c\x63\xff\x05\x0f\xf2\x17\x48\x3f\xc8\x46\xc7\x76\x28"
		"\x0e\x13\x8c\xfa\xd5\xf1\x97\xca\xe9\x76\x5f\x5e\xbc\x4f\x36\x52\x6d\x08\x79\xed\x23\x16\x0f\x22\x4e\x68\x7e\x95\x4b\x8b\x5d\x48\x9c\x60\x54\xd3\x0c\xe0\x37\xd7"
	};
	dict.addEmptyEntries(reinterpret_cast<const uint64*>(emptyEntries0), 0x25);

	const char parameterData[] =
	{
		"\x78\xda\x63\x60\xf8\xff\x9f\x01\x08\x38\x18\x18\x19\x78\xa0\x6c\x16\x20\x9b\x03\xcc\x66\x04\xb2\x61\x62\xb8\xd4\x31\x33\x08\x60\x91\x83\xb0\x99\x18\x44\x18\x58\x19\x64\x80\x3c\x05\x24\x3d\x08\xfd\x4c\x40\x36\x0b\x50\x3f\x3b\x50\x1d\xa6\x3c\xc8\x0c\x84\xd9\x4c\x40\x36\x2b\x4e\x75\xe8\x6e\xe2\x41\x72\x37\x00\x02\x45\x25\xbb"
	};
	dict.loadParameterInfo(reinterpret_cast<const uint8*>(parameterData), 0x0051);

	const Nativizer::CompactFunctionEntry functionList[] =
	{
		{ 0xe152669fa11bfc82, &exec_e152669fa11bfc82, 0x00000030 },
		{ 0x515e4e046d501985, &exec_515e4e046d501985, 0x00000007 },
		{ 0x662dea665c3ed4db, &exec_662dea665c3ed4db, 0x00000009 },
		{ 0x3799c9db3d218f0d, &exec_3799c9db3d218f0d, 0x00000004 },
		{ 0xc04eb2790ca3681c, &exec_c04eb2790ca3681c, 0x00000001 },
		{ 0xd3192f07ea740c8d, &exec_d3192f07ea740c8d, 0x00000029 },
		{ 0xc08206c070abfdf4, &exec_c08206c070abfdf4, 0x0000000b },
		{ 0x55fd5b11bcf78dc0, &exec_55fd5b11bcf78dc0, 0x0000002d },
		{ 0x8d8b5b4db9f26100, &exec_8d8b5b4db9f26100, 0x0000000e },
		{ 0x8f994c348b7b9912, &exec_8f994c348b7b9912, 0x00000012 },
		{ 0x6445f332528563e5, &exec_6445f332528563e5, 0x00000015 },
		{ 0x204f608a09580e3c, &exec_204f608a09580e3c, 0x0000001b },
		{ 0x835f847c4e4dc37a, &exec_835f847c4e4dc37a, 0x00000037 },
		{ 0x227c65efaaf8ecfe, &exec_227c65efaaf8ecfe, 0x00000033 },
		{ 0x5ace2431dbe31ab3, &exec_5ace2431dbe31ab3, 0x0000001e },
		{ 0x8857377091aaaab0, &exec_8857377091aaaab0, 0x00000023 },
		{ 0x69fe009be9d9b35b, &exec_69fe009be9d9b35b, 0x00000026 }
	};
	dict.loadFunctions(functionList, 0x0011);
}
