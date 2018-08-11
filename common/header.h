#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <type_traits>

namespace cnc {
	namespace common {
		template<
			typename protocol,
			typename byte_type_t = std::uint8_t,
			typename size_type_t = std::uintmax_t,
			typename = std::enable_if_t<std::is_enum_v<protocol::types>>,
			typename = std::enable_if_t<
			std::is_same_v<std::remove_cv_t<byte_type_t>, std::remove_cv_t<decltype(protocol::magic_byte)>>
			>
		>
			class header
		{
		public:
			using byte_type = byte_type_t;
			using size_type = size_type_t;

		private:
			bool m_valid = false;
			struct data {
				byte_type magic_byte = protocol::magic_byte;
				typename protocol::types type;
				size_type payload_size;
			} m_data;

			static constexpr std::size_t size = sizeof(data);

		public:
			static constexpr std::size_t get_size() { return size; }
			using byte_array = std::array<unsigned char, size>;

			// provide a default constructor (needed by std::future)
			header()
				: m_data{ 0x0, protocol::types::FIRST_MEMBER_UNUSED, 0 }
			{

			}

			header(header &&rhs)
				: header(std::cref(rhs))
			{

			}

			header &operator=(header &&rhs)
			{
				return operator=(std::cref(rhs));
			}

			header(const header &rhs)
				: m_valid(rhs.m_valid), m_data{ rhs.m_data.magic_byte, rhs.m_data.type, rhs.m_data.payload_size }
			{

			}

			header &operator=(const header &rhs)
			{
				m_valid = rhs.m_valid;
				m_data.magic_byte = rhs.m_data.magic_byte;
				m_data.type = rhs.m_data.type;
				m_data.payload_size = rhs.m_data.payload_size;
				return *this;
			}

			header(const byte_array& src)
				: m_data{ src[0], static_cast<typename protocol::types>(src[1]), *reinterpret_cast<const size_type *>(&src[2]) }
			{
				m_valid = m_data.magic_byte == protocol::magic_byte;
				m_valid = m_valid && m_data.type < protocol::types::LAST_MEMBER_UNUSED;
				m_valid = m_valid && m_data.type > protocol::types::FIRST_MEMBER_UNUSED;
			}

			header(typename protocol::types type, size_type payload_size)
				: m_valid(true), m_data{ protocol::magic_byte, type, payload_size }
			{

			}

			byte_array to_bytearray()
			{
				std::array<unsigned char, size> out;
				out[0] = m_data.magic_byte;
				out[1] = static_cast<std::uint8_t>(m_data.type);
				*reinterpret_cast<size_type *>(&out[2]) = m_data.payload_size;
				return out;
			}

			bool is_valid() const { return m_valid; }
			typename protocol::types get_type() const { return m_data.type; }
			size_type get_payload_size() const { return m_data.payload_size; }
		};
	}
}
