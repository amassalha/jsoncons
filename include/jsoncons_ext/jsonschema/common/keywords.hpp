// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONSCHEMA_COMMON_KEYWORDS_HPP
#define JSONCONS_JSONSCHEMA_COMMON_KEYWORDS_HPP

#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/uri.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpointer/jsonpointer.hpp>
#include <jsoncons_ext/jsonschema/common/format_validator.hpp>
#include <jsoncons_ext/jsonschema/common/keyword_validator.hpp>
#include <cassert>
#include <set>
#include <sstream>
#include <iostream>
#include <cassert>
#include <unordered_set>
#if defined(JSONCONS_HAS_STD_REGEX)
#include <regex>
#endif

namespace jsoncons {
namespace jsonschema {

    struct collecting_error_reporter : public error_reporter
    {
        std::vector<validation_output> errors;

    private:
        void do_error(const validation_output& o) final
        {
            errors.push_back(o);
        }
    };

    // contentEncoding

    template <class Json>
    class content_encoding_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::string content_encoding_;

    public:
        content_encoding_validator(const std::string& schema_path, const std::string& content_encoding)
            : keyword_validator_base<Json>(schema_path), 
              content_encoding_(content_encoding)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<content_encoding_validator>(this->schema_path(), content_encoding_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (content_encoding_ == "base64")
            {
                auto s = instance.template as<jsoncons::string_view>();
                std::string content;
                auto retval = jsoncons::decode_base64(s.begin(), s.end(), content);
                if (retval.ec != jsoncons::conv_errc::success)
                {
                    reporter.error(validation_output("contentEncoding", 
                                                     this->schema_path(), 
                                                     instance_location.to_uri_fragment(), 
                                                     "Content is not a base64 string"));
                    if (reporter.fail_early())
                    {
                        return;
                    }
                }
            }
            else if (!content_encoding_.empty())
            {
                reporter.error(validation_output("contentEncoding", 
                    this->schema_path(),
                    instance_location.to_uri_fragment(), 
                    "unable to check for contentEncoding '" + content_encoding_ + "'"));
                if (reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    // contentMediaType

    template <class Json>
    class content_media_type_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::string content_media_type_;

    public:
        content_media_type_validator(const std::string& schema_path, const std::string& content_media_type)
            : keyword_validator_base<Json>(schema_path), 
              content_media_type_(content_media_type)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<content_media_type_validator>(this->schema_path(), content_media_type_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (content_media_type_ == "application/Json")
            {
                auto sv = instance.as_string_view();
                json_string_reader reader(sv);
                std::error_code ec;
                reader.read(ec);

                if (ec)
                {
                    reporter.error(validation_output("contentMediaType", 
                                                     this->schema_path(), 
                                                     instance_location.to_uri_fragment(), 
                                                     std::string("Content is not JSON: ") + ec.message()));
                }
            }
        }
    };

    // format 

    template <class Json>
    class format_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        format_checker format_check_;

    public:
        format_validator(const std::string& schema_path, const format_checker& format_check)
            : keyword_validator_base<Json>(schema_path), format_check_(format_check)
        {

        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<format_validator>(this->schema_path(), format_check_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (format_check_ != nullptr) 
            {
                auto s = instance.template as<std::string>();

                format_check_(this->schema_path(), instance_location, s, reporter);
                if (reporter.error_count() > 0 && reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    // pattern 

#if defined(JSONCONS_HAS_STD_REGEX)
    template <class Json>
    class pattern_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::string pattern_string_;
        std::regex regex_;

    public:
        pattern_validator(const std::string& schema_path,
            const std::string& pattern_string, const std::regex& regex)
            : keyword_validator_base<Json>(schema_path), 
              pattern_string_(pattern_string), regex_(regex)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<pattern_validator>(this->schema_path(), pattern_string_, regex_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            auto s = instance.template as<std::string>();
            if (!std::regex_search(s, regex_))
            {
                std::string message("String \"");
                message.append(s);
                message.append("\" does not match pattern \"");
                message.append(pattern_string_);
                message.append("\"");
                reporter.error(validation_output("pattern", 
                    this->schema_path(),
                    instance_location.to_uri_fragment(), 
                    std::move(message)));
                if (reporter.fail_early())
                {
                    return;
                }
            }
        }
    };
#else
    template <class Json>
    class pattern_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

    public:
        pattern_validator(const std::string& schema_path)
            : keyword_validator_base<Json>(schema_path)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<pattern_validator>(this->schema_path());
        }

    private:

        void do_validate(const Json&, 
            const jsonpointer::json_pointer&,
            std::unordered_set<std::string>&, 
            error_reporter&,
            Json&) const final
        {
        }
    };
#endif

    // maxLength

    template <class Json>
    class max_length_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t max_length_;
    public:
        max_length_validator(const std::string& schema_path, std::size_t max_length)
            : keyword_validator_base<Json>(schema_path), max_length_(max_length)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<max_length_validator>(this->schema_path(), max_length_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            auto sv = instance.as_string_view();
            std::size_t length = unicode_traits::count_codepoints(sv.data(), sv.size());
            if (length > max_length_)
            {
                reporter.error(validation_output("maxLength", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 std::string("Expected maxLength: ") + std::to_string(max_length_)
                    + ", actual: " + std::to_string(length)));
                if (reporter.fail_early())
                {
                    return;
                }
            }          
        }
    };

    // maxItems

    template <class Json>
    class max_items_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t max_items_;
    public:
        max_items_validator(const std::string& schema_path, std::size_t max_items)
            : keyword_validator_base<Json>(schema_path), max_items_(max_items)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<max_items_validator>(this->schema_path(), max_items_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (instance.size() > max_items_)
            {
                std::string message("Expected maximum item count: " + std::to_string(max_items_));
                message.append(", found: " + std::to_string(instance.size()));
                reporter.error(validation_output("maxItems", 
                                                 this->schema_path(),
                                                 instance_location.to_uri_fragment(), 
                                                 std::move(message)));
                if (reporter.fail_early())
                {
                    return;
                }
            }          
        }
    };

    // minItems

    template <class Json>
    class min_items_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;

        std::size_t min_items_;
    public:
        min_items_validator(const std::string& schema_path, std::size_t min_items)
            : keyword_validator_base<Json>(schema_path), min_items_(min_items)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<min_items_validator>(this->schema_path(), min_items_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (instance.size() < min_items_)
            {
                std::string message("Expected maximum item count: " + std::to_string(min_items_));
                message.append(", found: " + std::to_string(instance.size()));
                reporter.error(validation_output("minItems", 
                                                 this->schema_path(),
                                                 instance_location.to_uri_fragment(), 
                                                 std::move(message)));
                if (reporter.fail_early())
                {
                    return;
                }
            }          
        }
    };

