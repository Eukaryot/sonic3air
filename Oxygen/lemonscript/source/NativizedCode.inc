#define NATIVIZED_CODE_AVAILABLE

// First occurrence: testA, line 28
static void exec_c04eb2790ca3681c(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<int16>(context.getParameter<uint32>(8), (uint16)context.getParameter<int64>());
}

// First occurrence: testA, line 29
static void exec_3799c9db3d218f0d(const RuntimeOpcodeContext context)
{
	context.writeValueStack<int16>(0, (int16)(uint16)context.readLocalVariable<int16>(context.getParameter<uint32>()));
	const int16 var0 = (uint16)context.readLocalVariable<int16>(context.getParameter<uint32>(4));
	const int64 var1 = 0;
	context.writeValueStack<int16>(1, (int16)((int16)(var0) < (int16)(var1)));
	context.moveValueStack(2);
}

// First occurrence: testA, line 29
static void exec_03963ab91fd65d96(const RuntimeOpcodeContext context)
{
	context.writeValueStack<int64>(0, (int64)(uint64)-(signed)context.getParameter<int64>());
	context.moveValueStack(1);
}

// First occurrence: testA, line 29
static void exec_375c0a2b0ce2f2ae(const RuntimeOpcodeContext context)
{
	const int16 var0 = (uint16)context.readValueStack<int16>(-1);
	const int16 var1 = ((uint16)(context.readValueStack<int16>(-2)) + (uint16)(var0));
	context.writeLocalVariable<int16>(context.getParameter<uint32>(), (uint16)var1);
	context.moveValueStack(-2);
}

// First occurrence: testA, line 30
static void exec_662dea665c3ed4db(const RuntimeOpcodeContext context)
{
	const int16 var0 = (uint16)context.readLocalVariable<int16>(context.getParameter<uint32>());
	const int64 var1 = 1;
	context.writeValueStack<int16>(0, (int16)((uint16)(var0) == (uint16)(var1)));
	context.moveValueStack(1);
}

// First occurrence: testB, line 35
static void exec_c08206c070abfdf4(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(8), (uint16)context.getParameter<int64>());
}

// First occurrence: testB, line 36
static void exec_d6ae77ecd8a102b1(const RuntimeOpcodeContext context)
{
	const int16 var1 = (uint16)-(signed)context.getParameter<int64>();
	context.writeLocalVariable<int16>(context.getParameter<uint32>(8), (uint16)var1);
}

// First occurrence: testB, line 37
static void exec_8d8b5b4db9f26100(const RuntimeOpcodeContext context)
{
	const uint16 var0 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>());
	const int16 var1 = (uint16)context.readLocalVariable<int16>(context.getParameter<uint32>(4));
	const uint16 var2 = ((uint16)(var0) + (uint16)(var1));
	context.writeValueStack<int16>(0, (int16)((uint16)(var2) == (uint16)(context.getParameter<int64>(8))));
	context.moveValueStack(1);
}

// First occurrence: testC, line 42
static void exec_8f994c348b7b9912(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint32>(context.getParameter<uint32>(8), (uint32)context.getParameter<int64>());
}

// First occurrence: testC, line 43
static void exec_6445f332528563e5(const RuntimeOpcodeContext context)
{
	const uint32 var1 = (uint32)context.readLocalVariable<uint32>(context.getParameter<uint32>(8));
	const uint32 var3 = ((uint32)(var1) + (uint32)(context.getParameter<int64>(12)));
	const uint32 var4 = ((uint32)(context.getParameter<int64>()) - (uint32)(var3));
	const uint32 var6 = OpcodeExecUtils::safeDivide<uint32>((uint32)var4, (uint32)context.getParameter<int64>(20));
	const int16 var7 = (uint16)var6;
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(28), (uint16)var7);
}

