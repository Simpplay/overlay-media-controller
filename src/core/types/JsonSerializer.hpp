#pragma once

#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <iomanip>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <ostream>

namespace omc::json
{
    /*
    |--------------------------------------------------------------------------
    | Forward declarations
    |--------------------------------------------------------------------------
    */

    class JsonSerializer;
    class JsonObject;
    class JsonArray;

    template<typename T>
    concept JsonSerializable =
        requires(const T& value)
    {
        { to_json(value) } -> std::same_as<std::string>;
    };

    /*
    |--------------------------------------------------------------------------
    | Type traits
    |--------------------------------------------------------------------------
    */

    template<typename T>
    struct is_optional : std::false_type {};

    template<typename T>
    struct is_optional<std::optional<T>> : std::true_type {};

    template<typename T>
    concept OptionalLike = is_optional<std::remove_cvref_t<T>>::value;

    // -------------------------------------------------------------------------

    template<typename T>
    struct is_variant : std::false_type {};

    template<typename... Ts>
    struct is_variant<std::variant<Ts...>> : std::true_type {};

    template<typename T>
    concept VariantLike = is_variant<std::remove_cvref_t<T>>::value;

    // -------------------------------------------------------------------------

    template<typename T>
    struct is_map : std::false_type {};

    template<typename K, typename V, typename C, typename Alloc>
    struct is_map<std::map<K, V, C, Alloc>> : std::true_type {};

    template<typename K, typename V, typename Hash, typename Eq, typename Alloc>
    struct is_map<std::unordered_map<K, V, Hash, Eq, Alloc>> : std::true_type {};

    template<typename T>
    concept MapLike = is_map<std::remove_cvref_t<T>>::value;

    // -------------------------------------------------------------------------
    // RangeLike replaces VectorLike: covers std::vector, std::set,
    // std::unordered_set, std::array, std::span, std::list, etc.
    // Explicitly excludes strings and maps so they keep their own overloads.
    // Also excludes types with a custom to_json() so those take priority.
    // -------------------------------------------------------------------------

    template<typename T>
    concept RangeLike =
        std::ranges::range<T>             &&
        !MapLike<T>                        &&
        !std::convertible_to<T, std::string_view> &&
        !JsonSerializable<T>;

    /*
    |--------------------------------------------------------------------------
    | JsonSerializer
    |--------------------------------------------------------------------------
    */

    class JsonSerializer
    {
    public:

        /*
        |--------------------------------------------------------------------------
        | Null
        |--------------------------------------------------------------------------
        */

        static std::string serialize(std::nullptr_t)
        {
            return "null";
        }

        // std::monostate (empty variant alternative) → null
        static std::string serialize(std::monostate)
        {
            return "null";
        }

        /*
        |--------------------------------------------------------------------------
        | Boolean
        |--------------------------------------------------------------------------
        */

        static std::string serialize(bool value)
        {
            return value ? "true" : "false";
        }

        /*
        |--------------------------------------------------------------------------
        | Integral
        |--------------------------------------------------------------------------
        */

        template<std::integral T>
            requires (!std::same_as<T, bool>)
        static std::string serialize(T value)
        {
            return std::to_string(value);
        }

        /*
        |--------------------------------------------------------------------------
        | Floating point
        | NaN and Inf are not valid JSON values; they serialize as null to avoid
        | producing malformed output.
        |--------------------------------------------------------------------------
        */

        template<std::floating_point T>
        static std::string serialize(T value)
        {
            if (std::isnan(value) || std::isinf(value))
            {
                return "null";
            }

            std::ostringstream out;
            out << std::setprecision(15) << value;
            return out.str();
        }

        /*
        |--------------------------------------------------------------------------
        | String
        |--------------------------------------------------------------------------
        */

        static std::string serialize(const char* value)
        {
            return serialize(std::string_view(value));
        }

        static std::string serialize(const std::string& value)
        {
            return serialize(std::string_view(value));
        }

        static std::string serialize(std::string_view value)
        {
            return "\"" + escape(value) + "\"";
        }

        /*
        |--------------------------------------------------------------------------
        | Enum → underlying integer
        |--------------------------------------------------------------------------
        */

        template<typename T>
            requires std::is_enum_v<T>
        static std::string serialize(T value)
        {
            return serialize(static_cast<std::underlying_type_t<T>>(value));
        }

        /*
        |--------------------------------------------------------------------------
        | Optional
        |--------------------------------------------------------------------------
        */

        template<OptionalLike T>
        static std::string serialize(const T& value)
        {
            return value.has_value() ? serialize(*value) : "null";
        }

        /*
        |--------------------------------------------------------------------------
        | Variant
        |--------------------------------------------------------------------------
        */

        template<VariantLike T>
        static std::string serialize(const T& value)
        {
            return std::visit(
                [](const auto& v) -> std::string
                {
                    return JsonSerializer::serialize(v);
                },
                value
            );
        }

        /*
        |--------------------------------------------------------------------------
        | Pair → [first, second]
        |--------------------------------------------------------------------------
        */

