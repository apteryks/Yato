/**
 * YATO library
 *
 * Apache License, Version 2.0
 * Copyright (c) 2016-2020 Alexey Gruzdev
 */

#ifndef _YATO_TOKEN_ITERATOR_H_
#define _YATO_TOKEN_ITERATOR_H_

#include <string>
#ifndef YATO_UNDEF_TOKENS_JOIN
# include <sstream>
#endif

#include "assertion.h"
#include "type_traits.h"
#include "storage.h"
#include "range.h"

#ifndef YATO_TOKEN_ITERATOR_PREDICATE_SIZE
# define YATO_TOKEN_ITERATOR_PREDICATE_SIZE (64)
#endif

namespace yato
{
    // Default compare operation
    template <typename ValueTy_>
    struct equal_to_value
    {
    private:
        ValueTy_ m_value;

    public:
        equal_to_value(ValueTy_ val)
            : m_value(std::move(val))
        { }

        equal_to_value(const equal_to_value&) = default;
        equal_to_value(equal_to_value&&) = default;

        equal_to_value& operator=(const equal_to_value&) = default;
        equal_to_value& operator=(equal_to_value&&) = default;

        ~equal_to_value() = default;

        template <typename Ty_>
        bool operator()(const Ty_ & v) const
        {
            return m_value == v;
        }
    };



    // End iterator for tokenizer
    class tokens_end_t
    {
        // Iterator traits
        using difference_type   = std::ptrdiff_t;
        using value_type        = void;
        using pointer           = void*;
        using reference         = void;
        using iterator_category = std::input_iterator_tag;
    };

#if !YATO_MSVC || (YATO_MSVC >= YATO_MSVC_2013)
    YATO_INLINE_VARIABLE constexpr tokens_end_t tokens_end{};
#endif


    template <typename IterBeg_,
              typename IterEnd_ = IterBeg_,
              typename Predicate_ = yato::equal_to_value<typename std::iterator_traits<IterBeg_>::value_type>
    >
    class token_iterator
    {
        static_assert(yato::is_iterator<IterBeg_>::value, "Invalid iterator type");

    private:
        using this_type = token_iterator<IterBeg_, IterEnd_, Predicate_>;

    public:
        using underlying_begin_iterator = IterBeg_;
        using underlying_end_iterator   = IterEnd_;
        using predicate_type = Predicate_;

        using token_range = yato::range<underlying_begin_iterator>;

        // Iterator traits
        using difference_type   = std::ptrdiff_t;
        using value_type        = token_range;
        using pointer           = std::unique_ptr<token_range>;
        using reference         = token_range;
        using iterator_category = std::input_iterator_tag;

    private:
        static YATO_CONSTEXPR_VAR size_t _predicate_storage_max_size = YATO_TOKEN_ITERATOR_PREDICATE_SIZE;
        using predicate_storage = storage<predicate_type, _predicate_storage_max_size>;

        underlying_begin_iterator m_token_begin;
        underlying_begin_iterator m_token_end;
        underlying_end_iterator m_end;
        predicate_storage m_predicate;
        bool m_skip_empty;

        void move_iterator_(underlying_begin_iterator & it)
        {
            while (it != m_end) {
                if ((*m_predicate)(*it)) {
                    break;
                }
                ++it;
            }
        }

        void init_token_()
        {
            // token_begin and token_end are at the first element
            move_iterator_(m_token_end);
            if (m_skip_empty && (m_token_begin == m_token_end)) {
                next_token_();
            }
        }

        void next_token_()
        {
            do {
                if (m_token_end != m_end) {
                    ++m_token_end;
                    m_token_begin = m_token_end;
                    move_iterator_(m_token_end);
                }
                else {
                    m_token_begin = m_end;
                    break; // EOS
                }
            } while(m_skip_empty && (m_token_begin == m_token_end));
        }

    public:
        token_iterator(underlying_begin_iterator first, underlying_end_iterator last, const predicate_type & predicate, bool skip_empty = false)
            : m_token_begin(std::move(first)), m_token_end(std::move(first)), m_end(std::move(last)), m_predicate(predicate), m_skip_empty(skip_empty)
        {
            YATO_ASSERT(std::distance(first, last) >= 0, "Invalid iterators pair");
            init_token_();
        }

