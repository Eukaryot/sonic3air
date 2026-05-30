#define NATIVIZED_CODE_AVAILABLE

// First occurrence: script01, line 28
static void exec_cb2baa5aec3ae364(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, context.getParameter<float>());
	context.writeValueStack<int64>(1, (uint64)context.getParameter<int64>(8));
	context.moveValueStack(2);
}

// First occurrence: script01, line 29
static void exec_77a1839043dd998f(const RuntimeOpcodeContext context)
{
	context.writeValueStack<double>(0, context.getParameter<double>());
	context.writeValueStack<int64>(1, (uint64)context.getParameter<int64>(8));
	context.moveValueStack(2);
}

// First occurrence: script01, line 31
static void exec_2c6a9db0533dc490(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, (float)context.getParameter<int64>());
	context.moveValueStack(1);
}

// First occurrence: script01, line 32
static void exec_471a710265fff6cd(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, ((float)context.getParameter<int64>() + context.getParameter<float>(8)));
	context.moveValueStack(1);
}

// First occurrence: runTests, line 4
static void exec_1e99bb14ecac1b69(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint64>(0, (uint64)context.getParameter<int64>());
	context.writeValueStack<int64>(1, (uint64)context.getParameter<int64>(8));
	context.moveValueStack(2);
}

// First occurrence: integerArithmeticTestA, line 31
static void exec_f198f4801343acae(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(8), (uint16)context.getParameter<int64>());
}

// First occurrence: integerArithmeticTestA, line 32
static void exec_d30d2a0ed02b013b(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint16>(0, context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4)));
	const AnyBaseValue var1((int64)0);
	context.writeValueStack<uint8>(1, (uint8)(var0.get<int16>() < var1.get<int16>()));
	context.moveValueStack(2);
}

// First occurrence: integerArithmeticTestA, line 32
static void exec_88dee4fe77b63531(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)(context.readValueStack<uint16>(-2) + context.readValueStack<uint16>(-1)));
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(), var0.get<uint16>());
	context.moveValueStack(-2);
}

// First occurrence: integerArithmeticTestA, line 33
static void exec_5187dc7e6e5a09c7(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var1((int64)1);
	context.writeValueStack<uint8>(0, (uint8)(var0.get<uint16>() == var1.get<uint16>()));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestB, line 38
static void exec_477cd77d3cd1e726(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(8), (uint16)context.getParameter<int64>());
}

// First occurrence: integerArithmeticTestB, line 40
static void exec_a73019784d9605d0(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var1((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4)));
	const AnyBaseValue var2((uint16)(var0.get<uint16>() + var1.get<uint16>()));
	context.writeValueStack<uint8>(0, (uint8)(var2.get<uint16>() == (uint16)(context.getParameter<int64>(8))));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestC, line 45
static void exec_f3548d4e78066474(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint32>(context.getParameter<uint32>(8), (uint32)context.getParameter<int64>());
}

// First occurrence: integerArithmeticTestC, line 46
static void exec_c8451f7e094b32c7(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var1((uint32)context.readLocalVariable<uint32>(context.getParameter<uint32>(8)));
	const AnyBaseValue var3((uint32)(var1.get<uint32>() + (uint32)(context.getParameter<int64>(12))));
	const AnyBaseValue var4((uint32)((uint32)context.getParameter<int64>() - var3.get<uint32>()));
	const AnyBaseValue var6((uint32)OpcodeExecUtils::safeDivide<uint32>((uint32)var4.get<uint32>(), (uint32)context.getParameter<int64>(20)));
	const AnyBaseValue var7((int16)(uint16)var6.get<int32>());
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(28), var7.get<uint16>());
}

