/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


#define DEFLATE_HASHING_SIZE 0x8000

namespace
{
	static const constexpr int LIT_TABLE[29][2] =
	{
		{ 0,3 },  { 0,4 },  { 0,5 },  { 0,6 },  { 0,7 },  { 0,8 },  { 0,9 },  { 0,10 }, { 1,11 },  { 1,13 },  { 1,15 },  { 1,17 },  { 2,19 },  { 2,23 },  { 2,27 },
		{ 2,31 }, { 3,35 }, { 3,43 }, { 3,51 }, { 3,59 }, { 4,67 }, { 4,83 }, { 4,99 }, { 4,115 }, { 5,131 }, { 5,163 }, { 5,195 }, { 5,227 }, { 0,258 }
	};

	static const constexpr int DIST_TABLE[30][2] =
	{
		{ 0,1 },    { 0,2 },     { 0,3 },     { 0,4 },     { 1,5 },     { 1,7 },     { 2,9 },     { 2,13 },     { 3,17 },     { 3,25 },
		{ 4,33 },   { 4,49 },    { 5,65 },    { 5,97 },    { 6,129 },   { 6,193 },   { 7,257 },   { 7,385 },    { 8,513 },    { 8,769 },
		{ 9,1025 }, { 9,1537 }, { 10,2049 }, { 10,3073 }, { 11,4097 }, { 11,6145 }, { 12,8193 }, { 12,12289 }, { 13,16385 }, { 13,24577 }
	};
}



class DeflateCodec
{
public:
	DeflateCodec();
	~DeflateCodec();

	uint8* decode(int& outsize, const void* input, int length);
	uint8* encode(int& outsize, const void* input, int length);

private:
	struct Tree
	{
		struct Node
		{
			int value;			// -1 for inner nodes only, otherwise >= 0
			Node* child[2] = { nullptr, nullptr };
		};
		Node mRoot;

		struct Shortcut : public Node
		{
			int bits = 0;
		};
		Shortcut mShortcuts[0x100];
	};

private:
	// Bit buffer
	void initBitbuf(uint8* ptr, int length);
	void alignBitbuf();
	void flushBitbuf();
	void fillBits();
	uint32 readBits(int count, bool inverse = false);
	void writeBits(uint32 value, int count, bool inverse = false);

	// Output
	void initOutput(int size);
	void expandOutput(int min_size);

	// Decoder
	bool createTree(Tree& tree, int count, const int* len_list);
	int readFromTree(const Tree& tree);
	bool readHuffmanTrees();

	// Encoder
	int hashCode(const uint8* triple);
	void outputLitCode(int value);
	void outputLenDist(int length, int distance);

private:
	// Bitstream
	uint8* bitmem = nullptr;
	uint8* bitmem_end = nullptr;
	uint32 bitbuf = 0;
	int bitfill = 0;

	// Output
	uint8* output = nullptr;
	int outpos = 0;
	int outsize = 0;

	// Huffman trees
	Tree lit_tree;
	Tree dist_tree;
	PodStructPool<Tree::Node, 128> mTreeNodePool;

	// Bit order inversion lookup (inverting the order of the lower 9 bits)
	int invertedBits[0x200];
};



DeflateCodec::DeflateCodec()
{
	for (int i = 0; i < 0x200; ++i)
	{
		// Invert order of bits
		int value = i;
		int newValue = 0;
		for (int i = 0; i < 9; ++i)
		{
			newValue = (newValue << 1) + (value & 1);
			value >>= 1;
		}
		invertedBits[i] = newValue;
	}
}

DeflateCodec::~DeflateCodec()
{
}

void DeflateCodec::initBitbuf(uint8* ptr, int length)
{
	bitmem = ptr;
	bitmem_end = ptr + length;
	bitbuf = 0;
	bitfill = 0;
}

void DeflateCodec::alignBitbuf()
{
	// Re-align bit buffer to previous full byte
	bitmem -= bitfill / 8;
	bitfill = 0;		// Discard unread bits
}

void DeflateCodec::flushBitbuf()
{
	// Flush remaining bits in bit buffer
	while (bitfill > 0)
	{
		RMX_ASSERT(bitmem < bitmem_end, "Invalid bit stream memory write");
		*bitmem = (uint8)bitbuf;
		++bitmem;
		bitbuf >>= 8;
		bitfill -= 8;
	}
	bitfill = 0;
}

void DeflateCodec::fillBits()
{
	while (bitfill <= 24)
	{
		if (bitmem < bitmem_end)	// After that, act like it's only zeroes
		{
			bitbuf |= ((uint32)(*bitmem) << bitfill);
		}
		bitfill += 8;
		++bitmem;	// Can reach and exceed bitmem_end
	}
}

