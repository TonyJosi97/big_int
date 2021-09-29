/** 
 *  @file   big_int.cc
 *  @brief  Main source file for the big int library
 *
 *  This file contains the source code the DS used as the big in library
 *
 *  @author         Tony Josi   https://tonyjosi97.github.io/profile/
 *  @copyright      Copyright (C) 2021 Tony Josi
 *  @bug            No known bugs.
 */

#include <stdexcept>
#include <algorithm>
#include <string.h>
#include <memory>
#include <cstdio>

#include "big_int.hpp"
#include "big_int_lib_log.hpp"


namespace {

    static inline int compare_bi_base_type(const BI_BASE_TYPE a, const BI_BASE_TYPE b);
    static inline int compare_bi_base_type_n_top(const BI_BASE_TYPE a, const BI_BASE_TYPE b, const int a_top, const int b_top) ;

}


void bi::big_int::_swap_big_int(bi::big_int &src) {

    using std::swap;

    swap(_data,         src._data);
    swap(_total_data,   src._total_data);
    swap(_top,          src._top);
    swap(_neg,          src._neg);

}


int bi::big_int::_big_int_expand(int req) {

    if (req > 0) {
        BI_BASE_TYPE *temp_buff = new BI_BASE_TYPE[_total_data + req];
        if(!temp_buff) {
            _BI_LOG(1, "_big_int_expand failed");
            throw std::length_error("Couldnt find enough memory");
        } else {
            std::copy_n(_data, _total_data, temp_buff);
            delete[] _data;
            _data = temp_buff;
            _total_data += req;
        }
        _BI_LOG(2, "_big_int_expand expanded to total: %d items", _total_data);
        return 0;
    } else {
        _BI_LOG(1, "_big_int_expand fail negetive expand value");
        return -1;
    }

}

int bi::big_int::big_int_from_string(const std::string &str_num) {

    size_t str_size = str_num.length(), num_decor_cnt = 0;
    size_t is_neg = 0;
    if (str_size > 0 && str_num[0] == '-') {
        is_neg = 1;
        num_decor_cnt += 1;
    }
    if((str_size > (1 + is_neg)) && str_num[is_neg] == '0' && (str_num[is_neg + 1] == 'x' || str_num[is_neg + 1] == 'X')) {
        num_decor_cnt += 2;
    }

    str_size -= num_decor_cnt;

    size_t extr_space_reqd = ((str_size % BI_HEX_STR_TO_DATA_SIZE == 0) ? \
    0 : (BI_HEX_STR_TO_DATA_SIZE - (str_size % BI_HEX_STR_TO_DATA_SIZE)));
    
    size_t base_t_aligned_size = str_size + extr_space_reqd;
    std::unique_ptr<char []> temp_str(new char[base_t_aligned_size]);
    memset(temp_str.get(), '0', extr_space_reqd);
    memcpy(temp_str.get() + extr_space_reqd, str_num.c_str() + num_decor_cnt, str_size);

    int str_cur_indx = static_cast<int>(base_t_aligned_size - BI_HEX_STR_TO_DATA_SIZE);

    for(; str_cur_indx >= 0; str_cur_indx -= static_cast<int>(BI_HEX_STR_TO_DATA_SIZE)) {
        if(sscanf(&(temp_str.get()[str_cur_indx]), BI_SSCANF_FORMAT_HEX, &_data[_top++]) == EOF) {
            big_int_clear();
            return -1;
        }
        if(_top == _total_data) {
            _big_int_expand(BI_DEFAULT_EXPAND_COUNT);
        }
    }

    if(big_int_is_zero() == 0 && is_neg) {
        return -1;
    }
    _neg = static_cast<bool>(is_neg);

    /* Remove extra zeroes in the MSB if the i/p string had them. */
    for(int i = _top - 1; i >= 0; i--) {
        if(_data[i] == 0) {
            _top--;
        } else {
            break;
        }
    }

    return 0;

}

