#pragma once
#include <array>
#include <type_traits>
#include <cstdint>

namespace cnc { namespace common {
	template<class MessageTypes>
	class header
	{
	public:
		static_assert(std::is_enum_v<MessageTypes>, "Types parameter must be an enum or enum class");
		using message_type = typename MessageTypes;
		using size_type = std::uint64_t;

	private:
		std::uint8_t m_magic_byte;
		message_type m_type;
		std::uint64_t m_payload_size;

	public:		
		static constexpr std::size_t size = sizeof(m_magic_byte) + sizeof(m_type) + sizeof(m_payload_size);
		using bytearray = std::array<std::uint8_t, size>;

	public:
		header() = default;
		header(header &&) = default;
		header &operator=(const header &) = default;
		header &operator=(header &&) = default;

		header(std::uint8_t magic_byte, message_type type, size_type payload_size)
			: m_magic_byte(magic_byte), m_type(type), m_payload_size(payload_size)
		{ }

		explicit header(const bytearray &src)
		{
			const std::uint8_t *data = src.data();
			m_magic_byte = *data++;

			typename std::underlying_type<message_type>::type type_value = 0;
			for (std::size_t i = 0; i < sizeof(type_value); i++)
				type_value |= static_cast<decltype(type_value)>(*data++) << ((sizeof(type_value) - i - 1) * 8);

			m_type = static_cast<message_type>(type_value);

			size_type size = 0;
			for (std::size_t i = 0; i < sizeof(size_type); i++)
				size |= static_cast<size_type>(*data++) << ((sizeof(size_type) - i - 1) * 8);

			m_payload_size = size;
		}

		std::uint8_t magic_byte() const noexcept { return m_magic_byte; }
		message_type type() const noexcept { return m_type; }
		size_type payload_size() const noexcept { return m_payload_size; }

		bytearray to_bytearray() const noexcept
		{
			bytearray bytes;
			std::uint8_t *data = bytes.data();

			*data++ = magic_byte();

			auto type_value = static_cast<typename std::underlying_type<message_type>::type>(type());
			for (std::size_t i = 0; i < sizeof(type_value); i++)
				*data++ = static_cast<std::uint8_t>(type_value >> ((sizeof(type_value) - i - 1) * 8));

			for (std::size_t i = 0; i < sizeof(size_type); i++)
				*data++ = static_cast<std::uint8_t>(payload_size() >> ((sizeof(size_type) - i - 1) * 8));

			return bytes;
		}
	};
} }
