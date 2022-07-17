#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>

#if defined(__GNUC__) || defined(__clang__)
#define SHA256_BYTESWAP __builtin_bswap32
#elif defined(_MSC_VER)
#define SHA256_BYTESWAP _byteswap_ulong
#else
#error Could not find the built-in byteswap function, unrecognized compiler.
#endif

namespace mincemeat {

struct sha256_stream {
public:
	static constexpr int block_size = 64;
	static constexpr int hash_size = 32;

	/**
	 * @brief Stream raw bytes into the hash algorithm.
	 *
	 * Call finish() when the data entering process is over.
	 */
	sha256_stream& operator<<(std::span<const std::uint8_t> data) noexcept {
		const uint8_t* current = data.data();
		std::size_t remaining_size = data.size();

		if (m_buffer_size > 0) {
			while (remaining_size > 0 && m_buffer_size < block_size) {
				m_buffer[m_buffer_size++] = *current++;
				remaining_size--;
			}
		}

		// full buffer
		if (m_buffer_size == block_size) {
			hash_block(m_buffer.data());
			m_num_bytes += block_size;
			m_buffer_size = 0;
		}

		// no more data ?
		if (remaining_size == 0) return *this;

		// process full blocks
		while (remaining_size >= block_size) {
			hash_block(current);
			current += block_size;
			m_num_bytes += block_size;
			remaining_size -= block_size;
		}

		// keep remaining bytes in buffer
		while (remaining_size > 0) {
			m_buffer[m_buffer_size++] = *current++;
			remaining_size--;
		}

		return *this;
	}

	template<typename T>
	requires std::ranges::sized_range<T> and std::ranges::contiguous_range<T> sha256_stream& operator<<(const T& data
	) noexcept {
		namespace rg = std::ranges;

		(*this) << std::span<const uint8_t>{
			reinterpret_cast<const uint8_t*>(rg::data(data)), rg::size(data) * sizeof(rg::range_value_t<T>)};

		return *this;
	}

