#define NATIVIZED_CODE_AVAILABLE

// First occurrence: main, line 21
static void exec_77af1b9043e92633(const RuntimeOpcodeContext context)
{
	context.writeValueStack<double>(0, context.getParameter<double>());
	context.writeValueStack<uint64>(1, context.getParameter<uint64>(8));
	context.moveValueStack(2);
}

// First occurrence: main, line 23
static void exec_cb39425aec467008(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, context.getParameter<float>());
	context.writeValueStack<uint64>(1, context.getParameter<uint64>(8));
	context.moveValueStack(2);
}

// First occurrence: main, line 28
static void exec_ef80c579a30de2f7(const RuntimeOpcodeContext context)
{
	context.writeLocalVariable<float>(context.getParameter<uint32>(8), (float)context.getParameter<double>());
}

// First occurrence: main, line 29
static void exec_9b1b31a55d570965(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, (float)context.readLocalVariable<float>(context.getParameter<uint32>()));
	context.writeValueStack<uint64>(1, context.getParameter<uint64>(4));
	context.moveValueStack(2);
}

// First occurrence: main, line 30
static void exec_1ba20d4c0e689ba4(const RuntimeOpcodeContext context)
{
	const float var0 = (float)context.readLocalVariable<float>(context.getParameter<uint32>());
	const float var1 = (float)context.readLocalVariable<float>(context.getParameter<uint32>(4));
	const float var4 = OpcodeExecUtils::safeDivide<float>((float)var1, (float)context.getParameter<int64>(8));
	context.writeValueStack<float>(0, ((float)(var0) + (float)(var4)));
	context.writeValueStack<uint64>(1, context.getParameter<uint64>(16));
	context.moveValueStack(2);
}

// First occurrence: main, line 36
static void exec_12f50d5b588ab9ab(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, (float)context.getParameter<int16>());
	context.moveValueStack(1);
}

// First occurrence: main, line 37
static void exec_815870eaf2ebe792(const RuntimeOpcodeContext context)
{
	context.writeValueStack<float>(0, ((float)(context.getParameter<int64>()) + (float)(context.getParameter<float>(8))));
	context.moveValueStack(1);
}


void createNativizedCodeLookup(Nativizer::LookupDictionary& dict)
{
	const char emptyEntries0[] =
	{
		"\xcd\x68\x5c\xcf\x79\xe7\xe5\xd9\x93\xf3\x3d\x9d\x71\xd4\xfb\x1d\x5c\xdf\xd8\xe4\x60\xeb\x5b\x21\x4d\x31\x77\x61\x74\x8e\x6c\x38\x55\xe2\xb0\x24\xd0\x17\xa5\x0f\xee\x31\x43\x3a\x8f\xc6\x2d\x53\x46\x79\x05\x19\xaf\xb6\x01\xf8\xad\xe4\x00\x64\x04\x8f\xaa\xd1\x2a\x16\x5e\x92\x96\x7a\x1f\xc4"
	};
	dict.addEmptyEntries(reinterpret_cast<const uint64*>(emptyEntries0), 0x09);

	const char parameterData[] =
	{
		"\x78\xda\x63\x60\xf8\xff\x9f\x01\x08\x38\x18\x18\x19\x04\xb0\xb2\x99\x18\x78\xa0\x6c\x16\xa0\x38\x32\x1b\x24\x27\xc0\xc0\xc6\x20\x01\x57\x8b\xd0\x03\xd2\x0f\x00\xf3\xe2\x10\xa9"
	};
	dict.loadParameterInfo(reinterpret_cast<const uint8*>(parameterData), 0x002c);

	const Nativizer::CompactFunctionEntry functionList[] =
	{
		{ 0xcb39425aec467008, &exec_cb39425aec467008, 0x00000004 },
		{ 0xef80c579a30de2f7, &exec_ef80c579a30de2f7, 0x00000007 },
		{ 0x77af1b9043e92633, &exec_77af1b9043e92633, 0x00000001 },
		{ 0x9b1b31a55d570965, &exec_9b1b31a55d570965, 0x0000000a },
		{ 0x1ba20d4c0e689ba4, &exec_1ba20d4c0e689ba4, 0x0000000d },
		{ 0x12f50d5b588ab9ab, &exec_12f50d5b588ab9ab, 0x00000012 },
		{ 0x815870eaf2ebe792, &exec_815870eaf2ebe792, 0x00000014 }
	};
	dict.loadFunctions(functionList, 0x0007);
}
