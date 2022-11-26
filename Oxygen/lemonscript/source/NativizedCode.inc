#define NATIVIZED_CODE_AVAILABLE

// First occurrence: runTests, line 4
static void exec_1e99bb14ecac1b69(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint64>(0, context.getParameter<uint64>());
	context.writeValueStack<int64>(1, context.getParameter<int64>(8));
	context.moveValueStack(2);
}

// First occurrence: integerArithmeticTestA, line 29
static void exec_f198f4801343acae(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(8), (uint16)context.getParameter<int16>());
}

// First occurrence: integerArithmeticTestA, line 29
static void exec_d30d2a0ed02b013b(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint16>(0, context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4)));
	const AnyBaseValue var1((int64)0);
	context.writeValueStack<int16>(1, ((int16)(var0.get<int16>()) < (int16)(var1.get<int16>())));
	context.moveValueStack(2);
}

// First occurrence: integerArithmeticTestA, line 29
static void exec_88dee4fe77b63531(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)((uint16)(context.readValueStack<uint16>(-2)) + (uint16)(context.readValueStack<uint16>(-1))));
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(), (uint16)var0.get<uint16>());
	context.moveValueStack(-2);
}

// First occurrence: integerArithmeticTestA, line 29
static void exec_5187dc7e6e5a09c7(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var1((int64)1);
	context.writeValueStack<uint16>(0, ((uint16)(var0.get<uint16>()) == (uint16)(var1.get<uint16>())));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestB, line 36
static void exec_477cd77d3cd1e726(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(8), (uint16)context.getParameter<uint16>());
}

// First occurrence: integerArithmeticTestB, line 36
static void exec_a73019784d9605d0(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var1((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4)));
	const AnyBaseValue var2((uint16)((uint16)(var0.get<uint16>()) + (uint16)(var1.get<uint16>())));
	context.writeValueStack<uint16>(0, ((uint16)(var2.get<uint16>()) == (uint16)(context.getParameter<int64>(8))));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestC, line 43
static void exec_f3548d4e78066474(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint32>(context.getParameter<uint32>(8), (uint32)context.getParameter<uint32>());
}

// First occurrence: integerArithmeticTestC, line 43
static void exec_dc126f45d70411fd(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var1((uint32)context.readLocalVariable<uint32>(context.getParameter<uint32>(8)));
	const AnyBaseValue var3((uint32)((uint32)(var1.get<uint32>()) + (uint32)(context.getParameter<uint16>(12))));
	const AnyBaseValue var4((uint32)((uint32)(context.getParameter<uint16>()) - (uint32)(var3.get<uint32>())));
	const AnyBaseValue var6((uint32)OpcodeExecUtils::safeDivide<uint32>((uint32)var4.get<uint32>(), (uint32)context.getParameter<uint16>(20)));
	const AnyBaseValue var7((int16)(uint16)var6.get<int16>());
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(28), (uint16)var7.get<uint16>());
}

// First occurrence: integerArithmeticTestC, line 43
static void exec_5aa27e417f55b5c8(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	context.writeValueStack<uint16>(0, ((uint16)(var0.get<uint16>()) == (uint16)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestD, line 50
static void exec_fc6ec5f5271cac15(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var1((int16)(uint16)var0.get<int16>());
	const AnyBaseValue var3((uint32)((uint32)(var1.get<uint32>()) << (uint32)((context.getParameter<int32>(4)) & 0x1f)));
	const AnyBaseValue var4((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(12)));
	const AnyBaseValue var5((int16)(uint16)var4.get<int16>());
	const AnyBaseValue var6((int32)OpcodeExecUtils::safeDivide<int32>((int32)var3.get<int32>(), (int32)var5.get<int32>()));
	context.writeLocalVariable<uint32>(context.getParameter<uint32>(16), (uint32)var6.get<uint32>());
}

// First occurrence: integerArithmeticTestD, line 50
static void exec_98ed931ef5ec7570(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint32)context.readLocalVariable<uint32>(context.getParameter<uint32>()));
	context.writeValueStack<uint32>(0, ((uint32)(var0.get<uint32>()) == (uint32)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestE, line 58
static void exec_e3032fdf1db3ca29(const RuntimeOpcodeContext context)
{
	OpcodeExecUtils::writeMemory<uint8>(*context.mControlFlow, context.getParameter<uint32>(8), (uint8)context.getParameter<uint8>());
}

// First occurrence: integerArithmeticTestE, line 58
static void exec_ad7fc214f6f1eddd(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint32)context.getParameter<uint32>());
	const AnyBaseValue var1((uint8)OpcodeExecUtils::readMemory<uint8>(*context.mControlFlow, var0.get<uint32>()));
	const AnyBaseValue var3((uint8)((uint8)(var1.get<uint8>()) & (uint8)(context.getParameter<uint8>(8))));
	OpcodeExecUtils::writeMemory<uint8>(*context.mControlFlow, var0.get<uint32>(), (uint8)var3.get<uint8>());
	context.writeLocalVariable<uint8>(context.getParameter<uint32>(16), (uint8)var3.get<uint8>());
}

// First occurrence: integerArithmeticTestE, line 58
static void exec_5e01ac1f3e13f0e0(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>()));
	context.writeValueStack<uint8>(0, ((uint8)(var0.get<uint8>()) == (uint8)(context.getParameter<int64>(4))));
	context.moveValueStack(1);
}