        token_iterator(underlying_begin_iterator first, underlying_end_iterator last, predicate_type && predicate, bool skip_empty = false)
            : m_token_begin(std::move(first)), m_token_end(std::move(first)), m_end(std::move(last)), m_predicate(std::move(predicate)), m_skip_empty(skip_empty)
        {
            YATO_ASSERT(std::distance(first, last) >= 0, "Invalid iterators pair");
            init_token_();
        }

        token_iterator(const token_iterator&) = default;
        token_iterator(token_iterator&&) = default;

        token_iterator& operator=(const token_iterator&) = default;
        token_iterator& operator=(token_iterator&&) = default;

        ~token_iterator() = default;

        bool has_next() const
        {
            return m_token_begin != m_end;
        }

        bool empty() const
        {
            return !has_next();
        }

        token_range peek() const
        {
            return yato::make_range(m_token_begin, m_token_end);
        }

        token_range next()
        {
            YATO_REQUIRES(has_next());
            auto r = yato::make_range(m_token_begin, m_token_end);
            next_token_();
            return r;
        }

        reference operator*() const
        {
            return peek();
        }

        pointer operator->() const
        {
            return std::make_unique<token_range>(peek());
        }

        this_type& operator++()
        {
            YATO_REQUIRES(has_next());
            next();
            return *this;
        }

        this_type& operator++(int)
        {
            YATO_REQUIRES(has_next());
            this_type tmp{*this};
            next();
            return tmp;
        }

        friend
        bool operator==(const this_type & lh, const this_type & rh)
        {
            return (lh.m_token_begin == rh.m_token_begin) && (lh.m_token_end == rh.m_token_end);
        }

        friend
        bool operator==(const this_type & lh, const tokens_end_t &)
        {
            return lh.empty();
        }

        friend
        bool operator==(const tokens_end_t &, const this_type & rh)
        {
            return rh.empty();
        }

        friend
        bool operator!=(const this_type & lh, const this_type & rh)
        {
            return (lh.m_token_begin != rh.m_token_begin) || (lh.m_token_end != rh.m_token_end);
        }

        friend
        bool operator!=(const this_type & lh, const tokens_end_t &)
        {
            return lh.has_next();
        }

        friend
        bool operator!=(const tokens_end_t &, const this_type & rh)
        {
            return rh.has_next();
        }
    };

    template <typename Iter1_, typename Iter2_, typename Pred_>
    auto make_token_iterator(Iter1_ && first, Iter2_ && last, Pred_ && p, bool skip_empty = false)
    {
        using iter1_type = yato::remove_cvref_t<Iter1_>;
        using iter2_type = yato::remove_cvref_t<Iter2_>;
        using pred_type = yato::remove_cvref_t<Pred_>;
        return yato::token_iterator<iter1_type, iter2_type, pred_type>(std::forward<Iter1_>(first), std::forward<Iter2_>(last), std::forward<Pred_>(p), skip_empty);
    }


    template <typename CharTy_>
    auto tokenize(const std::basic_string<CharTy_> & str, CharTy_ sep, bool skip_empty = false)
    {
        return make_token_iterator(std::cbegin(str), std::cend(str), yato::equal_to_value<CharTy_>{sep}, skip_empty);
    }

    template <typename CharTy_>
    auto tokenize(std::basic_string<CharTy_> & str, CharTy_ sep, bool skip_empty = false)
    {
        return make_token_iterator(std::begin(str), std::end(str), yato::equal_to_value<CharTy_>{sep}, skip_empty);
    }

    template <typename CharTy_>
    auto tokenize_n(const CharTy_* str, size_t n, CharTy_ sep, bool skip_empty = false)
    {
        return make_token_iterator(str, str + n, yato::equal_to_value<CharTy_>{sep}, skip_empty);
    }

    template <typename CharTy_>
    auto tokenize_n(CharTy_* str, size_t n, CharTy_ sep, bool skip_empty = false)
    {
        return make_token_iterator(str, str + n, yato::equal_to_value<CharTy_>{sep}, skip_empty);
    }