// First occurrence: integerArithmeticTestC, line 47
static void exec_5aa27e417f55b5c8(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	context.writeValueStack<uint8>(0, (uint8)(var0.get<uint16>() == (uint16)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestD, line 54
static void exec_6a267c3c542f83f9(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var1((int32)(uint32)var0.get<int16>());
	const AnyBaseValue var3((uint32)(var1.get<uint32>() << (uint32)(context.getParameter<int64>(4) & 0x1f)));
	const AnyBaseValue var4((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(12)));
	const AnyBaseValue var5((int32)(uint32)var4.get<int16>());
	const AnyBaseValue var6((int32)OpcodeExecUtils::safeDivide<int32>((int32)var3.get<int32>(), (int32)var5.get<int32>()));
	context.writeLocalVariable<uint32>(context.getParameter<uint32>(16), var6.get<uint32>());
}

// First occurrence: integerArithmeticTestD, line 55
static void exec_98ed931ef5ec7570(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint32)context.readLocalVariable<uint32>(context.getParameter<uint32>()));
	context.writeValueStack<uint8>(0, (uint8)(var0.get<uint32>() == (uint32)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestE, line 60
static void exec_e3032fdf1db3ca29(const RuntimeOpcodeContext context)
{
	OpcodeExecUtils::writeMemory<uint8>(*context.mControlFlow, context.getParameter<int64>(8), (uint8)context.getParameter<int64>());
}

// First occurrence: integerArithmeticTestE, line 61
static void exec_ad7fc214f6f1eddd(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint32)(uint32)context.getParameter<int64>());
	const AnyBaseValue var1((uint8)OpcodeExecUtils::readMemory<uint8>(*context.mControlFlow, var0.get<uint32>()));
	const AnyBaseValue var3((uint8)(var1.get<uint8>() & (uint8)(context.getParameter<int64>(8))));
	OpcodeExecUtils::writeMemory<uint8>(*context.mControlFlow, var0.get<uint32>(), var3.get<uint8>());
	context.writeLocalVariable<uint8>(context.getParameter<uint32>(16), var3.get<uint8>());
}

// First occurrence: integerArithmeticTestE, line 62
static void exec_5e01ac1f3e13f0e0(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>()));
	context.writeValueStack<uint8>(0, (var0.get<uint8>() == (uint8)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestF, line 67
static void exec_129ca8a6a7bc4114(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint8>(context.getParameter<uint32>(8), (uint8)context.getParameter<int64>());
}

// First occurrence: integerArithmeticTestF, line 69
static void exec_0b5a32e4ccfe4ad2(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint16>(0, context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var0((uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>(4)));
	const AnyBaseValue var1((uint8)-(signed)var0.get<uint8>());
	context.writeValueStack<int16>(1, (uint16)var1.get<int8>());
	const AnyBaseValue var2((uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>(8)));
	context.writeValueStack<int16>(2, (uint16)var2.get<int8>());
	context.moveValueStack(3);
}

// First occurrence: integerArithmeticTestF, line 69
static void exec_fc64b3bf8227da48(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(), context.readValueStack<uint16>(-1));
	context.moveValueStack(-1);
}

// First occurrence: floatArithmeticTestA, line 85
static void exec_826dcbeb35056aba(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<float>(context.getParameter<uint32>(8), (float)context.getParameter<double>());
}

// First occurrence: floatArithmeticTestA, line 86
static void exec_6e197dd5b6a2ddb6(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((float)context.readLocalVariable<float>(context.getParameter<uint32>()));
	const AnyBaseValue var1((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4)));
	const AnyBaseValue var2((float)(float)var1.get<int16>());
	const AnyBaseValue var3((float)(var0.get<float>() * var2.get<float>()));
	const AnyBaseValue var4((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(8)));
	const AnyBaseValue var5((float)(float)var4.get<int16>());
	const AnyBaseValue var6((float)context.readLocalVariable<float>(context.getParameter<uint32>(12)));
	const AnyBaseValue var7((float)(var5.get<float>() * var6.get<float>()));
	context.writeValueStack<uint8>(0, (uint8)(var3.get<float>() == var7.get<float>()));
	context.moveValueStack(1);
}

// First occurrence: floatArithmeticTestA, line 86
static void exec_357479c741322b87(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((float)context.readLocalVariable<float>(context.getParameter<uint32>()));
	const AnyBaseValue var1((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4)));
	const AnyBaseValue var2((float)(float)var1.get<int16>());
	const AnyBaseValue var3((float)(var0.get<float>() * var2.get<float>()));
	context.writeValueStack<uint8>(0, (uint8)(var3.get<float>() == (float)(context.getParameter<double>(8))));
	context.moveValueStack(1);
}

// First occurrence: floatArithmeticTestB, line 91
static void exec_e28f71091f95a0bb(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint8>(0, (uint8)(context.getParameter<double>() == context.getParameter<double>(8)));
	context.moveValueStack(1);
}

// First occurrence: main, line 23
static void exec_9bfd3f9a64512be9(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint64>(0, 0ull);
	context.writeValueStack<int64>(1, (uint64)context.getParameter<int64>());
	context.moveValueStack(2);
}


void createNativizedCodeLookup(Nativizer::LookupDictionary& dict)
{
	const char emptyEntries0[] =
	{
		"\xb1\xcf\x75\x0a\x79\xfa\x43\xde\xf4\xa3\xdf\xf6\xc4\xc8\x08\xb7\xc7\x71\x02\xb9\xe3\x2c\x56\x36\x65\x49\x3d\x46\xfd\xbf\x64\x61\x02\xb9\xcc\x09\x79\x81\x4c\x96\x61\x41\x5d\x44\x4a\x92\x20\xcb\xfb\x98\xb9\x19\xd1\xe8\x5f\xed\x63\x1b\x32\xd4\x82\x53\x9c\xc9\xd6\xd3\x02\x6a\x18\x92\x4f\x9b\x15\x95\x87\x4b\x37\xe8\x2d\x8d\x97\x2d\xaf\x95\x0b\xe2\xe9\x07\x3a\x38\x2f\xd4\x82\xed\x98\xc9\x5a\x4c\x62\x95\x8a\x1f\xd5\xc9\xd4\xaf\xf1\xc3\x57\xef\x33\x44\x6d\xa2\x5e\x97\xa3\x53\x65\x95\x23\x02\x1b\x8b\x7e\x0c\xcc\xc2"
		"\x02\x63\x4c\x89\x87\xfe\x1b\x25\x3a\x93\x89\x67\x00\x30\x14\x7b\xfd\x19\x92\x90\x34\xee\x94\x8c\x72\x11\x56\x4a\xb4\xaa\xf5\x54\xcf\xb4\xc9\x08\xa9\x15\xeb\xcf\x37\xc4\x61\x7c\x54\x33\xe7\xf5\x9d\x78\x02\x52\x00\xe5\x9f\xb6\x27\xbc\x53\xae\x7f\x4e\x94\x57\xbe\x9a\x26\xf8\x22\x84\x56\xc4\x09\xd8\x2d\xf3\x73\xdf\x7a\x14\xe6\xf5\x13\xa4\x7f\xf7\x1d\xa8\x93\x94\x99\x7a\x95\xd2\xf8\x2c\x29\xcb\x9a\x07\x83\xfd\xee\xc2\xd4\x9d\x5c\xf1\x01\x96\xb6\xfe\x4c\xf1\x0b\xe1\x1c\x3e\xe2\x3f\xdc\x85\x18\x62\xae\xd3\xb9\x8a"
		"\xed\x39\x00\x5d\x3c\x46\xd9\xf0\x3a\x91\x4a\xea\xf6\x45\xa5\x0d\xe0\xe0\x39\x25\x41\x26\xf8\x3d\x48\x98\x46\xcd\xfc\xde\x62\x87\xf2\xd8\xee\x28\xe8\x45\xf5\xf1\xdf\x87\x1e\x8e\x9a\xda\xf3\xe5\x23\xa2\xc0\x63\x67\x89\xd8\x38\xf0\x24\xae\xfe\x24\x23\x92\x66\x0d\xb2\x2c\x86\x66\x57\x52\x62\xdd\x1c\xde\xc2\x9b\x8d\xd4\x41\x0d\xe5\xc2\x13\x1b\x27\x86\xdb\xd0\x5b\x0e\x0a\xb1\x72\x3e\x38\x7f\x1a\xab\x2d\xd8\x75\x2a\x3b\xbc\xba\x99\x43\xb2\x7b\x7c\xe4\x43\x94\x23\x7f\xc7\x4d\x5e\x7c\xdc\x0c\x3c\x53\xd2\x8c\x17\x13"
		"\x2d\xc3\xfa\xbf\x55\xf9\xa4\xd9\xbb\x0d\x13\x51\x0e\x3d\xe0\xb5\xa5\xae\x28\xd4\xb4\xa1\x3f\x1c\x09\x1d\x44\xe6\xa8\x82\x8c\xba\x31\xc8\x58\x43\x90\x2f\x05\x77"
	};
	dict.addEmptyEntries(reinterpret_cast<const uint64*>(emptyEntries0), 0x35);

	const char parameterData[] =
	{
		"\x78\xda\x75\x90\xb1\x12\x80\x20\x0c\x43\x43\x2d\xea\xe0\xe0\xe0\xe8\xe0\xff\xff\x24\xc4\xc3\x0a\x1c\x94\x85\xd7\x36\xe9\xe5\x0a\xa4\x04\xbe\x1d\x01\xe7\x94\xed\x17\x67\x1e\x70\x7c\xac\xe4\xa2\x0f\x64\xeb\x79\xba\x65\xba\xa3\xb0\xe0\x42\xc4\xcd\xea\x69\x3c\xd5\x2f\x64\xa5\x7f\xa3\x6e\x9c\xf7\xf9\xde\xdc\xd1\xd5\xf5\x99\xb4\xa9\xab\x7f\xd4\xac\xff\x7e\xeb\x8d\xb7\x29\xb7\xc8\x5c\xff\x3a\xa1"
	};
	dict.loadParameterInfo(reinterpret_cast<const uint8*>(parameterData), 0x0062);

	const Nativizer::CompactFunctionEntry functionList[] =
	{
		{ 0xcb2baa5aec3ae364, &exec_cb2baa5aec3ae364, 0x00000001 },
		{ 0x471a710265fff6cd, &exec_471a710265fff6cd, 0x00000009 },
		{ 0x2c6a9db0533dc490, &exec_2c6a9db0533dc490, 0x00000007 },
		{ 0x5e01ac1f3e13f0e0, &exec_5e01ac1f3e13f0e0, 0x0000003b },
		{ 0x77a1839043dd998f, &exec_77a1839043dd998f, 0x00000004 },
		{ 0x1e99bb14ecac1b69, &exec_1e99bb14ecac1b69, 0x0000000c },
		{ 0x0b5a32e4ccfe4ad2, &exec_0b5a32e4ccfe4ad2, 0x00000041 },
		{ 0xf198f4801343acae, &exec_f198f4801343acae, 0x0000000f },
		{ 0xd30d2a0ed02b013b, &exec_d30d2a0ed02b013b, 0x00000012 },
		{ 0x88dee4fe77b63531, &exec_88dee4fe77b63531, 0x00000015 },
		{ 0xf3548d4e78066474, &exec_f3548d4e78066474, 0x00000020 },
		{ 0x5187dc7e6e5a09c7, &exec_5187dc7e6e5a09c7, 0x00000017 },
		{ 0xe3032fdf1db3ca29, &exec_e3032fdf1db3ca29, 0x00000034 },
		{ 0x477cd77d3cd1e726, &exec_477cd77d3cd1e726, 0x00000019 },
		{ 0xa73019784d9605d0, &exec_a73019784d9605d0, 0x0000001c },
		{ 0x129ca8a6a7bc4114, &exec_129ca8a6a7bc4114, 0x0000003e },
		{ 0xc8451f7e094b32c7, &exec_c8451f7e094b32c7, 0x00000023 },
		{ 0x5aa27e417f55b5c8, &exec_5aa27e417f55b5c8, 0x00000029 },
		{ 0x6a267c3c542f83f9, &exec_6a267c3c542f83f9, 0x0000002c },
		{ 0x98ed931ef5ec7570, &exec_98ed931ef5ec7570, 0x00000031 },
		{ 0xad7fc214f6f1eddd, &exec_ad7fc214f6f1eddd, 0x00000037 },
		{ 0xfc64b3bf8227da48, &exec_fc64b3bf8227da48, 0x00000045 },
		{ 0x826dcbeb35056aba, &exec_826dcbeb35056aba, 0x00000047 },
		{ 0x6e197dd5b6a2ddb6, &exec_6e197dd5b6a2ddb6, 0x0000004a },
		{ 0x357479c741322b87, &exec_357479c741322b87, 0x0000004f },
		{ 0xe28f71091f95a0bb, &exec_e28f71091f95a0bb, 0x00000053 },
		{ 0x9bfd3f9a64512be9, &exec_9bfd3f9a64512be9, 0x00000056 }
	};
	dict.loadFunctions(functionList, 0x001b);
}