        template<typename A, typename B>
        static std::string serialize(const std::pair<A, B>& p)
        {
            return "[" + serialize(p.first) + "," + serialize(p.second) + "]";
        }

        /*
        |--------------------------------------------------------------------------
        | Tuple → [elem0, elem1, ...]
        |--------------------------------------------------------------------------
        */

        template<typename... Ts>
        static std::string serialize(const std::tuple<Ts...>& t)
        {
            return serialize_tuple(t, std::index_sequence_for<Ts...>{});
        }

        /*
        |--------------------------------------------------------------------------
        | Range (vector, set, unordered_set, array, span, list, ...)
        |--------------------------------------------------------------------------
        */

        template<RangeLike T>
        static std::string serialize(const T& range)
        {
            std::ostringstream out;
            out << "[";

            bool first = true;
            for (const auto& elem : range)
            {
                if (!first) out << ",";
                out << serialize(elem);
                first = false;
            }

            out << "]";
            return out.str();
        }

        /*
        |--------------------------------------------------------------------------
        | Map → object
        |--------------------------------------------------------------------------
        */

        template<MapLike T>
        static std::string serialize(const T& map)
        {
            std::ostringstream out;
            out << "{";

            bool first = true;
            for (const auto& [key, value] : map)
            {
                if (!first) out << ",";
                out << serialize(key) << ":" << serialize(value);
                first = false;
            }

            out << "}";
            return out.str();
        }

        /*
        |--------------------------------------------------------------------------
        | Custom types via ADL (to_json)
        |--------------------------------------------------------------------------
        */

        template<JsonSerializable T>
        static std::string serialize(const T& value)
        {
            return to_json(value);
        }

    private:

        /*
        |--------------------------------------------------------------------------
        | Tuple helper
        |--------------------------------------------------------------------------
        */

        template<typename Tuple, std::size_t... Is>
        static std::string serialize_tuple(const Tuple& t, std::index_sequence<Is...>)
        {
            std::vector<std::string> parts;
            parts.reserve(sizeof...(Is));
            (parts.push_back(serialize(std::get<Is>(t))), ...);

            std::ostringstream out;
            out << "[";
            for (std::size_t i = 0; i < parts.size(); ++i)
            {
                if (i > 0) out << ",";
                out << parts[i];
            }
            out << "]";
            return out.str();
        }

        /*
        |--------------------------------------------------------------------------
        | String escaping
        |--------------------------------------------------------------------------
        */

        static std::string escape(std::string_view input)
        {
            std::ostringstream out;

            for (const char c : input)
            {
                switch (c)
                {
                case '\"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b";  break;
                case '\f': out << "\\f";  break;
                case '\n': out << "\\n";  break;
                case '\r': out << "\\r";  break;
                case '\t': out << "\\t";  break;
                default:
                {
                    const auto uc = static_cast<unsigned char>(c);
                    if (uc < 0x20)
                    {
                        out << "\\u"
                            << std::hex
                            << std::setw(4)
                            << std::setfill('0')
                            << static_cast<int>(uc);
                    }
                    else
                    {
                        out << c;
                    }
                    break;
                }
                }
            }

            return out.str();
        }
    };

    /*
    |--------------------------------------------------------------------------
    | Free function — convenience wrapper over JsonSerializer::serialize().
    |
    | Allows writing:
    |     omc::json::serialize(myDto)
    | instead of:
    |     omc::json::JsonSerializer::serialize(myDto)
    |--------------------------------------------------------------------------
    */

    template<typename T>
    std::string serialize(const T& value)
    {
        return JsonSerializer::serialize(value);
    }

    /*
    |--------------------------------------------------------------------------
    | JsonObject — fluent builder for JSON objects.
    |
    | Designed for writing to_json() implementations for DTOs:
    |
    |     std::string to_json(const UserDto& u)
    |     {
    |         return omc::json::JsonObject{}
    |             .field("id",    u.id)
    |             .field("name",  u.name)
    |             .field("email", u.email)
    |             .field_opt("phone", u.phone)   // omitted if nullopt
    |             .build();
    |     }
    |--------------------------------------------------------------------------
    */

    class JsonObject
    {
    public:

        // Always include this field.
        template<typename V>
        JsonObject& field(std::string_view key, const V& value)
        {
            entries_.emplace_back(
                JsonSerializer::serialize(key),
                JsonSerializer::serialize(value)
            );
            return *this;
        }

        // Include this field only if the optional has a value.
        // The field is completely absent from the output when empty,
        // unlike field() which would produce "key":null.
        template<OptionalLike V>
        JsonObject& field_opt(std::string_view key, const V& value)
        {
            if (value.has_value())
            {
                field(key, *value);
            }
            return *this;
        }

        // Build compact JSON: {"key1":val1,"key2":val2}
        [[nodiscard]] std::string build() const
        {
            std::ostringstream out;
            out << "{";

            for (std::size_t i = 0; i < entries_.size(); ++i)
            {
                if (i > 0) out << ",";
                out << entries_[i].first << ":" << entries_[i].second;
            }

            out << "}";
            return out.str();
        }