    template <typename Iter1_, typename Iter2_, typename CharTy_>
    auto tokenize(Iter1_ && first, Iter2_ && last, CharTy_ sep, bool skip_empty = false)
    {
        return make_token_iterator(std::forward<Iter1_>(first), std::forward<Iter2_>(last), yato::equal_to_value<CharTy_>{sep}, skip_empty);
    }

    template <typename Iter1_, typename Iter2_, typename Pred_>
    auto tokenize_if(Iter1_ && first, Iter2_ && last, Pred_ && p, bool skip_empty = false)
    {
        return make_token_iterator(std::forward<Iter1_>(first), std::forward<Iter2_>(last), std::forward<Pred_>(p), skip_empty);
    }

    template <typename Iter1_, typename Iter2_, typename CharTy_>
    auto tokenize(const yato::range<Iter1_, Iter2_> & range, CharTy_ sep, bool skip_empty = false)
    {
        return make_token_iterator(range.begin(), range.end(), yato::equal_to_value<CharTy_>{sep}, skip_empty);
    }

    template <typename Iter1_, typename Iter2_, typename Pred_>
    auto tokenize_if(const yato::range<Iter1_, Iter2_> & range, Pred_ && p, bool skip_empty = false)
    {
        return make_token_iterator(range.begin(), range.end(), std::forward<Pred_>(p), skip_empty);
    }


    template <typename CharTy_>
    auto tokens_crange(const std::basic_string<CharTy_> & str, CharTy_ sep, bool skip_empty = false)
    {
        return yato::make_range(tokenize(str, sep, skip_empty), tokens_end_t{});
    }

    template <typename CharTy_>
    auto tokens_range(std::basic_string<CharTy_> & str, CharTy_ sep, bool skip_empty = false)
    {
        return yato::make_range(tokenize(str, sep, skip_empty), tokens_end_t{});
    }

    template <typename CharTy_>
    auto tokens_crange_n(const CharTy_* & str, size_t n, CharTy_ sep, bool skip_empty = false)
    {
        return yato::make_range(tokenize_n(str, n, sep, skip_empty), tokens_end_t{});
    }

    template <typename CharTy_>
    auto tokens_range_n(CharTy_* str, size_t n, CharTy_ sep, bool skip_empty = false)
    {
        return yato::make_range(tokenize_n(str, n, sep, skip_empty), tokens_end_t{});
    }

    template <typename Iter1_, typename Iter2_, typename CharTy_>
    auto tokens_range(Iter1_ && first, Iter2_ && last, CharTy_ sep, bool skip_empty = false)
    {
        return yato::make_range(make_token_iterator(std::forward<Iter1_>(first), std::forward<Iter2_>(last), yato::equal_to_value<CharTy_>{sep}, skip_empty), tokens_end_t{});
    }

    template <typename Iter1_, typename Iter2_, typename Pred_>
    auto tokens_range_if(Iter1_ && first, Iter2_ && last, Pred_ && p, bool skip_empty = false)
    {
        return yato::make_range(make_token_iterator(std::forward<Iter1_>(first), std::forward<Iter2_>(last), std::forward<Pred_>(p), skip_empty), tokens_end_t{});
    }

    template <typename Iter1_, typename Iter2_, typename CharTy_>
    auto tokens_range(const yato::range<Iter1_, Iter2_> & range, CharTy_ sep, bool skip_empty = false)
    {
        return yato::make_range(make_token_iterator(range.begin(), range.end(), yato::equal_to_value<CharTy_>{sep}, skip_empty), tokens_end_t{});
    }

    template <typename Iter1_, typename Iter2_, typename Pred_>
    auto tokens_range_if(const yato::range<Iter1_, Iter2_> & range, Pred_ && p, bool skip_empty = false)
    {
        return yato::make_range(make_token_iterator(range.begin(), range.end(), std::forward<Pred_>(p), skip_empty), tokens_end_t{});
    }


    namespace details {