    // items

    template <class Json>
    class items_array_validator : public keyword_validator_base<Json>
    {
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;

        std::vector<schema_validator_type> item_validators_;
        schema_validator_type additional_items_validator_;
    public:
        items_array_validator(const std::string& schema_path, 
            std::vector<schema_validator_type>&& item_validators,
            schema_validator_type&& additional_items_validator)
            : keyword_validator_base<Json>(schema_path), 
              item_validators_(std::move(item_validators)), 
              additional_items_validator_(std::move(additional_items_validator))
        {
        }

        keyword_validator_type clone() const final 
        {
            std::vector<schema_validator_type> item_validators;
            for (auto& validator : item_validators_)
            {
                item_validators.push_back(validator->clone());
            }
            schema_validator_type additional_items_validator; 
            
            if (additional_items_validator_)
            {
                additional_items_validator = additional_items_validator_->clone();
            }

            return jsoncons::make_unique<items_array_validator>(this->schema_path(), std::move(item_validators),
                std::move(additional_items_validator));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter,
            Json& patch) const final
        {
            size_t index = 0;
            auto validator_it = item_validators_.cbegin();

            for (const auto& item : instance.array_range()) 
            {
                jsonpointer::json_pointer pointer(instance_location);

                if (validator_it != item_validators_.cend())
                {
                    pointer /= index++;
                    (*validator_it)->validate(item, pointer, evaluated_properties, reporter, patch);
                    ++validator_it;
                }
                else if (additional_items_validator_ != nullptr)
                {
                    pointer /= index++;
                    additional_items_validator_->validate(item, pointer, evaluated_properties, reporter, patch);
                }
                else
                    break;

            }
        }
    };

    // contains

    template <class Json>
    class contains_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;

        schema_validator_type validator_;
    public:
        contains_validator(const std::string& schema_path, 
            schema_validator_type&& validator)
            : keyword_validator_base<Json>(schema_path), 
              validator_(std::move(validator))
        {
        }

        keyword_validator_type clone() const final 
        {
            schema_validator_type validator;
            if (validator_)
            {
                validator = validator_->clone();
            }

            return jsoncons::make_unique<contains_validator>(std::string(this->schema_path()),
                std::move(validator));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter,
            Json& patch) const final
        {

            if (validator_) 
            {
                bool contained = false;
                collecting_error_reporter local_reporter;
                for (const auto& item : instance.array_range()) 
                {
                    std::size_t mark = local_reporter.errors.size();
                    validator_->validate(item, instance_location, evaluated_properties, local_reporter, patch);
                    if (mark == local_reporter.errors.size()) 
                    {
                        contained = true;
                        break;
                    }
                }
                if (!contained)
                {
                    reporter.error(validation_output("contains", 
                                                     this->schema_path(), 
                                                     instance_location.to_uri_fragment(), 
                                                     "Expected at least one array item to match \"contains\" schema", 
                                                     local_reporter.errors));
                    if (reporter.fail_early())
                    {
                        return;
                    }
                }
            }
        }
    };

    template <class Json>
    class items_object_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;

        schema_validator_type items_validator_;
    public:
        items_object_validator(const std::string& schema_path, 
            schema_validator_type&& items_validator)
            : keyword_validator_base<Json>(schema_path), 
              items_validator_(std::move(items_validator))
        {
        }