// First occurrence: integerArithmeticTestF, line 65
static void exec_129ca8a6a7bc4114(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint8>(context.getParameter<uint32>(8), (uint8)context.getParameter<int8>());
}

// First occurrence: integerArithmeticTestF, line 65
static void exec_5630085f01f056aa(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint16>(0, context.readLocalVariable<uint16>(context.getParameter<uint32>()));
	const AnyBaseValue var0((uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>(4)));
	const AnyBaseValue var1((uint8)(uint8)-(signed)var0.get<uint8>());
	context.writeValueStack<int8>(1, (uint8)var1.get<int8>());
	const AnyBaseValue var2((uint8)context.readLocalVariable<uint8>(context.getParameter<uint32>(8)));
	context.writeValueStack<int8>(2, (uint8)var2.get<int8>());
	context.moveValueStack(3);
}

// First occurrence: integerArithmeticTestF, line 65
static void exec_fc64b3bf8227da48(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<uint16>(context.getParameter<uint32>(), (uint16)context.readValueStack<uint16>(-1));
	context.moveValueStack(-1);
}

// First occurrence: floatArithmeticTestA, line 82
static void exec_b396655068a492ff(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<float>(context.getParameter<uint32>(8), (float)context.getParameter<double>());
}

// First occurrence: floatArithmeticTestA, line 82
static void exec_15314bea74480458(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((float)context.readLocalVariable<float>(context.getParameter<uint32>()));
	const AnyBaseValue var1((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4)));
	const AnyBaseValue var2((float)(float)var1.get<float>());
	const AnyBaseValue var3((float)((float)(var0.get<float>()) * (float)(var2.get<float>())));
	const AnyBaseValue var4((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(8)));
	const AnyBaseValue var5((float)(float)var4.get<float>());
	const AnyBaseValue var6((float)context.readLocalVariable<float>(context.getParameter<uint32>(12)));
	const AnyBaseValue var7((float)((float)(var5.get<float>()) * (float)(var6.get<float>())));
	context.writeValueStack<float>(0, ((float)(var3.get<float>()) == (float)(var7.get<float>())));
	context.moveValueStack(1);
}

// First occurrence: floatArithmeticTestA, line 82
static void exec_5be8525ff79a86dd(const RuntimeOpcodeContext context)
{
	const AnyBaseValue var0((float)context.readLocalVariable<float>(context.getParameter<uint32>()));
	const AnyBaseValue var1((uint16)context.readLocalVariable<uint16>(context.getParameter<uint32>(4)));
	const AnyBaseValue var2((float)(float)var1.get<float>());
	const AnyBaseValue var3((float)((float)(var0.get<float>()) * (float)(var2.get<float>())));
	context.writeValueStack<float>(0, ((float)(var3.get<float>()) == (float)(context.getParameter<double>(8))));
	context.moveValueStack(1);
}

// First occurrence: floatArithmeticTestB, line 91
static void exec_e28f71091f95a0bb(const RuntimeOpcodeContext context)
{
	context.writeValueStack<double>(0, ((double)(context.getParameter<double>()) == (double)(context.getParameter<double>(8))));
	context.moveValueStack(1);
}

// First occurrence: main, line 32
static void exec_cb2baa5aec3ae364(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, context.getParameter<float>());
	context.writeValueStack<int64>(1, context.getParameter<int64>(8));
	context.moveValueStack(2);
}