	/**
	 * @brief Finish the hashing process and produce a result.
	 *
	 * No more data can be streamed after this function is called. The hasher object should be discarded afterwards.
	 *
	 * @return Produced hash as an array of bytes.
	 */
	std::array<std::uint8_t, hash_size> finish() noexcept {
		// process remaining bytes
		hash_remaining();

		std::array<uint8_t, hash_size> result;
		uint8_t* current = result.data();

		for (int i = 0; i < hash_value_size; i++) {
			*current++ = (m_hash[i] >> 24) & 0xFF;
			*current++ = (m_hash[i] >> 16) & 0xFF;
			*current++ = (m_hash[i] >> 8) & 0xFF;
			*current++ = m_hash[i] & 0xFF;
		}

		return result;
	}

private:
	/// Process a single block of 64 bytes from the supplied pointer.
	void hash_block(const std::uint8_t* data) noexcept {
		auto rotate = [](std::uint32_t a, std::uint32_t c) -> std::uint32_t { return (a >> c) | (a << (32 - c)); };
		auto byteswap = [](std::uint32_t x) -> std::uint32_t { return SHA256_BYTESWAP(x); };

		// mix functions for hash_block()
		auto f1 = [&rotate](std::uint32_t e, std::uint32_t f, std::uint32_t g) -> std::uint32_t {
			std::uint32_t term1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
			std::uint32_t term2 = (e & f) ^ (~e & g);  //(g ^ (e & (f ^ g)))
			return term1 + term2;
		};

		auto f2 = [&rotate](std::uint32_t a, std::uint32_t b, std::uint32_t c) -> std::uint32_t {
			std::uint32_t term1 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
			std::uint32_t term2 = ((a | b) & c) | (a & b);  //(a & (b ^ c)) ^ (b & c);
			return term1 + term2;
		};

		// get last hash
		std::uint32_t a = m_hash[0];
		std::uint32_t b = m_hash[1];
		std::uint32_t c = m_hash[2];
		std::uint32_t d = m_hash[3];
		std::uint32_t e = m_hash[4];
		std::uint32_t f = m_hash[5];
		std::uint32_t g = m_hash[6];
		std::uint32_t h = m_hash[7];

		// data represented as 16x 32-bit words
		const std::uint32_t* input = reinterpret_cast<const std::uint32_t*>(data);
		// convert to big endian
		std::uint32_t words[64];
		int i;
		for (i = 0; i < 16; i++) {
			if constexpr (std::endian::native == std::endian::big) {
				words[i] = input[i];
			} else {
				words[i] = byteswap(input[i]);
			}
		}
		std::uint32_t x, y;  // temporaries

		// clang-format off
		// first round
		x = h + f1(e,f,g) + 0x428a2f98 + words[ 0]; y = f2(a,b,c); d += x; h = x + y;
		x = g + f1(d,e,f) + 0x71374491 + words[ 1]; y = f2(h,a,b); c += x; g = x + y;
		x = f + f1(c,d,e) + 0xb5c0fbcf + words[ 2]; y = f2(g,h,a); b += x; f = x + y;
		x = e + f1(b,c,d) + 0xe9b5dba5 + words[ 3]; y = f2(f,g,h); a += x; e = x + y;
		x = d + f1(a,b,c) + 0x3956c25b + words[ 4]; y = f2(e,f,g); h += x; d = x + y;
		x = c + f1(h,a,b) + 0x59f111f1 + words[ 5]; y = f2(d,e,f); g += x; c = x + y;
		x = b + f1(g,h,a) + 0x923f82a4 + words[ 6]; y = f2(c,d,e); f += x; b = x + y;
		x = a + f1(f,g,h) + 0xab1c5ed5 + words[ 7]; y = f2(b,c,d); e += x; a = x + y;

		// secound round
		x = h + f1(e,f,g) + 0xd807aa98 + words[ 8]; y = f2(a,b,c); d += x; h = x + y;
		x = g + f1(d,e,f) + 0x12835b01 + words[ 9]; y = f2(h,a,b); c += x; g = x + y;
		x = f + f1(c,d,e) + 0x243185be + words[10]; y = f2(g,h,a); b += x; f = x + y;
		x = e + f1(b,c,d) + 0x550c7dc3 + words[11]; y = f2(f,g,h); a += x; e = x + y;
		x = d + f1(a,b,c) + 0x72be5d74 + words[12]; y = f2(e,f,g); h += x; d = x + y;
		x = c + f1(h,a,b) + 0x80deb1fe + words[13]; y = f2(d,e,f); g += x; c = x + y;
		x = b + f1(g,h,a) + 0x9bdc06a7 + words[14]; y = f2(c,d,e); f += x; b = x + y;
		x = a + f1(f,g,h) + 0xc19bf174 + words[15]; y = f2(b,c,d); e += x; a = x + y;

		// extend to 24 words
		for (; i < 24; i++) words[i] =
			words[i-16] + (rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7]  + (rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

		// third round
		x = h + f1(e,f,g) + 0xe49b69c1 + words[16]; y = f2(a,b,c); d += x; h = x + y;
		x = g + f1(d,e,f) + 0xefbe4786 + words[17]; y = f2(h,a,b); c += x; g = x + y;
		x = f + f1(c,d,e) + 0x0fc19dc6 + words[18]; y = f2(g,h,a); b += x; f = x + y;
		x = e + f1(b,c,d) + 0x240ca1cc + words[19]; y = f2(f,g,h); a += x; e = x + y;
		x = d + f1(a,b,c) + 0x2de92c6f + words[20]; y = f2(e,f,g); h += x; d = x + y;
		x = c + f1(h,a,b) + 0x4a7484aa + words[21]; y = f2(d,e,f); g += x; c = x + y;
		x = b + f1(g,h,a) + 0x5cb0a9dc + words[22]; y = f2(c,d,e); f += x; b = x + y;
		x = a + f1(f,g,h) + 0x76f988da + words[23]; y = f2(b,c,d); e += x; a = x + y;

		// extend to 32 words
		for (; i < 32; i++) words[i] =
			words[i-16] + (rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7]  + (rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

		// fourth round
		x = h + f1(e,f,g) + 0x983e5152 + words[24]; y = f2(a,b,c); d += x; h = x + y;
		x = g + f1(d,e,f) + 0xa831c66d + words[25]; y = f2(h,a,b); c += x; g = x + y;
		x = f + f1(c,d,e) + 0xb00327c8 + words[26]; y = f2(g,h,a); b += x; f = x + y;
		x = e + f1(b,c,d) + 0xbf597fc7 + words[27]; y = f2(f,g,h); a += x; e = x + y;
		x = d + f1(a,b,c) + 0xc6e00bf3 + words[28]; y = f2(e,f,g); h += x; d = x + y;
		x = c + f1(h,a,b) + 0xd5a79147 + words[29]; y = f2(d,e,f); g += x; c = x + y;
		x = b + f1(g,h,a) + 0x06ca6351 + words[30]; y = f2(c,d,e); f += x; b = x + y;
		x = a + f1(f,g,h) + 0x14292967 + words[31]; y = f2(b,c,d); e += x; a = x + y;

		// extend to 40 words
		for (; i < 40; i++) words[i] =
			words[i-16] + (rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7]  + (rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

		// fifth round
		x = h + f1(e,f,g) + 0x27b70a85 + words[32]; y = f2(a,b,c); d += x; h = x + y;
		x = g + f1(d,e,f) + 0x2e1b2138 + words[33]; y = f2(h,a,b); c += x; g = x + y;
		x = f + f1(c,d,e) + 0x4d2c6dfc + words[34]; y = f2(g,h,a); b += x; f = x + y;
		x = e + f1(b,c,d) + 0x53380d13 + words[35]; y = f2(f,g,h); a += x; e = x + y;
		x = d + f1(a,b,c) + 0x650a7354 + words[36]; y = f2(e,f,g); h += x; d = x + y;
		x = c + f1(h,a,b) + 0x766a0abb + words[37]; y = f2(d,e,f); g += x; c = x + y;
		x = b + f1(g,h,a) + 0x81c2c92e + words[38]; y = f2(c,d,e); f += x; b = x + y;
		x = a + f1(f,g,h) + 0x92722c85 + words[39]; y = f2(b,c,d); e += x; a = x + y;

		// extend to 48 words
		for (; i < 48; i++) words[i] =
			words[i-16] + (rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7]  + (rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

		// sixth round
		x = h + f1(e,f,g) + 0xa2bfe8a1 + words[40]; y = f2(a,b,c); d += x; h = x + y;
		x = g + f1(d,e,f) + 0xa81a664b + words[41]; y = f2(h,a,b); c += x; g = x + y;
		x = f + f1(c,d,e) + 0xc24b8b70 + words[42]; y = f2(g,h,a); b += x; f = x + y;
		x = e + f1(b,c,d) + 0xc76c51a3 + words[43]; y = f2(f,g,h); a += x; e = x + y;
		x = d + f1(a,b,c) + 0xd192e819 + words[44]; y = f2(e,f,g); h += x; d = x + y;
		x = c + f1(h,a,b) + 0xd6990624 + words[45]; y = f2(d,e,f); g += x; c = x + y;
		x = b + f1(g,h,a) + 0xf40e3585 + words[46]; y = f2(c,d,e); f += x; b = x + y;
		x = a + f1(f,g,h) + 0x106aa070 + words[47]; y = f2(b,c,d); e += x; a = x + y;

		// extend to 56 words
		for (; i < 56; i++) words[i] =
			words[i-16] + (rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7]  + (rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

		// seventh round
		x = h + f1(e,f,g) + 0x19a4c116 + words[48]; y = f2(a,b,c); d += x; h = x + y;
		x = g + f1(d,e,f) + 0x1e376c08 + words[49]; y = f2(h,a,b); c += x; g = x + y;
		x = f + f1(c,d,e) + 0x2748774c + words[50]; y = f2(g,h,a); b += x; f = x + y;
		x = e + f1(b,c,d) + 0x34b0bcb5 + words[51]; y = f2(f,g,h); a += x; e = x + y;
		x = d + f1(a,b,c) + 0x391c0cb3 + words[52]; y = f2(e,f,g); h += x; d = x + y;
		x = c + f1(h,a,b) + 0x4ed8aa4a + words[53]; y = f2(d,e,f); g += x; c = x + y;
		x = b + f1(g,h,a) + 0x5b9cca4f + words[54]; y = f2(c,d,e); f += x; b = x + y;
		x = a + f1(f,g,h) + 0x682e6ff3 + words[55]; y = f2(b,c,d); e += x; a = x + y;

		// extend to 64 words
		for (; i < 64; i++) words[i] =
			words[i-16] + (rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7]  + (rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

		// eigth round
		x = h + f1(e,f,g) + 0x748f82ee + words[56]; y = f2(a,b,c); d += x; h = x + y;
		x = g + f1(d,e,f) + 0x78a5636f + words[57]; y = f2(h,a,b); c += x; g = x + y;
		x = f + f1(c,d,e) + 0x84c87814 + words[58]; y = f2(g,h,a); b += x; f = x + y;
		x = e + f1(b,c,d) + 0x8cc70208 + words[59]; y = f2(f,g,h); a += x; e = x + y;
		x = d + f1(a,b,c) + 0x90befffa + words[60]; y = f2(e,f,g); h += x; d = x + y;
		x = c + f1(h,a,b) + 0xa4506ceb + words[61]; y = f2(d,e,f); g += x; c = x + y;
		x = b + f1(g,h,a) + 0xbef9a3f7 + words[62]; y = f2(c,d,e); f += x; b = x + y;
		x = a + f1(f,g,h) + 0xc67178f2 + words[63]; y = f2(b,c,d); e += x; a = x + y;
		// clang-format on

		// update hash
		m_hash[0] += a;
		m_hash[1] += b;
		m_hash[2] += c;
		m_hash[3] += d;
		m_hash[4] += e;
		m_hash[5] += f;
		m_hash[6] += g;
		m_hash[7] += h;
	}

	/// Process remaining incomplete block from the internal buffer.
	void hash_remaining() noexcept {
		// the input bytes are considered as bits strings, where the first bit is the most significant bit of the byte

		// - append "1" bit to message
		// - append "0" bits until message length in bit mod 512 is 448
		// - append length as 64 bit integer

		// number of bits
		size_t paddedLength = m_buffer_size * 8;

		// plus one bit set to 1 (always appended)
		paddedLength++;

		// number of bits must be (numBits % 512) = 448
		size_t lower11Bits = paddedLength & 511;
		if (lower11Bits <= 448)
			paddedLength += 448 - lower11Bits;
		else
			paddedLength += 512 + 448 - lower11Bits;
		// convert from bits to bytes
		paddedLength /= 8;

		// only needed if additional data flows over into a second block
		unsigned char extra[block_size];

		// append a "1" bit, 128 => binary 10000000
		if (m_buffer_size < block_size)
			m_buffer[m_buffer_size] = 128;
		else
			extra[0] = 128;

		size_t i;
		for (i = m_buffer_size + 1; i < block_size; i++)
			m_buffer[i] = 0;
		for (; i < paddedLength; i++)
			extra[i - block_size] = 0;

		// add message length in bits as 64 bit number
		uint64_t msgBits = 8 * (m_num_bytes + m_buffer_size);
		// find right position
		unsigned char* addLength;
		if (paddedLength < block_size)
			addLength = m_buffer.data() + paddedLength;
		else
			addLength = extra + paddedLength - block_size;

		// must be big endian
		*addLength++ = (unsigned char)((msgBits >> 56) & 0xFF);
		*addLength++ = (unsigned char)((msgBits >> 48) & 0xFF);
		*addLength++ = (unsigned char)((msgBits >> 40) & 0xFF);
		*addLength++ = (unsigned char)((msgBits >> 32) & 0xFF);
		*addLength++ = (unsigned char)((msgBits >> 24) & 0xFF);
		*addLength++ = (unsigned char)((msgBits >> 16) & 0xFF);
		*addLength++ = (unsigned char)((msgBits >> 8) & 0xFF);
		*addLength = (unsigned char)(msgBits & 0xFF);

		// process blocks
		hash_block(m_buffer.data());
		// flowed over into a second block ?
		if (paddedLength > block_size) hash_block(extra);
	}

	/// Size of processed data in bytes
	uint64_t m_num_bytes = 0;
	/// Valid bytes in m_buffer
	size_t m_buffer_size = 0;

	/// Internal buffer holding remaining bytes that do not make up a full block
	std::array<std::uint8_t, block_size> m_buffer;

	/// Hash, stored as integers
	static constexpr int hash_value_size = 8;
	std::array<std::uint32_t, hash_value_size> m_hash = {
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
	};
};

namespace impl {
struct sha256_t {
public:
	static constexpr int block_size = sha256_stream::block_size;
	static constexpr int hash_size = sha256_stream::hash_size;

	/// Compute sha256 from a block of data.
	std::array<std::uint8_t, hash_size> operator()(std::span<const std::uint8_t> data) const noexcept {
		sha256_stream hasher;
		hasher << data;

		return hasher.finish();
	}

	/// Compute sha256 from provided text.
	std::array<std::uint8_t, hash_size> operator()(std::string_view text) const noexcept {
		return operator()({reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
	}
};
}  // namespace impl

impl::sha256_t sha256;

}  // namespace mincemeat

#undef SHA256_BYTESWAP