uint32 DeflateCodec::readBits(int count, bool inverse)
{
	fillBits();

	uint32 value = (bitbuf & ((1 << count) - 1));
	bitbuf >>= count;
	bitfill -= count;
	if (inverse)
	{
		// Invert order of bits
		RMX_ASSERT(count <= 9, "Bit inversion only supports up to 9 bits");
		value = invertedBits[value] >> (9 - count);
	}
	return value;
}

void DeflateCodec::writeBits(uint32 value, int count, bool inverse)
{
	while (bitfill >= 8)
	{
		RMX_ASSERT(bitmem < bitmem_end, "Invalid bit stream memory write");
		*bitmem = (uint8)bitbuf;
		bitbuf >>= 8;
		bitfill -= 8;
		++bitmem;
	}
	if (inverse)
	{
		// Invert order of bits
		RMX_ASSERT(count <= 9, "Bit inversion only supports up to 9 bits");
		value = invertedBits[value] >> (9 - count);
	}
	bitbuf |= (value << bitfill);
	bitfill += count;
}


void DeflateCodec::initOutput(int size)
{
	// Create output memory
	output = new uint8[size];
	outsize = size;
	outpos = 0;
}

void DeflateCodec::expandOutput(int min_size)
{
	// Increase output memory if needed
	if (outsize >= min_size)
		return;
	while (outsize < min_size)
	{
		if (outsize < 1024)
			outsize = 1024;
		else
			outsize *= 2;
	}
	uint8* new_output = new uint8[outsize];
	memcpy(new_output, output, outpos);
	delete[] output;
	output = new_output;
}

bool DeflateCodec::createTree(Tree& outTree, int count, const int* len_list)
{
	// Create tree structure
	outTree.mRoot.value = -1;
	outTree.mRoot.child[0] = nullptr;
	outTree.mRoot.child[1] = nullptr;

	int tree_code[0x120];		// 0x120 is the maximum possible value for count
	int code = 0;
	int len = 1;
	bool finished = false;
	while (!finished)
	{
		for (int i = 0; i < count; ++i)
		{
			if (len_list[i] == len)
			{
				tree_code[i] = code;
				++code;
				if (code == (1 << len))
				{
					finished = true;
					break;
				}
			}
		}

		// Increase code length
		++len;
		code *= 2;
		if (len > 20)
			return false;	// Error
	}

	// Now for the tree itself
	for (int i = 0; i < count; ++i)
	{
		len = len_list[i];
		if (len == 0)
			continue;

		Tree::Node* node = &outTree.mRoot;
		for (int j = 0; j < len; ++j)
		{
			const int side = (tree_code[i] >> (len-j-1)) & 1;
			if (nullptr == node->child[side])
			{
				node->child[side] = &mTreeNodePool.allocObject();
				node->child[side]->value = -1;
				node->child[side]->child[0] = nullptr;
				node->child[side]->child[1] = nullptr;
			}
			node = node->child[side];
		}
		node->value = i;
	}

	// Build shortcuts
	for (int i = 0; i < 0x100; ++i)
	{
		Tree::Shortcut& shortcut = outTree.mShortcuts[i];
		int buffer = i;
		const Tree::Node* current = &outTree.mRoot;
		shortcut.bits = 0;
		while (current->value == -1 && shortcut.bits < 8)
		{
			current = current->child[buffer & 1];
			RMX_ASSERT(nullptr != current, "Error in tree traversal");
			buffer >>= 1;
			++shortcut.bits;
		}

		shortcut.value = current->value;
		shortcut.child[0] = current->child[0];
		shortcut.child[1] = current->child[1];
	}
	return true;
}

int DeflateCodec::readFromTree(const Tree& tree)
{
	fillBits();
	const Tree::Shortcut& shortcut = tree.mShortcuts[bitbuf & 0xff];
	bitbuf >>= shortcut.bits;	// Consume bits
	bitfill -= shortcut.bits;

	if (shortcut.value != -1)
	{
		// Shortcut has a value, so code is not longer than 8 bits
		return shortcut.value;
	}
	else
	{
		// Shortcut covered only the first 8 bits, but there's still more
		fillBits();
		const Tree::Node* current = &shortcut;
		while (current->value == -1)
		{
			current = current->child[bitbuf & 1];
			bitbuf >>= 1;
			--bitfill;
		}
		return current->value;
	}
}