// First occurrence: main, line 32
static void exec_77a1839043dd998f(const RuntimeOpcodeContext context)
{
	context.writeValueStack<double>(0, context.getParameter<double>());
	context.writeValueStack<int64>(1, context.getParameter<int64>(8));
	context.moveValueStack(2);
}

// First occurrence: main, line 32
static void exec_2c52d3b053298e71(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, (float)context.getParameter<int16>());
	context.moveValueStack(1);
}

// First occurrence: main, line 32
static void exec_737448b82ccee872(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, ((float)(context.getParameter<int64>()) + (float)(context.getParameter<float>(8))));
	context.moveValueStack(1);
}

// First occurrence: main, line 32
static void exec_9bfd3f9a64512be9(const RuntimeOpcodeContext context)
{
	context.writeValueStack<uint64>(0, 0ull);
	context.writeValueStack<int64>(1, context.getParameter<int64>());
	context.moveValueStack(2);
}


void createNativizedCodeLookup(Nativizer::LookupDictionary& dict)
{
	const char emptyEntries0[] =
	{
		"\x02\xb9\xcc\x09\x79\x81\x4c\x96\x61\x41\x5d\x44\x4a\x92\x20\xcb\x63\x1b\x32\xd4\x82\x53\x9c\xc9\xfb\x98\xb9\x19\xd1\xe8\x5f\xed\xd6\xd3\x02\x6a\x18\x92\x4f\x9b\xe9\x3b\x12\x2a\xde\x18\xae\x1b\x97\x2d\xaf\x95\x0b\xe2\xe9\x07\x3a\x38\x2f\xd4\x82\xed\x98\xc9\x5a\x4c\x62\x95\x8a\x1f\xd5\xc9\xd4\xaf\xf1\xc3\x57\xef\x33\x44\x6d\xa2\x5e\x97\xa3\x53\x65\x95\xaf\x2f\x01\x29\x6d\x0c\xf7\xf4\x16\x95\x25\x40\x3a\xcb\x2d\x8b\x1e\x10\x40\xe9\xff\xdb\xf8\x34\x23\x02\x1b\x8b\x7e\x0c\xcc\xc2\x9c\x84\x8d\x57\xf9\x97\xde\x87"
		"\x02\x63\x4c\x89\x87\xfe\x1b\x25\x3a\x93\x89\x67\x00\x30\x14\x7b\xfd\x19\x92\x90\x34\xee\x94\x8c\xcf\xb4\xc9\x08\xa9\x15\xeb\xcf\x9d\x78\x02\x52\x00\xe5\x9f\xb6\x27\xbc\x53\xae\x7f\x4e\x94\x57\x26\xdd\xe4\x8b\xf0\xc2\x7e\xce\xbe\x9a\x26\xf8\x22\x84\x56\xc4\xe6\xf5\x13\xa4\x7f\xf7\x1d\xa8\x93\x94\x99\x7a\x95\xd2\xf8\x2c\xf8\x96\x95\x4a\xb4\x6e\x40\x55\x19\xa4\x6d\x0a\xb1\x98\xae\x38\xdc\x85\x18\x62\xae\xd3\xb9\x8a\x4f\xd8\xe3\x5f\x55\x74\x17\xd1\x8e\x2f\x6b\x48\x1d\x6b\x95\x80\xb1\xcf\x75\x0a\x79\xfa\x43\xde"
		"\xb4\xa9\xa1\xbe\x57\x1c\x95\x90\xdf\x87\x1e\x8e\x9a\xda\xf3\xe5\x23\xa2\xc0\x63\x67\x89\xd8\x38\xf0\x24\xae\xfe\x24\x23\x92\x66\x0d\xb2\x2c\x86\x66\x57\x52\x62\xc7\x71\x02\xb9\xe3\x2c\x56\x36\xdd\x1c\xde\xc2\x9b\x8d\xd4\x41\xce\xbb\xe6\x25\x4c\xf7\x9a\x55\xae\x75\x80\xc7\x2d\xd3\x35\xb0\xbc\xba\x99\x43\xb2\x7b\x7c\xe4\xbc\x4d\x05\x14\x1b\x51\xd4\xdb\x30\x50\xe4\xd9\x34\xe7\xa0\x37\xb5\x3c\x61\xf0\xce\x4e\x61\x70\xf7\x2c\x5c\x79\x30\xdf\x6e\xeb\x19\x65\x9b\x9f\x41\x70\x13\x16\x3f\x2e\xca\x83\x38\xab\x01\xf0"
		"\xb0\xa3\x96\x73\xa4\xbf\x4e\x64\x08\xee\x1b\xaa\x07\xf1\x9b\x47\x31\xc8\x58\x43\x90\x2f\x05\x77\x27\x34\xd1\xf6\xc4\xca\xf7\xb6\x0c\x77\xf6\xbd\x4a\x83\x88\x3b"
	};
	dict.addEmptyEntries(reinterpret_cast<const uint64*>(emptyEntries0), 0x35);

	const char parameterData[] =
	{
		"\x78\xda\x75\x90\xb1\x12\x80\x20\x0c\x43\x03\x16\x65\x70\x60\x70\x74\xf0\xff\x7f\x12\xa3\x88\x94\x83\x32\xbd\x86\xa4\x97\x2b\x90\x33\xf8\x22\x1c\x92\xe2\xfd\x63\x21\xc7\x97\x1d\xb9\x6a\x96\x6f\x99\xee\x28\xec\x71\x20\xe0\xe4\x74\xa9\x4c\xcb\x7b\xb2\x30\xbf\xd1\x37\xfe\xf7\xfd\x3c\x39\x98\xbe\xbe\x93\xa8\xb9\xe5\x47\xcf\xfa\xef\xaf\x5a\x32\x6e\x33\x67\xdd\xad\xdc\xeb\xd1\x6e\xba\x95\x3a\xa1"
	};
	dict.loadParameterInfo(reinterpret_cast<const uint8*>(parameterData), 0x0062);

	const Nativizer::CompactFunctionEntry functionList[] =
	{
		{ 0x1e99bb14ecac1b69, &exec_1e99bb14ecac1b69, 0x00000001 },
		{ 0xf198f4801343acae, &exec_f198f4801343acae, 0x00000004 },
		{ 0x88dee4fe77b63531, &exec_88dee4fe77b63531, 0x0000000a },
		{ 0xd30d2a0ed02b013b, &exec_d30d2a0ed02b013b, 0x00000007 },
		{ 0xf3548d4e78066474, &exec_f3548d4e78066474, 0x00000015 },
		{ 0x5187dc7e6e5a09c7, &exec_5187dc7e6e5a09c7, 0x0000000c },
		{ 0xe3032fdf1db3ca29, &exec_e3032fdf1db3ca29, 0x00000029 },
		{ 0x477cd77d3cd1e726, &exec_477cd77d3cd1e726, 0x0000000e },
		{ 0xa73019784d9605d0, &exec_a73019784d9605d0, 0x00000011 },
		{ 0x5630085f01f056aa, &exec_5630085f01f056aa, 0x00000036 },
		{ 0x129ca8a6a7bc4114, &exec_129ca8a6a7bc4114, 0x00000033 },
		{ 0xdc126f45d70411fd, &exec_dc126f45d70411fd, 0x00000018 },
		{ 0x5aa27e417f55b5c8, &exec_5aa27e417f55b5c8, 0x0000001e },
		{ 0xcb2baa5aec3ae364, &exec_cb2baa5aec3ae364, 0x0000004b },
		{ 0xfc6ec5f5271cac15, &exec_fc6ec5f5271cac15, 0x00000021 },
		{ 0x98ed931ef5ec7570, &exec_98ed931ef5ec7570, 0x00000026 },
		{ 0xad7fc214f6f1eddd, &exec_ad7fc214f6f1eddd, 0x0000002c },
		{ 0x5e01ac1f3e13f0e0, &exec_5e01ac1f3e13f0e0, 0x00000030 },
		{ 0xfc64b3bf8227da48, &exec_fc64b3bf8227da48, 0x0000003a },
		{ 0xb396655068a492ff, &exec_b396655068a492ff, 0x0000003c },
		{ 0x15314bea74480458, &exec_15314bea74480458, 0x0000003f },
		{ 0x5be8525ff79a86dd, &exec_5be8525ff79a86dd, 0x00000044 },
		{ 0xe28f71091f95a0bb, &exec_e28f71091f95a0bb, 0x00000048 },
		{ 0x77a1839043dd998f, &exec_77a1839043dd998f, 0x0000004e },
		{ 0x2c52d3b053298e71, &exec_2c52d3b053298e71, 0x00000051 },
		{ 0x737448b82ccee872, &exec_737448b82ccee872, 0x00000053 },
		{ 0x9bfd3f9a64512be9, &exec_9bfd3f9a64512be9, 0x00000056 }
	};
	dict.loadFunctions(functionList, 0x001b);
}