        // Build pretty-printed JSON.
        // indent: spaces per level (default 2).
        // depth:  nesting level of this object (default 0 = top level).
        //
        // Example at depth=0, indent=2:
        //   {
        //     "id": 1,
        //     "name": "Alice"
        //   }
        [[nodiscard]] std::string pretty(int indent = 2, int depth = 0) const
        {
            if (entries_.empty()) return "{}";

            const std::string pad(static_cast<std::size_t>(depth * indent), ' ');
            const std::string inner(static_cast<std::size_t>((depth + 1) * indent), ' ');

            std::ostringstream out;
            out << "{\n";

            for (std::size_t i = 0; i < entries_.size(); ++i)
            {
                out << inner
                    << entries_[i].first << ": " << entries_[i].second;

                if (i + 1 < entries_.size()) out << ",";
                out << "\n";
            }

            out << pad << "}";
            return out.str();
        }

        // Implicit conversion so JsonObject can be passed where std::string
        // is expected (e.g. as a nested field value).
        operator std::string() const { return build(); }

        friend std::ostream& operator<<(std::ostream& os, const JsonObject& obj)
        {
            return os << obj.build();
        }

    private:

        // Stores (serialized-key, serialized-value) pairs in insertion order.
        std::vector<std::pair<std::string, std::string>> entries_;
    };

    /*
    |--------------------------------------------------------------------------
    | JsonArray — fluent builder for JSON arrays.
    |
    | Example:
    |     auto json = omc::json::JsonArray{}
    |         .push(1)
    |         .push("hello")
    |         .push(myDto)
    |         .build();
    |
    | Or from an existing range:
    |     auto json = omc::json::JsonArray{}.push_range(myVec).build();
    |--------------------------------------------------------------------------
    */

    class JsonArray
    {
    public:

        template<typename V>
        JsonArray& push(const V& value)
        {
            elements_.emplace_back(JsonSerializer::serialize(value));
            return *this;
        }

        // Push every element of a range.
        template<std::ranges::range Range>
            requires (!std::convertible_to<Range, std::string_view>)
        JsonArray& push_range(const Range& range)
        {
            for (const auto& elem : range)
            {
                push(elem);
            }
            return *this;
        }

        // Build compact JSON: [val1,val2,val3]
        [[nodiscard]] std::string build() const
        {
            std::ostringstream out;
            out << "[";

            for (std::size_t i = 0; i < elements_.size(); ++i)
            {
                if (i > 0) out << ",";
                out << elements_[i];
            }

            out << "]";
            return out.str();
        }

        // Build pretty-printed JSON.
        [[nodiscard]] std::string pretty(int indent = 2, int depth = 0) const
        {
            if (elements_.empty()) return "[]";

            const std::string pad(static_cast<std::size_t>(depth * indent), ' ');
            const std::string inner(static_cast<std::size_t>((depth + 1) * indent), ' ');

            std::ostringstream out;
            out << "[\n";

            for (std::size_t i = 0; i < elements_.size(); ++i)
            {
                out << inner << elements_[i];
                if (i + 1 < elements_.size()) out << ",";
                out << "\n";
            }

            out << pad << "]";
            return out.str();
        }

        operator std::string() const { return build(); }

        friend std::ostream& operator<<(std::ostream& os, const JsonArray& arr)
        {
            return os << arr.build();
        }

    private:

        std::vector<std::string> elements_;
    };

} // namespace omc::json


/*
|--------------------------------------------------------------------------
| Usage examples (for reference — not compiled)
|--------------------------------------------------------------------------
|
| // --- Simple DTO ---
|
| struct AddressDto {
|     std::string street;
|     std::string city;
| };
|
| std::string to_json(const AddressDto& a) {
|     return omc::json::JsonObject{}
|         .field("street", a.street)
|         .field("city",   a.city)
|         .build();
| }
|
| // --- DTO with optional and nested ---
|
| struct UserDto {
|     int                      id;
|     std::string              name;
|     std::optional<std::string> phone;
|     AddressDto               address;
|     std::vector<std::string> roles;
| };
|
| std::string to_json(const UserDto& u) {
|     return omc::json::JsonObject{}
|         .field    ("id",      u.id)
|         .field    ("name",    u.name)
|         .field_opt("phone",   u.phone)   // absent when nullopt
|         .field    ("address", u.address) // calls to_json(AddressDto) via ADL
|         .field    ("roles",   u.roles)   // vector → JSON array
|         .build();
| }
|
| // --- Pretty print ---
|
| auto json = omc::json::JsonObject{}
|     .field("id",   42)
|     .field("name", "Alice")
|     .pretty();   // indent=2, depth=0
|
| // --- Fluent array ---
|
| auto arr = omc::json::JsonArray{}
|     .push(1)
|     .push(2)
|     .push(3)
|     .build();   // "[1,2,3]"
|
| // --- Free function ---
|
| std::string s = omc::json::serialize(myDto);
|
|--------------------------------------------------------------------------
*/