bool DeflateCodec::readHuffmanTrees()
{
	// Decode Huffman trees
	mTreeNodePool.clear();

	const int bits = readBits(14);
	const int hlit  = (bits & 0x1f) + 257;
	const int hdist = ((bits >> 5) & 0x1f) + 1;
	const int hclen = ((bits >> 10) & 0x0f) + 4;

	int code_len[19];
	for (int i = 0; i < 19; ++i)
		code_len[i] = 0;

	const int index[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	for (int i = 0; i < hclen; ++i)
		code_len[index[i]] = readBits(3);

	Tree length_tree;
	if (!createTree(length_tree, 19, code_len))
		return false;

	// Codes for both alphabets (literal/length and distance)
	int prev = 0;
	for (int num = 0; num < 2; ++num)
	{
		int list[288];
		const int list_size = (num == 0) ? hlit : hdist;

		// Read length codes
		for (int i = 0; i < list_size; ++i)
		{
			const int code = readFromTree(length_tree);
			if (code == -1)
				return false;

			// Evaluate code
			if (code <= 15)
			{
				list[i] = code;
				prev = code;
				continue;
			}
			if (code == 16)
			{
				const int cnt = 3 + readBits(2);
				for (int j = 0; j < cnt; ++j)
					list[i+j] = prev;
				i += (cnt-1);
				continue;
			}

			// Code 17 or 18
			const int cnt = (code == 17) ? (3 + readBits(3)) : (11 + readBits(7));
			for (int j = 0; j < cnt; ++j)
				list[i+j] = 0;
			i += (cnt-1);
		}

		// Build tree
		Tree& tree = (num == 0) ? lit_tree : dist_tree;
		createTree(tree, list_size, list);
	}
	return true;
}

#define RETURN_ERROR  { delete[] output; return nullptr; }

uint8* DeflateCodec::decode(int& outputSize, const void* input, int length)
{
	// Restore compressed data
	initBitbuf(static_cast<uint8*>(const_cast<void*>(input)), length);
	initOutput(length * 2);	// Just a rough guess
	uint8 buffer[258];

	// Read blocks
	bool finished = false;
	while (!finished)
	{
		// Block header
		const int bits = readBits(3);
		if (bits & 0x01)		// "bfinal" flag
			finished = true;

		const int btype = bits >> 1;
		if (btype == 3)			// Invalid value for btype
			RETURN_ERROR;

		// Uncompressed data?
		if (btype == 0)
		{
			alignBitbuf();		// Discard remaining bits of current byte
			unsigned short len = *(unsigned short*)&bitmem[0];
			//unsigned short nlen = *(unsigned short*)&bitmem[2];
			expandOutput(outpos + len);
			memcpy(&output[outpos], &bitmem[4], len);
			outpos += len;
			continue;
		}

		// Compressed data block
		if (btype == 2)
		{
			if (!readHuffmanTrees())	// Error
				RETURN_ERROR;
		}

		bool eob = false;		// End of block
		while (!eob)
		{
			// Read length/literal code
			int lit_code;
			if (btype == 1)
			{
				int bits = readBits(7, true);
				if (bits < 0x18)
				{
					lit_code = 256 + bits;					// Codes 256 - 279
				}
				else if (bits < 0x64)
				{
					bits = (bits << 1) + readBits(1);
					if (bits < 0xc0)
						lit_code = bits - 0x30;				// Codes 0 - 143
					else
						lit_code = 280 + bits - 0xc0;		// Codes 280 - 287
				}
				else
				{
					bits = (bits << 2) + readBits(2, true);
					lit_code = 144 + bits - 0x190;			// Codes 144 - 255
				}
			}
			else
			{
				lit_code = readFromTree(lit_tree);
			}

			// Evaluate code
			if (lit_code <= 255)
			{
				// Output literal
				expandOutput(outpos + 1);
				output[outpos] = (uint8)lit_code;
				++outpos;
			}
			else if (lit_code == 256)
			{
				// Reached end of block
				eob = true;
			}
			else if (lit_code < 286)
			{
				// Look up length in table
				int extra_bits = LIT_TABLE[lit_code-257][0];
				int length = LIT_TABLE[lit_code-257][1];
				if (extra_bits)
					length += readBits(extra_bits);

				// Get distance
				const int dist_code = (btype == 1) ? readBits(5, true) : readFromTree(dist_tree);
				if (dist_code >= 30)
					RETURN_ERROR;

				extra_bits = DIST_TABLE[dist_code][0];
				int distance = DIST_TABLE[dist_code][1];
				if (extra_bits)
					distance += readBits(extra_bits);

				// Copy output from further behind
				expandOutput(outpos + length);
				const uint8* src = &output[outpos - distance];
				if (length > distance)
				{
					memcpy(buffer, src, distance);
					int fill = distance;
					while (fill < length)
					{
						const int copy_len = std::min(fill, length - fill);
						memcpy(&buffer[fill], buffer, copy_len);
						fill += copy_len;
					}
					src = buffer;
				}
				memcpy(&output[outpos], src, length);
				outpos += length;
			}
			else  // lit_code >= 286
			{
				RETURN_ERROR;
			}
		}
	}
	outputSize = outsize;
	return output;
}

int DeflateCodec::hashCode(const uint8* triple)
{
	// Convert hash code in range [0, 16383] to a byte triple
	return ((triple[0] >> 3) + triple[1] + (triple[2] << 3) + (triple[0] << 6)) % DEFLATE_HASHING_SIZE;
}

void DeflateCodec::outputLitCode(int value)
{
	// Output length/literal code
	if (value < 144)
		writeBits(value + 0x30, 8, true);
	else if (value < 256)
		writeBits(value - 144 + 0x190, 9, true);
	else if (value < 280)
		writeBits(value - 256, 7, true);
	else
		writeBits(value - 280 + 0xc0, 8, true);
}

void DeflateCodec::outputLenDist(int length, int distance)
{
	int lc = 0;
	for (int i = 1; i < 29; ++i)
	{
		if (length < LIT_TABLE[i][1])
			break;
		++lc;
	}

	// Output length/distance combination and additional bits
	outputLitCode(257 + lc);
	int bit_count = LIT_TABLE[lc][0];
	if (bit_count)
		writeBits(length - LIT_TABLE[lc][1], bit_count);

	// Go on with distance
	int dc = 0;
	for (int i = 1; i < 30; ++i)
	{
		if (distance < DIST_TABLE[i][1])
			break;
		++dc;
	}

	// Output distance code and additional bits
	writeBits(dc, 5, true);
	bit_count = DIST_TABLE[dc][0];
	if (bit_count)
		writeBits(distance - DIST_TABLE[dc][1], bit_count);
}

uint8* DeflateCodec::encode(int& outputSize, const void* input, int length)
{
	// Compress data
	const uint8* data = (const uint8*)input;
	initOutput(length + 100);
	initBitbuf(output, outsize);

	int hash_table[DEFLATE_HASHING_SIZE];	// Position of most recent occurrence for each hash code
	int hash_queue[0x8000];					// Cyclic queue: Reference to previous occurrence
	for (int i = 0; i < DEFLATE_HASHING_SIZE; ++i)
		hash_table[i] = -1;

	// Write header
	writeBits(1, 1);		// bfinal = 1
	writeBits(1, 2);		// btype = 01  (i.e. use fixed Huffman codes)

	int pos = 0;
	while (pos < length)
	{
		int best_len = 1;
		int best_dist = 0;
		int hc = hashCode(data + pos);
		int position = hash_table[hc];
		int dist_limit = 32768;
		while (position >= 0 && best_len < 258)
		{
			// Abort if entry is "out-dated"
			int dist = (pos - position);
			if (dist > dist_limit)
				break;

			// Get number of identical bytes
			int count = 0;
			while (count < 258 && pos+count < length && data[pos+count] == data[position+count])
				++count;

			// Is that sequence longer that the maximum so far?
			if (count >= 3 && count > best_len)
			{
				best_len = count;
				best_dist = dist;

				// Performance optimization
				dist_limit = 32768;
				for (int i = 16; best_len >= i; i *= 2)
					dist_limit /= 2;
			}

			// Continue with next entry
			position = hash_queue[position & 0x7fff];
		}

		// Output result so far
		if (best_len == 1)
			outputLitCode(data[pos]);
		else
			outputLenDist(best_len, best_dist);

		// Go on and update hash table / queue
		for (int i = 0; i < best_len; ++i)
		{
			hc = hashCode(data + pos);
			hash_queue[pos & 0x7fff] = hash_table[hc];
			hash_table[hc] = pos;
			++pos;
		}
	}

	// Flush bit buffer
	outputLitCode(256);
	flushBitbuf();
	outputSize = (int)(bitmem - output);
	return output;
}



uint8* Deflate::decode(int& outputSize, const void* input, int length)
{
	DeflateCodec codec;
	return codec.decode(outputSize, input, length);
}

uint8* Deflate::encode(int& outputSize, const void* input, int length)
{
	DeflateCodec codec;
	return codec.encode(outputSize, input, length);
}