        keyword_validator_type clone() const final 
        {
            schema_validator_type items_validator;
            if (items_validator_)
            {
                items_validator = items_validator_->clone();
            }
            return jsoncons::make_unique<items_object_validator>(std::string(this->schema_path()),
                std::move(items_validator));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter,
            Json& patch) const final
        {
            size_t index = 0;
            if (items_validator_)
            {
                for (const auto& i : instance.array_range()) 
                {
                    jsonpointer::json_pointer pointer(instance_location);
                    pointer /= index;
                    items_validator_->validate(i, pointer, evaluated_properties, reporter, patch);
                    index++;
                }
            }
        }
    };

    // uniqueItems

    template <class Json>
    class unique_items_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        bool are_unique_;
    public:
        unique_items_validator(const std::string& schema_path, bool are_unique)
            : keyword_validator_base<Json>(schema_path), are_unique_(are_unique)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<unique_items_validator>(std::string(this->schema_path()),
                are_unique_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (are_unique_ && !array_has_unique_items(instance))
            {
                reporter.error(validation_output("uniqueItems", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Array items are not unique"));
                if (reporter.fail_early())
                {
                    return;
                }
            }
        }

        static bool array_has_unique_items(const Json& a) 
        {
            for (auto it = a.array_range().begin(); it != a.array_range().end(); ++it) 
            {
                for (auto jt = it+1; jt != a.array_range().end(); ++jt) 
                {
                    if (*it == *jt) 
                    {
                        return false; // contains duplicates 
                    }
                }
            }
            return true; // elements are unique
        }
    };

    // minLength

    template <class Json>
    class min_length_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t min_length_;

    public:
        min_length_validator(const std::string& schema_path, std::size_t min_length)
            : keyword_validator_base<Json>(schema_path), min_length_(min_length)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<min_length_validator>(std::string(this->schema_path()),
                min_length_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            auto sv = instance.as_string_view();
            std::size_t length = unicode_traits::count_codepoints(sv.data(), sv.size());
            if (length < min_length_) 
            {
                reporter.error(validation_output("minLength", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 std::string("Expected minLength: ") + std::to_string(min_length_)
                                          + ", actual: " + std::to_string(length)));
                if (reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    // string 

    template <class Json>
    class string_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        std::vector<keyword_validator_type> validators_;
    public:
        string_validator(const std::string& schema_path,
            std::vector<keyword_validator_type>&& validators)
            : keyword_validator_base<Json>(schema_path), validators_(std::move(validators))
        {
        }

        keyword_validator_type clone() const final 
        {
            std::vector<keyword_validator_type> validators;
            for (auto& validator : validators_)
            {
                validators.emplace_back(validator->clone());
            }

            return jsoncons::make_unique<string_validator>(std::string(this->schema_path()),
                std::move(validators));
        }

    private:
        void do_validate(const Json& instance,
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties,
            error_reporter& reporter,
            Json& patch) const final
        {
            for (const auto& validator : validators_)
            {
                validator->validate(instance, instance_location, evaluated_properties, reporter, patch);
                if (reporter.error_count() > 0 && reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    // not

    template <class Json>
    class not_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;

        schema_validator_type rule_;

    public:
        not_validator(const std::string& schema_path,
            schema_validator_type&& rule)
            : keyword_validator_base<Json>(schema_path), 
              rule_(std::move(rule))
        {
        }

        keyword_validator_type clone() const final 
        {
            schema_validator_type rule;
            if (rule_)
            {
                rule = rule_->clone();
            }

            return jsoncons::make_unique<not_validator>(std::string(this->schema_path()),
                std::move(rule));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            collecting_error_reporter local_reporter;
            rule_->validate(instance, instance_location, evaluated_properties, local_reporter, patch);

            if (local_reporter.errors.empty())
            {
                reporter.error(validation_output("not", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Instance must not be valid against schema"));
            }
        }
    };

    template <class Json>
    struct all_of_criterion
    {
        static const std::string& key()
        {
            static const std::string k("allOf");
            return k;
        }

        static bool is_complete(const Json&, 
                                const jsonpointer::json_pointer& instance_location, 
                                error_reporter& reporter, 
                                const collecting_error_reporter& local_reporter, 
                                std::size_t)
        {
            if (!local_reporter.errors.empty())
                reporter.error(validation_output("allOf", 
                                                 "",
                                                 instance_location.to_uri_fragment(), 
                                                 "At least one schema failed to match, but all are required to match. ", 
                                                 local_reporter.errors));
            return !local_reporter.errors.empty();
        }
    };

    template <class Json>
    struct any_of_criterion
    {
        static const std::string& key()
        {
            static const std::string k("anyOf");
            return k;
        }

        static bool is_complete(const Json&, 
                                const jsonpointer::json_pointer&, 
                                error_reporter&, 
                                const collecting_error_reporter&, 
                                std::size_t count)
        {
            //std::cout << "any_of_criterion is_complete\n";
            return count == 1;
        }
    };

    template <class Json>
    struct one_of_criterion
    {
        static const std::string& key()
        {
            static const std::string k("oneOf");
            return k;
        }

        static bool is_complete(const Json&, 
                                const jsonpointer::json_pointer& instance_location, 
                                error_reporter& reporter, 
                                const collecting_error_reporter&, 
                                std::size_t count)
        {
            if (count > 1)
            {
                std::string message(std::to_string(count));
                message.append(" subschemas matched, but exactly one is required to match");
                reporter.error(validation_output("oneOf", 
                                                 "", 
                                                 instance_location.to_uri_fragment(), 
                                                 std::move(message)));
            }
            return count > 1;
        }
    };

    template <class Json,class Criterion>
    class combining_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;

        std::vector<schema_validator_type> validators_;

    public:
        combining_validator(std::string&& schema_path,
             std::vector<schema_validator_type>&& validators)
            : keyword_validator_base<Json>(std::move(schema_path)),
              validators_(std::move(validators))
        {
        }

        keyword_validator_type clone() const final 
        {
            std::vector<schema_validator_type> validators;
            for (auto& validator : validators_)
            {
                validators.emplace_back(validator->clone());
            }

            return jsoncons::make_unique<combining_validator>(std::string(this->schema_path()),
                std::move(validators));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            //std::cout << "combining_validator.do_validate " << instance << " " << validators_.size() << ", " << instance_location.to_string() << "\n";

            size_t count = 0;

            collecting_error_reporter local_reporter;

            bool is_complete = false;
            for (auto& s : validators_) 
            {
                std::size_t mark = local_reporter.errors.size();
                s->validate(instance, instance_location, evaluated_properties, local_reporter, patch);
                if (!is_complete)
                {
                    if (mark == local_reporter.errors.size())
                        count++;
                    if (Criterion::is_complete(instance, instance_location, reporter, local_reporter, count))
                        is_complete = true;
                }
            }

            if (count == 0)
            {
                reporter.error(validation_output("combined", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "No schema matched, but one of them is required to match", 
                                                 local_reporter.errors));
            }
        }
    };

    template <class Json,class T>
    class maximum_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        T value_;

    public:
        maximum_validator(const std::string& schema_path, T value)
            : keyword_validator_base<Json>(schema_path), value_(value)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<maximum_validator>(std::string(this->schema_path()),
                value_);
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final 
        {
            T value = instance.template as<T>(); 
            if (value > value_)
            {
                reporter.error(validation_output("maximum", 
                    this->schema_path(), 
                    instance_location.to_uri_fragment(), 
                    instance.template as<std::string>() + " exceeds maximum of " + std::to_string(value_)));
            }
        }
    };

    template <class Json,class T>
    class exclusive_maximum_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        T value_;

    public:
        exclusive_maximum_validator(const std::string& schema_path, T value)
            : keyword_validator_base<Json>(schema_path), value_(value)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<exclusive_maximum_validator>(std::string(this->schema_path()),
                value_);
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final 
        {
            T value = instance.template as<T>(); 
            if (value >= value_)
            {
                reporter.error(validation_output("exclusiveMaximum", 
                    this->schema_path(), 
                    instance_location.to_uri_fragment(), 
                    instance.template as<std::string>() + " exceeds exclusiveMaximum of " + std::to_string(value_)));
            }
        }
    };

    template <class Json,class T>
    class minimum_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        T value_;

    public:
        minimum_validator(const std::string& schema_path, T value)
            : keyword_validator_base<Json>(schema_path), value_(value)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<minimum_validator>(std::string(this->schema_path()),
                value_);
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final 
        {
            T value = instance.template as<T>(); 
            if (value < value_)
            {
                reporter.error(validation_output("minimum", 
                    this->schema_path(), 
                    instance_location.to_uri_fragment(), 
                    instance.template as<std::string>() + " exceeds minimum of " + std::to_string(value_)));
            }
        }
    };

    template <class Json,class T>
    class exclusive_minimum_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        T value_;

    public:
        exclusive_minimum_validator(const std::string& schema_path, T value)
            : keyword_validator_base<Json>(schema_path), value_(value)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<exclusive_minimum_validator>(std::string(this->schema_path()),
                value_);
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final 
        {
            T value = instance.template as<T>(); 
            if (value <= value_)
            {
                reporter.error(validation_output("exclusiveMinimum", 
                    this->schema_path(), 
                    instance_location.to_uri_fragment(), 
                    instance.template as<std::string>() + " exceeds exclusiveMinimum of " + std::to_string(value_)));
            }
        }
    };

    template <class Json>
    class multiple_of_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        double value_;

    public:
        multiple_of_validator(const std::string& schema_path, double value)
            : keyword_validator_base<Json>(schema_path), value_(value)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<multiple_of_validator>(std::string(this->schema_path()),
                value_);
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final
        {
            double value = instance.template as<double>();
            if (value != 0) // Exclude zero
            {
                if (!is_multiple_of(value, static_cast<double>(value_)))
                {
                    reporter.error(validation_output("multipleOf", 
                        this->schema_path(),
                        instance_location.to_uri_fragment(), 
                        instance.template as<std::string>() + " is not a multiple of " + std::to_string(value_)));
                }
            }
        }

        static bool is_multiple_of(double x, double multiple_of) 
        {
            double rem = std::remainder(x, multiple_of);
            double eps = std::nextafter(x, 0) - x;
            return std::fabs(rem) < std::fabs(eps);
        }
    };

    template <class Json>
    class integer_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        std::vector<keyword_validator_type> validators_;
    public:
        integer_validator(const std::string& schema_path, 
            std::vector<keyword_validator_type>&& validators)
            : keyword_validator_base<Json>(schema_path), validators_(std::move(validators))
        {
        }

        keyword_validator_type clone() const final 
        {
            std::vector<keyword_validator_type> validators;
            for (auto& validator : validators_)
            {
                validators.emplace_back(validator->clone());
            }

            return jsoncons::make_unique<integer_validator>(std::string(this->schema_path()),
                std::move(validators));
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            if (!(instance.template is_integer<int64_t>() || (instance.is_double() && static_cast<double>(instance.template as<int64_t>()) == instance.template as<double>())))
            {
                reporter.error(validation_output("integer", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Instance is not an integer"));
                if (reporter.fail_early())
                {
                    return;
                }
            }
            for (const auto& validator : validators_)
            {
                validator->validate(instance, instance_location, evaluated_properties, reporter, patch);
                if (reporter.error_count() > 0 && reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    template <class Json>
    class number_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        std::vector<keyword_validator_type> validators_;
    public:
        number_validator(const std::string& schema_path, 
            std::vector<keyword_validator_type>&& validators)
            : keyword_validator_base<Json>(schema_path), validators_(std::move(validators))
        {
        }

        keyword_validator_type clone() const final 
        {
            std::vector<keyword_validator_type> validators;
            for (auto& validator : validators_)
            {
                validators.emplace_back(validator->clone());
            }

            return jsoncons::make_unique<number_validator>(std::string(this->schema_path()),
                std::move(validators));
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            if (!(instance.template is_integer<int64_t>() || instance.is_double()))
            {
                reporter.error(validation_output("number", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Instance is not a number"));
                if (reporter.fail_early())
                {
                    return;
                }
            }
            for (const auto& validator : validators_)
            {
                validator->validate(instance, instance_location, evaluated_properties, reporter, patch);
                if (reporter.error_count() > 0 && reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    // null

    template <class Json>
    class null_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

    public:
        null_validator(const std::string& schema_path)
            : keyword_validator_base<Json>(schema_path)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<null_validator>(std::string(this->schema_path()));
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final
        {
            if (!instance.is_null())
            {
                reporter.error(validation_output("null", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Expected to be null"));
            }
        }
    };

    template <class Json>
    class boolean_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

    public:
        boolean_validator(const std::string& schema_path)
            : keyword_validator_base<Json>(schema_path)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<boolean_validator>(std::string(this->schema_path()));
        }
    private:
        void do_validate(const Json&, 
            const jsonpointer::json_pointer&,
            std::unordered_set<std::string>&, 
            error_reporter&, 
            Json&) const final
        {
        }

    };

    template <class Json>
    class required_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::vector<std::string> items_;

    public:
        required_validator(const std::string& schema_path,
            const std::vector<std::string>& items)
            : keyword_validator_base<Json>(schema_path), items_(items)
        {
        }

        required_validator(const required_validator&) = delete;
        required_validator(required_validator&&) = default;
        required_validator& operator=(const required_validator&) = delete;
        required_validator& operator=(required_validator&&) = default;

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<required_validator>(std::string(this->schema_path()),
                items_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final
        {
            for (const auto& key : items_)
            {
                if (instance.find(key) == instance.object_range().end())
                {
                    reporter.error(validation_output("required", 
                                                     this->schema_path(), 
                                                     instance_location.to_uri_fragment(), 
                                                     "Required property \"" + key + "\" not found"));
                    if (reporter.fail_early())
                    {
                        return;
                    }
                }
            }
        }
    };

    // maxProperties

    template <class Json>
    class max_properties_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t max_properties_;
    public:
        max_properties_validator(const std::string& schema_path, std::size_t max_properties)
            : keyword_validator_base<Json>(schema_path), max_properties_(max_properties)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<max_properties_validator>(this->schema_path(), max_properties_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (instance.size() > max_properties_)
            {
                std::string message("Maximum properties: " + std::to_string(max_properties_));
                message.append(", found: " + std::to_string(instance.size()));
                reporter.error(validation_output("maxProperties", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 std::move(message)));
                if (reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    // minProperties

    template <class Json>
    class min_properties_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t min_properties_;
    public:
        min_properties_validator(const std::string& schema_path, std::size_t min_properties)
            : keyword_validator_base<Json>(schema_path), min_properties_(min_properties)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<min_properties_validator>(this->schema_path(), min_properties_);
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (instance.size() < min_properties_)
            {
                std::string message("Maximum properties: " + std::to_string(min_properties_));
                message.append(", found: " + std::to_string(instance.size()));
                reporter.error(validation_output("minProperties", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 std::move(message)));
                if (reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    template <class Json>
    class object_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;

        std::vector<keyword_validator_type> general_validators_;

        std::map<std::string, schema_validator_type> properties_;
    #if defined(JSONCONS_HAS_STD_REGEX)
        std::vector<std::pair<std::regex, schema_validator_type>> pattern_properties_;
    #endif
        schema_validator_type additional_properties_;

        std::map<std::string, keyword_validator_type> dependent_required_;
        std::map<std::string, schema_validator_type> dependent_schemas_;

        schema_validator_type property_name_validator_;

    public:
        object_validator(std::string&& schema_path,
            std::vector<keyword_validator_type>&& general_validators,
            std::map<std::string, schema_validator_type>&& properties,
        #if defined(JSONCONS_HAS_STD_REGEX)
            std::vector<std::pair<std::regex, schema_validator_type>>&& pattern_properties,
        #endif
            schema_validator_type&& additional_properties,
            std::map<std::string, keyword_validator_type>&& dependent_required,
            std::map<std::string, schema_validator_type>&& dependent_schemas,
            schema_validator_type&& property_name_validator
        )
            : keyword_validator_base<Json>(std::move(schema_path)), 
              general_validators_(std::move(general_validators)),
              properties_(std::move(properties)),
        #if defined(JSONCONS_HAS_STD_REGEX)
              pattern_properties_(std::move(pattern_properties)),
        #endif
              additional_properties_(std::move(additional_properties)),
              dependent_required_(std::move(dependent_required)),
              dependent_schemas_(std::move(dependent_schemas)),
              property_name_validator_(std::move(property_name_validator))
        {
        }

        keyword_validator_type clone() const final 
        {
            std::vector<keyword_validator_type> general_validators;
            for (auto& validator : general_validators_)
            {
                general_validators.emplace_back(validator->clone());
            }

            std::map<std::string, schema_validator_type> properties;
            for (auto& item : properties_)
            {
                properties.emplace(item.first, item.second->clone());
            }


        #if defined(JSONCONS_HAS_STD_REGEX)
            std::vector<std::pair<std::regex, schema_validator_type>> pattern_properties;
            for (auto& item : pattern_properties_)
            {
                pattern_properties.emplace_back(item.first, item.second->clone());
            }
        #endif
            schema_validator_type additional_properties;
            if (additional_properties_)
            {
                additional_properties = additional_properties_->clone();
            }

            std::map<std::string, keyword_validator_type> dependent_required;
            for (auto& item : dependent_required_)
            {
                dependent_required.emplace(item.first, item.second->clone());
            }

            std::map<std::string, schema_validator_type> dependent_schemas;
            for (auto& item : dependent_schemas_)
            {
                dependent_schemas.emplace(item.first, item.second->clone());
            }

            schema_validator_type property_name_validator;
            if (property_name_validator_)
            {
                property_name_validator = property_name_validator_->clone();
            }

            return jsoncons::make_unique<object_validator>(std::string(this->schema_path()),
                std::move(general_validators), std::move(properties),
        #if defined(JSONCONS_HAS_STD_REGEX)
                std::move(pattern_properties),
        #endif
                std::move(additional_properties),
                std::move(dependent_required),
                std::move(dependent_schemas),
                std::move(property_name_validator));
        }
    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            std::unordered_set<std::string> local_evaluated_properties;

            for (const auto& validator : general_validators_)
            {
                validator->validate(instance, instance_location, evaluated_properties, reporter, patch);
                if (reporter.error_count() > 0 && reporter.fail_early())
                {
                    return;
                }
            }

            for (const auto& prop : instance.object_range()) 
            {
                jsonpointer::json_pointer pointer(instance_location);
                pointer /= prop.key();

                if (property_name_validator_)
                    property_name_validator_->validate(prop.key(), instance_location, local_evaluated_properties, reporter, patch);

                bool a_prop_or_pattern_matched = false;
                auto properties_it = properties_.find(prop.key());

                // check if it is in "properties"
                if (properties_it != properties_.end()) 
                {
                    a_prop_or_pattern_matched = true;

                    std::size_t error_count = reporter.error_count();
                    properties_it->second->validate(prop.value(), pointer, local_evaluated_properties, reporter, patch);
                    if (reporter.error_count() == error_count)
                    {
                        local_evaluated_properties.insert(prop.key());
                    }
                }
                // Any property that doesn't match any of the property names in the properties keyword is ignored by this keyword.

    #if defined(JSONCONS_HAS_STD_REGEX)

                // check all matching "patternProperties"
                for (auto& schema_pp : pattern_properties_)
                    if (std::regex_search(prop.key(), schema_pp.first)) 
                    {
                        a_prop_or_pattern_matched = true;
                        std::size_t error_count = reporter.error_count();
                        schema_pp.second->validate(prop.value(), pointer, local_evaluated_properties, reporter, patch);
                        if (reporter.error_count() == error_count)
                        {
                            local_evaluated_properties.insert(prop.key());
                        }
                    }
    #endif

                // finally, check "additionalProperties" 
                //std::cout << "object_validator a_prop_or_pattern_matched " << a_prop_or_pattern_matched << ", " << bool(additional_properties_);
                if (!a_prop_or_pattern_matched && additional_properties_) 
                {
                    //std::cout << " !!!additionalProperties!!!";

                    collecting_error_reporter local_reporter;

                    additional_properties_->validate(prop.value(), pointer, local_evaluated_properties, local_reporter, patch);
                    if (!local_reporter.errors.empty())
                    {
                        reporter.error(validation_output("additionalProperties", 
                                                         additional_properties_->schema_path(), 
                                                         instance_location.to_uri_fragment(), 
                                                         "Additional prop \"" + prop.key() + "\" found but was invalid."));
                        if (reporter.fail_early())
                        {
                            return;
                        }
                    }
                    else
                    {
                        local_evaluated_properties.insert(prop.key());
                    }

                }
                //std::cout << "\n";
            }

            // reverse search
            for (auto const& prop : properties_) 
            {
                const auto finding = instance.find(prop.first);
                if (finding == instance.object_range().end()) 
                { 
                    // If prop is not in instance
                    auto default_value = prop.second->get_default_value();
                    if (default_value) 
                    { 
                        // If default value is available, update patch
                        jsonpointer::json_pointer pointer(instance_location);
                        pointer /= prop.first;

                        update_patch(patch, pointer, std::move(*default_value));
                    }
                }
            }

            for (const auto& dep : dependent_required_) 
            {
                auto prop = instance.find(dep.first);
                if (prop != instance.object_range().end()) 
                {
                    // if dependency-prop is present in instance
                    jsonpointer::json_pointer pointer(instance_location);
                    pointer /= dep.first;
                    dep.second->validate(instance, pointer, local_evaluated_properties, reporter, patch); // validate
                }
            }

            for (const auto& dep : dependent_schemas_) 
            {
                auto prop = instance.find(dep.first);
                if (prop != instance.object_range().end()) 
                {
                    // if dependency-prop is present in instance
                    jsonpointer::json_pointer pointer(instance_location);
                    pointer /= dep.first;
                    dep.second->validate(instance, pointer, local_evaluated_properties, reporter, patch); // validate
                }
            }

            //std::cout << "Evaluated properties\n";
            //for (const auto& s : local_evaluated_properties)
            //{
            //    std::cout << "    " << s << "\n";
            //}
            //std::cout << "\n";

            for (auto&& name : local_evaluated_properties)
            {
                evaluated_properties.emplace(std::move(name));
            }
        }

        void update_patch(Json& patch, const jsonpointer::json_pointer& instance_location, Json&& default_value) const
        {
            Json j;
            j.try_emplace("op", "add"); 
            j.try_emplace("path", instance_location.to_uri_fragment()); 
            j.try_emplace("value", std::forward<Json>(default_value)); 
            patch.push_back(std::move(j));
        }
    };

    template <class Json>
    class unevaluated_properties_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;

        schema_validator_type validator_;

    public:
        unevaluated_properties_validator(std::string&& schema_path,
            schema_validator_type&& validator
        )
            : keyword_validator_base<Json>(std::move(schema_path)), 
              validator_(std::move(validator))
        {
        }

        keyword_validator_type clone() const final 
        {
            schema_validator_type validator;
            if (validator_)
            {
                validator = validator_->clone();
            }
            return jsoncons::make_unique<unevaluated_properties_validator>(std::string(this->schema_path()),
                std::move(validator));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            //std::cout << "Evaluated properties\n";
            //for (const auto& s : evaluated_properties)
            //{
            //    std::cout << "    " << s << "\n";
            //}
            //std::cout << "\n";

            if (validator_)
            {
                for (const auto& prop : instance.object_range()) 
                {
                    auto prop_it = evaluated_properties.find(prop.key());

                    // check if it is in "evaluated_properties"
                    if (prop_it == evaluated_properties.end()) 
                    {
                        
                        std::size_t error_count = reporter.error_count();
                        validator_->validate(prop.value(), instance_location, evaluated_properties, reporter, patch);
                        if (reporter.error_count() == error_count)
                        {
                            evaluated_properties.insert(prop.key());
                        }
                    }
                }
            }
        }
    };

    // array

    template <class Json>
    class array_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;

        std::vector<keyword_validator_type> validators_;
    public:
        array_validator(const std::string& schema_path, std::vector<keyword_validator_type>&& validators)
            : keyword_validator_base<Json>(schema_path), validators_(std::move(validators))
        {
        }

        keyword_validator_type clone() const final 
        {
            std::vector<keyword_validator_type> validators;
            for (auto& validator : validators_)
            {
                validators.emplace_back(validator->clone());
            }

            return jsoncons::make_unique<array_validator>(std::string(this->schema_path()),
                std::move(validators));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            for (const auto& validator : validators_)
            {
                validator->validate(instance, instance_location, evaluated_properties, reporter, patch);
                if (reporter.error_count() > 0 && reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    template <class Json>
    class conditional_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;

        schema_validator_type if_validator_;
        schema_validator_type then_validator_;
        schema_validator_type else_validator_;

    public:
        conditional_validator(const std::string&& schema_path,
          schema_validator_type&& if_validator,
          schema_validator_type&& then_validator,
          schema_validator_type&& else_validator
        ) : keyword_validator_base<Json>(std::move(schema_path)), 
              if_validator_(std::move(if_validator)), 
              then_validator_(std::move(then_validator)), 
              else_validator_(std::move(else_validator))
        {
        }

        keyword_validator_type clone() const final 
        {
            schema_validator_type if_validator;
            if (if_validator_)
            {
                if_validator = if_validator_->clone();
            }
            schema_validator_type then_validator;
            if (then_validator_)
            {
                then_validator = then_validator_->clone();
            }
            schema_validator_type else_validator;
            if (else_validator_)
            {
                else_validator = else_validator_->clone();
            }

            return jsoncons::make_unique<conditional_validator>(std::string(this->schema_path()), 
                std::move(if_validator), std::move(then_validator), std::move(else_validator));
        }
    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            if (if_validator_) 
            {
                collecting_error_reporter local_reporter;

                if_validator_->validate(instance, instance_location, evaluated_properties, local_reporter, patch);
                if (local_reporter.errors.empty()) 
                {
                    if (then_validator_)
                        then_validator_->validate(instance, instance_location, evaluated_properties, reporter, patch);
                } 
                else 
                {
                    if (else_validator_)
                        else_validator_->validate(instance, instance_location, evaluated_properties, reporter, patch);
                }
            }
        }
    };

    // enum_validator

    template <class Json>
    class enum_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        Json value_;

    public:
        enum_validator(const std::string& path, const Json& sch)
            : keyword_validator_base<Json>(path), value_(sch)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<enum_validator>(std::string(this->schema_path()), Json(value_));
        }
    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            bool in_range = false;
            for (const auto& item : value_.array_range())
            {
                if (item == instance) 
                {
                    in_range = true;
                    break;
                }
            }

            if (!in_range)
            {
                reporter.error(validation_output("enum", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 instance.template as<std::string>() + " is not a valid enum value"));
                if (reporter.fail_early())
                {
                    return;
                }
            }
        }
    };

    // const_validator

    template <class Json>
    class const_validator : public keyword_validator_base<Json>
    {        
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        Json value_;

    public:
        const_validator(const std::string& path, const Json& sch)
            : keyword_validator_base<Json>(path), value_(sch)
        {
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<const_validator>(std::string(this->schema_path()), Json(value_));
        }
    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter,
            Json&) const final
        {
            if (value_ != instance)
                reporter.error(validation_output("const", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Instance is not const"));
        }
    };

    template <class Json>
    class type_validator : public keyword_validator_base<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;

        std::vector<keyword_validator_type> type_mapping_;
        std::vector<std::string> expected_types_;

    public:
        type_validator(const type_validator&) = delete;
        type_validator& operator=(const type_validator&) = delete;
        type_validator(type_validator&&) = default;
        type_validator& operator=(type_validator&&) = default;

        type_validator(std::string&& schema_path,
            std::vector<keyword_validator_type>&& type_mapping,
            std::vector<std::string>&& expected_types
 )
            : keyword_validator_base<Json>(std::move(schema_path)),
              type_mapping_(std::move(type_mapping)),
              expected_types_(std::move(expected_types))
        {
        }

        keyword_validator_type clone() const final 
        {
            std::vector<keyword_validator_type> type_mapping;
            for (auto& validator : type_mapping_)
            {
                if (validator)
                {
                    type_mapping.emplace_back(validator->clone());
                }
                else
                {
                    type_mapping.emplace_back(keyword_validator_type{});
                }
            }

            return jsoncons::make_unique<type_validator>(std::string(this->schema_path()),
                std::move(type_mapping), 
                std::vector<std::string>(expected_types_));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            auto& type = type_mapping_[(uint8_t) instance.type()];

            //std::cout << "anyOf validate " << instance;
            if (type)
                type->validate(instance, instance_location, evaluated_properties, reporter, patch);
            else
            {
                std::ostringstream ss;
                ss << "Expected " << expected_types_.size() << " ";
                for (std::size_t i = 0; i < expected_types_.size(); ++i)
                {
                        if (i > 0)
                        { 
                            ss << ", ";
                            if (i+1 == expected_types_.size())
                            { 
                                ss << "or ";
                            }
                        }
                        ss << expected_types_[i];
                        //std::cout << ", " << i << ". expected " << expected_types_[i];
                }
                ss << ", found " << instance.type();

                reporter.error(validation_output("type", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 ss.str()));
                if (reporter.fail_early())
                {
                    return;
                }
            }
            //std::cout << "\n";
        }
    };

} // namespace jsonschema
} // namespace jsoncons

#endif // JSONCONS_JSONSCHEMA_KEYWORDS_HPP