        template <typename CharTy_, typename Tokenizer_>
        inline
        std::vector<std::basic_string<CharTy_>> split_impl(Tokenizer_ t)
        {
            std::vector<std::basic_string<CharTy_>> res{};
            while (t.has_next()) {
                auto token = t.next();
                res.emplace_back(token.begin(), token.end());
            }
            return res;
        }

    } // details


    template <typename CharTy_>
    inline
    std::vector<std::basic_string<CharTy_>> split(const std::basic_string<CharTy_>& str, const CharTy_& sep, bool skip_empty = false)
    {
        return details::split_impl<CharTy_>(yato::tokenize(str, sep, skip_empty));
    }

    template <typename CharTy_>
    inline
    std::vector<std::basic_string<CharTy_>> split(const std::basic_string<CharTy_>& str, const std::basic_string<CharTy_>& seps, bool skip_empty = false)
    {
        return details::split_impl<CharTy_>(yato::tokenize_if(std::cbegin(str), std::cend(str), [&seps](const CharTy_& c) { return seps.find_first_of(c) != std::basic_string<CharTy_>::npos; }, skip_empty));
    }

    template <typename CharTy_>
    inline
    std::vector<std::basic_string<CharTy_>> split(const std::basic_string<CharTy_>& str, const CharTy_* seps, bool skip_empty = false)
    {
        YATO_ASSERT(seps != nullptr, "separators pointer is null");
        return split(str, static_cast<std::basic_string<CharTy_>>(seps), skip_empty);
    }

    template <typename CharTy_, typename Pred_>
    inline
    std::vector<std::basic_string<CharTy_>> split_if(const std::basic_string<CharTy_>& str, Pred_&& p, bool skip_empty = false)
    {
        return details::split_impl<CharTy_>(yato::tokenize_if(std::cbegin(str), std::cend(str), std::forward<Pred_>(p), skip_empty));
    }

    inline
    std::vector<std::string> split_n(const char* str, size_t n, const char& sep, bool skip_empty = false)
    {
        return details::split_impl<char>(yato::tokenize_n(str, n, sep, skip_empty));
    }

    inline
    std::vector<std::wstring> split_n(const wchar_t* str, size_t n, const wchar_t& sep, bool skip_empty = false)
    {
        return details::split_impl<wchar_t>(yato::tokenize_n(str, n, sep, skip_empty));
    }

#ifndef YATO_UNDEF_TOKENS_JOIN

    namespace details {

        template <typename CharTy_, typename Iter1_, typename Iter2_, typename Sep_, typename Pref_, typename Suff_>
        inline
        std::basic_string<CharTy_> join_impl(Iter1_ beg, Iter2_ end, const Sep_& sep, const Pref_& prefix, const Suff_& suffix)
        {
            std::basic_stringstream<CharTy_> builder{};
            builder << prefix;
            if (beg != end) {
                builder << *beg++;
            }
            while (beg != end) {
                builder << sep << *beg++;
            }
            builder << suffix;
            return builder.str();
        }

    } // details

    template <typename Iter1_, typename Iter2_, typename Sep_ = std::string, typename Pref_ = std::string, typename Suff_ = std::string>
    inline
    std::string join(Iter1_&& beg, Iter2_&& end, const Sep_& sep = Sep_{}, const Pref_& prefix = Pref_{}, const Suff_& suffix = Suff_{})
    {
        return details::join_impl<char>(std::forward<Iter1_>(beg), std::forward<Iter2_>(end), sep, prefix, suffix);
    }

    template <typename Iter1_, typename Iter2_, typename Sep_ = std::wstring, typename Pref_ = std::wstring, typename Suff_ = std::wstring>
    inline
    std::wstring wjoin(Iter1_&& beg, Iter2_&& end, const Sep_& sep = Sep_{}, const Pref_& prefix = Pref_{}, const Suff_& suffix = Suff_{})
    {
        return details::join_impl<wchar_t>(std::forward<Iter1_>(beg), std::forward<Iter2_>(end), sep, prefix, suffix);
    }


#endif // YATO_UNDEF_TOKENS_JOIN
}

#endif //_YATO_TOKEN_ITERATOR_H_
