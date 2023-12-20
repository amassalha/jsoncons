// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JMESPATH_JMESPATH_SEARCH_HPP
#define JSONCONS_JMESPATH_JMESPATH_SEARCH_HPP

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
#include <jsoncons_ext/jmespath/jmespath_evaluator.hpp>
#include <jsoncons_ext/jmespath/jmespath_parser.hpp>

namespace jsoncons { 
namespace jmespath {

    template <class Json>
    using jmespath_expression = typename jsoncons::jmespath::detail::jmespath_evaluator<Json,const Json&>::jmespath_expression;

    template<class Json>
    Json search(const Json& doc, const typename Json::string_view_type& path)
    {
        jsoncons::jmespath::detail::jmespath_evaluator<Json,const Json&> evaluator;
        std::error_code ec;
        auto expr = evaluator.compile(path.data(), path.size(), ec);
        if (ec)
        {
            JSONCONS_THROW(jmespath_error(ec, evaluator.line(), evaluator.column()));
        }
        auto result = expr.evaluate(doc, ec);
        if (ec)
        {
            JSONCONS_THROW(jmespath_error(ec));
        }
        return result;
    }

    template<class Json>
    Json search(const Json& doc, const typename Json::string_view_type& path, std::error_code& ec)
    {
        jsoncons::jmespath::detail::jmespath_evaluator<Json,const Json&> evaluator;
        auto expr = evaluator.compile(path.data(), path.size(), ec);
        if (ec)
        {
            return Json::null();
        }
        auto result = expr.evaluate(doc, ec);
        if (ec)
        {
            return Json::null();
        }
        return result;
    }

    template <class Json>
    jmespath_expression<Json> make_expression(const typename json::string_view_type& expr)
    {
        return jmespath_expression<Json>::compile(expr);
    }

    template <class Json>
    jmespath_expression<Json> make_expression(const typename json::string_view_type& expr,
                                              std::error_code& ec)
    {
        return jmespath_expression<Json>::compile(expr, ec);
    }


} // namespace jmespath
} // namespace jsoncons

#endif
