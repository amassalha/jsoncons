// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JMESPATH_JMESPATH_EVALUATOR_HPP
#define JSONCONS_JMESPATH_JMESPATH_EVALUATOR_HPP

#include <string>
#include <vector>
#include <unordered_map> // std::unordered_map
#include <memory>
#include <type_traits> // std::is_const
#include <limits> // std::numeric_limits
#include <utility> // std::move
#include <functional> // 
#include <algorithm> // std::stable_sort, std::reverse
#include <cmath> // std::abs
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jmespath/jmespath_error.hpp>

namespace jsoncons { 
namespace jmespath {

    enum class operator_kind
    {
        default_op, // Identifier, CurrentNode, Index, MultiSelectList, MultiSelectHash, FunctionExpression
        projection_op,
        flatten_projection_op, // FlattenProjection
        or_op,
        and_op,
        eq_op,
        ne_op,
        lt_op,
        lte_op,
        gt_op,
        gte_op,
        not_op
    };

    struct operator_table final
    {
        static int precedence_level(operator_kind oper)
        {
            switch (oper)
            {
                case operator_kind::projection_op:
                    return 11;
                case operator_kind::flatten_projection_op:
                    return 11;
                case operator_kind::or_op:
                    return 9;
                case operator_kind::and_op:
                    return 8;
                case operator_kind::eq_op:
                case operator_kind::ne_op:
                    return 6;
                case operator_kind::lt_op:
                case operator_kind::lte_op:
                case operator_kind::gt_op:
                case operator_kind::gte_op:
                    return 5;
                case operator_kind::not_op:
                    return 1;
                default:
                    return 1;
            }
        }

        static bool is_right_associative(operator_kind oper)
        {
            switch (oper)
            {
                case operator_kind::not_op:
                    return true;
                case operator_kind::projection_op:
                    return true;
                case operator_kind::flatten_projection_op:
                    return false;
                case operator_kind::or_op:
                case operator_kind::and_op:
                case operator_kind::eq_op:
                case operator_kind::ne_op:
                case operator_kind::lt_op:
                case operator_kind::lte_op:
                case operator_kind::gt_op:
                case operator_kind::gte_op:
                    return false;
                default:
                    return false;
            }
        }
    };

    enum class token_kind 
    {
        current_node,
        lparen,
        rparen,
        begin_multi_select_hash,
        end_multi_select_hash,
        begin_multi_select_list,
        end_multi_select_list,
        begin_filter,
        end_filter,
        pipe,
        separator,
        key,
        literal,
        expression,
        binary_operator,
        unary_operator,
        function,
        end_function,
        argument,
        begin_expression_type,
        end_expression_type,
        end_of_expression
    };

    struct literal_arg_t
    {
        explicit literal_arg_t() = default;
    };
    constexpr literal_arg_t literal_arg{};

    struct begin_expression_type_arg_t
    {
        explicit begin_expression_type_arg_t() = default;
    };
    constexpr begin_expression_type_arg_t begin_expression_type_arg{};

    struct end_expression_type_arg_t
    {
        explicit end_expression_type_arg_t() = default;
    };
    constexpr end_expression_type_arg_t end_expression_type_arg{};

    struct end_of_expression_arg_t
    {
        explicit end_of_expression_arg_t() = default;
    };
    constexpr end_of_expression_arg_t end_of_expression_arg{};

    struct separator_arg_t
    {
        explicit separator_arg_t() = default;
    };
    constexpr separator_arg_t separator_arg{};

    struct key_arg_t
    {
        explicit key_arg_t() = default;
    };
    constexpr key_arg_t key_arg{};

    struct lparen_arg_t
    {
        explicit lparen_arg_t() = default;
    };
    constexpr lparen_arg_t lparen_arg{};

    struct rparen_arg_t
    {
        explicit rparen_arg_t() = default;
    };
    constexpr rparen_arg_t rparen_arg{};

    struct begin_multi_select_hash_arg_t
    {
        explicit begin_multi_select_hash_arg_t() = default;
    };
    constexpr begin_multi_select_hash_arg_t begin_multi_select_hash_arg{};

    struct end_multi_select_hash_arg_t
    {
        explicit end_multi_select_hash_arg_t() = default;
    };
    constexpr end_multi_select_hash_arg_t end_multi_select_hash_arg{};

    struct begin_multi_select_list_arg_t
    {
        explicit begin_multi_select_list_arg_t() = default;
    };
    constexpr begin_multi_select_list_arg_t begin_multi_select_list_arg{};

    struct end_multi_select_list_arg_t
    {
        explicit end_multi_select_list_arg_t() = default;
    };
    constexpr end_multi_select_list_arg_t end_multi_select_list_arg{};

    struct begin_filter_arg_t
    {
        explicit begin_filter_arg_t() = default;
    };
    constexpr begin_filter_arg_t begin_filter_arg{};