// First occurrence: testC, line 44
static void exec_204f608a09580e3c(const RuntimeOpcodeContext context)
{
	const uint16 var0 = (uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>());
	context.writeValueStack<uint16>(0, (uint16)((uint16)(var0) == (uint16)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: testD, line 51
static void exec_5ace2431dbe31ab3(const RuntimeOpcodeContext context)
{
	const int16 var0 = (uint16)context.readLocalVariable<int16>(context.getParameter<uint32>());
	const int32 var3 = ((uint32)(var0) << (uint32)((context.getParameter<int64>(4)) & 0x1f));
	const int16 var4 = (uint16)context.readLocalVariable<int16>(context.getParameter<uint32>(12));
	const int32 var6 = OpcodeExecUtils::safeDivide<int32>((int32)var3, (int32)var4);
	context.writeLocalVariable<int32>(context.getParameter<uint32>(16), (uint32)var6);
}

// First occurrence: testD, line 52
static void exec_1c5c286080d5eea9(const RuntimeOpcodeContext context)
{
	const int32 var0 = (uint32)context.readLocalVariable<int32>(context.getParameter<uint32>());
	const int64 var2 = (uint64)-(signed)context.getParameter<int64>(4);
	const int32 var3 = (uint32)var2;
	context.writeValueStack<int32>(0, (int32)((uint32)(var0) == (uint32)(var3)));
	context.moveValueStack(1);
}

// First occurrence: testE, line 57
static void exec_69fe009be9d9b35b(const RuntimeOpcodeContext context)
{
	OpcodeExecUtils::writeMemory<uint8>(*context.mControlFlow, context.getParameter<int64>(8), (uint8)context.getParameter<int64>());
}

// First occurrence: testE, line 58
static void exec_d3192f07ea740c8d(const RuntimeOpcodeContext context)
{
	const uint32 var0 = context.getParameter<int64>();
	const uint8 var1 = OpcodeExecUtils::readMemory<uint8>(*context.mControlFlow, var0);
	const uint8 var3 = ((uint8)(var1) & (uint8)(context.getParameter<int64>(8)));
	OpcodeExecUtils::writeMemory<uint8>(*context.mControlFlow, var0, (uint8)var3);
	context.writeLocalVariable<uint8>(context.getParameter<uint32>(16), (uint8)var3);
}

// First occurrence: testE, line 59
static void exec_55fd5b11bcf78dc0(const RuntimeOpcodeContext context)
{
	const uint8 var0 = (uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>());
	context.writeValueStack<uint8>(0, (uint8)((uint8)(var0) == (uint8)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}


void createNativizedCodeLookup(Nativizer::LookupDictionary& dict)
{
	const char emptyEntries0[] =
	{
		"\x47\x16\xe0\x8a\x89\x8c\x93\xa5\xbb\x34\x71\xd5\xde\x5b\x44\x14\x35\xc4\x1a\x1f\x7c\x22\x02\xd4\x0e\x13\x8c\xfa\xd5\xf1\x97\xca\xfb\x1a\x8d\x2c\x24\x0b\x86\x6f\x70\xd7\x54\x9b\x7e\x95\x4b\x04\x2a\x37\x92\x0d\xb3\x74\x79\xde\x7d\x92\xd8\x90\x29\x06\xdb\xa9\x0f\x38\x8e\xfb\x6c\x38\xc8\x7c\x66\xfd\x9c\x99\xdd\x7b\x20\x49\xb1\x98\x08\xb4\x75\x8e\x6c\x2d\x61\x41\x5d\x44\x4a\x92\x20\xcb\xc7\xce\xbd\xd2\xd3\xee\x02\xe7\x4f\x2d\xa3\x7b\x59\x7a\x60\xa3\x16\xc2\x6d\xc7\xfe\x06\xbc\x15\xda\x00\xdb\xc1\x85\x79\x07\x91"
		"\x0c\x13\xe3\x09\x40\x15\xef\x1a\x65\x53\x1f\x0e\x54\xb7\x7f\xb6\x2b\x92\x50\xf9\x79\x41\xc7\xa2\xf7\xa3\xe9\xeb\x06\xd6\x1a\xbb\xcc\xf4\x31\xa5\x8a\xed\xbd\xf1\xad\x30\x2b\xb4\x1b\x2a\x13\x17\xa0\xc2\x0d\x3f\x35\xa3\xcc\x41\x54\x93\xa1\x09\xf0\x12\x6e\xae\x74\xb1\xbb\x30\xee\x54\x6b\xa0\x54\xa3\xfd\x48\x34\xf6\x66\x2c\x73\x89\x3b\x64\x0d\x83\xfb\xd0\xa2\xd9\x09\x80\x37\x46\x08\xaa\x16\x67\xa7\x52\xdc\xb2\x72\x5a\x7e\x9b\xa5\x5d\x5f\x7a\x60\xee\x6f\x32\x35\xb0\xe9\x82\xed\x0a\xe6\xe4\x5b\x9e\x1c\xc4\xca\x84"
		"\xa1\x85\x78\xb3\xfe\x5a\x7c\xc7\xa8\x28\x0f\x04\xce\x87\x3d\xaf\x7f\x29\x1c\x63\xff\x05\x0f\xf2\xe9\x76\x5f\x5e\xbc\x4f\x36\x52\x6d\x08\x79\xed\x23\x16\x0f\x22\x4e\x68\x7e\x95\x4b\x8b\x5d\x48\x69\x87\x8f\xb4\x94\xd3\xfc\x99"
	};
	dict.addEmptyEntries(reinterpret_cast<const uint64*>(emptyEntries0), 0x27);

	const char parameterData[] =
	{
		"\x78\xda\x63\x60\xf8\xff\x9f\x01\x08\x38\x18\x18\x19\x78\xa0\x6c\x16\x20\x9b\x03\x2e\x0e\xa2\x99\x80\x62\x30\x39\x4c\xf5\x1c\x40\x79\x54\xbd\xcc\x0c\x02\x58\xd5\x81\xd8\x4c\x0c\x22\x0c\xac\x0c\x32\x40\x9e\x02\x92\x1e\x84\x7e\x90\x59\x2c\x40\xfd\xec\x40\x75\x98\xf2\x20\x33\x04\x90\xec\x15\x00\x9a\x85\xae\x0e\x00\x8f\x0e\x23\xa7"
	};
	dict.loadParameterInfo(reinterpret_cast<const uint8*>(parameterData), 0x0052);

	const Nativizer::CompactFunctionEntry functionList[] =
	{
		{ 0x03963ab91fd65d96, &exec_03963ab91fd65d96, 0x00000007 },
		{ 0x69fe009be9d9b35b, &exec_69fe009be9d9b35b, 0x0000002b },
		{ 0x662dea665c3ed4db, &exec_662dea665c3ed4db, 0x0000000b },
		{ 0x3799c9db3d218f0d, &exec_3799c9db3d218f0d, 0x00000004 },
		{ 0xc04eb2790ca3681c, &exec_c04eb2790ca3681c, 0x00000001 },
		{ 0x55fd5b11bcf78dc0, &exec_55fd5b11bcf78dc0, 0x00000032 },
		{ 0x8d8b5b4db9f26100, &exec_8d8b5b4db9f26100, 0x00000013 },
		{ 0x375c0a2b0ce2f2ae, &exec_375c0a2b0ce2f2ae, 0x00000009 },
		{ 0xc08206c070abfdf4, &exec_c08206c070abfdf4, 0x0000000d },
		{ 0xd6ae77ecd8a102b1, &exec_d6ae77ecd8a102b1, 0x00000010 },
		{ 0x8f994c348b7b9912, &exec_8f994c348b7b9912, 0x00000017 },
		{ 0x6445f332528563e5, &exec_6445f332528563e5, 0x0000001a },
		{ 0x204f608a09580e3c, &exec_204f608a09580e3c, 0x00000020 },
		{ 0x5ace2431dbe31ab3, &exec_5ace2431dbe31ab3, 0x00000023 },
		{ 0x1c5c286080d5eea9, &exec_1c5c286080d5eea9, 0x00000028 },
		{ 0xd3192f07ea740c8d, &exec_d3192f07ea740c8d, 0x0000002e }
	};
	dict.loadFunctions(functionList, 0x0010);
}