int bi::big_int::big_int_unsigned_add(const bi::big_int &b) {

    int max_data_len, min_data_len;

    if (_top > b._top) {
        min_data_len = b._top;
        max_data_len = _top;
    } else {
        min_data_len = _top;
        max_data_len = b._top;
    }

    int i;

    BI_DOUBLE_BASE_TYPE sum = 0;
    BI_BASE_TYPE carry = 0;
    for(i = 0; i < min_data_len; ++i) {
        sum = static_cast<BI_DOUBLE_BASE_TYPE>(b._data[i]) + _data[i] + carry;
        carry = static_cast<BI_BASE_TYPE>((sum) >> 32);
        _data[i] = sum & BI_BASE_TYPE_MAX;
    }
    int top_cntr = 0;
    for(; i < max_data_len; ++i) {
        if(i >= _total_data) {
            _big_int_expand(BI_DEFAULT_EXPAND_COUNT);
        }
        if(i >= _top) {
            ++top_cntr;
        }
        if(i < _top) {
            sum = static_cast<BI_DOUBLE_BASE_TYPE>(_data[i]) + carry;
        } else {
            sum = static_cast<BI_DOUBLE_BASE_TYPE>(b._data[i]) + carry;
        }
        carry = static_cast<BI_BASE_TYPE>((sum) >> 32);
        _data[i] = sum & BI_BASE_TYPE_MAX;
    }
    _top += top_cntr;
    
    if (carry) {
        if(i >= _total_data) {
            _big_int_expand(BI_DEFAULT_EXPAND_COUNT);
        }
        _data[_top++] = carry;
    }

    return 0;

}

int bi::big_int::big_int_unsigned_add(const bi::big_int &b, bi::big_int *res) {

    res->big_int_clear();
    res->big_int_unsigned_add(b);
    res->big_int_unsigned_add(*this);
    return 0;

}

int bi::big_int::big_int_signed_add(const bi::big_int &b) {

    bi::big_int temp;
    big_int_signed_add(b, &temp);
    _swap_big_int(temp);
    return 0;

}

int bi::big_int::big_int_signed_add(const bi::big_int &b, bi::big_int *res) {

    res->big_int_clear();
    /* If the operands are of same sign. */
    if (_neg == b._neg) {

        big_int_unsigned_add(b, res);
        res->_neg = _neg;

    }
    /* If operands are of different sign. */
    else {

        /* Convert to same type. */
        _neg = !_neg;
        int comp_stat = big_int_compare(b);
        if (comp_stat == 1) {
            if (_neg) {
                b.big_int_unsigned_sub(*this, res);
                res->_neg = b._neg;
            } else {
                big_int_unsigned_sub(b, res);
                res->_neg = !_neg;
            }
        } else if (comp_stat == 0) {
            res->big_int_set_zero();
        } else {
            if (_neg) {
                big_int_unsigned_sub(b, res);
                res->_neg = !_neg;
            } else {
                b.big_int_unsigned_sub(*this, res);
                res->_neg = b._neg;
            }
        }

        /* Set back to previous type. */
        _neg = !_neg;

    }
    return 0;

}

int bi::big_int::big_int_set_zero() {

    return big_int_clear();

}

int bi::big_int::big_int_unsigned_sub(const bi::big_int &b, bi::big_int *res) const {

    int max, min;
    max = _top;
    min = b._top;

    int diff = max - min;
    if(diff < 0) {
        throw std::length_error("First param should be larger");
    }

    res->big_int_clear();
    BI_BASE_TYPE borrow = _sub_base_type(b._data, min, res);

    for(int i = min; i < max; i++) {
        if(i >= res->_total_data) {
            res->_big_int_expand(BI_DEFAULT_EXPAND_COUNT);
        }
        if(compare_bi_base_type(_data[i], borrow)) {
            res->_data[(res->_top)++] = _data[i] - borrow;
            borrow = 0;
        } else {
            res->_data[(res->_top)++] = static_cast<BI_BASE_TYPE>((static_cast<BI_DOUBLE_BASE_TYPE>(BI_BASE_TYPE_MAX) + 1 - borrow));
            borrow = 1;
        }
    }

    return 0;

}

int bi::big_int::big_int_unsigned_sub(const bi::big_int &b) {

    bi::big_int temp;
    big_int_unsigned_sub(b, &temp);
    temp._neg = this->_neg;
    _swap_big_int(temp);
    return 0;

}

int bi::big_int::big_int_clear() {

    memset(_data, 0, static_cast<size_t>(_total_data) * sizeof(BI_BASE_TYPE));
    _top            = 0;
    _neg            = false;
    return 0;
}

