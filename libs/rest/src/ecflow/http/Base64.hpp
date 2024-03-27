/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * Original license included below:
 *
 *
 *   License
 *   -------
 *   This software may be distributed, used, and modified under the terms of BSD license:
 *
 *   Redistribution and use in source and binary forms, with or without modification,
 *   are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name(s) of the above-listed copyright holder(s) nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ecflow_http_Base64_HPP
#define ecflow_http_Base64_HPP

#include <cstring>
#include <regex>

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */

inline unsigned char* base64_decode(const unsigned char* src, size_t len, size_t* out_len) {

    static const std::string base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    unsigned char dtable[256], *out, *pos, block[4], tmp;
    size_t olen;
    int pad = 0;

    memset(dtable, 0x80, 256);
    for (size_t i = 0; i < sizeof(base64_table) - 1; i++) {
        auto base64_char    = static_cast<size_t>(base64_table[i]);
        dtable[base64_char] = (unsigned char)i;
    }
    size_t assignment_char  = '=';
    dtable[assignment_char] = 0;

    size_t count = 0;
    for (size_t i = 0; i < len; i++) {
        if (dtable[src[i]] != 0x80) {
            count++;
        }
    }

    if (count == 0 || count % 4) {
        return nullptr;
    }

    olen = count / 4 * 3;
    pos = out = (unsigned char*)malloc(olen);
    if (out == nullptr) {
        return nullptr;
    }

    count = 0;
    for (size_t i = 0; i < len; i++) {
        tmp = dtable[src[i]];
        if (tmp == 0x80) {
            continue;
        }

        if (src[i] == '=') {
            pad++;
        }
        block[count] = tmp;
        count++;
        if (count == 4) {
            *pos++ = (block[0] << 2) | (block[1] >> 4);
            *pos++ = (block[1] << 4) | (block[2] >> 2);
            *pos++ = (block[2] << 6) | block[3];
            count  = 0;
            if (pad) {
                if (pad == 1) {
                    pos--;
                }
                else if (pad == 2) {
                    pos -= 2;
                }
                else {
                    /* Invalid padding */
                    free(out);
                    return nullptr;
                }
                break;
            }
        }
    }

    *out_len = pos - out;
    return out;
}

/* Wrapper for std::string /partio */

inline std::string base64_decode(const std::string& encoded) {
    size_t len         = 0;
    unsigned char* out = base64_decode(reinterpret_cast<const unsigned char*>(encoded.c_str()), encoded.size(), &len);
    if (!out) {
        throw std::runtime_error("Unable to decode base64");
    }

    std::string decoded(reinterpret_cast<char*>(out), len);
    free(out);
    return decoded;
}

/* Simple validation of base64 string.
 * - length has to be multiple of 4
 * - valid characters are  A-Za-z0-9+/
 * padding of 0-2 characters at the end of string can be =
 *
 * /partio
 */

inline bool base64_validate(const std::string& encoded) {
    if (encoded.size() % 4 != 0) {
        return false;
    }
    static const std::regex pattern("^([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{4}|[A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)$");

    return std::regex_match(encoded, pattern);
}

#endif /* ecflow_http_Base64_HPP */