    struct end_filter_arg_t
    {
        explicit end_filter_arg_t() = default;
    };
    constexpr end_filter_arg_t end_filter_arg{};

    struct pipe_arg_t
    {
        explicit pipe_arg_t() = default;
    };
    constexpr pipe_arg_t pipe_arg{};

    struct current_node_arg_t
    {
        explicit current_node_arg_t() = default;
    };
    constexpr current_node_arg_t current_node_arg{};

    struct end_function_arg_t
    {
        explicit end_function_arg_t() = default;
    };
    constexpr end_function_arg_t end_function_arg{};

    struct argument_arg_t
    {
        explicit argument_arg_t() = default;
    };
    constexpr argument_arg_t argument_arg{};

    struct slice
    {
        jsoncons::optional<int64_t> start_;
        jsoncons::optional<int64_t> stop_;
        int64_t step_;

        slice()
            : start_(), stop_(), step_(1)
        {
        }

        slice(const jsoncons::optional<int64_t>& start, const jsoncons::optional<int64_t>& end, int64_t step) 
            : start_(start), stop_(end), step_(step)
        {
        }

        slice(const slice& other)
            : start_(other.start_), stop_(other.stop_), step_(other.step_)
        {
        }

        slice& operator=(const slice& rhs) 
        {
            if (this != &rhs)
            {
                if (rhs.start_)
                {
                    start_ = rhs.start_;
                }
                else
                {
                    start_.reset();
                }
                if (rhs.stop_)
                {
                    stop_ = rhs.stop_;
                }
                else
                {
                    stop_.reset();
                }
                step_ = rhs.step_;
            }
            return *this;
        }

        int64_t get_start(std::size_t size) const
        {
            if (start_)
            {
                auto len = *start_ >= 0 ? *start_ : (static_cast<int64_t>(size) + *start_);
                return len <= static_cast<int64_t>(size) ? len : static_cast<int64_t>(size);
            }
            else
            {
                if (step_ >= 0)
                {
                    return 0;
                }
                else 
                {
                    return static_cast<int64_t>(size);
                }
            }
        }

        int64_t get_stop(std::size_t size) const
        {
            if (stop_)
            {
                auto len = *stop_ >= 0 ? *stop_ : (static_cast<int64_t>(size) + *stop_);
                return len <= static_cast<int64_t>(size) ? len : static_cast<int64_t>(size);
            }
            else
            {
                return step_ >= 0 ? static_cast<int64_t>(size) : -1;
            }
        }

        int64_t step() const
        {
            return step_; // Allow negative
        }
    };

    // dynamic_resources

    template<class Json, class JsonReference>
    class dynamic_resources
    {
        typedef typename Json::char_type char_type;
        typedef typename Json::char_traits_type char_traits_type;
        typedef std::basic_string<char_type,char_traits_type> string_type;
        typedef typename Json::string_view_type string_view_type;
        typedef JsonReference reference;
        using pointer = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
        typedef typename Json::const_pointer const_pointer;

        std::vector<std::unique_ptr<Json>> temp_storage_;

    public:
        ~dynamic_resources()
        {
        }

        reference number_type_name() 
        {
            static Json number_type_name(JSONCONS_STRING_CONSTANT(char_type, "number"));

            return number_type_name;
        }

        reference boolean_type_name()
        {
            static Json boolean_type_name(JSONCONS_STRING_CONSTANT(char_type, "boolean"));

            return boolean_type_name;
        }

        reference string_type_name()
        {
            static Json string_type_name(JSONCONS_STRING_CONSTANT(char_type, "string"));

            return string_type_name;
        }

        reference object_type_name()
        {
            static Json object_type_name(JSONCONS_STRING_CONSTANT(char_type, "object"));

            return object_type_name;
        }

        reference array_type_name()
        {
            static Json array_type_name(JSONCONS_STRING_CONSTANT(char_type, "array"));

            return array_type_name;
        }

        reference null_type_name()
        {
            static Json null_type_name(JSONCONS_STRING_CONSTANT(char_type, "null"));

            return null_type_name;
        }

        reference true_value() const
        {
            static const Json true_value(true, semantic_tag::none);
            return true_value;
        }

        reference false_value() const
        {
            static const Json false_value(false, semantic_tag::none);
            return false_value;
        }

        reference null_value() const
        {
            static const Json null_value(null_type(), semantic_tag::none);
            return null_value;
        }

        template <typename... Args>
        Json* create_json(Args&& ... args)
        {
            auto temp = jsoncons::make_unique<Json>(std::forward<Args>(args)...);
            Json* ptr = temp.get();
            temp_storage_.emplace_back(std::move(temp));
            return ptr;
        }
    };

} // namespace jmespath
} // namespace jsoncons

#endif