std::string     bi::big_int::big_int_to_string(bi::bi_base base) {

    size_t chars_per_data;

    if(base == bi::bi_base::BI_DEC) {
        chars_per_data = BI_SPRINF_FORMAT_DEC_CHARS;
    } else if(base == bi::bi_base::BI_HEX){ 
        chars_per_data = BI_SPRINF_FORMAT_HEX_CHARS;
    }

    size_t is_neg = 0;
    if (big_int_is_negetive()) {
        is_neg = 1;
    }

    std::unique_ptr<char []> char_temp_buff(new char[static_cast<size_t>(_top) * chars_per_data + 1 + is_neg]);
    memset(char_temp_buff.get(), '\0', static_cast<size_t>(_top) * chars_per_data + 1 + is_neg);
    if (is_neg) {
        char_temp_buff.get()[0] = '-';
    }

    for(int i = _top - 1; i >= 0; --i) {
        if(base == bi::bi_base::BI_DEC) {
            sprintf(char_temp_buff.get() + is_neg + ((static_cast<size_t>(_top) - 1) - static_cast<size_t>(i)) * chars_per_data, BI_SPRINF_FORMAT_DEC, _data[i]);
            _BI_LOG(3, BI_SPRINF_FORMAT_DEC_LOG, _data[i]);
        }
        else if(base == bi::bi_base::BI_HEX){ 
            sprintf(char_temp_buff.get() + is_neg + ((static_cast<size_t>(_top) - 1) - static_cast<size_t>(i)) * chars_per_data, BI_SPRINF_FORMAT_HEX, _data[i]);
            _BI_LOG(3, BI_SPRINF_FORMAT_HEX_LOG, _data[i]);
        }
    }

    std::string op_string(char_temp_buff.get());
    return op_string;

}

int bi::big_int::big_int_set_negetive(bool set_unset) {

    if (big_int_is_zero()) {
        return -1;
    }
    _neg = set_unset;
    return 0;

}

bool bi::big_int::big_int_is_zero() {

    for(int i = 0; i < _top; ++i) {
        if (_data[i] != 0) {
            return -1;
        }
    }
    return 0;

}


bool bi::big_int::big_int_is_negetive() {

    return _neg;

}


BI_BASE_TYPE bi::big_int::_sub_base_type(BI_BASE_TYPE *data_ptr, int min, bi::big_int *res_ptr) const {

    BI_BASE_TYPE borrow = 0;
    BI_DOUBLE_BASE_TYPE diff, temp1;
    for(int i = 0; i < min; ++i) {
        if(compare_bi_base_type(_data[i], data_ptr[i])) {
            diff = _data[i] - data_ptr[i] - borrow;
            borrow = 0;
        } else {
            temp1 = static_cast<BI_DOUBLE_BASE_TYPE>(_data[i]) + BI_BASE_TYPE_MAX + 1 - borrow;
            diff = temp1 - data_ptr[i];
            borrow = 1;
        }
        res_ptr->_data[(res_ptr->_top)++] = static_cast<BI_BASE_TYPE>(diff);
    }
    return borrow;
}

int bi::big_int::big_int_compare(const bi::big_int &other) const {

    if (_neg == other._neg) {
        if (_neg == 1) {
            return -1 * _compare_bi_base_type_n_top(other);
        } else {
            return _compare_bi_base_type_n_top(other);
        }
    } else {
        if (_neg == 1) {
            return -1;
        } else {
            return 1;
        }
    }
}

int bi::big_int::_compare_bi_base_type_n_top(const bi::big_int &other) const {

    if (_top == other._top) {
        int i = _top - 1;
        for(; i >= 0; i--) {
            if (_data[i] == other._data[i]) {
                continue;
            } else if (_data[i] > other._data[i]) {
                return 1;
            } else {
                return -1;
            }
        }
        return 0;
    } else if (_top > other._top) {
        return 1;
    } else {
        return -1;
    }

}

namespace {

    static inline int compare_bi_base_type(const BI_BASE_TYPE a, const BI_BASE_TYPE b) {

        if(a >= b)
            return 1;
        else
            return 0;

    }

    static inline int compare_bi_base_type_n_top(const BI_BASE_TYPE a, const BI_BASE_TYPE b, const int a_top, const int b_top) {

        if (a_top == b_top) {
            return compare_bi_base_type(a, b);
        } else if (a_top > b_top) {
            return 1;
        } else {
            return 0;
        }

    }

}